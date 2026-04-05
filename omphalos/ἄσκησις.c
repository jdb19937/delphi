/*
 * ἄσκησις.c — ἀναδρομὴ καὶ ἄσκησις τοῦ μεταμορφωτοῦ
 *
 * Backward pass (περικεκομμένη BPTT κατὰ θέσιν) + βελτιστοποιητὴς AdamW.
 * Κλίσεις σωρεύονται ἐν ἀσκ->κλίσεις, εἶτα βῆμα_ἀδάμ ἐνημερώνει σταθμά.
 */

#include "ὀμφαλός.h"
#include "phantasma/computo.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================
 * ἐσωτερικὰ βοηθήματα — διὰ pfr API (GPU ὅπου ὑπάρχει, CPU ὡς ἐφεδρεία)
 * ================================================================ */

/*
 * πίν_διαν_ἀνάστρ_σώρ — y += W^T * x  διὰ pfr_matvec_trans_f
 * W εἶναι (ἐξ_διαστ, εἰσ_διαστ), x εἶναι (ἐξ_διαστ,), y εἶναι (εἰσ_διαστ,).
 */
static void πίν_διαν_ἀνάστρ_σώρ(
    float *y, const float *W,
    const float *x, int ἐξ_διαστ, int εἰσ_διαστ
) {
    pfr_matrix_f_t A  = { ἐξ_διαστ, εἰσ_διαστ, (float *)W, NULL };
    pfr_vector_f_t xv = { ἐξ_διαστ, (float *)x, NULL };
    float *tmp = calloc((size_t)εἰσ_διαστ, sizeof(float));
    if (!tmp)
        return;
    pfr_vector_f_t tv = { εἰσ_διαστ, tmp, NULL };
    pfr_matvec_trans_f(&tv, &A, &xv);
    for (int i = 0; i < εἰσ_διαστ; i++)
        y[i] += tmp[i];
    free(tmp);
}

/*
 * ἐξωτ_γιν_σώρ — W += ἄλφα * x * y^T  διὰ pfr_ger_f
 * W εἶναι (μ, ν), x εἶναι (μ,), y εἶναι (ν,).
 */
static void ἐξωτ_γιν_σώρ(
    float *W, float ἄλφα,
    const float *x, const float *y, int μ, int ν
) {
    pfr_matrix_f_t A  = { μ, ν, W, NULL };
    pfr_vector_f_t xv = { μ, (float *)x, NULL };
    pfr_vector_f_t yv = { ν, (float *)y, NULL };
    pfr_ger_f(&A, ἄλφα, &xv, &yv);
}

/*
 * rms_κανόν_ἀναδρ — κλίσεις πρὸς RMSNorm.
 * δ_x += κλίσ_x, δ_σ += κλίσ_σ  (σωρευτικῶς).
 * x: εἴσοδος κανονισμοῦ, σ: σταθμὰ κανονισμοῦ, δ_y: κλίσις ἐξόδου.
 */
static void rms_κανόν_ἀναδρ(
    float *δ_x, float *δ_σ,
    const float *δ_y, const float *x,
    const float *σ, int ν
) {
    float ἄθρ = 0.0f;
    for (int i = 0; i < ν; i++)
        ἄθρ += x[i] * x[i];
    float ρ = 1.0f / sqrtf(ἄθρ / ν + 1e-5f);  /* = 1 / rms */

    for (int i = 0; i < ν; i++)
        δ_σ[i] += δ_y[i] * x[i] * ρ;

    float Α = 0.0f;
    for (int j = 0; j < ν; j++)
        Α += δ_y[j] * σ[j] * x[j];

    float γ = ρ * ρ * ρ / ν;
    for (int j = 0; j < ν; j++)
        δ_x[j] += ρ * σ[j] * δ_y[j] - γ * x[j] * Α;
}

/*
 * softmax_ἀναδρ — κλίσεις πρὸς softmax.
 * δ_εἰσ[i] = p[i] * (δ_ἐξ[i] - dot(δ_ἐξ, p)).
 */
static void softmax_ἀναδρ(
    float *δ_εἰσ, const float *δ_ἐξ,
    const float *p, int ν
) {
    float ἐσωτ = 0.0f;
    for (int i = 0; i < ν; i++)
        ἐσωτ += δ_ἐξ[i] * p[i];
    for (int i = 0; i < ν; i++)
        δ_εἰσ[i] = p[i] * (δ_ἐξ[i] - ἐσωτ);
}

/*
 * rope_ἀναδρ — κλίσεις πρὸς RoPE (ἀνάστροφος στροφή).
 */
static void rope_ἀναδρ(float *δ_v, int θέσις, int κεφ_διαστ)
{
    for (int i = 0; i < κεφ_διαστ; i += 2) {
        float συχν = 1.0f / powf(10000.0f, (float)i / κεφ_διαστ);
        float τιμ  = θέσις * συχν;
        float fcr  = cosf(τιμ), fci = sinf(τιμ);
        float v0   = δ_v[i], v1 = δ_v[i + 1];
        /* ἀνάστροφος στροφή: ἀρνητικὴ γωνία */
        δ_v[i]     = v0 * fcr + v1 * fci;
        δ_v[i + 1] = -v0 * fci + v1 * fcr;
    }
}

/* ================================================================
 * ἐκχώρησον / ἐλευθέρωσον ἄσκησιν
 * ================================================================ */

static float *ἐκχώρ_σταθμά_ἐπίπεδα(
    const ομφ_σύνθεσις_t *σ, int κοινά,
    ομφ_σταθμά_t *π
) {
    size_t ν    = ομφ_μέγεθος_σταθμῶν(σ, κοινά);
    float *δεδ  = calloc(ν, sizeof(float));
    if (!δεδ)
        return NULL;
    float *μετά = ομφ_σταθμά_ἄρχε_δείκτ(π, σ, δεδ);
    if (κοινά)
        π->σλεξ = π->ἴχνη;
    else
        π->σλεξ = μετά; /* μετὰ rms_τέλ */
    return δεδ;
}

int ομφ_ἄσκησις_ἄρχε(ομφ_ἄσκησις_t *ἀσκ, const ομφ_σύνθεσις_t *σ)
{
    memset(ἀσκ, 0, sizeof(*ἀσκ));

    /* κλίσεις (κοινά = 1 ὡς ἐν ὑποδείγματι) */
    ἀσκ->κλίσ_δεδ = ἐκχώρ_σταθμά_ἐπίπεδα(σ, 1, &ἀσκ->κλίσεις);
    if (!ἀσκ->κλίσ_δεδ)
        return -1;

    ἀσκ->ὁρμή_δεδ = ἐκχώρ_σταθμά_ἐπίπεδα(σ, 1, &ἀσκ->ὁρμή);
    if (!ἀσκ->ὁρμή_δεδ)
        return -1;

    ἀσκ->δύναμις_δεδ = ἐκχώρ_σταθμά_ἐπίπεδα(σ, 1, &ἀσκ->δύναμις);
    if (!ἀσκ->δύναμις_δεδ)
        return -1;

    int δ    = σ->διάστασις;
    int δκ   = σ->διάστ_κρυ;
    int Λ    = σ->στρώματα;
    int Κ    = σ->κεφαλαί;
    int Κκτ  = σ->κεφαλαί_κτ;
    int κτ_δ = (δ / Κ) * Κκτ;
    int μμ   = σ->μῆκος_μέγ;

#define ΑΜΝΗ(πεδίον, ν) \
    ἀσκ->πεδίον = calloc((size_t)(ν), sizeof(float)); \
    if (!ἀσκ->πεδίον) return -1;
    ΑΜΝΗ(μνήμη_x,      (size_t)(Λ + 1) * δ)
    ΑΜΝΗ(μνήμη_xb_πρ,  (size_t)Λ * δ)
    ΑΜΝΗ(μνήμη_xb_δδ,  (size_t)Λ * δ)
    ΑΜΝΗ(μνήμη_q,       (size_t)Λ * δ)
    ΑΜΝΗ(μνήμη_k,       (size_t)Λ * κτ_δ)
    ΑΜΝΗ(μνήμη_πρ,      (size_t)Λ * Κ * μμ)
    ΑΜΝΗ(μνήμη_hb,      (size_t)Λ * δκ)
    ΑΜΝΗ(μνήμη_hb2,     (size_t)Λ * δκ)
    ΑΜΝΗ(μνήμη_χτέλ,    (size_t)δ)
#undef ΑΜΝΗ
    return 0;
}

void ομφ_ἄσκησις_τέλει(ομφ_ἄσκησις_t *ἀσκ)
{
    free(ἀσκ->κλίσ_δεδ);
    free(ἀσκ->ὁρμή_δεδ);
    free(ἀσκ->δύναμις_δεδ);
    free(ἀσκ->μνήμη_x);
    free(ἀσκ->μνήμη_xb_πρ);
    free(ἀσκ->μνήμη_xb_δδ);
    free(ἀσκ->μνήμη_q);
    free(ἀσκ->μνήμη_k);
    free(ἀσκ->μνήμη_πρ);
    free(ἀσκ->μνήμη_hb);
    free(ἀσκ->μνήμη_hb2);
    free(ἀσκ->μνήμη_χτέλ);
    memset(ἀσκ, 0, sizeof(*ἀσκ));
}

void ομφ_κλίσεις_μηδένισον(ομφ_ἄσκησις_t *ἀσκ, const ομφ_σύνθεσις_t *σ)
{
    size_t ν = ομφ_μέγεθος_σταθμῶν(σ, 1);
    memset(ἀσκ->κλίσ_δεδ, 0, ν * sizeof(float));
}

/* ================================================================
 * ἀναδρομή — backward pass δι᾽ ἓν θέσιν
 * ================================================================ */

float ομφ_ἀναδρομή(
    ομφ_t *ομφ, ομφ_ἄσκησις_t *ἀσκ,
    int σημεῖον, int σημεῖον_στόχος, int θέσις
) {
    ομφ_σύνθεσις_t *σ = &ομφ->σύνθεσις;
    ομφ_σταθμά_t   *π = &ομφ->σταθμά;
    ομφ_σταθμά_t   *γ = &ἀσκ->κλίσεις;

    int δ      = σ->διάστασις;
    int δκ     = σ->διάστ_κρυ;
    int Λ      = σ->στρώματα;
    int Κ      = σ->κεφαλαί;
    int Κκτ    = σ->κεφαλαί_κτ;
    int κδ     = δ / Κ;
    int κτ_δ   = κδ * Κκτ;
    int μμ     = σ->μῆκος_μέγ;
    int Λξ     = σ->λεξ_μέγεθος;
    int κτ_πολ = Κ / Κκτ;

    /* --- προσωρινὰ ταμιεῖα --- */
    float *δ_x   = calloc((size_t)δ, sizeof(float));
    float *δ_xb  = calloc((size_t)δ, sizeof(float));
    float *δ_hb  = calloc((size_t)δκ, sizeof(float));
    float *δ_hb2 = calloc((size_t)δκ, sizeof(float));
    float *δ_q   = calloc((size_t)δ, sizeof(float));
    float *δ_k   = calloc((size_t)κτ_δ, sizeof(float));
    float *δ_v   = calloc((size_t)κτ_δ, sizeof(float));
    float *δ_λογ = calloc((size_t)Λξ, sizeof(float));
    if (
        !δ_x || !δ_xb || !δ_hb || !δ_hb2 ||
        !δ_q || !δ_k  || !δ_v  || !δ_λογ
    ) {
        free(δ_x);
        free(δ_xb);
        free(δ_hb);
        free(δ_hb2);
        free(δ_q);
        free(δ_k);
        free(δ_v);
        free(δ_λογ);
        return 0.0f;
    }

    /* 1. ζημία (cross-entropy) ἐκ λογιτῶν: dL/δ_λογ = softmax - one_hot */
    float *λογιταί = ομφ->κατάστ.λογιταί;

    memcpy(δ_λογ, λογιταί, (size_t)Λξ * sizeof(float));
    {
        pfr_vector_f_t dv = { Λξ, δ_λογ, NULL };
        pfr_softmax_f(&dv);
    }
    float ζημία = -logf(δ_λογ[σημεῖον_στόχος] + 1e-10f);
    δ_λογ[σημεῖον_στόχος] -= 1.0f;  /* κλίσις: softmax - one_hot */

    /* 2. ἀναδρομὴ διὰ σλεξ: δ_χτέλ = σλεξ^T * δ_λογ */
    memset(δ_x, 0, (size_t)δ * sizeof(float));
    πίν_διαν_ἀνάστρ_σώρ(δ_x, π->σλεξ, δ_λογ, Λξ, δ);
    ἐξωτ_γιν_σώρ(γ->σλεξ, 1.0f, δ_λογ, ἀσκ->μνήμη_χτέλ, Λξ, δ);

    /* 3. ἀναδρομὴ διὰ rmsnorm τέλους */
    float *δ_x_ἀναδρ = calloc((size_t)δ, sizeof(float));
    if (!δ_x_ἀναδρ) {
        free(δ_x); free(δ_xb); free(δ_hb); free(δ_hb2);
        free(δ_q); free(δ_k); free(δ_v); free(δ_λογ);
        return ζημία;
    }
    float *x_πρὸ_τέλ = ἀσκ->μνήμη_x + (size_t)Λ * δ;
    rms_κανόν_ἀναδρ(δ_x_ἀναδρ, γ->rms_τέλ, δ_x, x_πρὸ_τέλ, π->rms_τέλ, δ);
    memcpy(δ_x, δ_x_ἀναδρ, (size_t)δ * sizeof(float));

    /* 4. ἀναδρομὴ διὰ στρωμάτων (ἐν ἀνεστραμμένῃ τάξει) */
    for (int λ = Λ - 1; λ >= 0; λ--) {
        float *Σ_rms_πρ = π->rms_πρ   + (size_t)λ * δ;
        float *Σ_rms_δδ = π->rms_δδ   + (size_t)λ * δ;
        float *Σ_wq     = π->wq       + (size_t)λ * δ * δ;
        float *Σ_wk     = π->wk       + (size_t)λ * κτ_δ * δ;
        float *Σ_wv     = π->wv       + (size_t)λ * κτ_δ * δ;
        float *Σ_wo     = π->wo       + (size_t)λ * δ * δ;
        float *Σ_w1     = π->w1       + (size_t)λ * δκ * δ;
        float *Σ_w2     = π->w2       + (size_t)λ * δ * δκ;
        float *Σ_w3     = π->w3       + (size_t)λ * δκ * δ;

        float *Γ_rms_πρ = γ->rms_πρ   + (size_t)λ * δ;
        float *Γ_rms_δδ = γ->rms_δδ   + (size_t)λ * δ;
        float *Γ_wq     = γ->wq       + (size_t)λ * δ * δ;
        float *Γ_wk     = γ->wk       + (size_t)λ * κτ_δ * δ;
        float *Γ_wv     = γ->wv       + (size_t)λ * κτ_δ * δ;
        float *Γ_wo     = γ->wo       + (size_t)λ * δ * δ;
        float *Γ_w1     = γ->w1       + (size_t)λ * δκ * δ;
        float *Γ_w2     = γ->w2       + (size_t)λ * δ * δκ;
        float *Γ_w3     = γ->w3       + (size_t)λ * δκ * δ;

        float *xb_δδ    = ἀσκ->μνήμη_xb_δδ + (size_t)λ * δ;
        float *hb_μ     = ἀσκ->μνήμη_hb     + (size_t)λ * δκ;
        float *hb2_μ    = ἀσκ->μνήμη_hb2    + (size_t)λ * δκ;
        float *x_εἰσ    = ἀσκ->μνήμη_x      + (size_t)λ * δ;
        float *x_μέσ    = ἀσκ->μνήμη_x      + (size_t)(λ+1) * δ;

        /* --- ἀναδρομὴ FFN --- */
        float *δ_δδ = calloc((size_t)δ, sizeof(float));
        if (!δ_δδ)
            break;
        memcpy(δ_δδ, δ_x, (size_t)δ * sizeof(float));

        /* ἀναδρομὴ διὰ w2 */
        memset(δ_hb, 0, (size_t)δκ * sizeof(float));
        πίν_διαν_ἀνάστρ_σώρ(δ_hb, Σ_w2, δ_δδ, δ, δκ);
        {
            float *hb_μετά = calloc((size_t)δκ, sizeof(float));
            if (hb_μετά) {
                pfr_vector_f_t ov = { δκ, hb_μετά,        NULL };
                pfr_vector_f_t av = { δκ, (float *)hb_μ,  NULL };
                pfr_vector_f_t bv = { δκ, (float *)hb2_μ, NULL };
                pfr_swiglu_f(&ov, &av, &bv);
                ἐξωτ_γιν_σώρ(Γ_w2, 1.0f, δ_δδ, hb_μετά, δ, δκ);
                free(hb_μετά);
            }
        }

        /* ἀναδρομὴ διὰ SwiGLU */
        memset(δ_hb2, 0, (size_t)δκ * sizeof(float));
        for (int i = 0; i < δκ; i++) {
            float sig       = 1.0f / (1.0f + expf(-hb_μ[i]));
            float silu_τιμ  = hb_μ[i] * sig;
            float silu_κλίσ = sig * (1.0f + hb_μ[i] * (1.0f - sig));
            δ_hb2[i]        = δ_hb[i] * silu_τιμ;
            δ_hb[i]         = δ_hb[i] * hb2_μ[i] * silu_κλίσ;
        }

        /* ἀναδρομὴ διὰ w1 */
        memset(δ_xb, 0, (size_t)δ * sizeof(float));
        πίν_διαν_ἀνάστρ_σώρ(δ_xb, Σ_w1, δ_hb,  δκ, δ);
        ἐξωτ_γιν_σώρ(Γ_w1, 1.0f, δ_hb, xb_δδ, δκ, δ);

        /* ἀναδρομὴ διὰ w3 */
        πίν_διαν_ἀνάστρ_σώρ(δ_xb, Σ_w3, δ_hb2, δκ, δ);
        ἐξωτ_γιν_σώρ(Γ_w3, 1.0f, δ_hb2, xb_δδ, δκ, δ);

        free(δ_δδ);

        /* ἀναδρομὴ διὰ rmsnorm FFN */
        memset(δ_x_ἀναδρ, 0, (size_t)δ * sizeof(float));
        rms_κανόν_ἀναδρ(δ_x_ἀναδρ, Γ_rms_δδ, δ_xb, x_μέσ, Σ_rms_δδ, δ);
        for (int i = 0; i < δ; i++)
            δ_x[i] = δ_x[i] + δ_x_ἀναδρ[i];

        /* --- ἀναδρομὴ προσοχῆς --- */
        float *xb_πρ  = ἀσκ->μνήμη_xb_πρ + (size_t)λ * δ;
        float *q_μ    = ἀσκ->μνήμη_q      + (size_t)λ * δ;
        float *πρ_μ   = ἀσκ->μνήμη_πρ     + (size_t)λ * Κ * μμ;

        float *δ_πρ_ἐξ = calloc((size_t)δ, sizeof(float));
        if (!δ_πρ_ἐξ)
            break;
        memcpy(δ_πρ_ἐξ, δ_x, (size_t)δ * sizeof(float));

        /* ἀναδρομὴ διὰ wo */
        float *δ_xb_συνέν = calloc((size_t)δ, sizeof(float));
        if (!δ_xb_συνέν) {
            free(δ_πρ_ἐξ);
            break;
        }
        πίν_διαν_ἀνάστρ_σώρ(δ_xb_συνέν, Σ_wo, δ_πρ_ἐξ, δ, δ);
        {
            float *xb_πρὸ_wo = calloc((size_t)δ, sizeof(float));
            if (xb_πρὸ_wo) {
                for (int η = 0; η < Κ; η++) {
                    float *xη    = xb_πρὸ_wo + η * κδ;
                    int ηκτ      = η / κτ_πολ;
                    float *πρ_η  = πρ_μ + η * μμ;
                    for (int τ = 0; τ <= θέσις; τ++) {
                        float *v_τ = ομφ->κατάστ.κρύπτη_τιμή +
                            ((size_t)λ * μμ + τ) * κτ_δ + ηκτ * κδ;
                        float α = πρ_η[τ];
                        for (int i = 0; i < κδ; i++)
                            xη[i] += α * v_τ[i];
                    }
                }
                ἐξωτ_γιν_σώρ(Γ_wo, 1.0f, δ_πρ_ἐξ, xb_πρὸ_wo, δ, δ);
                free(xb_πρὸ_wo);
            }
        }

        /* ἀναδρομὴ διὰ προσοχῆς κεφαλὴ πρὸς κεφαλήν */
        memset(δ_q, 0, (size_t)δ * sizeof(float));
        memset(δ_k, 0, (size_t)κτ_δ * sizeof(float));
        memset(δ_v, 0, (size_t)κτ_δ * sizeof(float));

        float ἀντ_ρίζ_κδ = 1.0f / sqrtf((float)κδ);

        for (int η = 0; η < Κ; η++) {
            float *q_η   = q_μ    + η * κδ;
            float *δ_q_η = δ_q    + η * κδ;
            float *πρ_η  = πρ_μ   + η * μμ;
            float *δ_κ_η = δ_xb_συνέν + η * κδ;
            int ηκτ      = η / κτ_πολ;

            for (int τ = 0; τ <= θέσις; τ++) {
                float *δ_v_ηκτ = δ_v + ηκτ * κδ;
                float α         = πρ_η[τ];
                for (int i = 0; i < κδ; i++)
                    δ_v_ηκτ[i] += α * δ_κ_η[i];
            }

            float *δ_πρ_βαθμ = calloc((size_t)(θέσις + 1), sizeof(float));
            if (δ_πρ_βαθμ) {
                for (int τ = 0; τ <= θέσις; τ++) {
                    float *v_τ = ομφ->κατάστ.κρύπτη_τιμή +
                        ((size_t)λ * μμ + τ) * κτ_δ + ηκτ * κδ;
                    float βθ = 0.0f;
                    for (int i = 0; i < κδ; i++)
                        βθ += δ_κ_η[i] * v_τ[i];
                    δ_πρ_βαθμ[τ] = βθ;
                }

                float *δ_βαθμ_πρό = calloc((size_t)(θέσις + 1), sizeof(float));
                if (δ_βαθμ_πρό) {
                    softmax_ἀναδρ(δ_βαθμ_πρό, δ_πρ_βαθμ, πρ_η, θέσις + 1);
                    for (int τ = 0; τ <= θέσις; τ++)
                        δ_βαθμ_πρό[τ] *= ἀντ_ρίζ_κδ;

                    for (int τ = 0; τ <= θέσις; τ++) {
                        float *k_τ = ομφ->κατάστ.κρύπτη_κλείς +
                            ((size_t)λ * μμ + τ) * κτ_δ + ηκτ * κδ;
                        float δβπ = δ_βαθμ_πρό[τ];
                        for (int i = 0; i < κδ; i++)
                            δ_q_η[i] += δβπ * k_τ[i];
                    }

                    {
                        float *δ_k_ηκτ = δ_k + ηκτ * κδ;
                        float δβπ       = δ_βαθμ_πρό[θέσις];
                        for (int i = 0; i < κδ; i++)
                            δ_k_ηκτ[i] += δβπ * q_η[i];
                    }

                    free(δ_βαθμ_πρό);
                }
                free(δ_πρ_βαθμ);
            }
        }

        free(δ_πρ_ἐξ);
        free(δ_xb_συνέν);

        /* RoPE ἀναδρομὴ ἐν δ_q καὶ δ_k */
        for (int η = 0; η < Κ; η++)
            rope_ἀναδρ(δ_q + η * κδ, θέσις, κδ);
        for (int η = 0; η < Κκτ; η++)
            rope_ἀναδρ(δ_k + η * κδ, θέσις, κδ);

        /* ἀναδρομὴ διὰ wq, wk, wv */
        float *δ_xb_πρ = calloc((size_t)δ, sizeof(float));
        if (δ_xb_πρ) {
            πίν_διαν_ἀνάστρ_σώρ(δ_xb_πρ, Σ_wq, δ_q, δ, δ);
            ἐξωτ_γιν_σώρ(Γ_wq, 1.0f, δ_q, xb_πρ, δ, δ);

            πίν_διαν_ἀνάστρ_σώρ(δ_xb_πρ, Σ_wk, δ_k, κτ_δ, δ);
            ἐξωτ_γιν_σώρ(Γ_wk, 1.0f, δ_k, xb_πρ, κτ_δ, δ);

            πίν_διαν_ἀνάστρ_σώρ(δ_xb_πρ, Σ_wv, δ_v, κτ_δ, δ);
            ἐξωτ_γιν_σώρ(Γ_wv, 1.0f, δ_v, xb_πρ, κτ_δ, δ);

            /* ἀναδρομὴ διὰ rmsnorm προσοχῆς */
            memset(δ_x_ἀναδρ, 0, (size_t)δ * sizeof(float));
            rms_κανόν_ἀναδρ(
                δ_x_ἀναδρ, Γ_rms_πρ, δ_xb_πρ,
                x_εἰσ, Σ_rms_πρ, δ
            );
            for (int i = 0; i < δ; i++)
                δ_x[i] = δ_x[i] + δ_x_ἀναδρ[i];

            free(δ_xb_πρ);
        }
    }

    /* 5. κλίσις ἰχνῶν */
    for (int i = 0; i < δ; i++)
        γ->ἴχνη[(size_t)σημεῖον * δ + i] += δ_x[i];

    ἀσκ->σημεῖα_κατεργ++;

    free(δ_x);
    free(δ_xb);
    free(δ_hb);
    free(δ_hb2);
    free(δ_q);
    free(δ_k);
    free(δ_v);
    free(δ_λογ);
    free(δ_x_ἀναδρ);

    return ζημία;
}

/* ================================================================
 * κούρα κλίσεων (gradient clipping)
 * ================================================================ */

float ομφ_κλίσεις_κεῖρον(
    ομφ_ἄσκησις_t *ἀσκ, const ομφ_σύνθεσις_t *σ,
    float μέγ_κανών
) {
    size_t ν    = ομφ_μέγεθος_σταθμῶν(σ, 1);
    float *γ    = ἀσκ->κλίσ_δεδ;
    float κανών = 0.0f;
    for (size_t i = 0; i < ν; i++)
        κανών += γ[i] * γ[i];
    κανών = sqrtf(κανών);
    if (κανών > μέγ_κανών) {
        float κλίμαξ = μέγ_κανών / κανών;
        for (size_t i = 0; i < ν; i++)
            γ[i] *= κλίμαξ;
    }
    return κανών;
}

/* ================================================================
 * βῆμα AdamW
 * ================================================================ */

void ομφ_βῆμα_ἀδάμ(
    ομφ_t *ομφ, ομφ_ἄσκησις_t *ἀσκ,
    float βαθμός_μαθήσεως, float βῆτα1, float βῆτα2,
    float ἐψιλον, float ξήρανσις
) {
    ἀσκ->βῆμα++;
    float τ   = (float)ἀσκ->βῆμα;
    float δμ1 = 1.0f - powf(βῆτα1, τ);
    float δμ2 = 1.0f - powf(βῆτα2, τ);

    size_t ν = ομφ_μέγεθος_σταθμῶν(&ομφ->σύνθεσις, ομφ->σταθμά_κοινά);
    float *σ = ομφ->δεδομένα;
    float *γ = ἀσκ->κλίσ_δεδ;
    float *μ = ἀσκ->ὁρμή_δεδ;
    float *v = ἀσκ->δύναμις_δεδ;

    for (size_t i = 0; i < ν; i++) {
        float γι     = γ[i];
        μ[i]         = βῆτα1 * μ[i] + (1.0f - βῆτα1) * γι;
        v[i]         = βῆτα2 * v[i] + (1.0f - βῆτα2) * γι * γι;
        float μ_διορ = μ[i] / δμ1;
        float v_διορ = v[i] / δμ2;
        σ[i] -= βαθμός_μαθήσεως * (
            μ_διορ / (sqrtf(v_διορ) + ἐψιλον)
            + ξήρανσις * σ[i]
        );
    }

    ομφ_κλίσεις_μηδένισον(ἀσκ, &ομφ->σύνθεσις);
}
