/*
 * δίκτυον.c — operationes retis GAN
 *
 * Initium, forward pass, backward pass, Ε/Ε.
 * FC strata per phantasma matvec/ger.
 */

#include "εἰκών.h"
#include "computo.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================
 * βοηθήματα — auxiliaria
 * ================================================================ */

/* LCG simplex */
static float randf(unsigned int *s)
{
    *s = *s * 1664525u + 1013904223u;
    return (*s >> 8) / 16777216.0f;
}

static inline float leaky_relu(float x)
{
    return x > 0.0f ? x : 0.2f * x;
}

static inline float leaky_relu_d(float x)
{
    return x > 0.0f ? 1.0f : 0.2f;
}

static inline float sigmoidf(float x)
{
    if (x > 15.0f)
        return 1.0f;
    if (x < -15.0f)
        return 0.0f;
    return 1.0f / (1.0f + expf(-x));
}

/* ================================================================
 * GPU ταμιεῖα — κατάγραψον ἅπαξ, χρησιμοποίησον πολλάκις
 *
 * pfr_in_gpu_mitte: si .gpu==NULL, allocat novum buffer.
 *                   si .gpu!=NULL, solum memcpy (nulla allocatio).
 * Ergo: primam vocationem cum .gpu=NULL allocat; sequentes reuti.
 * ================================================================ */

#define EIK_GPU_MAX 128

static struct {
    struct {
        void *cpu;
        void *gpu;
        size_t bytes;
        int est_matrix;
        int m, n;
    } t[EIK_GPU_MAX];
    int num;
} gpu_reg;

/* registra matrix — allocat GPU buffer, servat handle */
static void gpu_reg_m(float *cpu, int m, int n)
{
    if (gpu_reg.num >= EIK_GPU_MAX)
        return;
    pfr_matrix_f_t tmp = { m, n, cpu, NULL };
    if (pfr_in_gpu_mitte_f(&tmp) != 0 || !tmp.gpu)
        return;
    int i = gpu_reg.num++;
    gpu_reg.t[i].cpu = cpu;
    gpu_reg.t[i].gpu = tmp.gpu;
    gpu_reg.t[i].bytes = (size_t)m * n * sizeof(float);
    gpu_reg.t[i].est_matrix = 1;
    gpu_reg.t[i].m = m;
    gpu_reg.t[i].n = n;
}

/* registra vector */
static void gpu_reg_v(float *cpu, int n)
{
    if (gpu_reg.num >= EIK_GPU_MAX)
        return;
    pfr_vector_f_t tmp = { n, cpu, NULL };
    if (pfr_in_gpu_mitte_vf(&tmp) != 0 || !tmp.gpu)
        return;
    int i = gpu_reg.num++;
    gpu_reg.t[i].cpu = cpu;
    gpu_reg.t[i].gpu = tmp.gpu;
    gpu_reg.t[i].bytes = (size_t)n * sizeof(float);
    gpu_reg.t[i].est_matrix = 0;
    gpu_reg.t[i].m = 0;
    gpu_reg.t[i].n = n;
}

/* quaere GPU handle pro CPU pointer */
static void *gpu_find(const void *cpu)
{
    for (int i = 0; i < gpu_reg.num; i++)
        if (gpu_reg.t[i].cpu == cpu)
            return gpu_reg.t[i].gpu;
    return NULL;
}

/* ================================================================
 * εικ_gpu_ἄρχε — registra omnes buffers semel
 * ================================================================ */

void εικ_gpu_ἄρχε(εικ_t *ε, εικ_ἄσκησις_t *ἀ)
{
    const εικ_σύνθεσις_t *σ = &ε->σύνθεσις;
    int εδ = σ->εικ_πλάτος * σ->εικ_ὕψος;
    gpu_reg.num = 0;

    /* σταθμά (matrices) */
    gpu_reg_m(ε->σταθμά.γw1, σ->γ_κρυ1, σ->ζ_διαστ);
    gpu_reg_m(ε->σταθμά.γw2, σ->γ_κρυ2, σ->γ_κρυ1);
    gpu_reg_m(ε->σταθμά.γw3, εδ, σ->γ_κρυ2);
    gpu_reg_m(ε->σταθμά.κw1, σ->κ_κρυ1, εδ);
    gpu_reg_m(ε->σταθμά.κw2, σ->κ_κρυ2, σ->κ_κρυ1);
    gpu_reg_m(ε->σταθμά.κw3, 1, σ->κ_κρυ2);

    if (ἀ) {
        /* κλίσεις σταθμῶν (matrices) */
        gpu_reg_m(ἀ->κλ.γw1, σ->γ_κρυ1, σ->ζ_διαστ);
        gpu_reg_m(ἀ->κλ.γw2, σ->γ_κρυ2, σ->γ_κρυ1);
        gpu_reg_m(ἀ->κλ.γw3, εδ, σ->γ_κρυ2);
        gpu_reg_m(ἀ->κλ.κw1, σ->κ_κρυ1, εδ);
        gpu_reg_m(ἀ->κλ.κw2, σ->κ_κρυ2, σ->κ_κρυ1);
        gpu_reg_m(ἀ->κλ.κw3, 1, σ->κ_κρυ2);

        /* ἐνεργοποιήσεις (vectors) */
        gpu_reg_v(ἀ->γ_z,   σ->ζ_διαστ);
        gpu_reg_v(ἀ->γ_h1,  σ->γ_κρυ1);
        gpu_reg_v(ἀ->γ_a1,  σ->γ_κρυ1);
        gpu_reg_v(ἀ->γ_h2,  σ->γ_κρυ2);
        gpu_reg_v(ἀ->γ_a2,  σ->γ_κρυ2);
        gpu_reg_v(ἀ->γ_h3,  εδ);
        gpu_reg_v(ἀ->γ_out, εδ);

        gpu_reg_v(ἀ->κ_in,  εδ);
        gpu_reg_v(ἀ->κ_h1,  σ->κ_κρυ1);
        gpu_reg_v(ἀ->κ_a1,  σ->κ_κρυ1);
        gpu_reg_v(ἀ->κ_h2,  σ->κ_κρυ2);
        gpu_reg_v(ἀ->κ_a2,  σ->κ_κρυ2);

        /* temp buffers */
        int μεγδ = εδ;
        if (σ->γ_κρυ2 > μεγδ)
            μεγδ = σ->γ_κρυ2;
        if (σ->κ_κρυ1 > μεγδ)
            μεγδ = σ->κ_κρυ1;
        gpu_reg_v(ἀ->δ_tmp,  μεγδ);
        gpu_reg_v(ἀ->κ_δεικ, εδ);
    }

    /* inference buffers */
    int h1_sz = σ->γ_κρυ1 > σ->κ_κρυ1 ? σ->γ_κρυ1 : σ->κ_κρυ1;
    int h2_sz = σ->γ_κρυ2 > σ->κ_κρυ2 ? σ->γ_κρυ2 : σ->κ_κρυ2;
    gpu_reg_v(ε->inf_h1,  h1_sz);
    gpu_reg_v(ε->inf_h2,  h2_sz);
    gpu_reg_v(ε->inf_out, εδ);
}

/* ================================================================
 * FC forward: y = W * x + b
 * W: (m, n), x: (n,), b: (m,), y: (m,)
 * ================================================================ */

static void fc_fwd(
    float *y, const float *W, const float *b,
    const float *x, int m, int n
) {
    pfr_matrix_f_t mW = { m, n, (float *)(uintptr_t)W, gpu_find(W) };
    pfr_vector_f_t vx = { n, (float *)(uintptr_t)x, gpu_find(x) };
    pfr_vector_f_t vy = { m, y, gpu_find(y) };
    pfr_in_gpu_mitte_f(&mW);   /* reuti: solum memcpy */
    pfr_in_gpu_mitte_vf(&vx);
    pfr_in_gpu_mitte_vf(&vy);
    pfr_matvec_f(&vy, &mW, &vx);
    pfr_ex_gpu_cape_vf(&vy);
    for (int i = 0; i < m; i++)
        y[i] += b[i];
}

/* dx = W^T * dy,  dx zeroed first */
static void fc_bwd_x(
    float *dx, const float *W, const float *dy,
    int m, int n
) {
    pfr_matrix_f_t mW  = { m, n, (float *)(uintptr_t)W, gpu_find(W) };
    pfr_vector_f_t vdy = { m, (float *)(uintptr_t)dy, gpu_find(dy) };
    pfr_vector_f_t vdx = { n, dx, gpu_find(dx) };
    memset(dx, 0, (size_t)n * sizeof(float));
    pfr_in_gpu_mitte_f(&mW);
    pfr_in_gpu_mitte_vf(&vdy);
    pfr_in_gpu_mitte_vf(&vdx);
    pfr_matvec_trans_f(&vdx, &mW, &vdy);
    pfr_ex_gpu_cape_vf(&vdx);
}

/* dW += dy * x^T  (accumulate) */
static void fc_bwd_w(
    float *dW, const float *dy, const float *x,
    int m, int n
) {
    pfr_matrix_f_t mdW = { m, n, dW, gpu_find(dW) };
    pfr_vector_f_t vdy = { m, (float *)(uintptr_t)dy, gpu_find(dy) };
    pfr_vector_f_t vx  = { n, (float *)(uintptr_t)x, gpu_find(x) };
    pfr_in_gpu_mitte_f(&mdW);
    pfr_in_gpu_mitte_vf(&vdy);
    pfr_in_gpu_mitte_vf(&vx);
    pfr_ger_f(&mdW, 1.0f, &vdy, &vx);
    pfr_ex_gpu_cape_f(&mdW);
}

/* db += dy  (accumulate) */
static void fc_bwd_b(float *db, const float *dy, int m)
{
    for (int i = 0; i < m; i++)
        db[i] += dy[i];
}

/* ================================================================
 * μέγεθος σταθμῶν
 * ================================================================ */

size_t εικ_μέγεθος_σταθμῶν(const εικ_σύνθεσις_t *σ)
{
    int εδ   = σ->εικ_πλάτος * σ->εικ_ὕψος;
    size_t n = 0;
    /* γενέτης */
    n += (size_t)σ->γ_κρυ1 * σ->ζ_διαστ + σ->γ_κρυ1;
    n += (size_t)σ->γ_κρυ2 * σ->γ_κρυ1  + σ->γ_κρυ2;
    n += (size_t)εδ * σ->γ_κρυ2           + εδ;
    /* κριτής */
    n += (size_t)σ->κ_κρυ1 * εδ           + σ->κ_κρυ1;
    n += (size_t)σ->κ_κρυ2 * σ->κ_κρυ1   + σ->κ_κρυ2;
    n += (size_t)1 * σ->κ_κρυ2            + 1;
    return n;
}

/* ================================================================
 * ἄρχε δείκτ
 * ================================================================ */

float *εικ_σταθμά_ἄρχε_δείκτ(
    εικ_σταθμά_t *π, const εικ_σύνθεσις_t *σ, float *δ
) {
    int εδ   = σ->εικ_πλάτος * σ->εικ_ὕψος;
    float *p = δ;

    π->γw1 = p;
    p += (size_t)σ->γ_κρυ1 * σ->ζ_διαστ;
    π->γb1 = p;
    p += σ->γ_κρυ1;
    π->γw2 = p;
    p += (size_t)σ->γ_κρυ2 * σ->γ_κρυ1;
    π->γb2 = p;
    p += σ->γ_κρυ2;
    π->γw3 = p;
    p += (size_t)εδ * σ->γ_κρυ2;
    π->γb3 = p;
    p += εδ;

    π->κw1 = p;
    p += (size_t)σ->κ_κρυ1 * εδ;
    π->κb1 = p;
    p += σ->κ_κρυ1;
    π->κw2 = p;
    p += (size_t)σ->κ_κρυ2 * σ->κ_κρυ1;
    π->κb2 = p;
    p += σ->κ_κρυ2;
    π->κw3 = p;
    p += σ->κ_κρυ2;
    π->κb3 = p;
    p += 1;

    return p;
}

/* ================================================================
 * ἄρχε τυχαίως — Xavier initium
 * ================================================================ */

static void xavier_init(float *W, int m, int n, unsigned int *s)
{
    float scale = sqrtf(6.0f / (float)(m + n));
    for (int i = 0; i < m * n; i++)
        W[i] = (randf(s) * 2.0f - 1.0f) * scale;
}

static int eik_alloc_inf(εικ_t *ε)
{
    const εικ_σύνθεσις_t *σ = &ε->σύνθεσις;
    int h1_sz = σ->γ_κρυ1 > σ->κ_κρυ1 ? σ->γ_κρυ1 : σ->κ_κρυ1;
    int h2_sz = σ->γ_κρυ2 > σ->κ_κρυ2 ? σ->γ_κρυ2 : σ->κ_κρυ2;
    int εδ = σ->εικ_πλάτος * σ->εικ_ὕψος;
    ε->inf_h1  = malloc((size_t)h1_sz * sizeof(float));
    ε->inf_h2  = malloc((size_t)h2_sz * sizeof(float));
    ε->inf_out = malloc((size_t)εδ * sizeof(float));
    if (!ε->inf_h1 || !ε->inf_h2 || !ε->inf_out)
        return -1;
    return 0;
}

int εικ_ἄρχε_τυχαίως(εικ_t *ε, const εικ_σύνθεσις_t *σ, unsigned int σπέρμα)
{
    ε->σύνθεσις = *σ;
    size_t n    = εικ_μέγεθος_σταθμῶν(σ);
    ε->μέγεθος  = n;
    ε->δεδομένα = calloc(n, sizeof(float));
    if (!ε->δεδομένα)
        return -1;
    if (eik_alloc_inf(ε) < 0)
        return -1;

    εικ_σταθμά_ἄρχε_δείκτ(&ε->σταθμά, σ, ε->δεδομένα);

    int εδ         = σ->εικ_πλάτος * σ->εικ_ὕψος;
    unsigned int s = σπέρμα;

    xavier_init(ε->σταθμά.γw1, σ->γ_κρυ1, σ->ζ_διαστ, &s);
    xavier_init(ε->σταθμά.γw2, σ->γ_κρυ2, σ->γ_κρυ1,  &s);
    xavier_init(ε->σταθμά.γw3, εδ,         σ->γ_κρυ2,  &s);

    xavier_init(ε->σταθμά.κw1, σ->κ_κρυ1, εδ,          &s);
    xavier_init(ε->σταθμά.κw2, σ->κ_κρυ2, σ->κ_κρυ1,  &s);
    xavier_init(ε->σταθμά.κw3, 1,          σ->κ_κρυ2,  &s);

    /* biases iam 0 (calloc) */
    return 0;
}

/* ================================================================
 * Ε/Ε — ἀνάγνωσις καὶ σωτηρία
 * ================================================================ */

#define EIK_MAGIC 0x454B4931  /* "EIK1" */

int εικ_σῶσον(const εικ_t *ε, const char *ὁδός)
{
    FILE *f = fopen(ὁδός, "wb");
    if (!f)
        return -1;

    unsigned int mag = EIK_MAGIC;
    fwrite(&mag, 4, 1, f);
    fwrite(&ε->σύνθεσις, sizeof(εικ_σύνθεσις_t), 1, f);
    fwrite(ε->δεδομένα, sizeof(float), ε->μέγεθος, f);
    fclose(f);
    return 0;
}

int εικ_ἀνάγνωθι(εικ_t *ε, const char *ὁδός)
{
    FILE *f = fopen(ὁδός, "rb");
    if (!f)
        return -1;

    unsigned int mag;
    if (fread(&mag, 4, 1, f) != 1 || mag != EIK_MAGIC) {
        fclose(f);
        return -1;
    }

    if (fread(&ε->σύνθεσις, sizeof(εικ_σύνθεσις_t), 1, f) != 1) {
        fclose(f);
        return -1;
    }

    size_t n    = εικ_μέγεθος_σταθμῶν(&ε->σύνθεσις);
    ε->μέγεθος  = n;
    ε->δεδομένα = malloc(n * sizeof(float));
    if (!ε->δεδομένα) {
        fclose(f);
        return -1;
    }

    if (fread(ε->δεδομένα, sizeof(float), n, f) != n) {
        free(ε->δεδομένα);
        ε->δεδομένα = NULL;
        fclose(f);
        return -1;
    }

    εικ_σταθμά_ἄρχε_δείκτ(&ε->σταθμά, &ε->σύνθεσις, ε->δεδομένα);
    if (eik_alloc_inf(ε) < 0) {
        free(ε->δεδομένα);
        ε->δεδομένα = NULL;
        fclose(f);
        return -1;
    }
    fclose(f);
    return 0;
}

void εικ_τέλει(εικ_t *ε)
{
    free(ε->δεδομένα);
    free(ε->inf_h1);
    free(ε->inf_h2);
    free(ε->inf_out);
    ε->δεδομένα = NULL;
}

/* ================================================================
 * γενέτης πρόσω (inferentia, sine memoria)
 * ================================================================ */

void εικ_γεν_πρόσω(εικ_t *ε, const float *z, float *ἔξοδος)
{
    const εικ_σύνθεσις_t *σ = &ε->σύνθεσις;
    const εικ_σταθμά_t *π   = &ε->σταθμά;
    int εδ = σ->εικ_πλάτος * σ->εικ_ὕψος;
    float *h1 = ε->inf_h1;
    float *h2 = ε->inf_h2;

    /* L1: z -> γ_κρυ1, LeakyReLU */
    fc_fwd(h1, π->γw1, π->γb1, z, σ->γ_κρυ1, σ->ζ_διαστ);
    for (int i = 0; i < σ->γ_κρυ1; i++)
        h1[i] = leaky_relu(h1[i]);

    /* L2: γ_κρυ1 -> γ_κρυ2, LeakyReLU */
    fc_fwd(h2, π->γw2, π->γb2, h1, σ->γ_κρυ2, σ->γ_κρυ1);
    for (int i = 0; i < σ->γ_κρυ2; i++)
        h2[i] = leaky_relu(h2[i]);

    /* L3: γ_κρυ2 -> εικ_διαστ, Tanh */
    fc_fwd(ἔξοδος, π->γw3, π->γb3, h2, εδ, σ->γ_κρυ2);
    for (int i = 0; i < εδ; i++)
        ἔξοδος[i] = tanhf(ἔξοδος[i]);
}

/* ================================================================
 * γενέτης πρόσω μετὰ μνήμης
 * ================================================================ */

void εικ_γεν_πρόσω_μνήμη(εικ_t *ε, εικ_ἄσκησις_t *ἀ, const float *z)
{
    const εικ_σύνθεσις_t *σ = &ε->σύνθεσις;
    const εικ_σταθμά_t *π   = &ε->σταθμά;
    int εδ = σ->εικ_πλάτος * σ->εικ_ὕψος;

    memcpy(ἀ->γ_z, z, (size_t)σ->ζ_διαστ * sizeof(float));

    /* L1 */
    fc_fwd(ἀ->γ_h1, π->γw1, π->γb1, z, σ->γ_κρυ1, σ->ζ_διαστ);
    for (int i = 0; i < σ->γ_κρυ1; i++)
        ἀ->γ_a1[i] = leaky_relu(ἀ->γ_h1[i]);

    /* L2 */
    fc_fwd(ἀ->γ_h2, π->γw2, π->γb2, ἀ->γ_a1, σ->γ_κρυ2, σ->γ_κρυ1);
    for (int i = 0; i < σ->γ_κρυ2; i++)
        ἀ->γ_a2[i] = leaky_relu(ἀ->γ_h2[i]);

    /* L3 */
    fc_fwd(ἀ->γ_h3, π->γw3, π->γb3, ἀ->γ_a2, εδ, σ->γ_κρυ2);
    for (int i = 0; i < εδ; i++)
        ἀ->γ_out[i] = tanhf(ἀ->γ_h3[i]);
}

/* ================================================================
 * κριτὴς πρόσω (inferentia)
 * ================================================================ */

float εικ_κρι_πρόσω(εικ_t *ε, const float *εικόνα)
{
    const εικ_σύνθεσις_t *σ = &ε->σύνθεσις;
    const εικ_σταθμά_t *π   = &ε->σταθμά;
    int εδ = σ->εικ_πλάτος * σ->εικ_ὕψος;
    float *h1 = ε->inf_h1;
    float *h2 = ε->inf_h2;

    fc_fwd(h1, π->κw1, π->κb1, εικόνα, σ->κ_κρυ1, εδ);
    for (int i = 0; i < σ->κ_κρυ1; i++)
        h1[i] = leaky_relu(h1[i]);

    fc_fwd(h2, π->κw2, π->κb2, h1, σ->κ_κρυ2, σ->κ_κρυ1);
    for (int i = 0; i < σ->κ_κρυ2; i++)
        h2[i] = leaky_relu(h2[i]);

    float out;
    fc_fwd(&out, π->κw3, π->κb3, h2, 1, σ->κ_κρυ2);
    out = sigmoidf(out);

    return out;
}

/* ================================================================
 * κριτὴς πρόσω μετὰ μνήμης
 * ================================================================ */

float εικ_κρι_πρόσω_μνήμη(εικ_t *ε, εικ_ἄσκησις_t *ἀ, const float *εικόνα)
{
    const εικ_σύνθεσις_t *σ = &ε->σύνθεσις;
    const εικ_σταθμά_t *π   = &ε->σταθμά;
    int εδ = σ->εικ_πλάτος * σ->εικ_ὕψος;

    memcpy(ἀ->κ_in, εικόνα, (size_t)εδ * sizeof(float));

    fc_fwd(ἀ->κ_h1, π->κw1, π->κb1, εικόνα, σ->κ_κρυ1, εδ);
    for (int i = 0; i < σ->κ_κρυ1; i++)
        ἀ->κ_a1[i] = leaky_relu(ἀ->κ_h1[i]);

    fc_fwd(ἀ->κ_h2, π->κw2, π->κb2, ἀ->κ_a1, σ->κ_κρυ2, σ->κ_κρυ1);
    for (int i = 0; i < σ->κ_κρυ2; i++)
        ἀ->κ_a2[i] = leaky_relu(ἀ->κ_h2[i]);

    fc_fwd(&ἀ->κ_h3, π->κw3, π->κb3, ἀ->κ_a2, 1, σ->κ_κρυ2);
    ἀ->κ_out = sigmoidf(ἀ->κ_h3);

    return ἀ->κ_out;
}

/* ================================================================
 * κριτὴς ἀναδρομή — BCE loss, σωρεύει κλίσεις κριτοῦ
 *
 * στόχος: 1.0 = ἀληθής, 0.0 = ψευδής
 * Πρέπει εικ_κρι_πρόσω_μνήμη νὰ κληθῇ πρωτίστως.
 * Ἀποδίδει BCE loss. Σῴζει κ_δεικ πρὸς γεν_ἀναδρομή.
 * ================================================================ */

float εικ_κρι_ἀναδρομή(εικ_t *ε, εικ_ἄσκησις_t *ἀ, float στόχος)
{
    const εικ_σύνθεσις_t *σ = &ε->σύνθεσις;
    const εικ_σταθμά_t *π   = &ε->σταθμά;
    int εδ = σ->εικ_πλάτος * σ->εικ_ὕψος;

    /* BCE loss */
    float y    = ἀ->κ_out;
    float eps  = 1e-7f;
    float loss = -(στόχος * logf(y + eps) + (1.0f - στόχος) * logf(1.0f - y + eps));

    /* δ3 = y - target (sigmoid + BCE combined gradient) */
    float d3 = y - στόχος;

    /* L3 backward: 1 x κ_κρυ2 */
    fc_bwd_w(ἀ->κλ.κw3, &d3, ἀ->κ_a2, 1, σ->κ_κρυ2);
    fc_bwd_b(ἀ->κλ.κb3, &d3, 1);

    /* δ2 = κw3^T * d3, elementwise * leaky_relu'(h2)
     * NB: δ2 in κ_δεικ, δ1 in δ_tmp — sic nulla collisio */
    float *δ2 = ἀ->κ_δεικ;
    fc_bwd_x(δ2, π->κw3, &d3, 1, σ->κ_κρυ2);
    for (int i = 0; i < σ->κ_κρυ2; i++)
        δ2[i] *= leaky_relu_d(ἀ->κ_h2[i]);

    /* L2 backward */
    fc_bwd_w(ἀ->κλ.κw2, δ2, ἀ->κ_a1, σ->κ_κρυ2, σ->κ_κρυ1);
    fc_bwd_b(ἀ->κλ.κb2, δ2, σ->κ_κρυ2);

    /* δ1 = κw2^T * δ2, * leaky_relu'(h1) */
    float *δ1 = ἀ->δ_tmp;
    fc_bwd_x(δ1, π->κw2, δ2, σ->κ_κρυ2, σ->κ_κρυ1);
    for (int i = 0; i < σ->κ_κρυ1; i++)
        δ1[i] *= leaky_relu_d(ἀ->κ_h1[i]);

    /* L1 backward */
    fc_bwd_w(ἀ->κλ.κw1, δ1, ἀ->κ_in, σ->κ_κρυ1, εδ);
    fc_bwd_b(ἀ->κλ.κb1, δ1, σ->κ_κρυ1);

    /* κλίσις ὡς πρὸς εἴσοδον (πρὸς γεν backward) —
     * δ1 in δ_tmp, κ_δεικ separatum, nulla collisio */
    fc_bwd_x(ἀ->κ_δεικ, π->κw1, δ1, σ->κ_κρυ1, εδ);

    return loss;
}

/* ================================================================
 * γενέτης ἀναδρομή — χρησιμοποιεῖ κ_δεικ ὡς κλίσιν ἐξόδου
 *
 * Πρέπει εικ_γεν_πρόσω_μνήμη + εικ_κρι_πρόσω_μνήμη + εικ_κρι_ἀναδρομή
 * νὰ κληθοῦν πρωτίστως.
 * ================================================================ */

void εικ_γεν_ἀναδρομή(εικ_t *ε, εικ_ἄσκησις_t *ἀ)
{
    const εικ_σύνθεσις_t *σ = &ε->σύνθεσις;
    const εικ_σταθμά_t *π   = &ε->σταθμά;
    int εδ = σ->εικ_πλάτος * σ->εικ_ὕψος;

    /* δ3 = κ_δεικ * tanh'(h3) */
    float *δ3 = ἀ->δ_tmp;
    for (int i = 0; i < εδ; i++) {
        float t = ἀ->γ_out[i];
        δ3[i]   = ἀ->κ_δεικ[i] * (1.0f - t * t);
    }

    /* L3 backward */
    fc_bwd_w(ἀ->κλ.γw3, δ3, ἀ->γ_a2, εδ, σ->γ_κρυ2);
    fc_bwd_b(ἀ->κλ.γb3, δ3, εδ);

    /* δ2 */
    float *δ2 = ἀ->κ_δεικ; /* re-use (safe: κ_δεικ consumed) */
    fc_bwd_x(δ2, π->γw3, δ3, εδ, σ->γ_κρυ2);
    for (int i = 0; i < σ->γ_κρυ2; i++)
        δ2[i] *= leaky_relu_d(ἀ->γ_h2[i]);

    fc_bwd_w(ἀ->κλ.γw2, δ2, ἀ->γ_a1, σ->γ_κρυ2, σ->γ_κρυ1);
    fc_bwd_b(ἀ->κλ.γb2, δ2, σ->γ_κρυ2);

    /* δ1 */
    float *δ1 = ἀ->δ_tmp;
    fc_bwd_x(δ1, π->γw2, δ2, σ->γ_κρυ2, σ->γ_κρυ1);
    for (int i = 0; i < σ->γ_κρυ1; i++)
        δ1[i] *= leaky_relu_d(ἀ->γ_h1[i]);

    fc_bwd_w(ἀ->κλ.γw1, δ1, ἀ->γ_z, σ->γ_κρυ1, σ->ζ_διαστ);
    fc_bwd_b(ἀ->κλ.γb1, δ1, σ->γ_κρυ1);
}
