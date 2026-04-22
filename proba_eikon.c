/*
 * proba_eikon.c — probationes GPU operationum pro eikon
 *
 * Probat singulas pfr operationes (matvec, matvec_trans, ger)
 * cum GPU buffers ad dimensiones quas eikon utitur.
 * Comparat GPU resultata cum CPU referentia.
 */

#include <phantasma/computo.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================
 * auxiliaria
 * ================================================================ */

static unsigned int g_semen = 42u;

static float randf(void)
{
    g_semen = g_semen * 1664525u + 1013904223u;
    return (g_semen >> 8) / 16777216.0f * 2.0f - 1.0f;
}

static void imple(float *v, int n)
{
    for (int i = 0; i < n; i++)
        v[i] = randf();
}

static float max_diff(const float *a, const float *b, int n)
{
    float d = 0.0f;
    for (int i = 0; i < n; i++) {
        float e = fabsf(a[i] - b[i]);
        if (e > d)
            d = e;
    }
    return d;
}

#define PROBA(nomen, corpus) do { \
    printf("  %-40s ", nomen); \
    fflush(stdout); \
    corpus \
    printf("OK\n"); \
    n_ok++; \
} while (0)

#define FALLI(msg) do { \
    printf("FALLIT: %s\n", msg); \
    n_fal++; \
    goto prox; \
} while (0)

/* ================================================================
 * proba matvec: y = W * x
 * ================================================================ */

static int proba_matvec(int m, int n, int *n_ok, int *n_fal)
{
    char nom[128];
    snprintf(nom, sizeof(nom), "matvec %dx%d", m, n);
    printf("  %-40s ", nom);
    fflush(stdout);

    float *W   = malloc((size_t)m * n * sizeof(float));
    float *x   = malloc((size_t)n * sizeof(float));
    float *y_c = calloc((size_t)m, sizeof(float));
    float *y_g = calloc((size_t)m, sizeof(float));
    if (!W || !x || !y_c || !y_g) {
        printf("FALLIT: malloc\n");
        (*n_fal)++;
        free(W);
        free(x);
        free(y_c);
        free(y_g);
        return -1;
    }

    imple(W, m * n);
    imple(x, n);

    /* CPU referentia */
    {
        pfr_matrix_f_t mW = { m, n, W, NULL };
        pfr_vector_f_t vx = { n, x, NULL };
        pfr_vector_f_t vy = { m, y_c, NULL };
        pfr_cpu_matvec_f(&vy, &mW, &vx);
    }

    /* GPU */
    {
        pfr_matrix_f_t mW = { m, n, W, NULL };
        pfr_vector_f_t vx = { n, x, NULL };
        pfr_vector_f_t vy = { m, y_g, NULL };

        printf("[alloc] ");
        fflush(stdout);
        if (pfr_in_gpu_mitte_f(&mW) != 0) {
            printf("FALLIT: in_gpu_mitte_f W\n");
            (*n_fal)++;
            goto cleanup_mv;
        }
        printf("[W->gpu] ");
        fflush(stdout);

        if (pfr_in_gpu_mitte_vf(&vx) != 0) {
            printf("FALLIT: in_gpu_mitte_vf x\n");
            (*n_fal)++;
            goto cleanup_mv;
        }
        printf("[x->gpu] ");
        fflush(stdout);

        if (pfr_in_gpu_mitte_vf(&vy) != 0) {
            printf("FALLIT: in_gpu_mitte_vf y\n");
            (*n_fal)++;
            goto cleanup_mv;
        }
        printf("[y->gpu] ");
        fflush(stdout);

        printf("[compute] ");
        fflush(stdout);
        int rc = pfr_matvec_f(&vy, &mW, &vx);
        if (rc != 0) {
            printf("FALLIT: matvec_f rc=%d\n", rc);
            (*n_fal)++;
            goto cleanup_mv;
        }

        printf("[y<-gpu] ");
        fflush(stdout);
        pfr_ex_gpu_cape_vf(&vy);

        float d = max_diff(y_c, y_g, m);
        if (d > 1e-3f) {
            printf("FALLIT: diff=%.6f\n", d);
            (*n_fal)++;
        } else {
            printf("OK (diff=%.2e)\n", d);
            (*n_ok)++;
        }
    }

cleanup_mv:
    free(W);
    free(x);
    free(y_c);
    free(y_g);
    return 0;
}

/* ================================================================
 * proba matvec_trans: y = W^T * x
 * ================================================================ */

static int proba_matvec_trans(int m, int n, int *n_ok, int *n_fal)
{
    char nom[128];
    snprintf(nom, sizeof(nom), "matvec_trans %dx%d", m, n);
    printf("  %-40s ", nom);
    fflush(stdout);

    float *W   = malloc((size_t)m * n * sizeof(float));
    float *x   = malloc((size_t)m * sizeof(float));
    float *y_c = calloc((size_t)n, sizeof(float));
    float *y_g = calloc((size_t)n, sizeof(float));
    if (!W || !x || !y_c || !y_g) {
        printf("FALLIT: malloc\n");
        (*n_fal)++;
        free(W);
        free(x);
        free(y_c);
        free(y_g);
        return -1;
    }

    imple(W, m * n);
    imple(x, m);

    /* CPU */
    {
        pfr_matrix_f_t mW = { m, n, W, NULL };
        pfr_vector_f_t vx = { m, x, NULL };
        pfr_vector_f_t vy = { n, y_c, NULL };
        pfr_cpu_matvec_trans_f(&vy, &mW, &vx);
    }

    /* GPU */
    {
        pfr_matrix_f_t mW = { m, n, W, NULL };
        pfr_vector_f_t vx = { m, x, NULL };
        pfr_vector_f_t vy = { n, y_g, NULL };

        if (
            pfr_in_gpu_mitte_f(&mW) != 0 ||
            pfr_in_gpu_mitte_vf(&vx) != 0 ||
            pfr_in_gpu_mitte_vf(&vy) != 0
        ) {
            printf("FALLIT: gpu alloc\n");
            (*n_fal)++;
            goto cleanup_mt;
        }

        int rc = pfr_matvec_trans_f(&vy, &mW, &vx);
        if (rc != 0) {
            printf("FALLIT: matvec_trans_f rc=%d\n", rc);
            (*n_fal)++;
            goto cleanup_mt;
        }

        pfr_ex_gpu_cape_vf(&vy);

        float d = max_diff(y_c, y_g, n);
        if (d > 1e-3f) {
            printf("FALLIT: diff=%.6f\n", d);
            (*n_fal)++;
        } else {
            printf("OK (diff=%.2e)\n", d);
            (*n_ok)++;
        }
    }

cleanup_mt:
    free(W);
    free(x);
    free(y_c);
    free(y_g);
    return 0;
}

/* ================================================================
 * proba ger: A += alpha * x * y^T
 * ================================================================ */

static int proba_ger(int m, int n, int *n_ok, int *n_fal)
{
    char nom[128];
    snprintf(nom, sizeof(nom), "ger %dx%d", m, n);
    printf("  %-40s ", nom);
    fflush(stdout);

    float *A_c = malloc((size_t)m * n * sizeof(float));
    float *A_g = malloc((size_t)m * n * sizeof(float));
    float *x   = malloc((size_t)m * sizeof(float));
    float *y   = malloc((size_t)n * sizeof(float));
    if (!A_c || !A_g || !x || !y) {
        printf("FALLIT: malloc\n");
        (*n_fal)++;
        free(A_c);
        free(A_g);
        free(x);
        free(y);
        return -1;
    }

    imple(A_c, m * n);
    memcpy(A_g, A_c, (size_t)m * n * sizeof(float));
    imple(x, m);
    imple(y, n);

    /* CPU */
    {
        pfr_matrix_f_t mA = { m, n, A_c, NULL };
        pfr_vector_f_t vx = { m, x, NULL };
        pfr_vector_f_t vy = { n, y, NULL };
        pfr_cpu_ger_f(&mA, 1.0f, &vx, &vy);
    }

    /* GPU */
    {
        pfr_matrix_f_t mA = { m, n, A_g, NULL };
        pfr_vector_f_t vx = { m, x, NULL };
        pfr_vector_f_t vy = { n, y, NULL };

        if (
            pfr_in_gpu_mitte_f(&mA) != 0 ||
            pfr_in_gpu_mitte_vf(&vx) != 0 ||
            pfr_in_gpu_mitte_vf(&vy) != 0
        ) {
            printf("FALLIT: gpu alloc\n");
            (*n_fal)++;
            goto cleanup_ger;
        }

        int rc = pfr_ger_f(&mA, 1.0f, &vx, &vy);
        if (rc != 0) {
            printf("FALLIT: ger_f rc=%d\n", rc);
            (*n_fal)++;
            goto cleanup_ger;
        }

        pfr_ex_gpu_cape_f(&mA);

        float d = max_diff(A_c, A_g, m * n);
        if (d > 1e-3f) {
            printf("FALLIT: diff=%.6f\n", d);
            (*n_fal)++;
        } else {
            printf("OK (diff=%.2e)\n", d);
            (*n_ok)++;
        }
    }

cleanup_ger:
    free(A_c);
    free(A_g);
    free(x);
    free(y);
    return 0;
}

/* ================================================================
 * proba fc_fwd end-to-end (ut eikon utitur)
 * ================================================================ */

static int proba_fc_fwd(int m, int n, int *n_ok, int *n_fal)
{
    char nom[128];
    snprintf(nom, sizeof(nom), "fc_fwd %dx%d (alloc+compute+free)", m, n);
    printf("  %-40s ", nom);
    fflush(stdout);

    float *W   = malloc((size_t)m * n * sizeof(float));
    float *b   = malloc((size_t)m * sizeof(float));
    float *x   = malloc((size_t)n * sizeof(float));
    float *y_c = calloc((size_t)m, sizeof(float));
    float *y_g = calloc((size_t)m, sizeof(float));
    if (!W || !b || !x || !y_c || !y_g) {
        printf("FALLIT: malloc\n");
        (*n_fal)++;
        free(W);
        free(b);
        free(x);
        free(y_c);
        free(y_g);
        return -1;
    }

    imple(W, m * n);
    imple(b, m);
    imple(x, n);

    /* CPU referentia */
    {
        pfr_matrix_f_t mW = { m, n, W, NULL };
        pfr_vector_f_t vx = { n, x, NULL };
        pfr_vector_f_t vy = { m, y_c, NULL };
        pfr_cpu_matvec_f(&vy, &mW, &vx);
        for (int i = 0; i < m; i++)
            y_c[i] += b[i];
    }

    /* GPU — simula quod eikon facit: alloc per call */
    {
        pfr_matrix_f_t mW = { m, n, W, NULL };
        pfr_vector_f_t vx = { n, x, NULL };
        pfr_vector_f_t vy = { m, y_g, NULL };

        pfr_in_gpu_mitte_f(&mW);
        pfr_in_gpu_mitte_vf(&vx);
        pfr_in_gpu_mitte_vf(&vy);
        pfr_matvec_f(&vy, &mW, &vx);
        pfr_ex_gpu_cape_vf(&vy);
        for (int i = 0; i < m; i++)
            y_g[i] += b[i];

        float d = max_diff(y_c, y_g, m);
        if (d > 1e-3f) {
            printf("FALLIT: diff=%.6f\n", d);
            (*n_fal)++;
        } else {
            printf("OK (diff=%.2e)\n", d);
            (*n_ok)++;
        }
    }

    free(W);
    free(b);
    free(x);
    free(y_c);
    free(y_g);
    return 0;
}

/* ================================================================
 * proba repetita (GPU memory leak?)
 * ================================================================ */

static int proba_repetita(int m, int n, int reps, int *n_ok, int *n_fal)
{
    char nom[128];
    snprintf(nom, sizeof(nom), "repetita %dx%d x%d (leak?)", m, n, reps);
    printf("  %-40s ", nom);
    fflush(stdout);

    float *W = malloc((size_t)m * n * sizeof(float));
    float *x = malloc((size_t)n * sizeof(float));
    float *y = calloc((size_t)m, sizeof(float));
    if (!W || !x || !y) {
        printf("FALLIT: malloc\n");
        (*n_fal)++;
        free(W);
        free(x);
        free(y);
        return -1;
    }

    imple(W, m * n);
    imple(x, n);

    for (int r = 0; r < reps; r++) {
        pfr_matrix_f_t mW = { m, n, W, NULL };
        pfr_vector_f_t vx = { n, x, NULL };
        pfr_vector_f_t vy = { m, y, NULL };

        pfr_in_gpu_mitte_f(&mW);
        pfr_in_gpu_mitte_vf(&vx);
        pfr_in_gpu_mitte_vf(&vy);
        pfr_matvec_f(&vy, &mW, &vx);
        pfr_ex_gpu_cape_vf(&vy);

        if ((r + 1) % 100 == 0) {
            printf("%d..", r + 1);
            fflush(stdout);
        }
    }

    printf("OK\n");
    (*n_ok)++;

    free(W);
    free(x);
    free(y);
    return 0;
}

/* ================================================================
 * main
 * ================================================================ */

int main(void)
{
    printf("=== proba eikon GPU ===\n\n");

    int gpu = pfr_computo_initia();
    printf("computator: %s\n\n", gpu == 0 ? "GPU" : "CPU");

    if (gpu != 0) {
        printf("GPU non adest — probationes GPU omissae.\n");
        pfr_computo_fini();
        return 0;
    }

    int n_ok = 0, n_fal = 0;

    /* --- 1. operationes singulae, dimensiones parvae --- */
    printf("1. operationes singulae (parvae):\n");
    proba_matvec(32, 16, &n_ok, &n_fal);
    proba_matvec_trans(32, 16, &n_ok, &n_fal);
    proba_ger(32, 16, &n_ok, &n_fal);
    printf("\n");

    /* --- 2. dimensiones eikon 16x16 --- */
    printf("2. dimensiones eikon 16x16 (z=16 g1=32 g2=128 ed=256):\n");
    proba_matvec(32, 16, &n_ok, &n_fal);      /* g_w1 */
    proba_matvec(128, 32, &n_ok, &n_fal);     /* g_w2 */
    proba_matvec(256, 128, &n_ok, &n_fal);    /* g_w3 */
    proba_matvec(128, 256, &n_ok, &n_fal);    /* d_w1 */
    proba_matvec(32, 128, &n_ok, &n_fal);     /* d_w2 */
    proba_matvec(1, 32, &n_ok, &n_fal);       /* d_w3 */
    proba_matvec_trans(32, 16, &n_ok, &n_fal);
    proba_matvec_trans(128, 32, &n_ok, &n_fal);
    proba_matvec_trans(128, 256, &n_ok, &n_fal);
    proba_ger(32, 16, &n_ok, &n_fal);
    proba_ger(128, 32, &n_ok, &n_fal);
    proba_ger(256, 128, &n_ok, &n_fal);
    proba_ger(128, 256, &n_ok, &n_fal);
    printf("\n");

    /* --- 3. dimensiones eikon 64x64 --- */
    printf("3. dimensiones eikon 64x64 (z=128 g1=512 g2=2048 ed=4096):\n");
    proba_matvec(512, 128, &n_ok, &n_fal);
    proba_matvec(2048, 512, &n_ok, &n_fal);
    proba_matvec(4096, 2048, &n_ok, &n_fal);
    proba_matvec(2048, 4096, &n_ok, &n_fal);
    proba_matvec(512, 2048, &n_ok, &n_fal);
    proba_matvec(1, 512, &n_ok, &n_fal);
    proba_matvec_trans(512, 128, &n_ok, &n_fal);
    proba_matvec_trans(2048, 512, &n_ok, &n_fal);
    proba_matvec_trans(4096, 2048, &n_ok, &n_fal);
    proba_matvec_trans(2048, 4096, &n_ok, &n_fal);
    proba_ger(512, 128, &n_ok, &n_fal);
    proba_ger(2048, 512, &n_ok, &n_fal);
    proba_ger(4096, 2048, &n_ok, &n_fal);
    proba_ger(2048, 4096, &n_ok, &n_fal);
    printf("\n");

    /* --- 4. fc_fwd compositum --- */
    printf("4. fc_fwd compositum:\n");
    proba_fc_fwd(32, 16, &n_ok, &n_fal);
    proba_fc_fwd(256, 128, &n_ok, &n_fal);
    proba_fc_fwd(4096, 2048, &n_ok, &n_fal);
    printf("\n");

    /* --- 5. repetitae (GPU memory leak) --- */
    printf("5. repetitae (memoria GPU):\n");
    proba_repetita(256, 128, 500, &n_ok, &n_fal);
    proba_repetita(4096, 2048, 100, &n_ok, &n_fal);
    proba_repetita(4096, 2048, 5000, &n_ok, &n_fal);
    proba_repetita(256, 128, 50000, &n_ok, &n_fal);
    printf("\n");

    /* --- summa --- */
    printf("=== %d OK, %d FALLIT ===\n", n_ok, n_fal);

    pfr_computo_fini();
    return n_fal > 0 ? 1 : 0;
}
