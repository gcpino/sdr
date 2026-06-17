#define _POSIX_C_SOURCE 200809L

#include "sdr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static void usage(void) {
    fprintf(stderr,
        "demod - demodulate an FM broadcast from an int16 I/Q file to a WAV\n"
        "\n"
        "usage: demod -i IN.iq -o OUT.wav -r RATE [-c CARRIER_OFFSET_HZ]\n"
        "                [-d DEVIATION_HZ] [-a AUDIO_RATE_HZ] [-e 50|75]\n"
        "\n"
        "  -i PATH        input IQ file (int16 I/Q interleaved)\n"
        "  -o PATH        output WAV file (16-bit PCM, mono)\n"
        "  -r HZ          IQ sample rate (e.g. 2400000)\n"
        "  -c HZ          carrier offset from DC in Hz (default 0)\n"
        "  -d HZ          FM deviation in Hz (default 75000, US broadcast)\n"
        "  -a HZ          audio sample rate (default 48000)\n"
        "  -e US          de-emphasis time constant: 50 (EU) or 75 (US, default)\n"
    );
}

int main(int argc, char **argv) {
    const char *in_path = NULL, *out_path = NULL;
    double rate = 0.0, carrier = 0.0, dev = 75000.0, audio_rate = 48000.0;
    int deemph = 75;

    int i = 1;
    while (i < argc) {
        if (!strcmp(argv[i], "-i") && i + 1 < argc) {
            in_path = argv[++i];
        } else if (!strcmp(argv[i], "-o") && i + 1 < argc) {
            out_path = argv[++i];
        } else if (!strcmp(argv[i], "-r") && i + 1 < argc) {
            rate = atof(argv[++i]);
        } else if (!strcmp(argv[i], "-c") && i + 1 < argc) {
            carrier = atof(argv[++i]);
        } else if (!strcmp(argv[i], "-d") && i + 1 < argc) {
            dev = atof(argv[++i]);
        } else if (!strcmp(argv[i], "-a") && i + 1 < argc) {
            audio_rate = atof(argv[++i]);
        } else if (!strcmp(argv[i], "-e") && i + 1 < argc) {
            deemph = atoi(argv[++i]);
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
    if (!in_path || !out_path || rate <= 0.0) {
        usage();
        return 2;
    }

    sdr_fm fm;
    if (sdr_fm_init(&fm, rate, audio_rate, carrier, dev, deemph) != 0) {
        fprintf(stderr, "sdr_fm_init failed (check rates and parameters)\n");
        return 1;
    }

    sdr_iq_file in;
    if (sdr_iq_open(&in, in_path, "rb") != 0) {
        fprintf(stderr, "cannot open %s: %s\n", in_path, strerror(errno));
        sdr_fm_destroy(&fm);
        return 1;
    }
    sdr_wav out;
    if (sdr_wav_open(&out, out_path, (int)audio_rate, 1) != 0) {
        fprintf(stderr, "cannot open %s: %s\n", out_path, strerror(errno));
        sdr_iq_close(&in);
        sdr_fm_destroy(&fm);
        return 1;
    }

    sdr_cpx ibuf[4096];
    double obuf[4096];
    long total_in = 0, total_out = 0;
    for (;;) {
        int got = sdr_iq_read(&in, ibuf, 4096);
        if (got <= 0) break;
        int produced = sdr_fm_process(&fm, ibuf, got, obuf, 4096);
        if (produced > 0) {
            if (sdr_wav_write(&out, obuf, produced) != produced) {
                fprintf(stderr, "wav write error\n");
                break;
            }
            total_out += produced;
        }
        total_in += got;
    }
    sdr_iq_close(&in);
    sdr_wav_close(&out);
    sdr_fm_destroy(&fm);
    fprintf(stderr, "demodulated %ld IQ samples -> %ld audio samples (%.3f s)\n",
            total_in, total_out, (double)total_out / audio_rate);
    return 0;
}
