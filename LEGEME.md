# Delphi

Bibliotheca oraculi in lingua C — interfacies unificata ad exemplaria linguarum magna (LLM) roganda per provisores multiplices. Synchrona et asynchrona interfacies. Praebet et provisores externos (OpenAI, xAI, Anthropic) et metamorphoten localem (ὀμφαλός).

> **Nota:** README.md ignorandum est. Anglice scriptum est ad barbaros ineruditos pecunia emungendos.

## Aedificatio

```
face
```

Producit `libdelphi.a`. Submoduli prius parandi sunt:

```
face para
```

Ad purgandum:

```
face purga
```

## Usus

### Interfacies synchrona

```
#include "oraculum.h"

oraculum_initia();

char *responsum;
oraculum_roga(
    "openai/gpt-5.4+high",
    "Responde Latine.",
    "Quid est sol?",
    &responsum
);
printf("%s\n", responsum);
free(responsum);

oraculum_fini();
```

### Interfacies asynchrona

```
oraculum_initia();

int fossa = oraculum_mitte(
    "anthropic/claude-sonnet-4-6",
    NULL,
    "Quid est luna?"
);

while (1) {
    oraculum_processus();
    char *resp;
    int st = oraculum_status(fossa, &resp);
    if (st == ORACULUM_PARATUM) {
        printf("%s\n", resp);
        free(resp);
        oraculum_dimitte(fossa);
        break;
    }
}

oraculum_fini();
```

## Provisores

Oraculum provisorem ex nomine sapientis eligit:

| Praefixum | Provisor | API |
|---|---|---|
| `openai/` | OpenAI | Responses API |
| `anthropic/` | Anthropic | Messages API |
| `xai/` | xAI | Chat Completions API |
| `fictus/` | Fictus | Nullum HTTP — responsa ficta ad probandum |
| `omphalos/` | ὀμφαλός | Locale — exemplar ex plica binaria |

Sine praefixo, provisor OpenAI praefinitus est. Suffixum `+low`, `+medium`, `+high` conatum indicat.

Claves API per variabiles ambientus configurantur (e.g. `OPENAI_API_KEY`).

Provisores externi per `oraculum_adde_provisorem()` addi possunt.

## Omphalos

Metamorphotes neuralis architecturae Llama-2 in directorio `omphalos/` iacet, Graece scriptus. Codex venditus est — non hic manutentus sed ex fonte externo inclusus. Provisor `omphalos/` exemplaria localia ex plica binaria (`.bin`) et lexatore (`.lex`) legit et signa generat sine rete, sine clavibus API.

## Numeri

```
oraculum_numeri_t num;
oraculum_numeri(&num);
printf("pendentes: %d  paratae: %d\n", num.pendentes, num.paratae);
printf("signa accepta: %ld  emissa: %ld\n",
    num.summa_signa_accepta, num.summa_signa_emissa);
```

Numeri per sapientum singulum:

```
oraculum_numeri_modelli_t arr[8];
int n = oraculum_numeri_per_sapientum(arr, 8);
for (int i = 0; i < n; i++)
    printf("%s: %ld missae, %ld successae\n",
        arr[i].sapientum, arr[i].missae, arr[i].successae);
```

## Plicae

| Plica | Descriptio |
|---|---|
| `oraculum.h` | Interfacies publica oraculi |
| `oraculum.c` | Dispatcher cum fossis et interfacie multiplici |
| `provisor.h` | Interfacies provisorum |
| `openai.c` | Provisor OpenAI (Responses API) |
| `anthropic.c` | Provisor Anthropic (Messages API) |
| `xai.c` | Provisor xAI (Chat Completions API) |
| `fictus.c` | Provisor fictus pro probatione |
| `utilia.h` | Functiones auxiliares communes |
| `utilia.c` | Implementatio auxiliarium |
| `omphalos/` | Metamorphotes neuralis (codex venditus) |

## Dependentiae

| Submodulum | Descriptio |
|---|---|
| `crispus/` | Cliens HTTPS sine dependentiis externis |
| `ison/` | Bibliotheca ISON sine dependentiis externis |
| `phantasma/` | Operationes matricum (GPU/CPU) |

Omnes submoduli inclusi sunt. Nullae dependentiae externae praeter compilatorem C et libc POSIX.

## Licentia

Libera. Utere ut vis.
