/*
 * exercita.c — exercitatio metamorphotae omphalos ex linea mandatorum
 *
 * Usus: exercita <dir> <via_corporis> [n_gradus] [dimens] [strata] [capita]
 *
 *   <dir>            directorium exemplaris (model.bin, tokenizer.bin)
 *   <via_corporis>   fasciculus corporis (UTF-8)
 *   [n_gradus]       gradus exercitationis (praedef. 1000)
 *   [dimens]         dimensio exemplaris (praedef. 128)
 *   [strata]         strata metamorphotae (praedef. 4)
 *   [capita]         capita attentionis (praedef. 4)
 *
 * Si <dir>/model.bin iam exstat, exercitatio continuatur.
 * Asservatio (checkpoint) fit per ASSERVATIO_GRADUS gradus.
 */

#include "omphalos/ὀμφαλός.h"
#include "omphalos/λεκτήρ.h"
#include "computo.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* ================================================================
 * parametri praedefiniti
 * ================================================================ */

#define PRAEDEF_LEX_MAG       1024
#define PRAEDEF_DIMENSIO      128
#define PRAEDEF_STRATA        4
#define PRAEDEF_CAPITA        4
#define PRAEDEF_CAPITA_CT     2
#define PRAEDEF_LONGITUDO_MAX 128
#define PRAEDEF_N_GRADUS      1000
#define PRAEDEF_AC_LONG       64
#define PRAEDEF_N_MINIM       4       /* fenestrae per gradum (minima fascis) */
#define PRAEDEF_RATIO_MAX     1e-4f   /* maxima ratio discendi (initio) */
#define PRAEDEF_RATIO_MIN     5e-6f   /* minima ratio discendi (fine) */
#define PRAEDEF_BETA1         0.9f
#define PRAEDEF_BETA2         0.999f
#define PRAEDEF_EPSILON       1e-8f
#define PRAEDEF_DESICCATIO    1e-3f
#define NUNTIUS_GRADUS        100
#define ASSERVATIO_GRADUS     500      /* serva per N gradus */

/* ================================================================
 * lectio fasciculi
 * ================================================================ */

static char *lege_plicam(const char *via)
{
    FILE *f = fopen(via, "rb");
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    long mag = ftell(f);
    rewind(f);
    char *tam = malloc((size_t)mag + 1);
    if (!tam) {
        fclose(f);
        return NULL;
    }
    if (fread(tam, 1, (size_t)mag, f) != (size_t)mag) {
        free(tam);
        fclose(f);
        return NULL;
    }
    tam[mag] = '\0';
    fclose(f);
    return tam;
}

/* ================================================================
 * exercitatio — p_semen: status RNG inter asservationes
 * ================================================================ */

static void exercita(
    ομφ_t *omf, ομφ_ἄσκησις_t *exc,
    const int *sig_corp, int n_corp,
    int n_gradus, int ac_long,
    int gradus_initium, unsigned int *p_semen
) {
    int ml = omf->σύνθεσις.μῆκος_μέγ;
    if (ac_long > ml - 1)
        ac_long = ml - 1;
    int max_init = n_corp - ac_long - 2;
    if (max_init <= 0)
        max_init = 1;

    for (int gradus = 0; gradus < n_gradus; gradus++) {
        ομφ_κλίσεις_μηδένισον(exc, &omf->σύνθεσις);
        float damnum_tot = 0.0f;

        for (int mi = 0; mi < PRAEDEF_N_MINIM; mi++) {
            *p_semen = *p_semen * 1664525u + 1013904223u;
            int init = (int)(*p_semen % (unsigned int)max_init);

            ομφ_κατάστ_ἀποκατάστησον(omf);
            for (int t = 0; t < ac_long && init + t + 1 < n_corp; t++) {
                int sig   = sig_corp[init + t];
                int scope = sig_corp[init + t + 1];
                ομφ_δρόμος_μνήμη(omf, exc, sig, t);
                damnum_tot += ομφ_ἀναδρομή(omf, exc, sig, scope, t);
            }
        }

        ομφ_κλίσεις_κεῖρον(exc, &omf->σύνθεσις, 1.0f);
        /* cosine annealing: ratio decrescit a max ad min */
        int g_abs = gradus_initium + gradus;
        int g_tot = gradus_initium + n_gradus;
        float rat = PRAEDEF_RATIO_MIN + 0.5f * (
            PRAEDEF_RATIO_MAX - PRAEDEF_RATIO_MIN
        ) * (1.0f + cosf(3.14159265f * (float)g_abs / (float)g_tot));
        ομφ_βῆμα_ἀδάμ(
            omf, exc, rat, PRAEDEF_BETA1, PRAEDEF_BETA2,
            PRAEDEF_EPSILON, PRAEDEF_DESICCATIO
        );

        if ((gradus_initium + gradus + 1) % NUNTIUS_GRADUS == 0) {
            float dm = damnum_tot / (float)(ac_long * PRAEDEF_N_MINIM);
            printf(
                "  gradus %5d  damnum=%.4f  perplexitas=%.2f\n",
                gradus_initium + gradus + 1, dm, expf(dm)
            );
            fflush(stdout);
        }
    }
}

/* ================================================================
 * main
 * ================================================================ */

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(
            stderr,
            "usus: %s <dir> <via_corporis> [n_gradus] [dimens] [strata] [capita]\n"
            "  praedef: n_gradus=%d dimens=%d strata=%d capita=%d ct=%d\n"
            "  si <dir>/model.bin exstat, exercitatio continuatur\n",
            argv[0], PRAEDEF_N_GRADUS, PRAEDEF_DIMENSIO, PRAEDEF_STRATA,
            PRAEDEF_CAPITA, PRAEDEF_CAPITA_CT
        );
        return 1;
    }

    const char *dir      = argv[1];
    const char *via_corp = argv[2];

    int n_gradus  = argc   >= 4 ? atoi(argv[3]) : PRAEDEF_N_GRADUS;
    int dimens    = argc   >= 5 ? atoi(argv[4]) : PRAEDEF_DIMENSIO;
    int strata    = argc   >= 6 ? atoi(argv[5]) : PRAEDEF_STRATA;
    int capita    = argc   >= 7 ? atoi(argv[6]) : PRAEDEF_CAPITA;
    int capita_ct = capita >= 4 ? capita / 2    : PRAEDEF_CAPITA_CT;

    printf("=== exercita: %s ===\n\n", dir);

    int gpu = pfr_computo_initia();
    printf("computator: %s\n\n", gpu == 0 ? "GPU" : "CPU");

    char via_bin[512], via_tok[512];
    snprintf(via_bin, sizeof(via_bin), "%s/model.bin", dir);
    snprintf(via_tok, sizeof(via_tok), "%s/tokenizer.bin", dir);

    /* --- 1. lege corpus --- */
    printf("1. lego corpus ex '%s'...\n", via_corp);
    char *corpus = lege_plicam(via_corp);
    if (!corpus || strlen(corpus) < 10) {
        fprintf(stderr, "error: corpus vacuum vel non inventum\n");
        pfr_computo_fini();
        return 1;
    }
    printf("   longitudo: %zu characteres\n\n", strlen(corpus));

    /* --- 2. lexator: tempta onerare vel exerce novum --- */
    ομφ_λεκτήρ_t *lex = ομφ_λεκτήρ_κτίσον(PRAEDEF_LEX_MAG);
    if (!lex) {
        fprintf(stderr, "error: ομφ_λεκτήρ_κτίσον defecit\n");
        free(corpus);
        pfr_computo_fini();
        return 1;
    }

    int resumptio = 0;
    {
        FILE *ft = fopen(via_tok, "rb");
        if (ft) { fclose(ft); }
        if (ft && ομφ_λεκτήρ_ἀνάγνωθι(lex, via_tok) == 0) {
            printf(
                "2. lexator oneratus ex '%s' (lex=%d)\n\n",
                via_tok, lex->λεξ_μέγεθος
            );
            resumptio = 1;
        } else {
            printf("2. exerceo lexatorem BPE (lex_mag=%d)...\n", PRAEDEF_LEX_MAG);
            if (ομφ_λεκτήρ_ἄσκησον(lex, corpus) < 0) {
                fprintf(stderr, "error: exercitatio lexatoris defecit\n");
                free(corpus);
                ομφ_λεκτήρ_κατάλυσον(lex);
                pfr_computo_fini();
                return 1;
            }
            printf("   lex_magnitudo: %d\n", lex->λεξ_μέγεθος);
            mkdir(dir, 0755);
            if (ομφ_λεκτήρ_σῶσον(lex, via_tok) < 0)
                fprintf(stderr, "monitum: lexator servari non potuit\n");
            else
                printf("   lexator servatus: %s\n\n", via_tok);
        }
    }

    int long_corp = (int)strlen(corpus);
    int *signa    = malloc((size_t)(long_corp + 4) * sizeof(int));
    if (!signa) {
        free(corpus);
        ομφ_λεκτήρ_κατάλυσον(lex);
        pfr_computo_fini();
        return 1;
    }
    int n_sig = 0;
    ομφ_λεκτήρ_τέμε(lex, corpus, 0, 0, signa, &n_sig);
    printf("   signa in corpore: %d\n\n", n_sig);
    free(corpus);

    /* --- 3. exemplar: onera vel incipe novum --- */
    ομφ_t omf;
    if (resumptio && ομφ_ἀνάγνωθι(&omf, via_bin) == 0) {
        printf(
            "3. exemplar oneratum ex '%s' (dimens=%d strata=%d lex=%d)\n\n",
            via_bin, omf.σύνθεσις.διάστασις, omf.σύνθεσις.στρώματα,
            omf.σύνθεσις.λεξ_μέγεθος
        );
        dimens    = omf.σύνθεσις.διάστασις;
        strata    = omf.σύνθεσις.στρώματα;
        capita    = omf.σύνθεσις.κεφαλαί;
        capita_ct = omf.σύνθεσις.κεφαλαί_κτ;
    } else {
        resumptio  = 0;
        int occult = (dimens * 8 / 3 + 7) & ~7;
        ομφ_σύνθεσις_t comp = {
            .διάστασις   = dimens,
            .διάστ_κρυ   = occult,
            .στρώματα    = strata,
            .κεφαλαί     = capita,
            .κεφαλαί_κτ  = capita_ct,
            .λεξ_μέγεθος = lex->λεξ_μέγεθος,
            .μῆκος_μέγ   = PRAEDEF_LONGITUDO_MAX,
        };
        printf(
            "3. incipio exemplar novum (dimens=%d occult=%d strata=%d capita=%d"
            " ct=%d lex=%d lm=%d)...\n",
            dimens, occult, strata, capita, capita_ct,
            lex->λεξ_μέγεθος, PRAEDEF_LONGITUDO_MAX
        );
        if (ομφ_ἄρχε_τυχαίως(&omf, &comp, 42u) < 0) {
            fprintf(stderr, "error: ομφ_ἄρχε_τυχαίως defecit\n");
            free(signa);
            ομφ_λεκτήρ_κατάλυσον(lex);
            pfr_computo_fini();
            return 1;
        }
        size_t n_par = ομφ_μέγεθος_σταθμῶν(&comp, 1);
        printf(
            "   parametri: %zu decimalia = %.1f KB\n\n",
            n_par, n_par * sizeof(float) / 1024.0
        );
    }

    /* --- 4. exercita per segmenta cum asservatione --- */
    printf(
        "4. exerceo (%d gradus, ac_long=%d, minim=%d, asservatio=%d)...\n",
        n_gradus, PRAEDEF_AC_LONG, PRAEDEF_N_MINIM, ASSERVATIO_GRADUS
    );
    ομφ_ἄσκησις_t exc;
    if (ομφ_ἄσκησις_ἄρχε(&exc, &omf.σύνθεσις) < 0) {
        fprintf(stderr, "error: ομφ_ἄσκησις_ἄρχε defecit\n");
        ομφ_τέλει(&omf);
        free(signa);
        ομφ_λεκτήρ_κατάλυσον(lex);
        pfr_computo_fini();
        return 1;
    }

    unsigned int semen = 42u;
    int gradus_facti   = 0;
    while (gradus_facti < n_gradus) {
        int fascis = ASSERVATIO_GRADUS;
        if (gradus_facti + fascis > n_gradus)
            fascis = n_gradus - gradus_facti;

        exercita(
            &omf, &exc, signa, n_sig, fascis, PRAEDEF_AC_LONG,
            gradus_facti, &semen
        );
        gradus_facti += fascis;

        /* asservatio intermedia */
        if (ομφ_σῶσον(&omf, via_bin) == 0)
            printf(
                "  [asservatio: gradus %d/%d -> %s]\n",
                gradus_facti, n_gradus, via_bin
            );
        else
            fprintf(stderr, "  monitum: asservatio defecit\n");
        fflush(stdout);
    }

    ομφ_ἄσκησις_τέλει(&exc);
    free(signa);
    printf("\n");

    /* --- 5. salvatio finalis (iam facta per asservationem) --- */
    printf("5. exemplar servatum: %s\n\n", via_bin);

    ομφ_τέλει(&omf);
    ομφ_λεκτήρ_κατάλυσον(lex);
    pfr_computo_fini();

    printf("=== exercitatio perfecta ===\n");
    return 0;
}
