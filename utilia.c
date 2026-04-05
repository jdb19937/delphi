/*
 * utilia.c — functiones utiles communes
 */

#include "utilia.h"
#include "ison.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

/* --- lexicon --- */

lexicon_t *lexicon_crea(void)
{
    lexicon_t *lex = calloc(1, sizeof(*lex));
    if (!lex)
        return NULL;
    lex->cap   = 16;
    lex->dicta = calloc((size_t)lex->cap, sizeof(dictum_t));
    if (!lex->dicta) {
        free(lex);
        return NULL;
    }
    return lex;
}

void lexicon_libera(lexicon_t *lex)
{
    if (lex) {
        free(lex->dicta);
        free(lex);
    }
}

static dictum_t *lexicon_quaere(const lexicon_t *lex, const char *clavis)
{
    for (int i = 0; i < lex->num; i++)
        if (strcmp(lex->dicta[i].clavis, clavis) == 0)
            return &lex->dicta[i];
    return NULL;
}

long lexicon_da(const lexicon_t *lex, const char *clavis)
{
    dictum_t *d = lexicon_quaere(lex, clavis);
    return d ? d->valor : 0;
}

static dictum_t *lexicon_para(lexicon_t *lex, const char *clavis)
{
    dictum_t *d = lexicon_quaere(lex, clavis);
    if (d)
        return d;
    if (lex->num >= lex->cap) {
        lex->cap *= 2;
        dictum_t *nova = realloc(
            lex->dicta,
            (size_t)lex->cap * sizeof(dictum_t)
        );
        if (!nova)
            return NULL;
        lex->dicta = nova;
    }
    d = &lex->dicta[lex->num++];
    snprintf(d->clavis, sizeof(d->clavis), "%s", clavis);
    d->valor = 0;
    return d;
}

void lexicon_pone(lexicon_t *lex, const char *clavis, long valor)
{
    dictum_t *d = lexicon_para(lex, clavis);
    if (d)
        d->valor = valor;
}

void lexicon_adde(lexicon_t *lex, const char *clavis, long additum)
{
    dictum_t *d = lexicon_para(lex, clavis);
    if (d)
        d->valor += additum;
}

int lexicon_numerus(const lexicon_t *lex)
{
    return lex ? lex->num : 0;
}

const dictum_t *lexicon_dictum(const lexicon_t *lex, int index)
{
    if (!lex || index < 0 || index >= lex->num)
        return NULL;
    return &lex->dicta[index];
}

/* --- claves compositae --- */

static void compone_clavem(
    char *buf, size_t mag,
    const char *prima, const char *secunda
) {
    snprintf(buf, mag, "%s:%s", prima, secunda);
}

void lexicon_adde_compositam(
    lexicon_t *lex, const char *prima,
    const char *secunda, long additum
) {
    char clavis[64];
    compone_clavem(clavis, sizeof(clavis), prima, secunda);
    lexicon_adde(lex, clavis, additum);
}

long lexicon_da_compositam(
    const lexicon_t *lex, const char *prima,
    const char *secunda
) {
    char clavis[64];
    compone_clavem(clavis, sizeof(clavis), prima, secunda);
    return lexicon_da(lex, clavis);
}

int lexicon_genera(const lexicon_t *lex, char genera[][64], int max_genera)
{
    if (!lex)
        return 0;
    int n = 0;
    for (int i = 0; i < lex->num && n < max_genera; i++) {
        const char *sep = strchr(lex->dicta[i].clavis, ':');
        if (!sep)
            continue;
        size_t lon = (size_t)(sep - lex->dicta[i].clavis);
        if (lon >= 64)
            lon = 63;
        int iam = 0;
        for (int j = 0; j < n; j++) {
            if (
                strncmp(genera[j], lex->dicta[i].clavis, lon) == 0
                && genera[j][lon] == '\0'
            ) {
                iam = 1;
                break;
            }
        }
        if (!iam) {
            memcpy(genera[n], lex->dicta[i].clavis, lon);
            genera[n][lon] = '\0';
            n++;
        }
    }
    return n;
}

/* --- prima_occurrentia --- */

int prima_occurrentia(
    const char *textus,
    const char *const *chordae, int num
) {
    int optimus = -1;
    const char *optima_pos = NULL;

    for (int i = 0; i < num; i++) {
        const char *pos = strstr(textus, chordae[i]);
        if (pos && (!optima_pos || pos < optima_pos)) {
            optima_pos = pos;
            optimus    = i;
        }
    }
    return optimus;
}

void lege_sapientum(
    const char *spec,
    char *provisor, size_t pmag,
    char *nomen,    size_t nmag,
    char *conatus,  size_t cmag
) {
    provisor[0] = '\0';
    conatus[0]  = '\0';

    /* provisor/ praefixum */
    const char *solidus       = strchr(spec, '/');
    const char *post_provisor = spec;
    if (solidus) {
        size_t plen = (size_t)(solidus - spec);
        if (plen >= pmag)
            plen = pmag - 1;
        memcpy(provisor, spec, plen);
        provisor[plen] = '\0';
        post_provisor  = solidus + 1;
    }

    /* nomen+conatus */
    const char *plus = strchr(post_provisor, '+');
    if (plus) {
        size_t nlen = (size_t)(plus - post_provisor);
        if (nlen >= nmag)
            nlen = nmag - 1;
        memcpy(nomen, post_provisor, nlen);
        nomen[nlen] = '\0';
        snprintf(conatus, cmag, "%s", plus + 1);
    } else {
        snprintf(nomen, nmag, "%s", post_provisor);
    }
}

char *lege_plicam(const char *via)
{
    FILE *f = fopen(via, "r");
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    long mag = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc((size_t)mag + 1);
    if (buf) {
        size_t n = fread(buf, 1, (size_t)mag, f);
        buf[n]   = '\0';
    }
    fclose(f);
    return buf;
}

char *lege_instructiones(
    const char *munda, const char *phylum,
    const char *genus
) {
    char via[256];
    snprintf(via, sizeof(via), "%s/%s/%s.md", munda, phylum, genus);
    char *res = lege_plicam(via);
    if (!res) {
        fprintf(stderr, "monitum: %s non inventum, praefinitum utor\n", via);
        char praefinitum[64];
        snprintf(praefinitum, sizeof(praefinitum), "age ut %s.", genus);
        return strdup(praefinitum);
    }
    return res;
}

int lege_parametra(const char *plica_c, ison_par_t *pares, int max)
{
    /* .c → .ison */
    size_t lon = strlen(plica_c);
    if (lon < 2)
        return 0;
    char *via = malloc(lon + 4);   /* .c → .ison = +3 */
    if (!via)
        return 0;
    memcpy(via, plica_c, lon + 1);
    if (via[lon-1] == 'c' && via[lon-2] == '.') {
        memcpy(via + lon - 1, "ison", 5);
    }
    char *ison = lege_plicam(via);
    free(via);
    if (!ison)
        return 0;
    int n = ison_lege(ison, pares, max);
    free(ison);
    return n;
}

int par_da_int(
    const ison_par_t *pares, int num,
    const char *clavis, int praefinitum
) {
    for (int i = 0; i < num; i++) {
        if (strcmp(pares[i].clavis, clavis) == 0)
            return atoi(pares[i].valor);
    }
    return praefinitum;
}

const char *par_da_chordam(
    const ison_par_t *pares, int num,
    const char *clavis, const char *praefinitum
) {
    for (int i = 0; i < num; i++) {
        if (strcmp(pares[i].clavis, clavis) == 0)
            return pares[i].valor;
    }
    return praefinitum;
}

/* --- I/O plena --- */

int mitte_plene(int fd, const void *data, size_t mag)
{
    const uint8_t *p = data;
    size_t missum    = 0;
    while (missum < mag) {
        ssize_t r = write(fd, p + missum, mag - missum);
        if (r <= 0)
            return -1;
        missum += (size_t)r;
    }
    return 0;
}

int lege_plene(int fd, void *data, size_t mag)
{
    uint8_t *p    = data;
    size_t lectum = 0;
    while (lectum < mag) {
        ssize_t r = read(fd, p + lectum, mag - lectum);
        if (r <= 0)
            return -1;
        lectum += (size_t)r;
    }
    return 0;
}

/* --- big-endian codificatio --- */

void scr16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)v;
}
void scr24(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 16);
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)v;
}
uint16_t leg16(const uint8_t *p) { return ((uint16_t)p[0] << 8) | p[1]; }
uint32_t leg24(const uint8_t *p) { return ((uint32_t)p[0] << 16) | ((uint32_t)p[1] << 8) | p[2]; }

/* --- hex codificatio --- */

void octeti_ad_hex(const uint8_t *src, size_t mag, char *dest)
{
    static const char tabulae[] = "0123456789abcdef";
    for (size_t i = 0; i < mag; i++) {
        dest[2*i]     = tabulae[(src[i] >> 4) & 0xf];
        dest[2*i + 1] = tabulae[src[i] & 0xf];
    }
}

int hex_ad_octetos(
    const char *hex, size_t hex_mag,
    uint8_t *dest, size_t dest_mag
) {
    if (hex_mag != dest_mag * 2)
        return -1;
    for (size_t i = 0; i < dest_mag; i++) {
        unsigned hi, lo;
        char c;
        c = hex[2*i];
        if      (c >= '0' && c <= '9')
            hi = (unsigned)(c - '0');
        else if (c >= 'a' && c <= 'f')
            hi = (unsigned)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F')
            hi = (unsigned)(c - 'A' + 10);
        else
            return -1;
        c = hex[2*i + 1];
        if      (c >= '0' && c <= '9')
            lo = (unsigned)(c - '0');
        else if (c >= 'a' && c <= 'f')
            lo = (unsigned)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F')
            lo = (unsigned)(c - 'A' + 10);
        else
            return -1;
        dest[i] = (uint8_t)((hi << 4) | lo);
    }
    return 0;
}

NORETURN void morire(
    const char *plica, int linea,
    const char *fmt, ...
) {
    fprintf(stderr, "%s:%d: ", plica, linea);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(1);
}

/* ================================================================
 * UTF-8 utilia
 * ================================================================ */

int utf8_longitudo(const unsigned char *p, size_t relicti)
{
    if (relicti == 0)
        return 0;
    unsigned char c = p[0];

    if (c < 0x80) {                       /* 1 byte: 0xxxxxxx */
        return 1;
    } else if (c < 0xC2) {               /* 0x80..0xC1: invalidus */
        return 0;
    } else if (c < 0xE0) {               /* 2 bytes: 110xxxxx 10xxxxxx */
        if (relicti < 2)
            return 0;
        if ((p[1] & 0xC0) != 0x80)
            return 0;
        return 2;
    } else if (c < 0xF0) {               /* 3 bytes: 1110xxxx ... */
        if (relicti < 3)
            return 0;
        if ((p[1] & 0xC0) != 0x80)
            return 0;
        if ((p[2] & 0xC0) != 0x80)
            return 0;
        /* surrogates (U+D800..U+DFFF) exclusi */
        if (c == 0xED && p[1] >= 0xA0)
            return 0;
        return 3;
    } else if (c < 0xF5) {               /* 4 bytes: 11110xxx ... */
        if (relicti < 4)
            return 0;
        if ((p[1] & 0xC0) != 0x80)
            return 0;
        if ((p[2] & 0xC0) != 0x80)
            return 0;
        if ((p[3] & 0xC0) != 0x80)
            return 0;
        return 4;
    }
    return 0;
}

size_t utf8_mundus(char *dest, size_t mag, const char *src)
{
    if (!dest || mag == 0)
        return 0;
    const unsigned char *s   = (const unsigned char *)src;
    const unsigned char *fin = s + strlen(src);
    size_t cursor = 0;

    while (s < fin && cursor + 1 < mag) {
        size_t relicti = (size_t)(fin - s);
        int lon        = utf8_longitudo(s, relicti);
        if (lon == 0) {
            s++;   /* salta byte invalidum */
            continue;
        }

        /* byte ASCII: filtra characteres non-impressibiles */
        if (lon == 1) {
            unsigned char c = s[0];
            if (c == '\n' || c == '\t' || (c >= 0x20 && c < 0x7F))
                dest[cursor++] = (char)c;
        } else {
            /* character multi-byte UTF-8 validus */
            if (cursor + (size_t)lon < mag) {
                for (int i = 0; i < lon; i++)
                    dest[cursor++] = (char)s[i];
            }
        }
        s += lon;
    }
    dest[cursor] = '\0';
    return cursor;
}
