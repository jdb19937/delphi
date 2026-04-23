/*
 * sermo.c — chat interactivum cum oraculo per sessionem
 *
 * Usus: ./sermo [-m sapientum] [-s instructiones]
 *
 * Imperata intra sermonem:
 *   /exit, /quit    exi
 *   /reset          sessionem novam incipe (historia deletur)
 *   /numerus        scribe numerum turnorum in historia
 */

#include "oraculum.h"
#include "utilia.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static void usus(FILE *fl, const char *nomen)
{
    fprintf(
        fl,
        "usus: %s [-m sapientum] [-s instructiones]\n"
        "\n"
        "  -m, --sapientum     forma: [provisor/]sapientum[+effort]\n"
        "  -s, --instructiones instructiones systematis\n"
        "  -h, --help          hoc auxilium monstra\n"
        "\n"
        "imperata intra sermonem:\n"
        "  /exit, /quit    exi\n"
        "  /reset          novam sessionem incipe\n"
        "  /numerus        scribe numerum turnorum\n",
        nomen
    );
}

/* lege lineam e stdin; reddit 0 si EOF, 1 si linea, -1 si error */
static int lege_lineam(char **linea, size_t *cap)
{
    size_t lon = 0;
    for (;;) {
        if (lon + 2 > *cap) {
            size_t novum = *cap ? *cap * 2 : 256;
            char *t = realloc(*linea, novum);
            if (!t)
                return -1;
            *linea = t;
            *cap   = novum;
        }
        int c = fgetc(stdin);
        if (c == EOF)
            return lon == 0 ? 0 : 1;
        if (c == '\n') {
            (*linea)[lon] = '\0';
            return 1;
        }
        (*linea)[lon++] = (char)c;
    }
}

int main(int argc, char **argv)
{
    const char *sapientum     = NULL;
    const char *instructiones = NULL;
    int argi = 1;

    while (argi < argc && argv[argi][0] == '-' && argv[argi][1] != '\0') {
        const char *arg = argv[argi];
        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            usus(stdout, argv[0]);
            return 0;
        } else if (strcmp(arg, "-m") == 0 || strcmp(arg, "--sapientum") == 0) {
            if (argi + 1 >= argc) {
                fprintf(stderr, "%s: %s postulat argumentum\n", argv[0], arg);
                usus(stderr, argv[0]);
                return 1;
            }
            sapientum = argv[++argi];
            argi++;
        } else if (strcmp(arg, "-s") == 0 || strcmp(arg, "--instructiones") == 0) {
            if (argi + 1 >= argc) {
                fprintf(stderr, "%s: %s postulat argumentum\n", argv[0], arg);
                usus(stderr, argv[0]);
                return 1;
            }
            instructiones = argv[++argi];
            argi++;
        } else if (strcmp(arg, "--") == 0) {
            argi++;
            break;
        } else {
            fprintf(stderr, "%s: optio ignota: %s\n", argv[0], arg);
            usus(stderr, argv[0]);
            return 1;
        }
    }

    if (argi < argc) {
        fprintf(stderr, "%s: argumenta extranea\n", argv[0]);
        usus(stderr, argv[0]);
        return 1;
    }

    srand((unsigned)time(NULL));
    if (oraculum_initia() < 0) {
        fprintf(stderr, "oraculum_initia defecit\n");
        return 1;
    }

    oraculum_sessio_t *sessio = oraculum_sessio_novam(sapientum, instructiones);
    if (!sessio) {
        fprintf(stderr, "sessionem creare non potui\n");
        oraculum_fini();
        return 1;
    }

    fprintf(stderr, "sermo cum %s. exi cum /exit aut Ctrl-D.\n",
            oraculum_sapientum());

    char  *linea = NULL;
    size_t cap   = 0;
    int    rc_exit = 0;

    for (;;) {
        if (isatty(0))
            fputs("> ", stderr);
        int lr = lege_lineam(&linea, &cap);
        if (lr < 0) {
            fprintf(stderr, "error legendi\n");
            rc_exit = 1;
            break;
        }
        if (lr == 0) {
            if (isatty(0))
                fputc('\n', stderr);
            break;
        }
        if (linea[0] == '\0')
            continue;

        if (strcmp(linea, "/exit") == 0 || strcmp(linea, "/quit") == 0)
            break;
        if (strcmp(linea, "/reset") == 0) {
            oraculum_sessio_dimitte(sessio);
            sessio = oraculum_sessio_novam(sapientum, instructiones);
            if (!sessio) {
                fprintf(stderr, "sessionem creare non potui\n");
                rc_exit = 1;
                break;
            }
            fprintf(stderr, "[sessio nova]\n");
            continue;
        }
        if (strcmp(linea, "/numerus") == 0) {
            fprintf(stderr, "[%d turni]\n", oraculum_sessio_numerus(sessio));
            continue;
        }

        /* purga UTF-8 invalidum in loco */
        size_t lon = strlen(linea);
        size_t novum_lon = utf8_mundus(linea, cap, linea);
        if (novum_lon != lon)
            fprintf(stderr,
                    "monitum: %zu bytes invalidos purgavi\n", lon - novum_lon);

        char *resp = NULL;
        int rc = oraculum_sessio_roga(sessio, linea, &resp);
        if (rc == 0 && resp) {
            printf("%s\n", resp);
            fflush(stdout);
        } else {
            fprintf(stderr, "error oraculi\n");
        }
        free(resp);
    }

    free(linea);
    oraculum_sessio_dimitte(sessio);
    oraculum_fini();
    return rc_exit;
}
