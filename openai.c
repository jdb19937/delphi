/*
 * openai.c — provisor OpenAI (Responses API)
 */

#include "provisor.h"

#include <ison/ison.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int para(
    const char *nomen, const char *conatus,
    const char *clavis_api,
    const char *instructiones, const char *rogatum,
    char **corpus, struct crispus_slist **capita
) {
    char *eff_input = ison_effuge(rogatum);
    if (!eff_input)
        return -1;

    char *eff_inst = NULL;
    if (instructiones) {
        eff_inst = ison_effuge(instructiones);
        if (!eff_inst) {
            free(eff_input);
            return -1;
        }
    }

    size_t mag = strlen(eff_input) + strlen(nomen) + 256;
    if (eff_inst)
        mag += strlen(eff_inst);

    char *buf = malloc(mag);
    if (!buf) {
        free(eff_input);
        free(eff_inst);
        return -1;
    }

    char *p = buf;
    p += sprintf(p, "{\"model\":\"%s\"", nomen);
    if (conatus[0])
        p += sprintf(p, ",\"reasoning\":{\"effort\":\"%s\"}", conatus);
    if (eff_inst)
        p += sprintf(p, ",\"instructions\":\"%s\"", eff_inst);
    p += sprintf(p, ",\"input\":\"%s\"}", eff_input);

    free(eff_input);
    free(eff_inst);

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

static int para_sessionem(
    const char *nomen, const char *conatus,
    const char *clavis_api,
    const char *instructiones,
    const char *const *roles, const char *const *textus, int n,
    char **corpus, struct crispus_slist **capita
) {
    char *eff_inst = NULL;
    if (instructiones) {
        eff_inst = ison_effuge(instructiones);
        if (!eff_inst)
            return -1;
    }

    char **eff = calloc(n > 0 ? n : 1, sizeof(char *));
    if (!eff) {
        free(eff_inst);
        return -1;
    }
    for (int i = 0; i < n; i++) {
        eff[i] = ison_effuge(textus[i]);
        if (!eff[i]) {
            for (int j = 0; j < i; j++)
                free(eff[j]);
            free(eff);
            free(eff_inst);
            return -1;
        }
    }

    size_t mag = strlen(nomen) + 256;
    if (eff_inst)
        mag += strlen(eff_inst);
    for (int i = 0; i < n; i++)
        mag += strlen(eff[i]) + strlen(roles[i]) + 32;

    char *buf = malloc(mag);
    if (!buf) {
        for (int i = 0; i < n; i++)
            free(eff[i]);
        free(eff);
        free(eff_inst);
        return -1;
    }

    char *p = buf;
    p += sprintf(p, "{\"model\":\"%s\"", nomen);
    if (conatus[0])
        p += sprintf(p, ",\"reasoning\":{\"effort\":\"%s\"}", conatus);
    if (eff_inst)
        p += sprintf(p, ",\"instructions\":\"%s\"", eff_inst);
    p += sprintf(p, ",\"input\":[");
    for (int i = 0; i < n; i++) {
        if (i)
            *p++ = ',';
        p += sprintf(
            p, "{\"role\":\"%s\",\"content\":\"%s\"}",
            roles[i], eff[i]
        );
    }
    p += sprintf(p, "]}");

    for (int i = 0; i < n; i++)
        free(eff[i]);
    free(eff);
    free(eff_inst);

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
    char *textus = ison_da_chordam(ison, "output[0].content[0].text");
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
    *accepta   = ison_da_numerum(ison, "usage.input_tokens");
    *emissa    = ison_da_numerum(ison, "usage.output_tokens");
    *recondita = ison_da_numerum(ison, "usage.input_tokens_details.cached_tokens");
    *cogitata  = ison_da_numerum(ison, "usage.output_tokens_details.reasoning_tokens");
}

const provisor_t provisor_openai = {
    .nomen      = "openai",
    .clavis_env = "OPENAI_API_KEY",
    .finis_url  = "https://api.openai.com/v1/responses",
    .para           = para,
    .para_sessionem = para_sessionem,
    .extrahe        = extrahe,
    .signa          = signa
};
