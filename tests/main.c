#include "test.h"

#include <stdio.h>

int g_tests_failed = 0;

int main(void) {
    fprintf(stderr, "=== sdr test suite ===\n");

    int (*tests[])(void) = {
        test_complex,
        test_nco,
        test_filter,
        test_disc,
        test_deemph,
        test_iq_io,
        test_wav,
        test_siggen,
        test_fm_roundtrip,
    };
    const char *names[] = {
        "complex arithmetic",
        "NCO",
        "FIR filter design + decimation",
        "FM discriminator",
        "de-emphasis",
        "IQ file I/O",
        "WAV writer",
        "synthetic FM signal generator",
        "FM round-trip end-to-end",
    };
    int n = sizeof(tests) / sizeof(tests[0]);
    for (int i = 0; i < n; i++) {
        fprintf(stderr, "[%s]\n", names[i]);
        (void)tests[i]();
    }
    fprintf(stderr, "=========================\n");
    if (g_tests_failed == 0) {
        fprintf(stderr, "All tests PASSED\n");
        return 0;
    }
    fprintf(stderr, "FAILED: %d test(s) reported failures\n", g_tests_failed);
    return 1;
}
