/*
 * sdr.h - Software Defined Radio library
 *
 * FM broadcast demodulator. Takes a stream of complex IQ samples
 * centered on an FM carrier, shifts the station to baseband, decimates,
 * discriminates, applies de-emphasis, and emits audio.
 *
 * Also includes a synthetic FM signal generator for self-contained
 * testing without RF hardware.
 *
 * All sample rates are in Hz, phases in radians, frequencies in Hz.
 * All audio output is mono float in approximately [-1, 1].
 *
 * The pipeline (default configuration):
 *   NCO shift -> FIR decimate by 5 (Fs/2.4M -> Fs/480k)
 *             -> FM discriminator at 480 kS/s
 *             -> FIR decimate by 10 (Fs/480k -> Fs/48k)
 *             -> De-emphasis at 48 kS/s
 *             -> Audio out
 */

#ifndef SDR_H
#define SDR_H

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Complex type ---- */

typedef struct {
    double re;
    double im;
} sdr_cpx;

sdr_cpx  sdr_cpx_add(sdr_cpx a, sdr_cpx b);
sdr_cpx  sdr_cpx_sub(sdr_cpx a, sdr_cpx b);
sdr_cpx  sdr_cpx_mul(sdr_cpx a, sdr_cpx b);
sdr_cpx  sdr_cpx_scale(sdr_cpx a, double s);
sdr_cpx  sdr_cpx_from_polar(double mag, double phase);
double   sdr_cpx_abs(sdr_cpx a);
double   sdr_cpx_arg(sdr_cpx a);

/* ---- NCO (Numerically Controlled Oscillator) ---- */

typedef struct {
    double phase;
    double phase_inc;
} sdr_nco;

void    sdr_nco_init(sdr_nco *n, double freq_hz, double fs);
sdr_cpx sdr_nco_step(sdr_nco *n);
void    sdr_nco_reset(sdr_nco *n);

/* ---- FIR filter (complex or real input) ---- */

typedef struct {
    double *taps;
    int n_taps;
    sdr_cpx *delay;
    int idx;
    int decim;
    int count;
} sdr_fir_cpx;

int     sdr_fir_cpx_init(sdr_fir_cpx *f, const double *taps, int n_taps, int decim);
void    sdr_fir_cpx_destroy(sdr_fir_cpx *f);
sdr_cpx sdr_fir_cpx_process(sdr_fir_cpx *f, sdr_cpx in);

typedef struct {
    double *taps;
    int n_taps;
    double *delay;
    int idx;
    int decim;
    int count;
} sdr_fir_real;

int     sdr_fir_real_init(sdr_fir_real *f, const double *taps, int n_taps, int decim);
void    sdr_fir_real_destroy(sdr_fir_real *f);
double  sdr_fir_real_process(sdr_fir_real *f, double in);

int sdr_fir_lowpass(double *taps, int n_taps, double cutoff_hz, double fs, int window);

/* ---- FM discriminator ---- */

typedef struct {
    sdr_cpx prev;
    int has_prev;
} sdr_fm_disc;

void   sdr_fm_disc_init(sdr_fm_disc *d);
double sdr_fm_disc_process(sdr_fm_disc *d, sdr_cpx in);

/* ---- De-emphasis (1-pole IIR low-pass) ---- */

typedef struct {
    double alpha;
    double y;
} sdr_deemph;

void   sdr_deemph_init(sdr_deemph *d, double tau_seconds, double fs);
double sdr_deemph_process(sdr_deemph *d, double x);

/* ---- FM demodulator pipeline ---- */

typedef struct {
    sdr_nco       nco;
    sdr_fir_cpx   fir1;
    sdr_fm_disc   disc;
    sdr_fir_real  fir2;
    sdr_deemph    deemph;

    double fs_in;
    double fs_intermediate;
    double fs_audio;
    double deviation_hz;
    double audio_gain;
    int deemph_us;
} sdr_fm;

int  sdr_fm_init(sdr_fm *fm, double fs_in, double audio_rate,
                 double station_offset_hz, double deviation_hz,
                 int deemph_us_50_or_75);
void sdr_fm_destroy(sdr_fm *fm);
int  sdr_fm_process(sdr_fm *fm, const sdr_cpx *in, int n_in,
                    double *out, int max_out);

/* ---- IQ file I/O (int16 I/Q interleaved) ---- */

typedef struct {
    FILE *fp;
    long n_samples;
} sdr_iq_file;

int sdr_iq_open(sdr_iq_file *f, const char *path, const char *mode);
int sdr_iq_read(sdr_iq_file *f, sdr_cpx *out, int n);
int sdr_iq_write(sdr_iq_file *f, const sdr_cpx *in, int n);
int sdr_iq_close(sdr_iq_file *f);

/* ---- WAV file writer (16-bit PCM) ---- */

typedef struct {
    FILE *fp;
    int sample_rate;
    int n_channels;
    long n_samples;
} sdr_wav;

int sdr_wav_open(sdr_wav *w, const char *path, int sample_rate, int n_channels);
int sdr_wav_write(sdr_wav *w, const double *samples, int n);
int sdr_wav_close(sdr_wav *w);

/* ---- Synthetic FM signal generator ---- */

typedef struct {
    double fs;
    double carrier_offset_hz;
    double dev_hz;
    double audio_freq_hz;
    double phase;
    double mod_integrator;
} sdr_fm_gen;

void sdr_fm_gen_init(sdr_fm_gen *g, double fs, double carrier_offset_hz,
                     double dev_hz, double audio_freq_hz);
void sdr_fm_gen_process(sdr_fm_gen *g, sdr_cpx *out, int n);

#ifdef __cplusplus
}
#endif

#endif /* SDR_H */
