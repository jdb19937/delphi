/*
 * proba_omphalos.c — probatio transformatoris omphalos
 *
 * 1. Legit corpus ex corpus/dieta_1526_*.txt
 * 2. Exercitat lexatorem BPE
 * 3. Initiat exemplar parvum
 * 4. Exercitat per N passus, imprimat damnum
 * 5. Servat exemplar et lexatorem
 * 6. Legit exemplar novum
 * 7. Generat textum ex promptu
 * 8. Probat interfaciem provisoris omphalos
 */

#include "omphalos/ὀμφαλός.h"
#include "omphalos/λεκτήρ.h"
#include "phantasma/computo.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================
 * lectio corporis
 * ================================================================ */

static char *lege_plicam(const char *via)
{
    FILE *f = fopen(via, "rb");
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    long mag = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc((size_t)mag + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    fread(buf, 1, (size_t)mag, f);
    buf[mag] = '\0';
    fclose(f);
    return buf;
}

static char *lege_corpus(void)
{
    const char *plicae[] = {
        "corpus/dieta_1526_i.txt",
        "corpus/dieta_1526_ii.txt",
        "corpus/dieta_1526_iii.txt",
        "corpus/dieta_1526_iv.txt",
        "corpus/dieta_1526_v.txt",
        "corpus/dieta_1526_vi.txt",
    };
    int n_plicae = 6;

    /* summa magnitudine */
    size_t mag_tot = 0;
    char **partes  = malloc((size_t)n_plicae * sizeof(char *));
    if (!partes)
        return NULL;
    for (int i = 0; i < n_plicae; i++) {
        partes[i] = lege_plicam(plicae[i]);
        if (partes[i])
            mag_tot += strlen(partes[i]) + 1;
        else
            fprintf(stderr, "admonitio: non potui legere '%s'\n", plicae[i]);
    }

    char *corpus = malloc(mag_tot + 1);
    if (!corpus) {
        for (int i = 0; i < n_plicae; i++)
            free(partes[i]);
        free(partes);
        return NULL;
    }
    corpus[0] = '\0';
    for (int i = 0; i < n_plicae; i++) {
        if (partes[i]) {
            strcat(corpus, partes[i]);
            strcat(corpus, "\n");
            free(partes[i]);
        }
    }
    free(partes);
    return corpus;
}

/* ================================================================
 * exercitatio
 * ================================================================ */

#define N_PASSUS       512
#define LONGITUDO_SEQ  48
#define N_MINI         4  /* fenestrae per passum (mini-manipulus) */
#define GRADUS_DISC    3e-3f
#define BETA1          0.9f
#define BETA2          0.999f
#define EPSILON        1e-8f
#define DESICATIO      1e-2f
#define NUNTIA_PASSUS  10

static void exercita(
    ομφ_t *ομφ, ομφ_ἄσκησις_t *ἀσκ,
    const int *signa_corp, int n_corp,
    int n_passus
) {
    int lm = ομφ->σύνθεσις.μῆκος_μέγ;
    int seq_lon = LONGITUDO_SEQ < lm ? LONGITUDO_SEQ : lm - 1;
    unsigned int semen = 42u;
    int max_inc = n_corp - seq_lon - 2;
    if (max_inc <= 0)
        max_inc = 1;

    for (int passus = 0; passus < n_passus; passus++) {
        ομφ_κλίσεις_μηδένισον(ἀσκ, &ομφ->σύνθεσις);
        float damnum_tot = 0.0f;

        for (int mini = 0; mini < N_MINI; mini++) {
            semen   = semen * 1664525u + 1013904223u;
            int inc = (int)(semen % (unsigned int)max_inc);

            ομφ_κατάστ_ἀποκατάστησον(ομφ);
            for (int t = 0; t < seq_lon && inc + t + 1 < n_corp; t++) {
                int sig  = signa_corp[inc + t];
                int targ = signa_corp[inc + t + 1];
                ομφ_δρόμος_μνήμη(ομφ, ἀσκ, sig, t);
                damnum_tot += ομφ_ἀναδρομή(ομφ, ἀσκ, sig, targ, t);
            }
        }

        ομφ_κλίσεις_κεῖρον(ἀσκ, &ομφ->σύνθεσις, 1.0f);
        ομφ_βῆμα_ἀδάμ(ομφ, ἀσκ, GRADUS_DISC, BETA1, BETA2, EPSILON, DESICATIO);

        if ((passus + 1) % NUNTIA_PASSUS == 0) {
            float damnum_med = damnum_tot / (float)(seq_lon * N_MINI);
            printf(
                "  passus %4d/%d  damnum=%.4f  perplexitas=%.2f\n",
                passus + 1, n_passus,
                damnum_med, expf(damnum_med)
            );
            fflush(stdout);
        }
    }
}

/* ================================================================
 * generatio textus
 * ================================================================ */

static void genera_et_imprime(
    ομφ_t *ομφ, ομφ_λεκτήρ_t *λεκ,
    const char *promptus, int n_genera
) {
    ομφ_σύνθεσις_t *σ = &ομφ->σύνθεσις;
    int V = σ->λεξ_μέγεθος;

    int n_buf  = (int)strlen(promptus) + 4;
    int *signa = malloc((size_t)n_buf * sizeof(int));
    if (!signa)
        return;
    int n_sig = 0;
    ομφ_λεκτήρ_τέμε(λεκ, promptus, 1, 0, signa, &n_sig);

    ομφ_κατάστ_ἀποκατάστησον(ομφ);
    float *logitae = NULL;
    for (int t = 0; t < n_sig && t < σ->μῆκος_μέγ - 1; t++)
        logitae = ομφ_δρόμος(ομφ, signa[t], t);
    int pos = n_sig < σ->μῆκος_μέγ ? n_sig : σ->μῆκος_μέγ - 1;
    free(signa);

    printf("%s", promptus);
    fflush(stdout);

    unsigned int semen = 12345u;
    for (int i = 0; i < n_genera && pos < σ->μῆκος_μέγ && logitae; i++) {
        /* sampling cum temperatura 0.9 */
        float *prob = malloc((size_t)V * sizeof(float));
        if (!prob)
            break;
        memcpy(prob, logitae, (size_t)V * sizeof(float));
        for (int j = 0; j < V; j++)
            prob[j] /= 0.9f;
        float mx = prob[0];
        for (int j = 1; j < V; j++)
            if (prob[j] > mx)
                mx = prob[j];
        float s = 0.0f;
        for (int j = 0; j < V; j++) {
            prob[j] = expf(prob[j] - mx);
            s += prob[j];
        }
        for (int j = 0; j < V; j++)
            prob[j] /= s;
        semen     = semen * 1664525u + 1013904223u;
        float r   = ((float)(semen & 0x7fffffff)) / (float)0x7fffffff;
        float cdf = 0.0f;
        int prox  = V - 1;
        for (int j = 0; j < V; j++) {
            cdf += prob[j];
            if (r < cdf) {
                prox = j;
                break;
            }
        }
        free(prob);
        if (prox == 2)
            break;
        printf("%s", ομφ_λεκτήρ_ἀπόδος(λεκ, prox));
        fflush(stdout);
        logitae = ομφ_δρόμος(ομφ, prox, pos++);
    }
    printf("\n");
}

/* ================================================================
 * main
 * ================================================================ */

int main(void)
{
    printf("=== proba_omphalos ===\n\n");

    /* initia computo (GPU vel CPU fallback) */
    int gpu = pfr_computo_initia();
    printf("computo: %s\n\n", gpu == 0 ? "GPU" : "CPU");

    /* --- 1. lege corpus --- */
    printf("1. lego corpus...\n");
    char *corpus = lege_corpus();
    if (!corpus || strlen(corpus) < 10) {
        fprintf(stderr, "corpus vac vel non inventum\n");
        return 1;
    }
    printf("   longitudo corporis: %zu characteres\n\n", strlen(corpus));

    /* --- 2. exercita lexatorem --- */
    printf("2. exercito lexatorem BPE (vocab_max=512)...\n");
    ομφ_λεκτήρ_t *λεκ = ομφ_λεκτήρ_κτίσον(1024);
    if (!λεκ || ομφ_λεκτήρ_ἄσκησον(λεκ, corpus) < 0) {
        fprintf(stderr, "lexator exercitatio defecit\n");
        free(corpus);
        return 1;
    }
    printf("   vocab_magnitudo: %d\n", λεκ->λεξ_μέγεθος);

    /* tokeniza corpus */
    int corp_lon    = (int)strlen(corpus);
    int *signa_corp = malloc((size_t)(corp_lon + 4) * sizeof(int));
    if (!signa_corp) {
        free(corpus);
        ομφ_λεκτήρ_κατάλυσον(λεκ);
        return 1;
    }
    int n_signa_corp = 0;
    ομφ_λεκτήρ_τέμε(λεκ, corpus, 0, 0, signa_corp, &n_signa_corp);
    printf("   signa in corpore: %d\n\n", n_signa_corp);
    free(corpus);

    /* serva lexatorem */
    if (ομφ_λεκτήρ_σῶσον(λεκ, "omphalos_dieta_1526.lex") < 0)
        fprintf(stderr, "admonitio: lexator servari non potuit\n");
    else
        printf("   lexator servatus: omphalos_dieta_1526.lex\n\n");

    /* --- 3. initia exemplar --- */
    printf(
        "3. initio exemplar (dim=64, strata=4, capita=4, vocab=%d)...\n",
        λεκ->λεξ_μέγεθος
    );
    ομφ_σύνθεσις_t σύνθεσις = {
        .διάστασις     = 64,
        .διάστ_κρυ     = 172,
        .στρώματα      = 4,
        .κεφαλαί       = 4,
        .κεφαλαί_κτ    = 4,
        .λεξ_μέγεθος   = λεκ->λεξ_μέγεθος,
        .μῆκος_μέγ     = 64,
    };
    ομφ_t ομφ;
    if (ομφ_ἄρχε_τυχαίως(&ομφ, &σύνθεσις, 42u) < 0) {
        fprintf(stderr, "ομφ_ἄρχε_τυχαίως defecit\n");
        free(signa_corp);
        ομφ_λεκτήρ_κατάλυσον(λεκ);
        return 1;
    }
    size_t n_param = ομφ_μέγεθος_σταθμῶν(&σύνθεσις, 1);
    printf(
        "   parametri: %zu floats = %.1f KB\n\n",
        n_param, n_param * sizeof(float) / 1024.0
    );

    /* GPU: mitte pondera et activationes in GPU (si adest et prodest) */
    if (gpu == 0 && σύνθεσις.διάστασις >= 256) {
        if (ομφ_gpu_ἄρχε(&ομφ) == 0) {
            printf("   GPU buffers allocati\n\n");
        } else {
            printf("   GPU buffers defecerunt — CPU adhibetur\n\n");
        }
    } else if (gpu == 0) {
        printf(
            "   dim=%d < 256: GPU non prodest, CPU adhibetur\n\n",
            σύνθεσις.διάστασις
        );
    }

    /* --- 4. exercita --- */
    printf("4. exercito (%d passus)...\n", N_PASSUS);
    ομφ_ἄσκησις_t ἀσκ;
    if (ομφ_ἄσκησις_ἄρχε(&ἀσκ, &σύνθεσις) < 0) {
        fprintf(stderr, "ομφ_ἄσκησις_ἄρχε defecit\n");
        ομφ_τέλει(&ομφ);
        free(signa_corp);
        ομφ_λεκτήρ_κατάλυσον(λεκ);
        return 1;
    }
    exercita(&ομφ, &ἀσκ, signa_corp, n_signa_corp, N_PASSUS);
    ομφ_ἄσκησις_τέλει(&ἀσκ);
    free(signa_corp);
    printf("\n");

    /* --- 5. serva --- */
    printf("5. servo exemplar...\n");
    if (ομφ_σῶσον(&ομφ, "omphalos_dieta_1526.bin") < 0)
        fprintf(stderr, "admonitio: ομφ_σῶσον defecit\n");
    else
        printf("   exemplar servatum: omphalos_dieta_1526.bin\n\n");
    ομφ_gpu_τέλει();
    ομφ_τέλει(&ομφ);

    /* --- 6. lege exemplar novum --- */
    printf("6. lego exemplar ex plica...\n");
    ομφ_t ομφ2;
    if (ομφ_ἀνάγνωθι(&ομφ2, "omphalos_dieta_1526.bin") < 0) {
        fprintf(stderr, "ομφ_ἀνάγνωθι defecit\n");
        ομφ_λεκτήρ_κατάλυσον(λεκ);
        pfr_computo_fini();
        return 1;
    }
    printf(
        "   exemplar lectum: dim=%d strata=%d vocab=%d\n",
        ομφ2.σύνθεσις.διάστασις, ομφ2.σύνθεσις.στρώματα,
        ομφ2.σύνθεσις.λεξ_μέγεθος
    );
    if (gpu == 0 && ομφ2.σύνθεσις.διάστασις >= 256)
        ομφ_gpu_ἄρχε(&ομφ2);
    printf("\n");

    /* --- 7. inferentiae --- */
    printf("7. genero textum...\n");
    printf("   [promptus: \"Carolus Imperator\"]\n   ");
    genera_et_imprime(&ομφ2, λεκ, "Carolus Imperator", 80);
    printf("\n");
    printf("   [promptus: \"Diomedes\"]\n   ");
    genera_et_imprime(&ομφ2, λεκ, "Diomedes", 80);
    printf("\n");
    printf("   [promptus: \"avis\"]\n   ");
    genera_et_imprime(&ομφ2, λεκ, "avis", 80);
    printf("\n");

    /* --- 8. proba generationis directa --- */
    printf("8. probo generationem directam ex lexatore novo...\n");
    {
        ομφ_λεκτήρ_t *λεκ2 = ομφ_λεκτήρ_κτίσον(ΟΜΦ_ΛΕΞ_ΛΕΞΙΚΟΝ_ΠΡΟΕΠΙΛ);
        if (λεκ2 && ομφ_λεκτήρ_ἀνάγνωθι(λεκ2, "omphalos_dieta_1526.lex") == 0) {
            printf("   [promptus: \"bestia\"]\n   ");
            genera_et_imprime(&ομφ2, λεκ2, "bestia", 64);
            ομφ_λεκτήρ_κατάλυσον(λεκ2);
        } else {
            printf("   lexator novus legi non potuit\n");
            if (λεκ2)
                ομφ_λεκτήρ_κατάλυσον(λεκ2);
        }
    }
    printf("\n");

    ομφ_gpu_τέλει();
    ομφ_τέλει(&ομφ2);
    ομφ_λεκτήρ_κατάλυσον(λεκ);
    pfr_computo_fini();

    printf("=== proba finita ===\n");
    return 0;
}
