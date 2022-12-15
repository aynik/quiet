#include "quiet.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

float freq2rad(float freq) { return freq * 2 * M_PI; }

const int sample_rate = 44100;

float normalize_freq(float freq, float sample_rate) {
    return freq2rad(freq / (float)(sample_rate));
}

int encode_stdout(FILE *payload, const quiet_encoder_options *opt) {
    quiet_encoder *e = quiet_encoder_create(opt, sample_rate);

    size_t block_len = 16384;
    uint8_t *readbuf = malloc(block_len * sizeof(uint8_t));
    size_t samplebuf_len = 16384;
    quiet_sample_t *samplebuf = malloc(samplebuf_len * sizeof(quiet_sample_t));
    bool done = false;
    if (readbuf == NULL) {
        return 1;
    }
    if (samplebuf == NULL) {
        return 1;
    }

    while (!done) {
        size_t nread = fread(readbuf, sizeof(uint8_t), block_len, payload);
        if (nread == 0) {
            break;
        } else if (nread < block_len) {
            done = true;
        }

        size_t frame_len = quiet_encoder_get_frame_len(e);
        for (size_t i = 0; i < nread; i += frame_len) {
            frame_len = (frame_len > (nread - i)) ? (nread - i) : frame_len;
            quiet_encoder_send(e, readbuf + i, frame_len);
        }

        ssize_t written = samplebuf_len;
        while (written == samplebuf_len) {
            written = quiet_encoder_emit(e, samplebuf, samplebuf_len);
            if (written > 0) {
                fwrite(samplebuf, sizeof(quiet_sample_t), written, stdout);
            }
        }
    }

    quiet_encoder_destroy(e);
    free(readbuf);
    free(samplebuf);
    fflush(stdout);
    fclose(stdout);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2 || argc > 4) {
        printf("usage: encode_file <profilename> [<input_source>]\n");
        exit(1);
    }

    FILE *input;
    if ((argc == 2) || strncmp(argv[2], "-", 2) == 0) {
        input = stdin;
    } else {
        input = fopen(argv[2], "rb");
        if (!input) {
            fprintf(stderr, "failed to open %s: ", argv[2]);
            perror(NULL);
            exit(1);
        }
    }

    quiet_encoder_options *encodeopt =
        quiet_encoder_profile_filename(QUIET_PROFILES_LOCATION, argv[1]);

    if (!encodeopt) {
        printf("failed to read profile %s from %s\n", argv[1], QUIET_PROFILES_LOCATION);
        exit(1);
    }

    encode_stdout(input, encodeopt);

    fclose(input);
    free(encodeopt);

    return 0;
}
