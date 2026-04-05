/*
 * proba_fictus.c — probatio provisoris ficti
 */

#include "oraculum.h"

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    if (oraculum_initia() < 0) {
        fprintf(stderr, "oraculum_initia defecit\n");
        return 1;
    }

    printf("=== proba_fictus ===\n\n");

    for (int i = 0; i < 5; i++) {
        char *responsum = NULL;
        int rc = oraculum_roga(
            "fictus/probatio", NULL,
            "dic aliquid", &responsum
        );

        if (rc < 0)
            printf("erratum: %s\n", responsum ? responsum : "ignotum");
        else
            printf("responsum %d: %s\n", i + 1, responsum ? responsum : "(vacuum)");

        free(responsum);
    }

    oraculum_fini();

    printf("\n=== proba finita ===\n");
    return 0;
}
