#include "sdr.h"
#include "test.h"

int test_siggen(void) {
    sdr_fm_gen g;
    sdr_fm_gen_init(&g, 2400000.0, 100000.0, 25000.0, 1000.0);

    sdr_cpx buf[2400];
    sdr_fm_gen_process(&g, buf, 2400);

    double energy = 0.0;
    for (int i = 0; i < 2400; i++) {
        energy += buf[i].re * buf[i].re + buf[i].im * buf[i].im;
    }
    BA_TEST_ASSERT_NEAR(energy, 2400.0, 1.0);

    int zero_crossings = 0;
    double prev_re = buf[0].re;
    for (int i = 1; i < 2400; i++) {
        if (prev_re >= 0.0 && buf[i].re < 0.0) zero_crossings++;
        prev_re = buf[i].re;
    }
    BA_TEST_ASSERT(zero_crossings > 80 && zero_crossings < 110);

    return 0;
}
