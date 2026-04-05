/*
 * fare.c — imperativum oraculi
 *
 * Usus: ./fare [-m sapientum] [-s instructiones] rogatum ...
 *
 * exempla:
 *   ./fare -m openai/gpt-5.4+high dic mihi fabulam
 *   ./fare -m anthropic/claude-sonnet-4-6 -s "responde Latine" salve
 */

#include "oraculum.h"

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

int main(int argc, char **argv)
{
    const char *sapientum = NULL;
    const char *instructiones = NULL;
    int argi = 1;

    while (argi < argc && argv[argi][0] == '-') {
        if (strcmp(argv[argi], "-m") == 0 && argi + 1 < argc) {
            sapientum = argv[++argi];
            argi++;
        } else if (strcmp(argv[argi], "-s") == 0 && argi + 1 < argc) {
            instructiones = argv[++argi];
            argi++;
        } else {
            break;
        }
    }

    if (argi >= argc) {
        fprintf(
            stderr,
            "usus: %s [-m sapientum] [-s instructiones] rogatum ...\n"
            "\n"
            "  -m forma: [provisor/]sapientum[+effort]\n"
            "     provisores: openai, anthropic, xai\n"
            "     effort: low, medium, high\n"
            "\n"
            "exempla:\n"
            "  %s -m gpt-5.4 salve\n"
            "  %s -m openai/gpt-5.4+high -s 'responde Latine' quid est\n"
            "  %s -m anthropic/claude-sonnet-4-6 dic fabulam\n",
            argv[0], argv[0], argv[0], argv[0]
        );
        return 1;
    }

    char *rogatum = coniunge(argc, argv, argi);
    if (!rogatum) {
        fprintf(stderr, "memoria defecit\n");
        return 1;
    }

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
}
