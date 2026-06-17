#include "sdr.h"
#include "test.h"

int test_disc(void) {
    sdr_fm_disc d0;
    sdr_fm_disc_init(&d0);
    double dphi_zero = 0.0;
    for (int i = 0; i < 100; i++) {
        sdr_cpx s = {1.0, 0.0};
        dphi_zero += sdr_fm_disc_process(&d0, s);
    }
    BA_TEST_ASSERT_NEAR(dphi_zero, 0.0, 1e-9);

    sdr_fm_disc d2;
    sdr_fm_disc_init(&d2);
    double dphi_pos = 0.0;
    for (int i = 0; i < 100; i++) {
        sdr_cpx s = {cos(0.05 * (double)i), sin(0.05 * (double)i)};
        dphi_pos += sdr_fm_disc_process(&d2, s);
    }
    BA_TEST_ASSERT_NEAR(dphi_pos, 4.95, 0.01);

    sdr_fm_disc d3;
    sdr_fm_disc_init(&d3);
    double dphi_neg = 0.0;
    for (int i = 0; i < 100; i++) {
        sdr_cpx s = {cos(-0.05 * (double)i), sin(-0.05 * (double)i)};
        dphi_neg += sdr_fm_disc_process(&d3, s);
    }
    BA_TEST_ASSERT_NEAR(dphi_neg, -4.95, 0.01);

    return 0;
}
