#define _POSIX_C_SOURCE 200809L

#include "sdr.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SDR_TWO_PI (2.0 * M_PI)

/* ---------- Complex type ---------- */

sdr_cpx sdr_cpx_add(sdr_cpx a, sdr_cpx b) { sdr_cpx r = {a.re + b.re, a.im + b.im}; return r; }
sdr_cpx sdr_cpx_sub(sdr_cpx a, sdr_cpx b) { sdr_cpx r = {a.re - b.re, a.im - b.im}; return r; }
sdr_cpx sdr_cpx_mul(sdr_cpx a, sdr_cpx b) {
    sdr_cpx r = {a.re * b.re - a.im * b.im, a.re * b.im + a.im * b.re};
    return r;
}
sdr_cpx sdr_cpx_scale(sdr_cpx a, double s) { sdr_cpx r = {a.re * s, a.im * s}; return r; }
sdr_cpx sdr_cpx_from_polar(double mag, double phase) {
    sdr_cpx r = {mag * cos(phase), mag * sin(phase)};
    return r;
}
double sdr_cpx_abs(sdr_cpx a) { return sqrt(a.re * a.re + a.im * a.im); }
double sdr_cpx_arg(sdr_cpx a) { return atan2(a.im, a.re); }

/* ---------- NCO ---------- */

void sdr_nco_init(sdr_nco *n, double freq_hz, double fs) {
    n->phase = 0.0;
    n->phase_inc = SDR_TWO_PI * freq_hz / fs;
}

sdr_cpx sdr_nco_step(sdr_nco *n) {
    sdr_cpx r = {cos(n->phase), sin(n->phase)};
    n->phase += n->phase_inc;
    if (n->phase >= SDR_TWO_PI) n->phase -= SDR_TWO_PI;
    if (n->phase < 0.0) n->phase += SDR_TWO_PI;
    return r;
}

void sdr_nco_reset(sdr_nco *n) { n->phase = 0.0; }

/* ---------- Filter design: windowed-sinc low-pass ---------- */

int sdr_fir_lowpass(double *taps, int n_taps, double cutoff_hz, double fs, int window) {
    if (n_taps < 1 || !taps) return -1;
    double fc = cutoff_hz / fs;
    double M = (double)(n_taps - 1) / 2.0;
    double sum = 0.0;
    for (int n = 0; n < n_taps; n++) {
        double k = (double)n - M;
        double sinc;
        if (k == 0.0) {
            sinc = 2.0 * fc;
        } else {
            sinc = sin(SDR_TWO_PI * fc * k) / (M_PI * k);
        }
        double w;
        switch (window) {
            case 1: {
                double x = SDR_TWO_PI * (double)n / (double)(n_taps - 1);
                w = 0.54 - 0.46 * cos(x);
                break;
            }
            case 2: {
                double x = SDR_TWO_PI * (double)n / (double)(n_taps - 1);
                w = 0.42 - 0.5 * cos(x) + 0.08 * cos(2.0 * x);
                break;
            }
            case 3: {
                double x = SDR_TWO_PI * (double)n / (double)(n_taps - 1);
                w = 0.5 - 0.5 * cos(x);
                break;
            }
            default: w = 1.0;
        }
        taps[n] = sinc * w;
        sum += taps[n];
    }
    if (sum != 0.0) {
        for (int n = 0; n < n_taps; n++) taps[n] /= sum;
    }
    return 0;
}

/* ---------- FIR complex ---------- */

int sdr_fir_cpx_init(sdr_fir_cpx *f, const double *taps, int n_taps, int decim) {
    if (n_taps < 1 || !taps || decim < 1) return -1;
    f->taps = (double *)malloc(sizeof(double) * n_taps);
    if (!f->taps) return -1;
    memcpy(f->taps, taps, sizeof(double) * n_taps);
    f->delay = (sdr_cpx *)calloc((size_t)n_taps, sizeof(sdr_cpx));
    if (!f->delay) { free(f->taps); return -1; }
    f->n_taps = n_taps;
    f->idx = 0;
    f->decim = decim;
    f->count = 0;
    return 0;
}

void sdr_fir_cpx_destroy(sdr_fir_cpx *f) {
    if (!f) return;
    free(f->taps);
    free(f->delay);
    f->taps = NULL;
    f->delay = NULL;
    f->n_taps = 0;
}

sdr_cpx sdr_fir_cpx_process(sdr_fir_cpx *f, sdr_cpx in) {
    f->delay[f->idx] = in;
    f->idx = (f->idx + 1) % f->n_taps;
    f->count++;
    if (f->count < f->n_taps) {
        sdr_cpx z = {0.0, 0.0};
        return z;
    }
    if ((f->count % f->decim) != 0) {
        sdr_cpx z = {0.0, 0.0};
        return z;
    }
    sdr_cpx acc = {0.0, 0.0};
    int read = f->idx;
    for (int k = 0; k < f->n_taps; k++) {
        read = (read - 1 + f->n_taps) % f->n_taps;
        sdr_cpx d = f->delay[read];
        double t = f->taps[k];
        acc.re += d.re * t;
        acc.im += d.im * t;
    }
    return acc;
}

/* ---------- FIR real ---------- */

int sdr_fir_real_init(sdr_fir_real *f, const double *taps, int n_taps, int decim) {
    if (n_taps < 1 || !taps || decim < 1) return -1;
    f->taps = (double *)malloc(sizeof(double) * n_taps);
    if (!f->taps) return -1;
    memcpy(f->taps, taps, sizeof(double) * n_taps);
    f->delay = (double *)calloc((size_t)n_taps, sizeof(double));
    if (!f->delay) { free(f->taps); return -1; }
    f->n_taps = n_taps;
    f->idx = 0;
    f->decim = decim;
    f->count = 0;
    return 0;
}

void sdr_fir_real_destroy(sdr_fir_real *f) {
    if (!f) return;
    free(f->taps);
    free(f->delay);
    f->taps = NULL;
    f->delay = NULL;
    f->n_taps = 0;
}

double sdr_fir_real_process(sdr_fir_real *f, double in) {
    f->delay[f->idx] = in;
    f->idx = (f->idx + 1) % f->n_taps;
    f->count++;
    if (f->count < f->n_taps) return 0.0;
    if ((f->count % f->decim) != 0) return 0.0;
    double acc = 0.0;
    int read = f->idx;
    for (int k = 0; k < f->n_taps; k++) {
        read = (read - 1 + f->n_taps) % f->n_taps;
        acc += f->delay[read] * f->taps[k];
    }
    return acc;
}

/* ---------- FM discriminator ---------- */

void sdr_fm_disc_init(sdr_fm_disc *d) {
    d->prev.re = 1.0;
    d->prev.im = 0.0;
    d->has_prev = 0;
}

double sdr_fm_disc_process(sdr_fm_disc *d, sdr_cpx in) {
    if (!d->has_prev) {
        d->prev = in;
        d->has_prev = 1;
        return 0.0;
    }
    sdr_cpx conj_prev = {d->prev.re, -d->prev.im};
    sdr_cpx prod = sdr_cpx_mul(in, conj_prev);
    double phase = atan2(prod.im, prod.re);
    d->prev = in;
    return phase;
}

/* ---------- De-emphasis ---------- */

void sdr_deemph_init(sdr_deemph *d, double tau_seconds, double fs) {
    double dt = 1.0 / fs;
    d->alpha = exp(-dt / tau_seconds);
    d->y = 0.0;
}

double sdr_deemph_process(sdr_deemph *d, double x) {
    d->y = d->alpha * d->y + (1.0 - d->alpha) * x;
    return d->y;
}

/* ---------- FM demodulator pipeline ---------- */

#define SDR_DEC1 5
#define SDR_DEC2 10
#define SDR_TAPS1 129
#define SDR_TAPS2 129

int sdr_fm_init(sdr_fm *fm, double fs_in, double audio_rate,
                double station_offset_hz, double deviation_hz,
                int deemph_us_50_or_75) {
    if (fs_in <= 0.0 || audio_rate <= 0.0 || deviation_hz <= 0.0) return -1;
    if (deemph_us_50_or_75 != 50 && deemph_us_50_or_75 != 75) return -1;

    double fs_inter = fs_in / SDR_DEC1;
    if (audio_rate != fs_inter / SDR_DEC2) {
        return -1;
    }

    double taps1[SDR_TAPS1];
    double taps2[SDR_TAPS2];

    double cutoff1 = 0.8 * (fs_inter / 2.0);
    if (sdr_fir_lowpass(taps1, SDR_TAPS1, cutoff1, fs_in, 1) != 0) return -1;

    double cutoff2 = 0.6 * (audio_rate / 2.0);
    if (cutoff2 > 15000.0) cutoff2 = 15000.0;
    if (sdr_fir_lowpass(taps2, SDR_TAPS2, cutoff2, fs_inter, 1) != 0) return -1;

    sdr_nco_init(&fm->nco, -station_offset_hz, fs_in);

    if (sdr_fir_cpx_init(&fm->fir1, taps1, SDR_TAPS1, SDR_DEC1) != 0) return -1;
    sdr_fm_disc_init(&fm->disc);
    if (sdr_fir_real_init(&fm->fir2, taps2, SDR_TAPS2, SDR_DEC2) != 0) {
        sdr_fir_cpx_destroy(&fm->fir1);
        return -1;
    }

    sdr_deemph_init(&fm->deemph, (double)deemph_us_50_or_75 * 1e-6, audio_rate);

    fm->fs_in = fs_in;
    fm->fs_intermediate = fs_inter;
    fm->fs_audio = audio_rate;
    fm->deviation_hz = deviation_hz;
    fm->audio_gain = fs_inter / (SDR_TWO_PI * deviation_hz);
    fm->deemph_us = deemph_us_50_or_75;
    return 0;
}

void sdr_fm_destroy(sdr_fm *fm) {
    if (!fm) return;
    sdr_fir_cpx_destroy(&fm->fir1);
    sdr_fir_real_destroy(&fm->fir2);
}

int sdr_fm_process(sdr_fm *fm, const sdr_cpx *in, int n_in,
                   double *out, int max_out) {
    if (!fm || !in || !out || n_in < 0 || max_out < 0) return -1;
    int written = 0;
    for (int n = 0; n < n_in; n++) {
        sdr_cpx osc = sdr_nco_step(&fm->nco);
        sdr_cpx mixed = sdr_cpx_mul(in[n], osc);
        sdr_cpx filtered = sdr_fir_cpx_process(&fm->fir1, mixed);
        sdr_cpx disc = {0.0, 0.0};
        if (fm->fir1.count >= fm->fir1.n_taps &&
            (fm->fir1.count % SDR_DEC1) == 0) {
            disc.re = 0.0;
            disc.im = 0.0;
            double phase = sdr_fm_disc_process(&fm->disc, filtered);
            double audio = sdr_fir_real_process(&fm->fir2, phase * fm->audio_gain);
            if (fm->fir2.count >= fm->fir2.n_taps &&
                (fm->fir2.count % SDR_DEC2) == 0) {
                double d = sdr_deemph_process(&fm->deemph, audio);
                if (written < max_out) out[written++] = d;
            }
        }
        (void)disc;
    }
    return written;
}

/* ---------- IQ file I/O (int16 I/Q interleaved) ---------- */

int sdr_iq_open(sdr_iq_file *f, const char *path, const char *mode) {
    f->fp = fopen(path, mode);
    if (!f->fp) return -1;
    fseek(f->fp, 0, SEEK_END);
    long end = ftell(f->fp);
    fseek(f->fp, 0, SEEK_SET);
    f->n_samples = (end < 0) ? -1 : end / (long)sizeof(int16_t) / 2;
    return 0;
}

int sdr_iq_read(sdr_iq_file *f, sdr_cpx *out, int n) {
    if (!f || !f->fp || n < 0) return -1;
    int16_t buf[4096 * 2];
    int total = 0;
    while (total < n) {
        int want = n - total;
        if (want > 4096) want = 4096;
        size_t got = fread(buf, sizeof(int16_t), (size_t)want * 2, f->fp);
        if (got == 0) break;
        int samples = (int)(got / 2);
        for (int i = 0; i < samples; i++) {
            out[total + i].re = (double)buf[2 * i] / 32768.0;
            out[total + i].im = (double)buf[2 * i + 1] / 32768.0;
        }
        total += samples;
    }
    return total;
}

int sdr_iq_write(sdr_iq_file *f, const sdr_cpx *in, int n) {
    if (!f || !f->fp || n < 0) return -1;
    int16_t buf[4096 * 2];
    int total = 0;
    while (total < n) {
        int want = n - total;
        if (want > 4096) want = 4096;
        for (int i = 0; i < want; i++) {
            double ir = in[total + i].re;
            double ii = in[total + i].im;
            if (ir >  1.0) ir =  1.0;
            if (ir < -1.0) ir = -1.0;
            if (ii >  1.0) ii =  1.0;
            if (ii < -1.0) ii = -1.0;
            buf[2 * i]     = (int16_t)lrint(ir * 32767.0);
            buf[2 * i + 1] = (int16_t)lrint(ii * 32767.0);
        }
        size_t wrote = fwrite(buf, sizeof(int16_t), (size_t)want * 2, f->fp);
        if (wrote != (size_t)want * 2) return -1;
        total += want;
    }
    return total;
}

int sdr_iq_close(sdr_iq_file *f) {
    if (!f || !f->fp) return -1;
    int r = fclose(f->fp);
    f->fp = NULL;
    return r;
}

/* ---------- WAV file writer (16-bit PCM mono or stereo) ---------- */

static void wav_write_u32_le(FILE *fp, uint32_t v) {
    uint8_t b[4] = {(uint8_t)(v & 0xff), (uint8_t)((v >> 8) & 0xff),
                    (uint8_t)((v >> 16) & 0xff), (uint8_t)((v >> 24) & 0xff)};
    fwrite(b, 1, 4, fp);
}

static void wav_write_u16_le(FILE *fp, uint16_t v) {
    uint8_t b[2] = {(uint8_t)(v & 0xff), (uint8_t)((v >> 8) & 0xff)};
    fwrite(b, 1, 2, fp);
}

int sdr_wav_open(sdr_wav *w, const char *path, int sample_rate, int n_channels) {
    if (!w || !path || sample_rate <= 0 || n_channels < 1) return -1;
    w->fp = fopen(path, "wb");
    if (!w->fp) return -1;
    w->sample_rate = sample_rate;
    w->n_channels = n_channels;
    w->n_samples = 0;

    fwrite("RIFF", 1, 4, w->fp);
    wav_write_u32_le(w->fp, 0);
    fwrite("WAVE", 1, 4, w->fp);
    fwrite("fmt ", 1, 4, w->fp);
    wav_write_u32_le(w->fp, 16);
    wav_write_u16_le(w->fp, 1);
    wav_write_u16_le(w->fp, (uint16_t)n_channels);
    wav_write_u32_le(w->fp, (uint32_t)sample_rate);
    wav_write_u32_le(w->fp, (uint32_t)(sample_rate * n_channels * 2));
    wav_write_u16_le(w->fp, (uint16_t)(n_channels * 2));
    wav_write_u16_le(w->fp, 16);
    fwrite("data", 1, 4, w->fp);
    wav_write_u32_le(w->fp, 0);
    return 0;
}

int sdr_wav_write(sdr_wav *w, const double *samples, int n) {
    if (!w || !w->fp || n < 0) return -1;
    int16_t buf[4096];
    int total = 0;
    while (total < n) {
        int want = n - total;
        if (want > 4096) want = 4096;
        for (int i = 0; i < want; i++) {
            double s = samples[total + i];
            if (s >  1.0) s =  1.0;
            if (s < -1.0) s = -1.0;
            buf[i] = (int16_t)lrint(s * 32767.0);
        }
        size_t wrote = fwrite(buf, sizeof(int16_t), (size_t)want, w->fp);
        if (wrote != (size_t)want) return -1;
        total += want;
    }
    w->n_samples += n;
    return total;
}

int sdr_wav_close(sdr_wav *w) {
    if (!w || !w->fp) return -1;
    long data_bytes = w->n_samples * 2;
    fseek(w->fp, 4, SEEK_SET);
    wav_write_u32_le(w->fp, (uint32_t)(data_bytes + 36));
    fseek(w->fp, 40, SEEK_SET);
    wav_write_u32_le(w->fp, (uint32_t)data_bytes);
    int r = fclose(w->fp);
    w->fp = NULL;
    return r;
}

/* ---------- Synthetic FM signal generator ---------- */

void sdr_fm_gen_init(sdr_fm_gen *g, double fs, double carrier_offset_hz,
                     double dev_hz, double audio_freq_hz) {
    g->fs = fs;
    g->carrier_offset_hz = carrier_offset_hz;
    g->dev_hz = dev_hz;
    g->audio_freq_hz = audio_freq_hz;
    g->phase = 0.0;
    g->mod_integrator = 0.0;
}

void sdr_fm_gen_process(sdr_fm_gen *g, sdr_cpx *out, int n) {
    double carrier_inc = SDR_TWO_PI * g->carrier_offset_hz / g->fs;
    double audio_inc   = SDR_TWO_PI * g->audio_freq_hz / g->fs;
    double k = g->dev_hz / g->fs;
    for (int i = 0; i < n; i++) {
        double audio = sin(g->phase);
        g->mod_integrator += k * audio;
        double mod_phase = SDR_TWO_PI * g->mod_integrator;
        out[i].re = cos(carrier_inc * (double)i + mod_phase);
        out[i].im = sin(carrier_inc * (double)i + mod_phase);
        g->phase += audio_inc;
        if (g->phase >= SDR_TWO_PI) g->phase -= SDR_TWO_PI;
    }
}
