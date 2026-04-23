/*
 * fare.c — imperativum oraculi
 *
 * Usus: ./fare [-m sapientum] [-s instructiones] [-f file]... [-r file | rogatum...]
 *
 * exempla:
 *   ./fare -m openai/gpt-5.4+high dic mihi fabulam
 *   ./fare -m anthropic/claude-sonnet-4-6 -s "responde Latine" salve
 *   ./fare -f notae.txt quid dicit hoc scriptum
 *   ./fare -r rogatum.txt -f contextus.txt
 */

#include "oraculum.h"
#include "utilia.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static char *coniunge(int argc, char **argv, int ab)
{
    size_t lon = 0;
    for (int i = ab; i < argc; i++)
        lon += strlen(argv[i]) + 1;
    if (lon == 0)
        return strdup("");

    char *buf = malloc(lon);
    if (!buf)
        return NULL;
    char *p = buf;
    for (int i = ab; i < argc; i++) {
        if (i > ab)
            *p++ = ' ';
        size_t n = strlen(argv[i]);
        memcpy(p, argv[i], n);
        p += n;
    }
    *p = '\0';
    return buf;
}

static char *lege_fasciculum(const char *semita, size_t *lon_ex)
{
    FILE *fl = fopen(semita, "rb");
    if (!fl)
        return NULL;
    if (fseek(fl, 0, SEEK_END) != 0) {
        fclose(fl);
        return NULL;
    }
    long lon = ftell(fl);
    if (lon < 0) {
        fclose(fl);
        return NULL;
    }
    rewind(fl);
    char *buf = malloc((size_t)lon + 1);
    if (!buf) {
        fclose(fl);
        return NULL;
    }
    size_t lectum = fread(buf, 1, (size_t)lon, fl);
    fclose(fl);
    buf[lectum] = '\0';
    if (lon_ex)
        *lon_ex = lectum;
    return buf;
}

static int adiunge(char **basis, size_t *lon, size_t *cap, const char *src, size_t n)
{
    if (*lon + n + 1 > *cap) {
        size_t novum = *cap ? *cap * 2 : 256;
        while (novum < *lon + n + 1)
            novum *= 2;
        char *t = realloc(*basis, novum);
        if (!t)
            return -1;
        *basis = t;
        *cap   = novum;
    }
    memcpy(*basis + *lon, src, n);
    *lon += n;
    (*basis)[*lon] = '\0';
    return 0;
}

static void usus(FILE *fl, const char *nomen)
{
    fprintf(
        fl,
        "usus: %s [-m sapientum] [-s instructiones] [-f file]... [-r file | rogatum...]\n"
        "\n"
        "  -m, --sapientum     forma: [provisor/]sapientum[+effort]\n"
        "                      provisores: openai, anthropic, xai\n"
        "                      effort: low, medium, high\n"
        "  -s, --instructiones instructiones systematis\n"
        "  -f, --file FILE     fasciculum ut appendicem adiunge (repeti potest)\n"
        "  -r, --rogatum FILE  rogatum ex fasciculo lege (loco argumentorum)\n"
        "  -h, --help          hoc auxilium monstra\n"
        "\n"
        "exempla:\n"
        "  %s -m gpt-5.4 salve\n"
        "  %s -m openai/gpt-5.4+high -s 'responde Latine' quid est\n"
        "  %s -f notae.txt -f contextus.txt quid dicunt\n"
        "  %s -r rogatum.txt -f data.csv\n",
        nomen, nomen, nomen, nomen, nomen
    );
}

int main(int argc, char **argv)
{
    const char *sapientum = NULL;
    const char *instructiones = NULL;
    const char *rogatum_fasc = NULL;
    const char **fasciculi = NULL;
    int fasciculi_n = 0;
    int argi = 1;
    int rc_exit = 1;

    fasciculi = calloc((size_t)argc, sizeof(*fasciculi));
    if (!fasciculi) {
        fprintf(stderr, "memoria defecit\n");
        return 1;
    }

    while (argi < argc && argv[argi][0] == '-' && argv[argi][1] != '\0') {
        const char *arg = argv[argi];
        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            usus(stdout, argv[0]);
            free(fasciculi);
            return 0;
        } else if (strcmp(arg, "-m") == 0 || strcmp(arg, "--sapientum") == 0) {
            if (argi + 1 >= argc) {
                fprintf(stderr, "%s: %s postulat argumentum\n", argv[0], arg);
                goto err_usus;
            }
            sapientum = argv[++argi];
            argi++;
        } else if (strcmp(arg, "-s") == 0 || strcmp(arg, "--instructiones") == 0) {
            if (argi + 1 >= argc) {
                fprintf(stderr, "%s: %s postulat argumentum\n", argv[0], arg);
                goto err_usus;
            }
            instructiones = argv[++argi];
            argi++;
        } else if (strcmp(arg, "-f") == 0 || strcmp(arg, "--file") == 0) {
            if (argi + 1 >= argc) {
                fprintf(stderr, "%s: %s postulat argumentum\n", argv[0], arg);
                goto err_usus;
            }
            fasciculi[fasciculi_n++] = argv[++argi];
            argi++;
        } else if (strcmp(arg, "-r") == 0 || strcmp(arg, "--rogatum") == 0) {
            if (argi + 1 >= argc) {
                fprintf(stderr, "%s: %s postulat argumentum\n", argv[0], arg);
                goto err_usus;
            }
            rogatum_fasc = argv[++argi];
            argi++;
        } else if (strcmp(arg, "--") == 0) {
            argi++;
            break;
        } else {
            fprintf(stderr, "%s: optio ignota: %s\n", argv[0], arg);
            goto err_usus;
        }
    }

    if (rogatum_fasc && argi < argc) {
        fprintf(stderr, "%s: cum -r data sunt, argumenta rogati non admittuntur\n", argv[0]);
        goto err_usus;
    }
    if (!rogatum_fasc && argi >= argc) {
        goto err_usus;
    }

    char  *rogatum = NULL;
    size_t rog_lon = 0;
    size_t rog_cap = 0;

    if (rogatum_fasc) {
        size_t n = 0;
        rogatum = lege_fasciculum(rogatum_fasc, &n);
        if (!rogatum) {
            fprintf(stderr, "%s: fasciculum legere non potui: %s\n", argv[0], rogatum_fasc);
            free(fasciculi);
            return 1;
        }
        rog_lon = n;
        rog_cap = n + 1;
    } else {
        rogatum = coniunge(argc, argv, argi);
        if (!rogatum) {
            fprintf(stderr, "memoria defecit\n");
            free(fasciculi);
            return 1;
        }
        rog_lon = strlen(rogatum);
        rog_cap = rog_lon + 1;
    }

    for (int i = 0; i < fasciculi_n; i++) {
        size_t n = 0;
        char  *contentum = lege_fasciculum(fasciculi[i], &n);
        if (!contentum) {
            fprintf(stderr, "%s: fasciculum legere non potui: %s\n", argv[0], fasciculi[i]);
            free(rogatum);
            free(fasciculi);
            return 1;
        }
        char caput[1024];
        int  cn = snprintf(caput, sizeof(caput),
                           "\n\n--- appendix: %s ---\n", fasciculi[i]);
        if (cn < 0 || cn >= (int)sizeof(caput))
            cn = (int)strlen(caput);
        int ok = adiunge(&rogatum, &rog_lon, &rog_cap, caput, (size_t)cn) == 0
              && adiunge(&rogatum, &rog_lon, &rog_cap, contentum, n) == 0;
        free(contentum);
        if (!ok) {
            fprintf(stderr, "memoria defecit\n");
            free(rogatum);
            free(fasciculi);
            return 1;
        }
    }

    free(fasciculi);
    fasciculi = NULL;

    /* purga UTF-8 invalidum et controllos ante submissionem */
    size_t rog_pre = rog_lon;
    rog_lon        = utf8_mundus(rogatum, rog_cap, rogatum);
    if (rog_lon != rog_pre)
        fprintf(stderr, "monitum: %zu bytes invalidos e rogato purgavi\n",
                rog_pre - rog_lon);

    srand((unsigned)time(NULL));
    oraculum_initia();

    char *resp = NULL;
    int rc     = oraculum_roga(sapientum, instructiones, rogatum, &resp);

    if (rc == 0 && resp)
        printf("%s\n", resp);
    else
        fprintf(stderr, "error: %s\n", resp ? resp : "ignotus");

    free(resp);
    free(rogatum);
    oraculum_fini();

    return rc == 0 ? 0 : 1;

err_usus:
    usus(stderr, argv[0]);
    free(fasciculi);
    return rc_exit;
}
