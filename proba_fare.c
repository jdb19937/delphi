/*
 * proba_fare.c — probatio imperativi fare
 *
 * Probat fare per provisorem fictum (sine clavibus API).
 * Si claves API adsunt, etiam provisores veros probat.
 * Contenta responsi inspiciuntur ubi expectata nota sunt.
 */

#include "oraculum.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* curre fare, cape exitum in tubo, confirma exitum et contenta */
static int curre(
    const char *descriptio, char *const argv[],
    const char *expectatum
) {
    printf("  %s: ", descriptio);
    fflush(stdout);

    int tubi[2];
    if (pipe(tubi) < 0) {
        printf("pipe defecit\n");
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        printf("fork defecit\n");
        close(tubi[0]);
        close(tubi[1]);
        return 1;
    }
    if (pid == 0) {
        close(tubi[0]);
        dup2(tubi[1], STDOUT_FILENO);
        close(tubi[1]);
        execv("./fare", argv);
        _exit(127);
    }

    close(tubi[1]);

    char buf[4096];
    size_t lon = 0;
    ssize_t n;
    while ((n = read(tubi[0], buf + lon, sizeof(buf) - lon - 1)) > 0)
        lon += (size_t)n;
    buf[lon] = '\0';
    close(tubi[0]);

    int status;
    waitpid(pid, &status, 0);

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        printf(
            "defecit (status %d)\n",
            WIFEXITED(status) ? WEXITSTATUS(status) : -1
        );
        return 1;
    }

    if (expectatum && !strcasestr(buf, expectatum)) {
        printf(
            "defecit — '%s' non inventum in responso: %s\n",
            expectatum, buf
        );
        return 1;
    }

    printf("successit\n");
    return 0;
}

static int curre_expecta_erratum(const char *descriptio, char *const argv[])
{
    printf("  %s: ", descriptio);
    fflush(stdout);

    pid_t pid = fork();
    if (pid < 0) {
        printf("fork defecit\n");
        return 1;
    }
    if (pid == 0) {
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);
        execv("./fare", argv);
        _exit(127);
    }

    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        printf("successit (erratum expectatum)\n");
        return 0;
    }
    printf("defecit (successus inexpectatus)\n");
    return 1;
}

int main(void)
{
    int errores = 0;

    printf("=== proba_fare ===\n\n");

    /* --- probationes sine clavibus API --- */

    printf("sine clavibus:\n");

    char *argv_vacuus[] = { "./fare", NULL };
    errores += curre_expecta_erratum("sine argumentis", argv_vacuus);

    char *argv_fictus[] = {
        "./fare", "-m", "fictus/probatio", "dic", "aliquid", NULL
    };
    errores += curre("provisor fictus", argv_fictus, NULL);

    char *argv_inst[] = {
        "./fare", "-m", "fictus/probatio",
        "-s", "responde breviter", "salve", NULL
    };
    errores += curre("fictus cum instructionibus", argv_inst, NULL);

    /* --- probationes cum clavibus API --- */

    const char *openai    = getenv("OPENAI_API_KEY");
    const char *anthropic = getenv("ANTHROPIC_API_KEY");
    const char *xai       = getenv("XAI_API_KEY");

    if (openai && *openai) {
        printf("\nOpenAI:\n");
        char *argv_oa[] = {
            "./fare", "-m", "openai/gpt-5.4-mini",
            "dic 'salve' et nihil aliud", NULL
        };
        errores += curre("gpt-5.4-mini", argv_oa, "salve");
    } else {
        printf("\nadmonitio: OPENAI_API_KEY deest — probae omissae\n");
    }

    if (anthropic && *anthropic) {
        printf("\nAnthropic:\n");
        char *argv_an[] = {
            "./fare", "-m", "anthropic/claude-haiku-4-5",
            "dic 'salve' et nihil aliud", NULL
        };
        errores += curre("claude-haiku-4-5", argv_an, "salve");
    } else {
        printf("\nadmonitio: ANTHROPIC_API_KEY deest — probae omissae\n");
    }

    if (xai && *xai) {
        printf("\nxAI:\n");
        char *argv_xai[] = {
            "./fare", "-m", "xai/grok-4-mini",
            "dic 'salve' et nihil aliud", NULL
        };
        errores += curre("grok-3-mini", argv_xai, "salve");
    } else {
        printf("\nadmonitio: XAI_API_KEY deest — probae omissae\n");
    }

    printf("\n=== proba finita: %d errores ===\n", errores);
    return errores > 0 ? 1 : 0;
}
