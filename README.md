# delphi

**A unified oracle for large language models — one interface to query OpenAI, Anthropic, xAI, or a locally trained transformer, in under 1,500 lines of C. Synchronous and asynchronous. Parallel request pooling. Token accounting. Zero external dependencies.**

## Why delphi Exists

Every application that talks to an LLM API ends up building the same scaffolding: HTTP client initialization, JSON request construction, response parsing, token counting, error handling, provider-specific quirks. Then they do it again for a second provider. Then a third. The result is always the same — a tangle of provider-specific code that makes switching models a multi-day refactor and running them in parallel an architectural nightmare.

delphi eliminates the scaffolding entirely. A single function call sends a prompt to any supported provider. A model string like `"openai/gpt-5.4+high"` or `"anthropic/claude-sonnet-4-6"` is all it takes to select the provider, the model, and the reasoning effort. The same interface that queries a cloud API can query a locally trained neural network running entirely on your own hardware — no network, no API keys, no cloud dependency whatsoever.

## The Oracle Interface

Two calling conventions, both trivially simple. Synchronous for when you need the answer now:

```c
#include "oraculum.h"

oraculum_initia();

char *responsum;
oraculum_roga("openai/gpt-5.4+high", "Responde Latine.",
              "Quid est sol?", &responsum);
printf("%s\n", responsum);
free(responsum);

oraculum_fini();
```

Asynchronous for when you need to fire dozens of requests and collect them as they arrive:

```c
int f1 = oraculum_mitte("anthropic/claude-sonnet-4-6", NULL, "Quid est luna?");
int f2 = oraculum_mitte("xai/grok-3", NULL, "Quid est stella?");

while (1) {
    oraculum_processus();
    char *resp;
    if (oraculum_status(f1, &resp) == ORACULUM_PARATUM) {
        printf("Anthropic: %s\n", resp);
        free(resp);
        oraculum_dimitte(f1);
    }
    if (oraculum_status(f2, &resp) == ORACULUM_PARATUM) {
        printf("xAI: %s\n", resp);
        free(resp);
        oraculum_dimitte(f2);
    }
    /* ... */
}
```

Up to 32 concurrent requests in flight. No threads. No event loops. Pure POSIX process concurrency under the hood, completely invisible to the caller.

## Providers

Every major LLM provider speaks a slightly different dialect of HTTP and JSON. delphi speaks all of them natively — each provider is a compact implementation of a single, clean interface, handling request construction, response extraction, and token counting:

| Provider | API | Model String |
|---|---|---|
| **OpenAI** | Responses API | `openai/gpt-5.4` |
| **Anthropic** | Messages API | `anthropic/claude-sonnet-4-6` |
| **xAI** | Chat Completions API | `xai/grok-3` |
| **Local** | No network | `omphalos/my-model` |

Append `+low`, `+medium`, or `+high` to any model string to control reasoning effort where supported. Drop the provider prefix entirely and it defaults to OpenAI. Adding a new provider means implementing three functions — prepare the request, extract the response, count the tokens — and registering it with a single call.

## The Local Oracle

Most LLM libraries treat "local inference" as someone else's problem. delphi includes a complete Llama-2 transformer implementation — the omphalos — capable of loading a trained model from a binary file and generating tokens entirely offline. RMSNorm, rotary positional embeddings, SwiGLU activations, grouped-query attention, AdamW training with cosine annealing — all implemented from first principles, accelerated by GPU when available, falling back to CPU transparently.

The local provider plugs into the exact same oracle interface as every cloud provider. Your application code doesn't change. Your calling conventions don't change. You just point it at a different model string and the network disappears.

## Token Accounting

Every request tracks token usage — input tokens, cached tokens, output tokens, reasoning tokens — broken down per model. Query the running totals at any time, or drill into per-model statistics. Built in from the start, not bolted on after the fact.

```c
oraculum_numeri_t num;
oraculum_numeri(&num);

oraculum_numeri_modelli_t arr[8];
int n = oraculum_numeri_per_sapientum(arr, 8);
```

## The Stack

delphi builds on three libraries, each self-contained, each dependency-free:

- **crispus** — A complete TLS 1.2 HTTPS client with every cryptographic primitive implemented from scratch. SHA-256, AES-128-GCM, ECDHE over P-256, RSA, X.509 — no OpenSSL, no libcurl, no external crypto.
- **ison** — A zero-allocation JSON library that navigates arbitrarily nested documents by dot-path without ever building a parse tree.
- **phantasma** — GPU-accelerated matrix operations for the local transformer, with transparent CPU fallback.

The entire dependency graph is included as git submodules. One `make` invocation builds everything. No package managers. No configuration steps. No hoping that your system's version of some shared library happens to be compatible.

## Building

```bash
make
```

That's it. A C compiler, `ar`, and a POSIX system.

## License

Free. Public domain. Use however you like.
