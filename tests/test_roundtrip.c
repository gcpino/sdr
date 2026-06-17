#include "sdr.h"
#include "test.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static double correlation(const double *a, const double *b, int n) {
    double ma = 0.0, mb = 0.0;
    for (int i = 0; i < n; i++) { ma += a[i]; mb += b[i]; }
    ma /= n; mb /= n;
    double c = 0.0, da = 0.0, db = 0.0;
    for (int i = 0; i < n; i++) {
        double xa = a[i] - ma;
        double xb = b[i] - mb;
        c += xa * xb;
        da += xa * xa;
        db += xb * xb;
    }
    if (da <= 0.0 || db <= 0.0) return 0.0;
    return c / sqrt(da * db);
}

int test_fm_roundtrip(void) {
    const double fs = 2400000.0;
    const double audio = 1000.0;
    const double dev = 25000.0;
    const int n_iq = 240000;
    const int n_audio_expected = n_iq / 50;

    sdr_fm_gen g;
    sdr_fm_gen_init(&g, fs, 0.0, dev, audio);

    sdr_cpx *iq = (sdr_cpx *)malloc(sizeof(sdr_cpx) * n_iq);
    sdr_fm_gen_process(&g, iq, n_iq);

    sdr_fm fm;
    BA_TEST_ASSERT(sdr_fm_init(&fm, fs, 48000.0, 0.0, dev, 75) == 0);

    double *out = (double *)malloc(sizeof(double) * (n_audio_expected + 1000));
    int produced = 0;
    int block = 4096;
    for (int off = 0; off < n_iq; off += block) {
        int n = block;
        if (off + n > n_iq) n = n_iq - off;
        int p = sdr_fm_process(&fm, iq + off, n, out + produced, n_audio_expected + 1000 - produced);
        if (p > 0) produced += p;
    }
    BA_TEST_ASSERT(produced > n_audio_expected - 100);

    int start = produced - 8000;
    if (start < 0) start = 0;
    int len = produced - start;
    if (len > 8000) len = 8000;

    double best_corr = 0.0;
    int best_lag = 0;
    double *ref = (double *)malloc(sizeof(double) * len);
    for (int lag = -30; lag <= 30; lag++) {
        int s = start + lag;
        if (s < 0) continue;
        if (s + len > produced) continue;
        for (int i = 0; i < len; i++) {
            ref[i] = sin(2.0 * M_PI * audio * (double)(s + i) / 48000.0);
        }
        double c = correlation(out + s, ref, len);
        if (c > best_corr) { best_corr = c; best_lag = lag; }
    }
    free(ref);
    fprintf(stderr, "  best time-domain correlation: %.4f (lag=%d)\n", best_corr, best_lag);

    int zc[10] = {0};
    int n_zc = 0;
    for (int i = 1; i < produced && n_zc < 10; i++) {
        if (out[i-1] < 0.0 && out[i] >= 0.0) {
            zc[n_zc++] = i;
        }
    }
    double measured_freq = 0.0;
    if (n_zc >= 2) {
        int per = zc[1] - zc[0];
        measured_freq = 48000.0 / per;
        fprintf(stderr, "  measured period: %d samples -> freq: %.1f Hz\n", per, measured_freq);
    }

    int s_start = 0;
    if (produced > 4000) s_start = produced - 4000;
    int s_len = produced - s_start;
    double a = 0.0, b = 0.0, e = 0.0;
    for (int i = 0; i < s_len; i++) {
        double phase = 2.0 * M_PI * audio * (double)i / 48000.0;
        double v = out[s_start + i];
        a += v * sin(phase);
        b += v * cos(phase);
        e += v * v;
    }
    double tone_mag = 2.0 * sqrt(a * a + b * b) / s_len;
    double tone_ratio = (tone_mag * tone_mag) / (e / s_len);
    fprintf(stderr, "  1 kHz tone magnitude: %.4f  tone/total ratio: %.4f\n",
            tone_mag, tone_ratio);

    BA_TEST_ASSERT(measured_freq > 990.0 && measured_freq < 1010.0);
    BA_TEST_ASSERT(tone_ratio > 0.7);

    free(out);
    free(iq);
    sdr_fm_destroy(&fm);
    return 0;
}
