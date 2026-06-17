#ifndef SDR_TEST_H
#define SDR_TEST_H

#include <stdio.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

extern int g_tests_failed;

#define BA_TEST_ASSERT(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "  FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        g_tests_failed++; \
        return 1; \
    } \
} while (0)

#define BA_TEST_ASSERT_NEAR(a, b, eps) do { \
    double _a = (double)(a); \
    double _b = (double)(b); \
    if (fabs(_a - _b) > (eps)) { \
        fprintf(stderr, "  FAIL: %s:%d: %.6f vs %.6f (eps=%.6f)\n", \
                __FILE__, __LINE__, _a, _b, (double)(eps)); \
        g_tests_failed++; \
        return 1; \
    } \
} while (0)

int test_complex(void);
int test_nco(void);
int test_filter(void);
int test_disc(void);
int test_deemph(void);
int test_iq_io(void);
int test_wav(void);
int test_siggen(void);
int test_fm_roundtrip(void);

#endif
