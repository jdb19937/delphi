/*
 * exercita_eikon.c — exercitatio GAN eikon ex linea mandatorum
 *
 * Usus: exercita_eikon <dir_imaginum> <dir_exemplaris> [n_gradus] [eik_dim]
 *
 *   <dir_imaginum>     directorium GIF (e.g. nova/)
 *   <dir_exemplaris>   directorium exemplaris (model.bin)
 *   [n_gradus]         gradus exercitationis (praedef. 10000)
 *   [eik_dim]          latitudo/altitudo imaginis (praedef. 64, e.g. 16, 32)
 *
 * Si <dir_exemplaris>/model.bin iam exstat, exercitatio continuatur.
 * Asservatio fit per ASSERVATIO_GRADUS gradus.
 * Imagines generatae scribuntur per SPECIMEN_GRADUS gradus.
 */

#include "eikon/εἰκών.h"
#include "computo.h"
#include "phantasma.h"

#include <dirent.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* ================================================================
 * parametri praedefiniti
 * ================================================================ */

#define PRAEDEF_Z_DIAST       128
#define PRAEDEF_G_KRU1        512
#define PRAEDEF_G_KRU2        2048
#define PRAEDEF_EIK_LAT       64
#define PRAEDEF_EIK_ALT       64
#define PRAEDEF_K_KRU1        2048
#define PRAEDEF_K_KRU2        512

#define PRAEDEF_N_GRADUS      10000
#define PRAEDEF_FASCIS        16     /* minima fascis */
#define PRAEDEF_LR            2e-4f
#define PRAEDEF_BETA1         0.5f
#define PRAEDEF_BETA2         0.999f
#define PRAEDEF_EPSILON       1e-8f

#define NUNTIUS_GRADUS        50
#define ASSERVATIO_GRADUS     500
#define SPECIMEN_GRADUS       500
#define SPECIMEN_N            16     /* imagines per specimen */

/* ================================================================
 * RNG
 * ================================================================ */

static unsigned int g_semen = 42u;

static float randf(void)
{
    g_semen = g_semen * 1664525u + 1013904223u;
    return (g_semen >> 8) / 16777216.0f;
}

static float randn(void)
{
    float u1 = randf();
    if (u1 < 1e-10f) u1 = 1e-10f;
    float u2 = randf();
    return sqrtf(-2.0f * logf(u1)) * cosf(6.2831853f * u2);
}

static int randi(int n)
{
    g_semen = g_semen * 1664525u + 1013904223u;
    return (int)(g_semen % (unsigned int)n);
}

/* ================================================================
 * lectio imaginum GIF per phantasma
 * ================================================================ */

/*
 * lege_gif_canum — legit GIF, convertit ad canos float [-1,1].
 * Redimensionat ad dest_lat x dest_alt per subsampling.
 * Reddit 0 si bene.
 */
static int lege_gif_canum(
    const char *via, float *dest,
    int dest_lat, int dest_alt)
{
    pfr_gif_lector_t *l = pfr_gif_lege_initia(via);
    if (!l)
        return -1;

    int lat, alt;
    if (pfr_gif_lege_dimensiones(l, &lat, &alt) < 0) {
        pfr_gif_lege_fini(l);
        return -1;
    }

    uint32_t *pix = malloc((size_t)lat * alt * sizeof(uint32_t));
    if (!pix) { pfr_gif_lege_fini(l); return -1; }

    if (pfr_gif_lege_tabulam(l, pix) < 0) {
        free(pix);
        pfr_gif_lege_fini(l);
        return -1;
    }
    pfr_gif_lege_fini(l);

    /* subsample ad dest_lat x dest_alt, convertit ad canos [-1,1] */
    float sx = (float)lat / (float)dest_lat;
    float sy = (float)alt / (float)dest_alt;

    for (int dy = 0; dy < dest_alt; dy++) {
        for (int dx = 0; dx < dest_lat; dx++) {
            int ox = (int)(dx * sx);
            int oy = (int)(dy * sy);
            if (ox >= lat) ox = lat - 1;
            if (oy >= alt) oy = alt - 1;
            uint32_t argb = pix[oy * lat + ox];
            int r = (argb >> 16) & 0xFF;
            int g = (argb >> 8)  & 0xFF;
            int b =  argb        & 0xFF;
            /* luminantia -> [-1, 1] */
            float gray = (r * 0.299f + g * 0.587f + b * 0.114f) / 127.5f - 1.0f;
            dest[dy * dest_lat + dx] = gray;
        }
    }

    free(pix);
    return 0;
}

/* ================================================================
 * scriptio specimenorum GIF
 * ================================================================ */

/*
 * scribe_specimen_gif — scribit graticulum N imaginum in unum GIF.
 * imagines: N tabellae de εικ_διαστ float [-1,1].
 * Creat graticulum sqrt(N) x sqrt(N).
 */
static int scribe_specimen_gif(
    const char *via, const float *imagines, int n,
    int eik_lat, int eik_alt)
{
    int cols = (int)ceilf(sqrtf((float)n));
    int rows = (n + cols - 1) / cols;
    int tot_lat = cols * eik_lat;
    int tot_alt = rows * eik_alt;

    uint32_t *pix = calloc((size_t)tot_lat * tot_alt, sizeof(uint32_t));
    if (!pix) return -1;

    for (int i = 0; i < n; i++) {
        int col = i % cols;
        int row = i / cols;
        const float *img = imagines + (size_t)i * eik_lat * eik_alt;

        for (int y = 0; y < eik_alt; y++) {
            for (int x = 0; x < eik_lat; x++) {
                float v = (img[y * eik_lat + x] + 1.0f) * 127.5f;
                int g = (int)v;
                if (g < 0)   g = 0;
                if (g > 255) g = 255;
                int px = col * eik_lat + x;
                int py = row * eik_alt + y;
                pix[py * tot_lat + px] =
                    0xFF000000u | ((uint32_t)g << 16) | ((uint32_t)g << 8) | (uint32_t)g;
            }
        }
    }

    pfr_gif_t *gif = pfr_gif_initia(via, tot_lat, tot_alt, 0, 1);
    if (!gif) { free(pix); return -1; }
    pfr_gif_modum_pone(gif, PFR_QUANT_MEDIANA, PFR_DITHER_NULLUM);
    pfr_gif_tabulam_adde(gif, pix);
    pfr_gif_fini(gif);

    free(pix);
    return 0;
}

/* ================================================================
 * lectio directorii
 * ================================================================ */

static int lege_directorium(
    const char *dir, float **data, int *n_img,
    int eik_lat, int eik_alt)
{
    DIR *d = opendir(dir);
    if (!d) return -1;

    int cap = 256, num = 0;
    int eik_dim = eik_lat * eik_alt;
    float *buf = malloc((size_t)cap * eik_dim * sizeof(float));
    if (!buf) { closedir(d); return -1; }

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        const char *nom = ent->d_name;
        size_t len = strlen(nom);
        if (len < 4) continue;
        if (strcmp(nom + len - 4, ".gif") != 0) continue;

        if (num >= cap) {
            cap *= 2;
            float *nb = realloc(buf, (size_t)cap * eik_dim * sizeof(float));
            if (!nb) { free(buf); closedir(d); return -1; }
            buf = nb;
        }

        char via[1024];
        snprintf(via, sizeof(via), "%s/%s", dir, nom);

        if (lege_gif_canum(via, buf + (size_t)num * eik_dim,
                           eik_lat, eik_alt) == 0)
        {
            num++;
            if (num % 100 == 0)
                printf("   %d imagines lectae...\n", num);
        }
    }
    closedir(d);

    *data  = buf;
    *n_img = num;
    return 0;
}

/* ================================================================
 * main
 * ================================================================ */

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr,
            "usus: %s <dir_imaginum> <dir_exemplaris> [n_gradus] [eik_dim]\n"
            "  praedef: n_gradus=%d fascis=%d lr=%.0e eik_dim=%d\n",
            argv[0], PRAEDEF_N_GRADUS, PRAEDEF_FASCIS, PRAEDEF_LR,
            PRAEDEF_EIK_LAT
        );
        return 1;
    }

    const char *dir_img = argv[1];
    const char *dir_mod = argv[2];
    int n_gradus = argc >= 4 ? atoi(argv[3]) : PRAEDEF_N_GRADUS;
    int eik_dim  = argc >= 5 ? atoi(argv[4]) : PRAEDEF_EIK_LAT;

    printf("=== exercita eikon GAN ===\n\n");

    int gpu = pfr_computo_initia();
    printf("computator: %s\n\n", gpu == 0 ? "GPU" : "CPU");

    char via_bin[512], via_spec[512];
    snprintf(via_bin, sizeof(via_bin), "%s/model.bin", dir_mod);

    /* --- 1. lege imagines --- */
    printf("1. lego imagines ex '%s'...\n", dir_img);
    float *data  = NULL;
    int    n_img = 0;

    if (lege_directorium(dir_img, &data, &n_img,
                         eik_dim, eik_dim) < 0 || n_img == 0)
    {
        fprintf(stderr, "error: imagines non inventae in '%s'\n", dir_img);
        pfr_computo_fini();
        return 1;
    }
    printf("   imagines: %d (%dx%d cani)\n\n", n_img, eik_dim, eik_dim);

    /* --- 2. exemplar: onera vel incipe novum --- */
    εικ_t eik;

    if (εικ_ἀνάγνωθι(&eik, via_bin) == 0) {
        printf("2. exemplar oneratum ex '%s'\n", via_bin);
        printf("   z=%d g1=%d g2=%d eik=%dx%d k1=%d k2=%d\n\n",
            eik.σύνθεσις.ζ_διαστ,
            eik.σύνθεσις.γ_κρυ1, eik.σύνθεσις.γ_κρυ2,
            eik.σύνθεσις.εικ_πλάτος, eik.σύνθεσις.εικ_ὕψος,
            eik.σύνθεσις.κ_κρυ1, eik.σύνθεσις.κ_κρυ2
        );
    } else {
        /* scala strata occulta proportionaliter ad dimensionem imaginis */
        int img_pixels = eik_dim * eik_dim;
        int g_h2 = img_pixels / 2;  if (g_h2 < 64)  g_h2 = 64;
        int g_h1 = g_h2 / 4;        if (g_h1 < 32)  g_h1 = 32;
        int z_d  = g_h1 / 2;        if (z_d  < 16)  z_d  = 16;
        int k_h1 = g_h2;
        int k_h2 = g_h1;

        εικ_σύνθεσις_t conf = {
            .ζ_διαστ    = z_d,
            .γ_κρυ1     = g_h1,
            .γ_κρυ2     = g_h2,
            .εικ_πλάτος = eik_dim,
            .εικ_ὕψος   = eik_dim,
            .κ_κρυ1     = k_h1,
            .κ_κρυ2     = k_h2,
        };

        printf("2. incipio exemplar novum\n");
        printf("   z=%d g1=%d g2=%d eik=%dx%d k1=%d k2=%d\n",
            conf.ζ_διαστ, conf.γ_κρυ1, conf.γ_κρυ2,
            conf.εικ_πλάτος, conf.εικ_ὕψος,
            conf.κ_κρυ1, conf.κ_κρυ2
        );

        if (εικ_ἄρχε_τυχαίως(&eik, &conf, 42u) < 0) {
            fprintf(stderr, "error: εικ_ἄρχε_τυχαίως defecit\n");
            free(data);
            pfr_computo_fini();
            return 1;
        }

        size_t n_par = εικ_μέγεθος_σταθμῶν(&conf);
        printf("   parametri: %zu = %.1f MB\n", n_par,
               n_par * sizeof(float) / (1024.0 * 1024.0));
        mkdir(dir_mod, 0755);
    }
    printf("\n");

    /* --- 3. praepara exercitationem --- */
    εικ_ἄσκησις_t exc;
    if (εικ_ἄσκησις_ἄρχε(&exc, &eik.σύνθεσις) < 0) {
        fprintf(stderr, "error: εικ_ἄσκησις_ἄρχε defecit\n");
        εικ_τέλει(&eik);
        free(data);
        pfr_computo_fini();
        return 1;
    }

    εικ_gpu_ἄρχε(&eik, &exc);

    int eik_pixels = eik.σύνθεσις.εικ_πλάτος * eik.σύνθεσις.εικ_ὕψος;
    int z_dim   = eik.σύνθεσις.ζ_διαστ;
    float *z    = malloc((size_t)z_dim * sizeof(float));
    if (!z) { fprintf(stderr, "error: malloc z\n"); return 1; }

    printf("3. exerceo (%d gradus, fascis=%d, lr=%.0e)...\n\n",
           n_gradus, PRAEDEF_FASCIS, PRAEDEF_LR);

    /* --- 4. circuitus exercitationis --- */
    float sum_loss_d = 0.0f, sum_loss_g = 0.0f;
    int n_report = 0;

    for (int gradus = 0; gradus < n_gradus; gradus++) {
        float loss_d = 0.0f, loss_g = 0.0f;

        /* --- Κριτής (Discriminator) --- */
        εικ_κλ_μηδέν(&exc, &eik.σύνθεσις);

        for (int mi = 0; mi < PRAEDEF_FASCIS; mi++) {
            /* vera imago */
            int idx = randi(n_img);
            float *real = data + (size_t)idx * eik_pixels;

            εικ_κρι_πρόσω_μνήμη(&eik, &exc, real);
            loss_d += εικ_κρι_ἀναδρομή(&eik, &exc, 0.9f); /* label smoothing */

            /* falsa imago */
            for (int i = 0; i < z_dim; i++)
                z[i] = randn();
            εικ_γεν_πρόσω_μνήμη(&eik, &exc, z);
            εικ_κρι_πρόσω_μνήμη(&eik, &exc, exc.γ_out);
            loss_d += εικ_κρι_ἀναδρομή(&eik, &exc, 0.0f);
        }

        /* scala κλίσεων */
        {
            float s = 1.0f / (float)(2 * PRAEDEF_FASCIS);
            size_t off = (size_t)(eik.σταθμά.κw1 - eik.δεδομένα);
            size_t n_k = eik.μέγεθος - off;
            for (size_t i = 0; i < n_k; i++)
                exc.κλ_δεδ[off + i] *= s;
        }
        εικ_βῆμα_ἀδάμ_κρι(&eik, &exc, PRAEDEF_LR,
                           PRAEDEF_BETA1, PRAEDEF_BETA2, PRAEDEF_EPSILON);

        /* --- Γενέτης (Generator) --- */
        εικ_κλ_μηδέν(&exc, &eik.σύνθεσις);

        for (int mi = 0; mi < PRAEDEF_FASCIS; mi++) {
            for (int i = 0; i < z_dim; i++)
                z[i] = randn();
            εικ_γεν_πρόσω_μνήμη(&eik, &exc, z);
            εικ_κρι_πρόσω_μνήμη(&eik, &exc, exc.γ_out);
            loss_g += εικ_κρι_ἀναδρομή(&eik, &exc, 1.0f);
            εικ_γεν_ἀναδρομή(&eik, &exc);
        }

        {
            float s = 1.0f / (float)PRAEDEF_FASCIS;
            size_t n_g = (size_t)(eik.σταθμά.κw1 - eik.δεδομένα);
            for (size_t i = 0; i < n_g; i++)
                exc.κλ_δεδ[i] *= s;
        }
        εικ_βῆμα_ἀδάμ_γεν(&eik, &exc, PRAEDEF_LR,
                           PRAEDEF_BETA1, PRAEDEF_BETA2, PRAEDEF_EPSILON);

        /* accumula pro nuntio */
        loss_d /= (float)(2 * PRAEDEF_FASCIS);
        loss_g /= (float)PRAEDEF_FASCIS;
        sum_loss_d += loss_d;
        sum_loss_g += loss_g;
        n_report++;

        /* nuntius */
        if ((gradus + 1) % NUNTIUS_GRADUS == 0) {
            printf("  gradus %5d  D_loss=%.4f  G_loss=%.4f\n",
                   gradus + 1,
                   sum_loss_d / n_report,
                   sum_loss_g / n_report);
            fflush(stdout);
            sum_loss_d = 0.0f;
            sum_loss_g = 0.0f;
            n_report   = 0;
        }

        /* asservatio */
        if ((gradus + 1) % ASSERVATIO_GRADUS == 0) {
            if (εικ_σῶσον(&eik, via_bin) == 0)
                printf("  [asservatio: gradus %d -> %s]\n",
                       gradus + 1, via_bin);
            else
                fprintf(stderr, "  monitum: asservatio defecit\n");
        }

        /* specimen */
        if ((gradus + 1) % SPECIMEN_GRADUS == 0) {
            float *spec = malloc(
                (size_t)SPECIMEN_N * eik_pixels * sizeof(float));
            if (spec) {
                unsigned int sp_seed = (unsigned int)(gradus + 1);
                for (int si = 0; si < SPECIMEN_N; si++) {
                    float zs[256];
                    for (int i = 0; i < z_dim; i++) {
                        sp_seed = sp_seed * 1664525u + 1013904223u;
                        float u1 = (sp_seed >> 8) / 16777216.0f;
                        if (u1 < 1e-10f) u1 = 1e-10f;
                        sp_seed = sp_seed * 1664525u + 1013904223u;
                        float u2 = (sp_seed >> 8) / 16777216.0f;
                        zs[i] = sqrtf(-2.0f * logf(u1))
                               * cosf(6.2831853f * u2);
                    }
                    εικ_γεν_πρόσω(&eik, zs, spec + (size_t)si * eik_pixels);
                }

                snprintf(via_spec, sizeof(via_spec),
                         "%s/specimen_%05d.gif", dir_mod, gradus + 1);
                if (scribe_specimen_gif(via_spec, spec, SPECIMEN_N,
                    eik.σύνθεσις.εικ_πλάτος,
                    eik.σύνθεσις.εικ_ὕψος) == 0)
                {
                    printf("  [specimen: %s]\n", via_spec);
                }
                free(spec);
            }
            fflush(stdout);
        }
    }

    /* --- 5. salvatio finalis --- */
    if (εικ_σῶσον(&eik, via_bin) == 0)
        printf("\n5. exemplar servatum: %s\n", via_bin);
    else
        fprintf(stderr, "\nerror: salvatio finalis defecit\n");

    free(z);
    εικ_ἄσκησις_τέλει(&exc);
    εικ_τέλει(&eik);
    free(data);
    pfr_computo_fini();

    printf("\n=== exercitatio perfecta ===\n");
    return 0;
}
