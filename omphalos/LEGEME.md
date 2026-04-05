# ὀμφαλός

Lapis sacer in centro oraculi Delphici — metamorphotes neuralis
architecturae Llama-2, Graece scriptus.

## Inventio

Omphalos primum effossus est dum oraculum ipsum aedificabatur.
Provisores externi (OpenAI, xAI, Anthropic) per rete rogabantur,
sed quaestio orta est: potestne oraculum ipsum cogitare, sine
rete, sine clavibus API, sine nubibus alienis?

Fundamenta sub templo reperta sunt — stratum post stratum
operationum mathematicarum in saxo insculptarum:

1. **RMSNorm** — normalizatio radicis mediae quadratorum, qua
   activationes stabiliuntur ante quodque stratum.
2. **RoPE** — codificatio positionalis rotatoria, qua ordo
   verborum in memoriam inscribitur.
3. **SwiGLU** — porta activationis in retibus praepositis (FFN),
   quae informationes seligit per multiplicem non-linearitatem.
4. **GQA** — attentio quaestionum gruppatim, qua claves et
   valores inter capita communicantur ad memoriam servandam.

## Effossio

Effossio in quinque strata processit:

### Stratum I — λεκτήρ (lexator BPE)

Ante omnia verba in numeros convertenda erant. Lexator
Byte-Pair Encoding ex corpore exercetur: initium ab 256 signis
octeticis, coniugat paria frequentissima donec lexicon plenum sit.
Sic textus Latinus (vel quaecumque lingua) in signa comprimitur.

### Stratum II — δρόμος (cursus porro)

Ipsum cor metamorphotae. Signum intrat, per stratos attentionis
et retia praeposita transit, logitae exeunt. Praedicit quod signum
proximum sit. Operationes omnes per phantasma/computo fiunt, ut
GPU (Metal vel CUDA) vel CPU adhibeatur sine mutatione codicis.

### Stratum III — ἄσκησις (exercitatio)

Retropropagatio per truncatam BPTT: gradientes accumulantur per
positiones, tunc AdamW optimizator pondera emendat. Cosinus
annealing gradum discendi a maximo ad minimum ducit.

### Stratum IV — χορηγός (provisor)

Pons inter omphalon et oraculum. Implementat interfaciem
provisoris ut oraculum possit locale exemplar rogare eadem
via qua provisores externos rogat. Nullum HTTP, nulla clavis
API — exemplar ex plica binaria (.bin) et lexatore (.lex) legitur,
signa generantur per deiectum multinomialem cum temperatura.

### Stratum V — ἄσκησον et δοκιμή

Instrumenta ad exercitandum et probandum. Trainer ex linea
mandatorum corpus legit, lexatorem et exemplar exercet, puncta
securitatis servat. Probatio omnia simul verificat: exercitationem,
serializationem, generationem, provisionem.

## Tabulae

| Tabula | Descriptio |
|--------|------------|
| `ὀμφαλός.h` | Typi communes: σύνθεσις, σταθμά, κατάστασις, ἄσκησις |
| `λεκτήρ.h` | Interfacies lexatoris BPE |
| `δρόμος.c` | Cursus porro, GPU administratio, E/E binaria |
| `λεκτήρ.c` | Implementatio lexatoris BPE |
| `ἄσκησις.c` | Retropropagatio et AdamW |
| `χορηγός.c` | Provisor oraculi pro exemplaribus localibus |
| `ἄσκησον.c` | Instrumentum exercitationis ex linea mandatorum |
| `δοκιμή.c` | Probatio integra metamorphotae |

## Dependentiae

- `phantasma/computo.h` — operationes matricum (GPU/CPU)
- `provisor.h` — interfacies provisorum oraculi
- `utilia.h` — functiones auxiliares

## Status

Omphalos vivit et spirat. Exemplaria parva (64-256 dimensionum)
exerceri possunt in corpore Latino; generatio textus imperfecta
sed demonstrabilis est. Lapis adhuc poliendus est.
