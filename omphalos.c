/*
 * omphalos.c — provisor_t omphalos pro oraculo
 *
 * Implementat interfaciem provisoris pro localibus ratiocinationibus
 * ex exemplari transformatoris omphalos. Nullum HTTP, nullum API.
 */

#include "omphalos/ὀμφαλός.h"
#include "omphalos/λεκτήρ.h"
#include "provisor.h"
#include "utilia.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================
 * status globalis
 * ================================================================ */

static char nomen_currens[256] = "praefinitum";

static ομφ_t         exemplar;
static ομφ_λεκτήρ_t *lexator_currens   = NULL;
static char          nomen_exempl[256] = "";
static int           initiatum         = 0;

/* ================================================================
 * initiatio exemplaris
 * ================================================================ */

void omphalos_pone_nomen(const char *nomen)
{
    snprintf(nomen_currens, sizeof(nomen_currens), "%s", nomen);
}

static int lege_exempl(const char *nomen)
{
    if (strcmp(nomen, nomen_exempl) == 0 && initiatum)
        return 0;

    if (initiatum) {
        ομφ_τέλει(&exemplar);
        if (lexator_currens) {
            ομφ_λεκτήρ_κατάλυσον(lexator_currens);
            lexator_currens = NULL;
        }
        initiatum = 0;
    }

    char via_bin[512], via_tok[512];
    snprintf(via_bin, sizeof(via_bin), "%s/model.bin", nomen);
    snprintf(via_tok, sizeof(via_tok), "%s/tokenizer.bin", nomen);

    if (ομφ_ἀνάγνωθι(&exemplar, via_bin) < 0) {
        fprintf(stderr, "omphalos: legere non potui '%s'\n", via_bin);
        return -1;
    }

    int lx = exemplar.σύνθεσις.λεξ_μέγεθος;
    lexator_currens = ομφ_λεκτήρ_κτίσον(lx);
    if (!lexator_currens) {
        ομφ_τέλει(&exemplar);
        return -1;
    }

    if (ομφ_λεκτήρ_ἀνάγνωθι(lexator_currens, via_tok) < 0) {
        fprintf(stderr, "omphalos: legere non potui '%s'\n", via_tok);
        ομφ_λεκτήρ_κατάλυσον(lexator_currens);
        lexator_currens = NULL;
        ομφ_τέλει(&exemplar);
        return -1;
    }

    snprintf(nomen_exempl, sizeof(nomen_exempl), "%s", nomen);
    initiatum = 1;
    ομφ_gpu_ἄρχε(&exemplar);
    return 0;
}

/* ================================================================
 * para — stub pro localibus provisoribus
 * ================================================================ */

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
    *corpus = strdup(rogatum ? rogatum : "");
    *capita = NULL;
    return *corpus ? 0 : -1;
}

/* ================================================================
 * extrahe — ratiocinationes transformatoris omphalos
 * ================================================================ */

#define SIGNA_MAX         256
#define GENERATIONES_MAX  256
#define TEMPERATURA_PRAEF 0.8f
#define TOPP_PRAEF        0.9f

static unsigned long long semen_glob = 42;

static float fortuitus(unsigned long long *r)
{
    *r ^= *r >> 12;
    *r ^= *r << 25;
    *r ^= *r >> 27;
    return (float)((*r * 0x2545F4914F6CDD1Dull) >> 40) / 16777216.0f;
}

typedef struct {
    float p;
    int i;
} pi_t;

static int pi_comp(const void *a, const void *b)
{
    float d = ((const pi_t *)b)->p - ((const pi_t *)a)->p;
    return (d > 0) - (d < 0);
}

static char *extrahe(const char *rogatum)
{
    if (lege_exempl(nomen_currens) < 0)
        return strdup("omphalos: exemplar legere non potui");

    ομφ_t          *omph = &exemplar;
    ομφ_λεκτήρ_t   *lex  = lexator_currens;
    ομφ_σύνθεσις_t *s    = &omph->σύνθεσις;
    int V = s->λεξ_μέγεθος;

    int n_rog    = (int)strlen(rogatum) + 4;
    int *sig_rog = malloc((size_t)n_rog * sizeof(int));
    if (!sig_rog)
        return strdup("memoria deest");
    int n_sig_rog = 0;
    ομφ_λεκτήρ_τέμε(lex, rogatum, 1, 0, sig_rog, &n_sig_rog);

    ομφ_κατάστ_ἀποκατάστησον(omph);

    float *logitae = NULL;
    for (int t = 0; t < n_sig_rog && t < s->μῆκος_μέγ - 1; t++)
        logitae = ομφ_δρόμος(omph, sig_rog[t], t);

    int positio = n_sig_rog < s->μῆκος_μέγ ? n_sig_rog : s->μῆκος_μέγ - 1;
    free(sig_rog);

    int sig_gen[SIGNA_MAX];
    int n_gen = 0;
    unsigned long long r = semen_glob;
    float temp = TEMPERATURA_PRAEF;
    float topp = TOPP_PRAEF;

    pi_t *pi = malloc((size_t)V * sizeof(pi_t));
    if (!pi)
        return strdup("memoria deest");

    for (int pas = 0; pas < GENERATIONES_MAX && positio < s->μῆκος_μέγ; pas++) {
        if (!logitae)
            break;

        for (int i = 0; i < V; i++)
            logitae[i] /= temp;
        float mx = logitae[0];
        for (int i = 1; i < V; i++)
            if (logitae[i] > mx)
                mx = logitae[i];
        float summa = 0.0f;
        for (int i = 0; i < V; i++) {
            logitae[i] = expf(logitae[i] - mx);
            summa += logitae[i];
        }
        for (int i = 0; i < V; i++)
            logitae[i] /= summa;

        float limen = (1.0f - topp) / (float)(V - 1);
        int n_p     = 0;
        for (int i = 0; i < V; i++) {
            if (logitae[i] >= limen) {
                pi[n_p].p = logitae[i];
                pi[n_p].i = i;
                n_p++;
            }
        }
        qsort(pi, n_p, sizeof(pi_t), pi_comp);

        float cumul = 0.0f;
        int finis   = n_p - 1;
        for (int i = 0; i < n_p; i++) {
            cumul += pi[i].p;
            if (cumul > topp) {
                finis = i;
                break;
            }
        }

        float fort = fortuitus(&r) * cumul;
        float acc  = 0.0f;
        int prox   = pi[0].i;
        for (int i = 0; i <= finis; i++) {
            acc += pi[i].p;
            if (fort < acc) {
                prox = pi[i].i;
                break;
            }
        }

        if (prox == 2)
            break;
        sig_gen[n_gen++] = prox;
        logitae = ομφ_δρόμος(omph, prox, positio++);
    }

    free(pi);

    size_t cru_mag = (size_t)n_gen * ΟΜΦ_ΛΕΞ_ΛΕΞΗΜΑ_ΜΕΓ + 1;
    char *crudum   = malloc(cru_mag);
    if (!crudum)
        return strdup("memoria deest");
    size_t cursor = 0;
    for (int i = 0; i < n_gen; i++) {
        const char *l = ομφ_λεκτήρ_ἀπόδος(lex, sig_gen[i]);
        size_t lm     = strlen(l);
        if (cursor + lm < cru_mag) {
            memcpy(crudum + cursor, l, lm);
            cursor += lm;
        }
    }
    crudum[cursor] = '\0';

    char *exitus = malloc(cru_mag);
    if (!exitus) {
        free(crudum);
        return strdup("memoria deest");
    }
    utf8_mundus(exitus, cru_mag, crudum);
    free(crudum);
    return exitus;
}

/* ================================================================
 * signa
 * ================================================================ */

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

/* ================================================================
 * provisor_t
 * ================================================================ */

const provisor_t provisor_omphalos = {
    .nomen      = "omphalos",
    .clavis_env = "",
    .finis_url  = "",
    .para       = para,
    .extrahe    = extrahe,
    .signa      = signa
};
