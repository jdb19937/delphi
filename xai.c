/*
 * oracula/xai.c — provisor xAI (Chat Completions API)
 */

#include "provisor.h"
#include "ison.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int para(
    const char *nomen, const char *conatus,
    const char *clavis_api,
    const char *instructiones, const char *rogatum,
    char **corpus, struct crispus_slist **capita
) {
    (void)conatus;

    char *eff_user = ison_effuge(rogatum);
    if (!eff_user)
        return -1;

    char *eff_sys = NULL;
    if (instructiones) {
        eff_sys = ison_effuge(instructiones);
        if (!eff_sys) {
            free(eff_user);
            return -1;
        }
    }

    size_t mag = strlen(eff_user) + strlen(nomen) + 256;
    if (eff_sys)
        mag += strlen(eff_sys);

    char *buf = malloc(mag);
    if (!buf) {
        free(eff_user);
        free(eff_sys);
        return -1;
    }

    char *p = buf;
    p += sprintf(p, "{\"model\":\"%s\",\"messages\":[", nomen);
    if (eff_sys)
        p += sprintf(p, "{\"role\":\"system\",\"content\":\"%s\"},", eff_sys);
    p += sprintf(p, "{\"role\":\"user\",\"content\":\"%s\"}]}", eff_user);

    free(eff_user);
    free(eff_sys);

    char caput_auth[512];
    snprintf(
        caput_auth, sizeof(caput_auth),
        "Authorization: Bearer %s", clavis_api
    );

    struct crispus_slist *c = NULL;
    c = crispus_slist_adde(c, "Content-Type: application/json");
    c = crispus_slist_adde(c, caput_auth);

    *corpus = buf;
    *capita = c;
    return 0;
}

static char *extrahe(const char *ison)
{
    char *textus = ison_da_chordam(ison, "choices[0].message.content");
    if (textus)
        return textus;

    char *error = ison_da_chordam(ison, "error.message");
    if (error)
        return error;

    return strdup(ison);
}

static void signa(
    const char *ison, long *accepta, long *recondita,
    long *emissa, long *cogitata
) {
    *accepta   = ison_da_numerum(ison, "usage.prompt_tokens");
    *emissa    = ison_da_numerum(ison, "usage.completion_tokens");
    *recondita = 0;
    *cogitata  = ison_da_numerum(ison, "usage.completion_tokens_details.reasoning_tokens");
}

const provisor_t provisor_xai = {
    .nomen      = "xai",
    .clavis_env = "XAI_API_KEY",
    .finis_url  = "https://api.x.ai/v1/chat/completions",
    .para       = para,
    .extrahe    = extrahe,
    .signa      = signa
};
