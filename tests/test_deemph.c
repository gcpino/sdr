#include "sdr.h"
#include "test.h"

int test_deemph(void) {
    sdr_deemph d;
    sdr_deemph_init(&d, 75e-6, 48000.0);

    BA_TEST_ASSERT(d.alpha > 0.7 && d.alpha < 0.8);

    for (int i = 0; i < 10000; i++) (void)sdr_deemph_process(&d, 1.0);
    BA_TEST_ASSERT_NEAR(d.y, 1.0, 1e-3);

    sdr_deemph d2;
    sdr_deemph_init(&d2, 75e-6, 48000.0);
    double max_low = 0.0;
    for (int i = 0; i < 48000; i++) {
        double s = sin(2.0 * M_PI * 100.0 * (double)i / 48000.0);
        double y = sdr_deemph_process(&d2, s);
        if (fabs(y) > max_low) max_low = fabs(y);
    }
    BA_TEST_ASSERT(max_low > 0.9);

    sdr_deemph d3;
    sdr_deemph_init(&d3, 75e-6, 48000.0);
    double max_high = 0.0;
    for (int i = 0; i < 48000; i++) {
        double s = sin(2.0 * M_PI * 10000.0 * (double)i / 48000.0);
        double y = sdr_deemph_process(&d3, s);
        if (fabs(y) > max_high) max_high = fabs(y);
    }
    BA_TEST_ASSERT(max_high < max_low);

    sdr_deemph d4;
    sdr_deemph_init(&d4, 50e-6, 48000.0);
    BA_TEST_ASSERT(d4.alpha < d.alpha);

    return 0;
}
