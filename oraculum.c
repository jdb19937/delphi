/*
 * oraculum.c — dispatcher oraculi cum provisoribus multis
 *
 * Interfacies publica manet eadem. Provisor eligitur ex modello:
 *   "openai/gpt-5.4+high" → provisor_openai
 *   "xai/grok-3"          → provisor_xai
 *   "anthropic/claude-..."  → provisor_anthropic
 *   "gpt-5.4" (sine praefixo) → provisor_openai (praefinitus)
 */

#include "oraculum.h"
#include "provisor.h"
#include "utilia.h"

#include <crispus/crispus.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SAPIENTUM_PRAEFINITUM "fictus/default"
#define FOSSA_MAX 32

/* --- provisores --- */

static const provisor_t *provisores[8] = {
    &provisor_openai,
    &provisor_xai,
    &provisor_anthropic,
    &provisor_fictus,
    &provisor_omphalos,
    NULL
};

/* omphalos_pone_nomen — communicat nomen exemplaris ante extrahe() */
extern void omphalos_pone_nomen(const char *nomen);

void oraculum_adde_provisorem(const provisor_t *prov)
{
    for (int i = 0; i < 7; i++) {
        if (!provisores[i]) {
            provisores[i]     = prov;
            provisores[i + 1] = NULL;
            return;
        }
    }
}

static const provisor_t *quaere_provisorem(const char *nomen)
{
    if (!nomen || !*nomen)
        return &provisor_openai;
    for (int i = 0; provisores[i]; i++)
        if (strcmp(provisores[i]->nomen, nomen) == 0)
            return provisores[i];
    return &provisor_openai;
}

/* --- memoria ad responsum colligendum --- */

struct memoria {
    char  *data;
    size_t magnitudo;
};

static size_t scribe_fn(void *contenta, size_t mag, size_t nmemb, void *usor)
{
    size_t realis = mag * nmemb;
    struct memoria *mem = usor;
    char *novum = realloc(mem->data, mem->magnitudo + realis + 1);
    if (!novum)
        return 0;
    mem->data = novum;
    memcpy(mem->data + mem->magnitudo, contenta, realis);
    mem->magnitudo += realis;
    mem->data[mem->magnitudo] = '\0';
    return realis;
}

/* --- resolve sapientum et provisorem --- */

static char sapientum_currens[128];
static const provisor_t *provisor_currentis;

/* resolve "default" in nomen sapientis frontieris */
static const char *resolve_default(const char *provisor, const char *nomen)
{
    if (strcmp(nomen, "default") != 0)
        return nomen;
    if (!*provisor || strcmp(provisor, "openai") == 0)
        return "gpt-5.4";
    if (strcmp(provisor, "anthropic") == 0)
        return "claude-sonnet-4-6";
    if (strcmp(provisor, "xai") == 0)
        return "grok-4.20";
    return nomen;
}

static const provisor_t *resolve(
    const char *sapientum,
    char *nomen, size_t nmag,
    char *conatus, size_t cmag
) {
    const char *spec = sapientum;
    if (!spec || !*spec)
        spec = SAPIENTUM_PRAEFINITUM;

    char provisor[64];
    lege_sapientum(
        spec, provisor, sizeof(provisor),
        nomen, nmag, conatus, cmag
    );

    const provisor_t *prov = quaere_provisorem(provisor);

    /* resolve "default" ad nomen frontieris */
    const char *resolutum = resolve_default(provisor, nomen);
    if (resolutum != nomen)
        snprintf(nomen, nmag, "%s", resolutum);

    /* serva sapientum plenum "provisor/nomen" */
    snprintf(
        sapientum_currens, sizeof(sapientum_currens),
        "%s/%s", prov->nomen, nomen
    );
    provisor_currentis = prov;

    return prov;
}

/* --- para crispum cum provisore --- */

static int para_crispum(
    CRISPUS *crispus, const char *sapientum,
    const char *instructiones, const char *rogatum,
    char **corpus_out, struct crispus_slist **capita_out,
    struct memoria *mem
) {
    char nomen[128], conatus[32];
    const provisor_t *prov = resolve(
        sapientum,
        nomen, sizeof(nomen),
        conatus, sizeof(conatus)
    );

    const char *clavis = getenv(prov->clavis_env);
    if (!clavis || !*clavis)
        return -1;

    char *corpus = NULL;
    struct crispus_slist *capita = NULL;

    if (
        prov->para(
            nomen, conatus, clavis,
            instructiones, rogatum,
            &corpus, &capita
        ) < 0
    )
        return -1;

    mem->data      = NULL;
    mem->magnitudo = 0;

    crispus_facilis_pone(crispus, CRISPUSOPT_URL, prov->finis_url);
    crispus_facilis_pone(crispus, CRISPUSOPT_CAMPI_POSTAE, corpus);
    crispus_facilis_pone(crispus, CRISPUSOPT_CAPITA_HTTP, capita);
    crispus_facilis_pone(crispus, CRISPUSOPT_FUNCTIO_SCRIBENDI, scribe_fn);
    crispus_facilis_pone(crispus, CRISPUSOPT_DATA_SCRIBENDI, mem);
    crispus_facilis_pone(crispus, CRISPUSOPT_TEMPUS, 60L);

    *corpus_out = corpus;
    *capita_out = capita;
    return 0;
}

/* --- fossae asynchronae --- */

struct fossa {
    enum fossa_actum actum;
    CRISPUS *manubrium;
    struct crispus_slist *capita;
    char *corpus;
    struct memoria mem;
    char *responsum;
    int exitus;
    const provisor_t *provisor;
    char sapientum[128];    /* sapientum plenum pro attributione */
};

static CRISPUSM *multi;
static struct fossa fossae[FOSSA_MAX];

/* numeri per sapientum (clavis = "sapientum:metrica") */
static lexicon_t *numeri_lex;

static void fossa_fini_transferum(struct fossa *f, CRISPUScode rc)
{
    const provisor_t *prov = f->provisor ? f->provisor : &provisor_openai;

    if (rc != CRISPUSE_OK) {
        char err[256];
        snprintf(
            err, sizeof(err), "crispus error: %s",
            crispus_facilis_error(rc)
        );
        f->responsum = strdup(err);
        f->exitus    = -1;
    } else {
        long codex;
        crispus_facilis_info(f->manubrium, CRISPUSINFO_CODEX_RESPONSI, &codex);
        if (!f->mem.data) {
            f->responsum = strdup("responsum vacuum");
            f->exitus    = -1;
        } else {
            f->responsum = prov->extrahe(f->mem.data);
            f->exitus    = (codex >= 200 && codex < 300 && f->responsum) ? 0 : -1;
        }
    }

    const char *nomen = f->sapientum[0] ? f->sapientum : "ignotum";

    if (f->exitus == 0)
        lexicon_adde_compositam(numeri_lex, nomen, "successae", 1);
    else
        lexicon_adde_compositam(numeri_lex, nomen, "errores", 1);

    if (f->mem.data && prov->signa) {
        long accepta = 0, recondita = 0, emissa = 0, cogitata = 0;
        prov->signa(f->mem.data, &accepta, &recondita, &emissa, &cogitata);
        lexicon_adde_compositam(numeri_lex, nomen, "accepta", accepta);
        lexicon_adde_compositam(numeri_lex, nomen, "recondita", recondita);
        lexicon_adde_compositam(numeri_lex, nomen, "emissa", emissa);
        lexicon_adde_compositam(numeri_lex, nomen, "cogitata", cogitata);
    }

    crispus_multi_remove(multi, f->manubrium);
    crispus_facilis_fini(f->manubrium);
    crispus_slist_libera(f->capita);
    free(f->corpus);
    free(f->mem.data);
    f->manubrium     = NULL;
    f->capita        = NULL;
    f->corpus        = NULL;
    f->mem.data      = NULL;
    f->mem.magnitudo = 0;
    f->actum         = FOSSA_PERFECTA;
}

/* --- interfacies publica --- */

int oraculum_initia(void)
{
    if (crispus_orbis_initia(CRISPUS_GLOBAL_DEFAULT) != CRISPUSE_OK)
        return -1;
    multi = crispus_multi_initia();
    if (!multi)
        return -1;
    memset(fossae, 0, sizeof(fossae));
    numeri_lex = lexicon_crea();
    return 0;
}

void oraculum_fini(void)
{
    for (int i = 0; i < FOSSA_MAX; i++) {
        if (fossae[i].actum == FOSSA_VOLANS && fossae[i].manubrium) {
            crispus_multi_remove(multi, fossae[i].manubrium);
            crispus_facilis_fini(fossae[i].manubrium);
            crispus_slist_libera(fossae[i].capita);
            free(fossae[i].corpus);
            free(fossae[i].mem.data);
        }
        free(fossae[i].responsum);
        fossae[i].actum = FOSSA_LIBERA;
    }
    if (multi)
        crispus_multi_fini(multi);
    lexicon_libera(numeri_lex);
    numeri_lex = NULL;
    crispus_orbis_fini();
}

/* --- synchrona --- */

int oraculum_roga(
    const char *sapientum, const char *instructiones,
    const char *rogatum, char **responsum
) {
    *responsum = NULL;

    /* resolve provisorem */
    char nomen[128], conatus[32];
    const provisor_t *prov = resolve(
        sapientum,
        nomen, sizeof(nomen),
        conatus, sizeof(conatus)
    );

    /* provisores locales: genera responsum statim, sine rete */
    if (
        strcmp(prov->nomen, "munda") == 0 ||
        strcmp(prov->nomen, "fictus") == 0
    ) {
        *responsum = prov->extrahe(rogatum);
        return *responsum ? 0 : -1;
    }
    if (strcmp(prov->nomen, "omphalos") == 0) {
        omphalos_pone_nomen(nomen);
        *responsum = prov->extrahe(rogatum);
        return *responsum ? 0 : -1;
    }

    CRISPUS *crispus = crispus_facilis_initia();
    if (!crispus) {
        *responsum = strdup("crispus_facilis_initia defecit");
        return -1;
    }

    char *corpus = NULL;
    struct crispus_slist *capita = NULL;
    struct memoria mem = { NULL, 0 };

    if (
        para_crispum(
            crispus, sapientum, instructiones, rogatum,
            &corpus, &capita, &mem
        ) < 0
    ) {
        crispus_facilis_fini(crispus);
        *responsum = strdup("clavis API deest vel para defecit");
        return -1;
    }

    CRISPUScode rc = crispus_facilis_age(crispus);
    int exitus     = -1;

    if (rc != CRISPUSE_OK) {
        char err[256];
        snprintf(
            err, sizeof(err), "crispus error: %s",
            crispus_facilis_error(rc)
        );
        *responsum = strdup(err);
    } else {
        long codex;
        crispus_facilis_info(crispus, CRISPUSINFO_CODEX_RESPONSI, &codex);
        if (!mem.data) {
            *responsum = strdup("responsum vacuum");
        } else {
            *responsum = prov->extrahe(mem.data);
            if (codex >= 200 && codex < 300 && *responsum)
                exitus = 0;
        }
    }

    free(mem.data);
    free(corpus);
    crispus_facilis_fini(crispus);
    crispus_slist_libera(capita);
    return exitus;
}

/* --- sessio --- */

struct oraculum_sessio {
    char  sapientum[128];
    char *instructiones;    /* potest esse NULL */
    char **roles;
    char **textus;
    int   numerus;
    int   capacitas;
};

static int sessio_cresce(oraculum_sessio_t *s)
{
    if (s->numerus < s->capacitas)
        return 0;
    int nova  = s->capacitas ? s->capacitas * 2 : 8;
    char **nr = realloc(s->roles,  nova * sizeof(char *));
    char **nt = realloc(s->textus, nova * sizeof(char *));
    if (!nr || !nt) {
        free(nr);
        free(nt);
        return -1;
    }
    s->roles     = nr;
    s->textus    = nt;
    s->capacitas = nova;
    return 0;
}

static int sessio_appende(
    oraculum_sessio_t *s, const char *role, const char *textus
) {
    if (sessio_cresce(s) < 0)
        return -1;
    char *r = strdup(role);
    char *t = strdup(textus);
    if (!r || !t) {
        free(r);
        free(t);
        return -1;
    }
    s->roles[s->numerus]  = r;
    s->textus[s->numerus] = t;
    s->numerus++;
    return 0;
}

oraculum_sessio_t *oraculum_sessio_novam(
    const char *sapientum, const char *instructiones
) {
    oraculum_sessio_t *s = calloc(1, sizeof(*s));
    if (!s)
        return NULL;
    snprintf(
        s->sapientum, sizeof(s->sapientum), "%s",
        (sapientum && *sapientum) ? sapientum : SAPIENTUM_PRAEFINITUM
    );
    if (instructiones) {
        s->instructiones = strdup(instructiones);
        if (!s->instructiones) {
            free(s);
            return NULL;
        }
    }
    return s;
}

int oraculum_sessio_numerus(const oraculum_sessio_t *s)
{
    return s ? s->numerus : 0;
}

void oraculum_sessio_dimitte(oraculum_sessio_t *s)
{
    if (!s)
        return;
    for (int i = 0; i < s->numerus; i++) {
        free(s->roles[i]);
        free(s->textus[i]);
    }
    free(s->roles);
    free(s->textus);
    free(s->instructiones);
    free(s);
}

/* concatena historiam cum annotationibus pro provisoribus qui
 * non implementant para_sessionem */
static char *sessio_concatena(
    const char *const *roles, const char *const *textus, int n
) {
    size_t mag = 1;
    for (int i = 0; i < n; i++)
        mag += strlen(roles[i]) + strlen(textus[i]) + 4;
    char *buf = malloc(mag);
    if (!buf)
        return NULL;
    char *p = buf;
    for (int i = 0; i < n; i++) {
        if (i)
            *p++ = '\n', *p++ = '\n';
        p += sprintf(p, "%s: %s", roles[i], textus[i]);
    }
    *p = '\0';
    return buf;
}

int oraculum_sessio_roga(
    oraculum_sessio_t *s, const char *rogatum, char **responsum
) {
    *responsum = NULL;
    if (!s || !rogatum)
        return -1;

    /* appende temporarie turnum usoris */
    if (sessio_appende(s, "user", rogatum) < 0)
        return -1;

    char nomen[128], conatus[32];
    const provisor_t *prov = resolve(
        s->sapientum,
        nomen, sizeof(nomen),
        conatus, sizeof(conatus)
    );

    /* provisores locales: nulla rete, concat et extrahe */
    if (
        strcmp(prov->nomen, "munda") == 0 ||
        strcmp(prov->nomen, "fictus") == 0 ||
        strcmp(prov->nomen, "omphalos") == 0
    ) {
        char *concat = sessio_concatena(
            (const char *const *)s->roles,
            (const char *const *)s->textus,
            s->numerus
        );
        if (!concat) {
            s->numerus--;
            free(s->roles[s->numerus]);
            free(s->textus[s->numerus]);
            return -1;
        }
        if (strcmp(prov->nomen, "omphalos") == 0)
            omphalos_pone_nomen(nomen);
        char *resp = prov->extrahe(concat);
        free(concat);
        if (!resp) {
            s->numerus--;
            free(s->roles[s->numerus]);
            free(s->textus[s->numerus]);
            return -1;
        }
        if (sessio_appende(s, "assistant", resp) < 0) {
            free(resp);
            s->numerus--;
            free(s->roles[s->numerus]);
            free(s->textus[s->numerus]);
            return -1;
        }
        *responsum = resp;
        return 0;
    }

    const char *clavis = getenv(prov->clavis_env);
    if (!clavis || !*clavis) {
        s->numerus--;
        free(s->roles[s->numerus]);
        free(s->textus[s->numerus]);
        return -1;
    }

    char *corpus = NULL;
    struct crispus_slist *capita = NULL;
    int pr;

    if (prov->para_sessionem) {
        pr = prov->para_sessionem(
            nomen, conatus, clavis, s->instructiones,
            (const char *const *)s->roles,
            (const char *const *)s->textus,
            s->numerus, &corpus, &capita
        );
    } else {
        char *concat = sessio_concatena(
            (const char *const *)s->roles,
            (const char *const *)s->textus,
            s->numerus
        );
        if (!concat) {
            s->numerus--;
            free(s->roles[s->numerus]);
            free(s->textus[s->numerus]);
            return -1;
        }
        pr = prov->para(
            nomen, conatus, clavis, s->instructiones, concat,
            &corpus, &capita
        );
        free(concat);
    }

    if (pr < 0) {
        s->numerus--;
        free(s->roles[s->numerus]);
        free(s->textus[s->numerus]);
        return -1;
    }

    CRISPUS *crispus = crispus_facilis_initia();
    if (!crispus) {
        free(corpus);
        crispus_slist_libera(capita);
        s->numerus--;
        free(s->roles[s->numerus]);
        free(s->textus[s->numerus]);
        return -1;
    }

    struct memoria mem = { NULL, 0 };
    crispus_facilis_pone(crispus, CRISPUSOPT_URL, prov->finis_url);
    crispus_facilis_pone(crispus, CRISPUSOPT_CAMPI_POSTAE, corpus);
    crispus_facilis_pone(crispus, CRISPUSOPT_CAPITA_HTTP, capita);
    crispus_facilis_pone(crispus, CRISPUSOPT_FUNCTIO_SCRIBENDI, scribe_fn);
    crispus_facilis_pone(crispus, CRISPUSOPT_DATA_SCRIBENDI, &mem);
    crispus_facilis_pone(crispus, CRISPUSOPT_TEMPUS, 60L);

    CRISPUScode rc = crispus_facilis_age(crispus);
    int exitus     = -1;
    char *resp     = NULL;

    if (rc == CRISPUSE_OK && mem.data) {
        long codex;
        crispus_facilis_info(crispus, CRISPUSINFO_CODEX_RESPONSI, &codex);
        char *extractum = prov->extrahe(mem.data);
        if (codex >= 200 && codex < 300 && extractum) {
            resp   = extractum;
            exitus = 0;
        } else {
            free(extractum);
        }
    }

    free(mem.data);
    free(corpus);
    crispus_facilis_fini(crispus);
    crispus_slist_libera(capita);

    if (exitus != 0 || !resp) {
        free(resp);
        s->numerus--;
        free(s->roles[s->numerus]);
        free(s->textus[s->numerus]);
        return -1;
    }

    if (sessio_appende(s, "assistant", resp) < 0) {
        free(resp);
        s->numerus--;
        free(s->roles[s->numerus]);
        free(s->textus[s->numerus]);
        return -1;
    }

    *responsum = resp;
    return 0;
}

/* --- asynchrona --- */

int oraculum_mitte(
    const char *sapientum, const char *instructiones,
    const char *rogatum
) {
    int fi = -1;
    for (int i = 0; i < FOSSA_MAX; i++) {
        if (fossae[i].actum == FOSSA_LIBERA) {
            fi = i;
            break;
        }
    }
    if (fi < 0)
        return -1;

    /* resolve provisorem ut sciamus an munda sit */
    char nomen[128], conatus[32];
    const provisor_t *prov = resolve(
        sapientum,
        nomen, sizeof(nomen),
        conatus, sizeof(conatus)
    );

    struct fossa *f = &fossae[fi];
    memset(f, 0, sizeof(*f));
    f->provisor = prov;
    snprintf(f->sapientum, sizeof(f->sapientum), "%s", sapientum_currens);
    lexicon_adde_compositam(numeri_lex, f->sapientum, "missae", 1);

    /* provisores locales: genera responsum statim, sine rete */
    if (
        strcmp(prov->nomen, "munda") == 0 ||
        strcmp(prov->nomen, "fictus") == 0 ||
        strcmp(prov->nomen, "omphalos") == 0
    ) {
        if (strcmp(prov->nomen, "omphalos") == 0)
            omphalos_pone_nomen(nomen);
        f->responsum = prov->extrahe(rogatum);
        f->exitus    = 0;
        f->actum     = FOSSA_PERFECTA;
        lexicon_adde_compositam(numeri_lex, f->sapientum, "successae", 1);
        return fi;
    }

    CRISPUS *crispus = crispus_facilis_initia();
    if (!crispus) {
        lexicon_adde_compositam(numeri_lex, f->sapientum, "errores", 1);
        f->responsum = strdup("crispus_facilis_initia defecit");
        f->exitus    = -1;
        f->actum     = FOSSA_PERFECTA;
        return fi;
    }

    if (
        para_crispum(
            crispus, sapientum, instructiones, rogatum,
            &f->corpus, &f->capita, &f->mem
        ) < 0
    ) {
        crispus_facilis_fini(crispus);
        lexicon_adde_compositam(numeri_lex, f->sapientum, "errores", 1);
        f->responsum = strdup("para defecit (clavis API deest?)");
        f->exitus    = -1;
        f->actum     = FOSSA_PERFECTA;
        return fi;
    }

    f->actum     = FOSSA_VOLANS;
    f->manubrium = crispus;
    crispus_multi_adde(multi, crispus);
    return fi;
}

void oraculum_processus(void)
{
    if (!multi)
        return;

    int running;
    crispus_multi_age(multi, &running);

    CRISPUSMsg *msg;
    int remaining;
    while ((msg = crispus_multi_lege(multi, &remaining))) {
        if (msg->msg != CRISPUSMSG_PERFECTUM)
            continue;

        CRISPUS *crispus = msg->easy_handle;
        for (int i = 0; i < FOSSA_MAX; i++) {
            if (fossae[i].actum == FOSSA_VOLANS && fossae[i].manubrium == crispus) {
                fossa_fini_transferum(&fossae[i], msg->data.result);
                break;
            }
        }
    }
}

int oraculum_status(int fi, char **responsum)
{
    *responsum = NULL;
    if (fi < 0 || fi >= FOSSA_MAX)
        return ORACULUM_ERRATUM;

    struct fossa *f = &fossae[fi];
    if (f->actum == FOSSA_VOLANS)
        return ORACULUM_PENDENS;
    if (f->actum == FOSSA_PERFECTA) {
        *responsum   = f->responsum;
        f->responsum = NULL;
        return (f->exitus == 0) ? ORACULUM_PARATUM : ORACULUM_ERRATUM;
    }
    return ORACULUM_ERRATUM;
}

void oraculum_dimitte(int fi)
{
    if (fi < 0 || fi >= FOSSA_MAX)
        return;
    free(fossae[fi].responsum);
    fossae[fi].responsum = NULL;
    fossae[fi].actum     = FOSSA_LIBERA;
}

void oraculum_numeri(oraculum_numeri_t *num)
{
    num->pendentes = 0;
    num->paratae   = 0;
    num->liberae   = 0;
    for (int i = 0; i < FOSSA_MAX; i++) {
        switch (fossae[i].actum) {
        case FOSSA_LIBERA:
            num->liberae++;
            break;
        case FOSSA_VOLANS:
            num->pendentes++;
            break;
        case FOSSA_PERFECTA:
            num->paratae++;
            break;
        }
    }
    /* summas computa ex lexico iterando per genera */
    num->summa_missae = 0;
    num->summa_successae = 0;
    num->summa_errores = 0;
    num->summa_signa_accepta = 0;
    num->summa_signa_recondita = 0;
    num->summa_signa_emissa = 0;
    num->summa_signa_cogitata = 0;

    char genera[16][64];
    int ngen = lexicon_genera(numeri_lex, genera, 16);
    for (int i = 0; i < ngen; i++) {
        num->summa_missae    += lexicon_da_compositam(numeri_lex, genera[i], "missae");
        num->summa_successae += lexicon_da_compositam(numeri_lex, genera[i], "successae");
        num->summa_errores   += lexicon_da_compositam(numeri_lex, genera[i], "errores");
        num->summa_signa_accepta   += lexicon_da_compositam(numeri_lex, genera[i], "accepta");
        num->summa_signa_recondita += lexicon_da_compositam(numeri_lex, genera[i], "recondita");
        num->summa_signa_emissa    += lexicon_da_compositam(numeri_lex, genera[i], "emissa");
        num->summa_signa_cogitata  += lexicon_da_compositam(numeri_lex, genera[i], "cogitata");
    }
}

const char *oraculum_sapientum(void)
{
    return sapientum_currens[0] ? sapientum_currens : SAPIENTUM_PRAEFINITUM;
}

int oraculum_numeri_per_sapientum(oraculum_numeri_modelli_t *tabulatum, int maximus)
{
    if (!numeri_lex)
        return 0;
    char genera[16][64];
    int ngen = lexicon_genera(numeri_lex, genera, 16);
    if (ngen > maximus)
        ngen = maximus;
    for (int i = 0; i < ngen; i++) {
        snprintf(
            tabulatum[i].sapientum, sizeof(tabulatum[i].sapientum),
            "%s", genera[i]
        );
        tabulatum[i].volantes = 0;
        tabulatum[i].paratae  = 0;
        tabulatum[i].missae      = lexicon_da_compositam(numeri_lex, genera[i], "missae");
        tabulatum[i].successae   = lexicon_da_compositam(numeri_lex, genera[i], "successae");
        tabulatum[i].errores     = lexicon_da_compositam(numeri_lex, genera[i], "errores");
        tabulatum[i].signa_accepta   = lexicon_da_compositam(numeri_lex, genera[i], "accepta");
        tabulatum[i].signa_recondita = lexicon_da_compositam(numeri_lex, genera[i], "recondita");
        tabulatum[i].signa_emissa    = lexicon_da_compositam(numeri_lex, genera[i], "emissa");
        tabulatum[i].signa_cogitata  = lexicon_da_compositam(numeri_lex, genera[i], "cogitata");
    }
    /* computa volantes et paratae ex fossis vivis */
    for (int i = 0; i < FOSSA_MAX; i++) {
        if (fossae[i].actum == FOSSA_LIBERA)
            continue;
        const char *nomen = fossae[i].sapientum;
        if (!nomen[0])
            continue;
        for (int j = 0; j < ngen; j++) {
            if (strcmp(tabulatum[j].sapientum, nomen) == 0) {
                if (fossae[i].actum == FOSSA_VOLANS)
                    tabulatum[j].volantes++;
                if (fossae[i].actum == FOSSA_PERFECTA)
                    tabulatum[j].paratae++;
                break;
            }
        }
    }
    return ngen;
}
