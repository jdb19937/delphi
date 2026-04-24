/*
 * oraculum.h — interfacies ad OpenAI Responses API
 *
 * Oraculum rogat et responsa accipit per crispus.
 * Praebet et synchronam et asynchronam interfaciem.
 * Instructiones et rogatum separantur.
 */

#ifndef ORACULUM_H
#define ORACULUM_H

/* status fossae */
enum fossa_actum {
    FOSSA_LIBERA   = 0,     /* vacua, parata ad usum */
    FOSSA_VOLANS   = 1,     /* rogatum missum, responsum expectatur */
    FOSSA_PERFECTA = 2      /* responsum acceptum, legendum */
};

/* initia crispus et crispus_multi — semel voca */
int oraculum_initia(void);

/* adde provisorem externum (e.g. fictus) */
struct provisor;
void oraculum_adde_provisorem(const struct provisor *prov);

/* munda crispus */
void oraculum_fini(void);

/* --- interfacies synchrona --- */

/* instructiones potest esse NULL */
int oraculum_roga(
    const char *sapientum, const char *instructiones,
    const char *rogatum, char **responsum
);

/* --- sessio (colloquium multi-rogati) --- */

typedef struct oraculum_sessio oraculum_sessio_t;

/*
 * Crea sessionem novam. instructiones potest esse NULL.
 * sapientum potest esse NULL (utetur praefinitum).
 */
oraculum_sessio_t *oraculum_sessio_novam(
    const char *sapientum, const char *instructiones
);

/*
 * Appende rogatum usoris, mitte totam historiam, et si successum
 * redde responsum adsistentis (appendendum historiae).
 * Si provisor habet para_sessionem, historia mittitur structurata
 * (e.g. ut array ISON pro OpenAI); alioquin concatenatur cum
 * annotationibus personarum.
 * Reddit 0 si successum. In errato, *responsum = NULL et historia
 * non mutatur.
 */
int oraculum_sessio_roga(
    oraculum_sessio_t *s, const char *rogatum, char **responsum
);

/* Quantum turnorum in historia (includens rogata et responsa). */
int oraculum_sessio_numerus(const oraculum_sessio_t *s);

void oraculum_sessio_dimitte(oraculum_sessio_t *s);

/* --- interfacies asynchrona --- */

#define ORACULUM_PENDENS  0
#define ORACULUM_PARATUM  1
#define ORACULUM_ERRATUM -1

/* sapientum potest esse NULL (utetur praefinitum) */
int oraculum_mitte(
    const char *sapientum, const char *instructiones,
    const char *rogatum
);

void oraculum_processus(void);

int oraculum_status(int fossa, char **responsum);

void oraculum_dimitte(int fossa);

/* --- status et numeri --- */

typedef struct {
    int pendentes;          /* fossae cum actum=1 */
    int paratae;            /* fossae cum actum=2 */
    int liberae;            /* fossae cum actum=0 */
    long summa_missae;
    long summa_successae;
    long summa_errores;
    long summa_signa_accepta;
    long summa_signa_recondita;
    long summa_signa_emissa;
    long summa_signa_cogitata;
} oraculum_numeri_t;

/* da numeros currentibus */
void oraculum_numeri(oraculum_numeri_t *num);

/* da nomen exemplaris currentis */
const char *oraculum_sapientum(void);

/* numeri signorum per sapientum singulum */
typedef struct {
    char sapientum[64];
    int volantes;
    int paratae;
    long missae;
    long successae;
    long errores;
    long signa_accepta;
    long signa_recondita;
    long signa_emissa;
    long signa_cogitata;
} oraculum_numeri_modelli_t;

/* da numeros per sapientum. reddit numerum sapientiorum scriptorum. */
int oraculum_numeri_per_sapientum(oraculum_numeri_modelli_t *arr, int max);

/* --- imago: generatio imaginis (GIF) --- */

#include <stddef.h>

/*
 * Generat imaginem quadratam (latus x latus) ex rogato.
 * Reddit bytes GIF in *bytes_out (vocans liberet), magnitudinem in *mag_out.
 * Si provisor imaginem non supportat, reddit imaginem nigram praefinitam.
 * Reddit 0 si successum, -1 si erratum.
 */
int oraculum_imago_roga(
    const char *sapientum, const char *rogatum, int latus,
    unsigned char **bytes_out, size_t *mag_out
);

#endif /* ORACULUM_H */
