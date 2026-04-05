/*
 * proba_xai.c — probatio provisoris xAI
 */

#include "oraculum.h"

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    const char *clavis = getenv("XAI_API_KEY");
    if (!clavis || !*clavis) {
        printf("admonitio: XAI_API_KEY deest — proba omissa\n");
        return 0;
    }

    if (oraculum_initia() < 0) {
        fprintf(stderr, "oraculum_initia defecit\n");
        return 1;
    }

    printf("=== proba_xai ===\n\n");

    char *responsum = NULL;
    int rc = oraculum_roga(
        "xai/grok-3-mini", NULL,
        "dic 'salve' et nihil aliud", &responsum
    );

    if (rc < 0)
        printf("erratum: %s\n", responsum ? responsum : "ignotum");
    else
        printf("responsum: %s\n", responsum ? responsum : "(vacuum)");

    free(responsum);
    oraculum_fini();

    printf("\n=== proba finita ===\n");
    return rc < 0 ? 1 : 0;
}
