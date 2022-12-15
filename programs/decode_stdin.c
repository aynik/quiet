#ifdef QUIET_DEBUG
#define DEBUG_OFDMFLEXFRAMESYNC 1
#define DEBUG_FLEXFRAMESYNC 1
#endif
#include "quiet.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

const int sample_rate = 44100;

float freq2rad(float freq) { return freq * 2 * M_PI; }

void recv_all(quiet_decoder *d, uint8_t *buf,
              size_t bufsize, FILE *payload) {
    for (;;) {
        ssize_t read = quiet_decoder_recv(d, buf, bufsize);
        if (read < 0) {
            break;
        }
        fwrite(buf, 1, read, payload);
    }
}

int decode_stdin(FILE *payload, quiet_decoder_options *opt) {
    quiet_decoder *d = quiet_decoder_create(opt, sample_rate);

    size_t wantread = 16384;
    quiet_sample_t *samplebuf = malloc(wantread * sizeof(quiet_sample_t));
    if (samplebuf == NULL) {
        return 1;
    }
    bool done = false;
    size_t bufsize = 1 << 13;
    uint8_t *buf = malloc(bufsize);
    while (!done) {
        size_t nread = fread(samplebuf, sizeof(quiet_sample_t), wantread, stdin);

        if (nread == 0) {
            break;
        } else if (nread < wantread) {
            done = true;
        }

        quiet_decoder_consume(d, samplebuf, nread);
        recv_all(d, buf, bufsize, payload);
    }

    quiet_decoder_flush(d);
    recv_all(d, buf, bufsize, payload);

    free(samplebuf);
    free(buf);
    quiet_decoder_destroy(d);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2 || argc > 4) {
        printf("usage: decode_file <profilename> [<output_destination>]\n");
        exit(1);
    }

    FILE *output;
    if ((argc == 2) || strncmp(argv[2], "-", 2) == 0) {
        output = stdout;
    } else {
        output = fopen(argv[2], "wb");
        if (!output) {
            fprintf(stderr, "failed to open %s: ", argv[2]);
            perror(NULL);
            exit(1);
        }
    }

    quiet_decoder_options *decodeopt =
        quiet_decoder_profile_filename(QUIET_PROFILES_LOCATION, argv[1]);

    if (!decodeopt) {
        printf("failed to read profile %s from %s\n", argv[1], QUIET_PROFILES_LOCATION);
        exit(1);
    }

#ifdef QUIET_DEBUG
    decodeopt->is_debug = true;
#endif

    decode_stdin(output, decodeopt);

    fclose(output);
    free(decodeopt);

    return 0;
}
