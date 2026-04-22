/*
 * genera_eikon.c — generatio imaginum ex exemplare eikon
 *
 * Usus: genera_eikon <dir_exemplaris> <dir_exitus> [n_imagines] [semen]
 *
 *   <dir_exemplaris>   directorium cum model.bin
 *   <dir_exitus>       directorium exitus pro GIF
 *   [n_imagines]       numerus imaginum (praedef. 16)
 *   [semen]            semen RNG (praedef. 0 = tempus)
 */

#include "eikon/εἰκών.h"

#include <phantasma/computo.h>
#include <phantasma/phantasma.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

/* ================================================================
 * RNG
 * ================================================================ */

static unsigned int g_semen;

static float randn(void)
{
    g_semen  = g_semen * 1664525u + 1013904223u;
    float u1 = (g_semen >> 8) / 16777216.0f;
    if (u1 < 1e-10f)
        u1 = 1e-10f;
    g_semen  = g_semen * 1664525u + 1013904223u;
    float u2 = (g_semen >> 8) / 16777216.0f;
    return sqrtf(-2.0f * logf(u1)) * cosf(6.2831853f * u2);
}

/* ================================================================
 * scriptio unius imaginis GIF (per phantasma)
 * ================================================================ */

static int scribe_gif(const char *via, const float *img, int lat, int alt)
{
    /* upscale 2x ad 128x128 per nearest neighbor */
    int out_lat = lat * 2;
    int out_alt = alt * 2;

    uint32_t *pix = malloc((size_t)out_lat * out_alt * sizeof(uint32_t));
    if (!pix)
        return -1;

    for (int y = 0; y < out_alt; y++) {
        for (int x = 0; x < out_lat; x++) {
            float v = (img[(y / 2) * lat + (x / 2)] + 1.0f) * 127.5f;
            int g   = (int)v;
            if (g < 0)
                g = 0;
            if (g > 255)
                g = 255;
            pix[y * out_lat + x] =
                0xFF000000u | ((uint32_t)g << 16) | ((uint32_t)g << 8) | (uint32_t)g;
        }
    }

    pfr_gif_t *gif = pfr_gif_initia(via, out_lat, out_alt, 0, 1);
    if (!gif) {
        free(pix);
        return -1;
    }
    pfr_gif_modum_pone(gif, PFR_QUANT_MEDIANA, PFR_DITHER_NULLUM);
    pfr_gif_tabulam_adde(gif, pix);
    pfr_gif_fini(gif);

    free(pix);
    return 0;
}

/* ================================================================
 * main
 * ================================================================ */

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(
            stderr,
            "usus: %s <dir_exemplaris> <dir_exitus> [n_imagines] [semen]\n",
            argv[0]
        );
        return 1;
    }

    const char *dir_mod = argv[1];
    const char *dir_out = argv[2];
    int n_img    = argc >= 4 ? atoi(argv[3]) : 16;
    unsigned int semen = argc >= 5 ? (unsigned int)atoi(argv[4]) : 0;

    if (semen == 0)
        semen = (unsigned int)time(NULL);
    g_semen = semen;

    printf("=== genera eikon ===\n\n");

    pfr_computo_initia();

    /* onera exemplar */
    char via_bin[512];
    snprintf(via_bin, sizeof(via_bin), "%s/model.bin", dir_mod);

    εικ_t eik;
    if (εικ_ἀνάγνωθι(&eik, via_bin) < 0) {
        fprintf(stderr, "error: non potuit onerare '%s'\n", via_bin);
        pfr_computo_fini();
        return 1;
    }

    εικ_gpu_ἄρχε(&eik, NULL);

    printf(
        "exemplar: z=%d eik=%dx%d\n",
        eik.σύνθεσις.ζ_διαστ,
        eik.σύνθεσις.εικ_πλάτος, eik.σύνθεσις.εικ_ὕψος
    );
    printf("semen: %u\n", semen);
    printf("imagines: %d\n\n", n_img);

    int z_dim   = eik.σύνθεσις.ζ_διαστ;
    int eik_dim = eik.σύνθεσις.εικ_πλάτος * eik.σύνθεσις.εικ_ὕψος;
    float *z    = malloc((size_t)z_dim * sizeof(float));
    float *img  = malloc((size_t)eik_dim * sizeof(float));
    if (!z || !img) {
        fprintf(stderr, "error: malloc defecit\n");
        return 1;
    }

    mkdir(dir_out, 0755);

    for (int i = 0; i < n_img; i++) {
        for (int j = 0; j < z_dim; j++)
            z[j] = randn();

        εικ_γεν_πρόσω(&eik, z, img);

        char via[1024];
        snprintf(via, sizeof(via), "%s/gen_%04d.gif", dir_out, i);

        if (
            scribe_gif(
                via, img, eik.σύνθεσις.εικ_πλάτος,
                eik.σύνθεσις.εικ_ὕψος
            ) == 0
        ) {
            printf("  %s\n", via);
        } else {
            fprintf(stderr, "  error: %s\n", via);
        }
    }

    free(z);
    free(img);
    εικ_τέλει(&eik);
    pfr_computo_fini();

    printf("\n=== generatio perfecta ===\n");
    return 0;
}
