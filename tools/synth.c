#include "sdr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>

static void usage(void) {
    fprintf(stderr,
        "synth - generate a synthetic FM-modulated IQ file\n"
        "\n"
        "usage: synth -o OUT.iq -r RATE -d DEVIATION -a AUDIO_FREQ [-c CARRIER_OFFSET]\n"
        "                [-n N_SAMPLES]\n"
        "\n"
        "  -o PATH        output IQ file (int16 I/Q interleaved)\n"
        "  -r HZ          IQ sample rate (e.g. 2400000)\n"
        "  -d HZ          FM deviation in Hz (e.g. 25000 or 75000)\n"
        "  -a HZ          audio sine frequency (e.g. 1000)\n"
        "  -c HZ          carrier offset from DC in Hz (default 0)\n"
        "  -n N           number of IQ samples (default 240000 = 0.1 s at 2.4 MS/s)\n"
    );
}

int main(int argc, char **argv) {
    const char *out = NULL;
    double rate = 0.0, dev = 0.0, audio = 0.0, carrier = 0.0;
    long n_samples = 240000;

    int i = 1;
    while (i < argc) {
        if (!strcmp(argv[i], "-o") && i + 1 < argc) {
            out = argv[++i];
        } else if (!strcmp(argv[i], "-r") && i + 1 < argc) {
            rate = atof(argv[++i]);
        } else if (!strcmp(argv[i], "-d") && i + 1 < argc) {
            dev = atof(argv[++i]);
        } else if (!strcmp(argv[i], "-a") && i + 1 < argc) {
            audio = atof(argv[++i]);
        } else if (!strcmp(argv[i], "-c") && i + 1 < argc) {
            carrier = atof(argv[++i]);
        } else if (!strcmp(argv[i], "-n") && i + 1 < argc) {
            n_samples = atol(argv[++i]);
        } else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            usage();
            return 0;
        } else {
            fprintf(stderr, "unknown arg: %s\n", argv[i]);
            usage();
            return 2;
        }
        i++;
    }
    if (!out || rate <= 0.0 || dev <= 0.0 || audio <= 0.0 || n_samples <= 0) {
        usage();
        return 2;
    }

    sdr_fm_gen g;
    sdr_fm_gen_init(&g, rate, carrier, dev, audio);

    sdr_iq_file f;
    if (sdr_iq_open(&f, out, "wb") != 0) {
        fprintf(stderr, "cannot open %s: %s\n", out, strerror(errno));
        return 1;
    }

    sdr_cpx buf[4096];
    long written = 0;
    while (written < n_samples) {
        int want = (int)(n_samples - written);
        if (want > 4096) want = 4096;
        sdr_fm_gen_process(&g, buf, want);
        if (sdr_iq_write(&f, buf, want) != want) {
            fprintf(stderr, "write error\n");
            sdr_iq_close(&f);
            return 1;
        }
        written += want;
    }
    sdr_iq_close(&f);
    fprintf(stderr, "wrote %ld IQ samples (%.3f s) to %s\n",
            n_samples, (double)n_samples / rate, out);
    return 0;
}
