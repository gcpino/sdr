#include "sdr.h"
#include "test.h"

int test_nco(void) {
    sdr_nco n;
    sdr_nco_init(&n, 1000.0, 48000.0);

    BA_TEST_ASSERT_NEAR(n.phase_inc, 2.0 * M_PI * 1000.0 / 48000.0, 1e-12);

    int n_zero_crossings = 0;
    double prev = 0.0;
    sdr_cpx s = {0.0, 0.0};
    for (int i = 0; i < 48000; i++) {
        prev = s.re;
        s = sdr_nco_step(&n);
        if (prev >= 0.0 && s.re < 0.0) n_zero_crossings++;
    }
    BA_TEST_ASSERT(n_zero_crossings >= 999 && n_zero_crossings <= 1001);

    sdr_nco_reset(&n);
    BA_TEST_ASSERT_NEAR(n.phase, 0.0, 1e-12);

    sdr_nco n2;
    sdr_nco_init(&n2, 0.0, 48000.0);
    sdr_cpx dc = sdr_nco_step(&n2);
    BA_TEST_ASSERT_NEAR(dc.re, 1.0, 1e-12);
    BA_TEST_ASSERT_NEAR(dc.im, 0.0, 1e-12);

    return 0;
}
