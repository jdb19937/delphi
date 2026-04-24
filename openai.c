/*
 * openai.c — provisor OpenAI (Responses API)
 */

#include "provisor.h"

#include <crispus/crispus.h>
#include <ison/ison.h>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

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

/* --- imago --- */

/* base64 decodatio RFC 4648 */
static int b64_char(int c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

static unsigned char *b64_decoda(const char *in, size_t *out_n)
{
    size_t cap = strlen(in) * 3 / 4 + 4;
    unsigned char *out = malloc(cap);
    if (!out) return NULL;
    size_t n = 0;
    int quad[4], qi = 0;
    for (; *in; in++) {
        if (*in == '=' || *in == '\n' || *in == '\r' || *in == ' ' || *in == '\t')
            continue;
        int v = b64_char((unsigned char)*in);
        if (v < 0) continue;
        quad[qi++] = v;
        if (qi == 4) {
            out[n++] = (unsigned char)((quad[0] << 2) | (quad[1] >> 4));
            out[n++] = (unsigned char)((quad[1] << 4) | (quad[2] >> 2));
            out[n++] = (unsigned char)((quad[2] << 6) | quad[3]);
            qi = 0;
        }
    }
    if (qi == 2) {
        out[n++] = (unsigned char)((quad[0] << 2) | (quad[1] >> 4));
    } else if (qi == 3) {
        out[n++] = (unsigned char)((quad[0] << 2) | (quad[1] >> 4));
        out[n++] = (unsigned char)((quad[1] << 4) | (quad[2] >> 2));
    }
    *out_n = n;
    return out;
}

struct mem { unsigned char *d; size_t n, cap; };

static int mem_appende(struct mem *m, const void *p, size_t n)
{
    if (m->n + n > m->cap) {
        while (m->n + n > m->cap)
            m->cap = m->cap ? m->cap * 2 : 4096;
        unsigned char *nd = realloc(m->d, m->cap);
        if (!nd) return -1;
        m->d = nd;
    }
    memcpy(m->d + m->n, p, n);
    m->n += n;
    return 0;
}

/* transforma PNG bytes in GIF bytes quadratae lateris per magick */
static int png_ad_gif(
    const unsigned char *png, size_t png_n, int latus,
    unsigned char **gif_out, size_t *gif_n
) {
    int in_pipe[2], out_pipe[2];
    if (pipe(in_pipe) != 0) return -1;
    if (pipe(out_pipe) != 0) {
        close(in_pipe[0]); close(in_pipe[1]);
        return -1;
    }

    char resize_arg[32];
    snprintf(resize_arg, sizeof(resize_arg), "%dx%d!", latus, latus);

    pid_t pid = fork();
    if (pid < 0) {
        close(in_pipe[0]); close(in_pipe[1]);
        close(out_pipe[0]); close(out_pipe[1]);
        return -1;
    }
    if (pid == 0) {
        dup2(in_pipe[0], 0);
        dup2(out_pipe[1], 1);
        close(in_pipe[0]); close(in_pipe[1]);
        close(out_pipe[0]); close(out_pipe[1]);
        execlp("magick", "magick", "png:-", "-resize", resize_arg, "gif:-", (char *)NULL);
        execlp("convert", "convert", "png:-", "-resize", resize_arg, "gif:-", (char *)NULL);
        _exit(127);
    }

    close(in_pipe[0]);
    close(out_pipe[1]);

    signal(SIGPIPE, SIG_IGN);

    /* scribe PNG in stdin fili */
    size_t written = 0;
    while (written < png_n) {
        ssize_t w = write(in_pipe[1], png + written, png_n - written);
        if (w <= 0) {
            if (errno == EINTR) continue;
            break;
        }
        written += w;
    }
    close(in_pipe[1]);

    /* lege GIF ex stdout fili */
    struct mem m = {0};
    unsigned char tmp[4096];
    for (;;) {
        ssize_t r = read(out_pipe[0], tmp, sizeof(tmp));
        if (r < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (r == 0) break;
        if (mem_appende(&m, tmp, (size_t)r) < 0) {
            free(m.d);
            close(out_pipe[0]);
            waitpid(pid, NULL, 0);
            return -1;
        }
    }
    close(out_pipe[0]);

    int status = 0;
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0 || m.n < 6) {
        free(m.d);
        return -1;
    }

    *gif_out = m.d;
    *gif_n   = m.n;
    return 0;
}

struct response_mem {
    char *data;
    size_t n;
};

static size_t response_scribe(void *p, size_t s, size_t nm, void *u)
{
    size_t realis = s * nm;
    struct response_mem *m = u;
    char *nd = realloc(m->data, m->n + realis + 1);
    if (!nd) return 0;
    m->data = nd;
    memcpy(m->data + m->n, p, realis);
    m->n += realis;
    m->data[m->n] = '\0';
    return realis;
}

static char *quaere_imago_b64(const char *ison)
{
    /* iterare output[i] donec inveniatur typus image_generation_call */
    for (int i = 0; i < 16; i++) {
        char via_typi[64], via_result[64];
        snprintf(via_typi, sizeof(via_typi), "output[%d].type", i);
        char *typus = ison_da_chordam(ison, via_typi);
        if (!typus)
            return NULL;
        int cong = (strcmp(typus, "image_generation_call") == 0);
        free(typus);
        if (cong) {
            snprintf(via_result, sizeof(via_result), "output[%d].result", i);
            return ison_da_chordam(ison, via_result);
        }
    }
    return NULL;
}

static int imago(
    const char *nomen, const char *clavis_api,
    const char *rogatum, int latus,
    unsigned char **bytes_out, size_t *mag_out
) {
    (void)nomen;

    char *eff = ison_effuge(rogatum);
    if (!eff) return -1;

    size_t body_cap = strlen(eff) + 512;
    char *body = malloc(body_cap);
    if (!body) { free(eff); return -1; }
    snprintf(
        body, body_cap,
        "{\"model\":\"gpt-5.4\","
        "\"input\":[{\"role\":\"user\",\"content\":\"%s\"}],"
        "\"tools\":[{\"type\":\"image_generation\",\"size\":\"1024x1024\"}]}",
        eff
    );
    free(eff);

    char caput_auth[512];
    snprintf(caput_auth, sizeof(caput_auth),
             "Authorization: Bearer %s", clavis_api);

    struct crispus_slist *capita = NULL;
    capita = crispus_slist_adde(capita, "Content-Type: application/json");
    capita = crispus_slist_adde(capita, caput_auth);

    CRISPUS *c = crispus_facilis_initia();
    if (!c) {
        free(body);
        crispus_slist_libera(capita);
        return -1;
    }

    struct response_mem m = { NULL, 0 };
    crispus_facilis_pone(c, CRISPUSOPT_URL, "https://api.openai.com/v1/responses");
    crispus_facilis_pone(c, CRISPUSOPT_CAMPI_POSTAE, body);
    crispus_facilis_pone(c, CRISPUSOPT_CAPITA_HTTP, capita);
    crispus_facilis_pone(c, CRISPUSOPT_FUNCTIO_SCRIBENDI, response_scribe);
    crispus_facilis_pone(c, CRISPUSOPT_DATA_SCRIBENDI, &m);
    crispus_facilis_pone(c, CRISPUSOPT_TEMPUS, 180L);

    CRISPUScode rc = crispus_facilis_age(c);
    long codex = 0;
    crispus_facilis_info(c, CRISPUSINFO_CODEX_RESPONSI, &codex);
    crispus_facilis_fini(c);
    crispus_slist_libera(capita);
    free(body);

    if (rc != CRISPUSE_OK || codex < 200 || codex >= 300 || !m.data) {
        free(m.data);
        return -1;
    }

    char *b64 = quaere_imago_b64(m.data);
    free(m.data);
    if (!b64) return -1;

    size_t png_n = 0;
    unsigned char *png = b64_decoda(b64, &png_n);
    free(b64);
    if (!png || png_n < 8) {
        free(png);
        return -1;
    }

    int r = png_ad_gif(png, png_n, latus, bytes_out, mag_out);
    free(png);
    return r;
}

const provisor_t provisor_openai = {
    .nomen      = "openai",
    .clavis_env = "OPENAI_API_KEY",
    .finis_url  = "https://api.openai.com/v1/responses",
    .para           = para,
    .para_sessionem = para_sessionem,
    .extrahe        = extrahe,
    .signa          = signa,
    .imago          = imago
};
