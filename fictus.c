/*
 * fictus.c — provisor fictus pro probatione
 *
 * Non HTTP rogat — reddit sententias Latinas fictas.
 * sapientum: fictus/probatio
 */

#include "provisor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --- provisor interfacies --- */

static int para(
    const char *nomen, const char *conatus,
    const char *clavis_api,
    const char *instructiones, const char *rogatum,
    char **corpus, struct crispus_slist **capita
) {
    (void)nomen;
    (void)conatus;
    (void)clavis_api;
    (void)instructiones;
    (void)capita;

    *corpus = strdup(rogatum);
    *capita = NULL;
    return 0;
}

static const char *const sententiae[] = {
    "Quidquid Latine dictum sit, altum videtur.",
    "Fiesta lente!",
    "Alea iacta est.",
    "Cogito, ergo sum.",
    "Dum spiro, spero.",
    "Semper und err, sub und err.",
    "Carpe diem.",
    "Par coquorum faces ad astra feret!",
    "Venti, vidi, vinci.",
    "Memento mori.",
    "Sic transit gloria mundi.",
    "Errare humanum est.",
    "Ad astra per aspera.",
};
#define SENTENTIAE_NUM (int)(sizeof(sententiae) / sizeof(sententiae[0]))

static char *extrahe(const char *ison)
{
    (void)ison;
    return strdup(sententiae[rand() % SENTENTIAE_NUM]);
}

static void signa(
    const char *ison, long *accepta, long *recondita,
    long *emissa, long *cogitata
) {
    (void)ison;
    *accepta   = 0;
    *recondita = 0;
    *emissa    = 0;
    *cogitata  = 0;
}

const provisor_t provisor_fictus = {
    .nomen      = "fictus",
    .clavis_env = "",
    .finis_url  = "",
    .para       = para,
    .extrahe    = extrahe,
    .signa      = signa
};
