/*
 * utilia.h — functiones utiles communes
 */

#ifndef UTILIA_H
#define UTILIA_H

#include <stddef.h>
#include <stdint.h>
struct ison_par;

#ifdef __GNUC__
#define NORETURN __attribute__((noreturn))
#else
#define NORETURN
#endif

/* --- ANSI colores et stili --- */

#define ANSI_RST  "\033[0m"
#define ANSI_BLD  "\033[1m"
#define ANSI_DIM  "\033[2m"
#define ANSI_RED  "\033[31m"
#define ANSI_GRN  "\033[32m"
#define ANSI_YEL  "\033[33m"
#define ANSI_MAG  "\033[35m"
#define ANSI_CYN  "\033[36m"

/*
 * quaere primam occurrentiam cuiuslibet chordae ex serie in textu.
 * reddit indicem chordae inventae, vel -1 si nulla inventa.
 */
int prima_occurrentia(
    const char *textus,
    const char *const *chordae, int num
);

/*
 * scribe nuntium erroris ad stderr et exi.
 * includit nomen plicae et numerum lineae.
 */
NORETURN void morire(
    const char *plica, int linea,
    const char *fmt, ...
);

/* macro: morire cum __FILE__ et __LINE__ automatice */
#define MORIRE(...) morire(__FILE__, __LINE__, __VA_ARGS__)

/*
 * lege chordam modelli: "openai/gpt-5.4+high"
 *   → provisor: "openai" (vel "" si absens)
 *   → nomen: "gpt-5.4"
 *   → conatus:  "high" (vel "" si absens)
 * omnia scribuntur in pulvinaria data. spec non mutatur.
 */
void lege_sapientum(
    const char *spec,
    char *provisor, size_t pmag,
    char *nomen, size_t nmag,
    char *conatus,  size_t cmag
);

/*
 * lege totum plicam in memoriam. vocans liberet per free().
 * reddit NULL si error.
 */
/* --- lexicon: dictionarium clavis(chorda) → valor(long) --- */

typedef struct {
    char clavis[64];
    long valor;
} dictum_t;

typedef struct {
    dictum_t *dicta;
    int num;
    int cap;
} lexicon_t;

lexicon_t *lexicon_crea(void);
void lexicon_libera(lexicon_t *lex);
long lexicon_da(const lexicon_t *lex, const char *clavis);
void lexicon_pone(lexicon_t *lex, const char *clavis, long valor);
void lexicon_adde(lexicon_t *lex, const char *clavis, long additum);
int lexicon_numerus(const lexicon_t *lex);
const dictum_t *lexicon_dictum(const lexicon_t *lex, int index);

/* lexicon cum clavibus compositis "prima:secunda" */
void lexicon_adde_compositam(
    lexicon_t *lex, const char *prima,
    const char *secunda, long additum
);
long lexicon_da_compositam(
    const lexicon_t *lex, const char *prima,
    const char *secunda
);

/* enumera genera unica (partes primas ante ':') in tabulam */
int lexicon_genera(const lexicon_t *lex, char genera[][64], int max_genera);

/* --- plicae --- */

char *lege_plicam(const char *via);

/*
 * lege_instructiones — legit plicam .md ex mundo.
 * construit "{munda}/{phylum}/{genus}.md", legit, monet si absens.
 */
char *lege_instructiones(
    const char *munda, const char *phylum,
    const char *genus
);

/*
 * lege plicam .ison iuxta plicam .c fontem et extrahe parametra.
 * reddit numerum parium lectorum. pares scribitur in struct ison_par tabulam.
 * e.g. si plica_c = "cellae/animae/ursus.c", legit "cellae/animae/ursus.ison"
 */
int lege_parametra(const char *plica_c, struct ison_par *pares, int max);

/* da valorem numericam ex paribus per clavem, vel praefinitum si non inventa */
int par_da_int(
    const struct ison_par *pares, int num,
    const char *clavis, int praefinitum
);

/* da valorem chordae ex paribus — reddit in pulvinar statum, vel praefinitum */
const char *par_da_chordam(
    const struct ison_par *pares, int num,
    const char *clavis, const char *praefinitum
);

/* --- I/O plena (cum retry) --- */

int mitte_plene(int fd, const void *data, size_t mag);
int lege_plene(int fd, void *data, size_t mag);

/* --- big-endian codificatio --- */

void scr16(uint8_t *p, uint16_t v);
void scr24(uint8_t *p, uint32_t v);
uint16_t leg16(const uint8_t *p);
uint32_t leg24(const uint8_t *p);

/* --- hex codificatio --- */

void octeti_ad_hex(const uint8_t *src, size_t mag, char *dest);
int  hex_ad_octetos(
    const char *hex, size_t hex_mag,
    uint8_t *dest, size_t dest_mag
);

/* --- UTF-8 utilia --- */

/*
 * utf8_longitudo — numerat bytes in uno charactere UTF-8 valido apud *p.
 * reddit 1..4 si validus, 0 si invalidus (truncatus vel male formatus).
 */
int utf8_longitudo(const unsigned char *p, size_t relicti);

/*
 * utf8_mundus — scribit src in dest (mag bytes max incl. '\0'),
 * omittens bytes qui UTF-8 invalidum formant et characteres non-impressibiles
 * (exceptis '\n', '\t', et spatiis).
 * reddit numerum bytorum scriptorum (sine '\0').
 */
size_t utf8_mundus(char *dest, size_t mag, const char *src);

#endif /* UTILIA_H */
