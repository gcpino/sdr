#include "sdr.h"
#include "test.h"

int test_complex(void) {
    sdr_cpx a = {1.0, 2.0};
    sdr_cpx b = {3.0, 4.0};

    sdr_cpx s = sdr_cpx_add(a, b);
    BA_TEST_ASSERT_NEAR(s.re, 4.0, 1e-12);
    BA_TEST_ASSERT_NEAR(s.im, 6.0, 1e-12);

    sdr_cpx d = sdr_cpx_sub(b, a);
    BA_TEST_ASSERT_NEAR(d.re, 2.0, 1e-12);
    BA_TEST_ASSERT_NEAR(d.im, 2.0, 1e-12);

    sdr_cpx m = sdr_cpx_mul(a, b);
    BA_TEST_ASSERT_NEAR(m.re, -5.0, 1e-12);
    BA_TEST_ASSERT_NEAR(m.im, 10.0, 1e-12);

    sdr_cpx sc = sdr_cpx_scale(a, 2.5);
    BA_TEST_ASSERT_NEAR(sc.re, 2.5, 1e-12);
    BA_TEST_ASSERT_NEAR(sc.im, 5.0, 1e-12);

    sdr_cpx p = sdr_cpx_from_polar(1.0, 0.0);
    BA_TEST_ASSERT_NEAR(p.re, 1.0, 1e-12);
    BA_TEST_ASSERT_NEAR(p.im, 0.0, 1e-12);

    p = sdr_cpx_from_polar(2.0, M_PI / 2.0);
    BA_TEST_ASSERT_NEAR(p.re, 0.0, 1e-12);
    BA_TEST_ASSERT_NEAR(p.im, 2.0, 1e-12);

    sdr_cpx r = {3.0, 4.0};
    BA_TEST_ASSERT_NEAR(sdr_cpx_abs(r), 5.0, 1e-12);
    BA_TEST_ASSERT_NEAR(sdr_cpx_arg(r), atan2(4.0, 3.0), 1e-12);

    return 0;
}
