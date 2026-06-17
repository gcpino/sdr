#include "sdr.h"
#include "test.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int test_iq_io(void) {
    const char *path = "build/test_iq_tmp.bin";
    sdr_iq_file f;
    BA_TEST_ASSERT(sdr_iq_open(&f, path, "wb") == 0);

    sdr_cpx in[100];
    for (int i = 0; i < 100; i++) {
        in[i].re = (double)i / 100.0;
        in[i].im = -(double)i / 100.0;
    }
    BA_TEST_ASSERT(sdr_iq_write(&f, in, 100) == 100);
    sdr_iq_close(&f);

    BA_TEST_ASSERT(sdr_iq_open(&f, path, "rb") == 0);
    BA_TEST_ASSERT(f.n_samples == 100);

    sdr_cpx out[100];
    int got = sdr_iq_read(&f, out, 100);
    BA_TEST_ASSERT(got == 100);
    for (int i = 0; i < 100; i++) {
        BA_TEST_ASSERT_NEAR(out[i].re, (double)i / 100.0, 2.0 / 32768.0);
        BA_TEST_ASSERT_NEAR(out[i].im, -(double)i / 100.0, 2.0 / 32768.0);
    }
    sdr_iq_close(&f);
    remove(path);
    return 0;
}

int test_wav(void) {
    const char *path = "build/test_wav_tmp.wav";
    sdr_wav w;
    BA_TEST_ASSERT(sdr_wav_open(&w, path, 48000, 1) == 0);

    double samples[1000];
    for (int i = 0; i < 1000; i++) {
        samples[i] = sin(2.0 * M_PI * 440.0 * (double)i / 48000.0);
    }
    BA_TEST_ASSERT(sdr_wav_write(&w, samples, 1000) == 1000);
    sdr_wav_close(&w);

    FILE *fp = fopen(path, "rb");
    BA_TEST_ASSERT(fp != NULL);
    char riff[4];
    BA_TEST_ASSERT(fread(riff, 1, 4, fp) == 4);
    BA_TEST_ASSERT(memcmp(riff, "RIFF", 4) == 0);
    fseek(fp, 8, SEEK_SET);
    char wave[4];
    BA_TEST_ASSERT(fread(wave, 1, 4, fp) == 4);
    BA_TEST_ASSERT(memcmp(wave, "WAVE", 4) == 0);
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    BA_TEST_ASSERT(size == 44 + 2000);
    fclose(fp);
    remove(path);
    return 0;
}
