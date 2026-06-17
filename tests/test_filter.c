#include "sdr.h"
#include "test.h"

int test_filter(void) {
    double taps[63];
    int r = sdr_fir_lowpass(taps, 63, 1000.0, 48000.0, 1);
    BA_TEST_ASSERT(r == 0);

    double sum = 0.0;
    for (int i = 0; i < 63; i++) sum += taps[i];
    BA_TEST_ASSERT_NEAR(sum, 1.0, 1e-9);

    sdr_fir_real f;
    r = sdr_fir_real_init(&f, taps, 63, 1);
    BA_TEST_ASSERT(r == 0);
    double max_dc = 0.0;
    for (int i = 0; i < 1000; i++) {
        double y = sdr_fir_real_process(&f, 1.0);
        if (fabs(y) > max_dc) max_dc = fabs(y);
    }
    BA_TEST_ASSERT_NEAR(max_dc, 1.0, 0.05);
    sdr_fir_real_destroy(&f);

    r = sdr_fir_lowpass(taps, 63, 1000.0, 48000.0, 1);
    BA_TEST_ASSERT(r == 0);
    sdr_fir_real f2;
    r = sdr_fir_real_init(&f2, taps, 63, 1);
    BA_TEST_ASSERT(r == 0);
    double max_high = 0.0;
    for (int i = 0; i < 48000; i++) {
        double s = sin(2.0 * M_PI * 20000.0 * (double)i / 48000.0);
        double y = sdr_fir_real_process(&f2, s);
        if (fabs(y) > max_high) max_high = fabs(y);
    }
    BA_TEST_ASSERT(max_high < 0.1);
    sdr_fir_real_destroy(&f2);

    sdr_fir_real f3;
    r = sdr_fir_real_init(&f3, taps, 63, 4);
    BA_TEST_ASSERT(r == 0);
    int zeros_seen = 0;
    for (int i = 0; i < 10000; i++) {
        double y = sdr_fir_real_process(&f3, (i == 0) ? 1.0 : 0.0);
        if (i > 0 && i < 100 && fabs(y) < 1e-9) zeros_seen++;
    }
    BA_TEST_ASSERT(zeros_seen >= 50);
    sdr_fir_real_destroy(&f3);

    return 0;
}
