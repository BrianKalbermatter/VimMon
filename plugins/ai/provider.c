#include "provider.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>   // access(): ¿existe y es ejecutable?
#include <curl/curl.h>

// ── La tabla ──────────────────────────────────────────────────
// Agregar un modelo es agregar una fila. Si el modelo habla el protocolo de
// OpenAI (Kimi, Qwen, DeepSeek, Groq, y hasta Ollama en /v1), ya funciona.
const AIProvider ai_providers[] = {
    { "llama",    "Llama 3.2 3B",        TRANSPORT_OLLAMA,
      "http://localhost:11434/api/generate", "llama3.2:3b",      NULL },

    { "qwen",     "Qwen2.5 Coder 7B",    TRANSPORT_OLLAMA,
      "http://localhost:11434/api/generate", "qwen2.5-coder:7b", NULL },

    { "claude",   "Claude Code",         TRANSPORT_CLI,
      "claude",                              "",                 NULL },

    { "kimi",     "Kimi (Moonshot)",     TRANSPORT_OPENAI,
      "https://api.moonshot.cn/v1/chat/completions", "moonshot-v1-8k",
      "MOONSHOT_API_KEY" },

    { "qwen-api", "Qwen Max (DashScope)", TRANSPORT_OPENAI,
      "https://dashscope-intl.aliyuncs.com/compatible-mode/v1/chat/completions",
      "qwen-max", "DASHSCOPE_API_KEY" },
};

const int ai_provider_count = (int)(sizeof(ai_providers) / sizeof(ai_providers[0]));

// Indice del activo. Arranca en 0 (llama local) para no cambiar el default.
static int activo = 0;

const AIProvider *ai_provider_active(void) {
    return &ai_providers[activo];
}

const char *ai_transport_name(AITransport t) {
    switch (t) {
        case TRANSPORT_OLLAMA: return "ollama";
        case TRANSPORT_OPENAI: return "http";
        case TRANSPORT_CLI:    return "cli";
    }
    return "?";
}

int ai_provider_use(const char *id_or_index) {
    if (!id_or_index || id_or_index[0] == '\0') return -1;

    // Por numero de menu: "ai use 2"
    char *fin;
    long n = strtol(id_or_index, &fin, 10);
    if (*fin == '\0' && n >= 1 && n <= ai_provider_count) {
        activo = (int)(n - 1);
        return 0;
    }

    // Por id: "ai use qwen"
    for (int i = 0; i < ai_provider_count; i++) {
        if (strcmp(ai_providers[i].id, id_or_index) == 0) {
            activo = i;
            return 0;
        }
    }
    return -1;
}

// ── Chequeos de disponibilidad ────────────────────────────────

// Descarta el cuerpo de la respuesta: solo nos importa si el server contesta.
static size_t descartar(char *p, size_t size, size_t nmemb, void *ud) {
    (void)p; (void)ud;
    return size * nmemb;
}

// Ollama vivo = GET /api/tags responde. Timeout corto: esto corre al listar
// el menu y no queremos que la consola se cuelgue un minuto.
static int ollama_vivo(void) {
    CURL *curl = curl_easy_init();
    if (!curl) return 0;

    curl_easy_setopt(curl, CURLOPT_URL,           "http://localhost:11434/api/tags");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, descartar);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS,    800L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL,      1L);

    CURLcode rc = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return rc == CURLE_OK;
}

// ¿El binario esta en el PATH? Recorremos PATH a mano en vez de llamar a
// `which`, que implicaria lanzar un proceso solo para preguntar esto.
static int binario_en_path(const char *nombre) {
    const char *path = getenv("PATH");
    if (!path) return 0;

    char copia[4096];
    snprintf(copia, sizeof(copia), "%s", path);

    for (char *dir = strtok(copia, ":"); dir; dir = strtok(NULL, ":")) {
        char completo[4096];
        snprintf(completo, sizeof(completo), "%s/%s", dir, nombre);
        if (access(completo, X_OK) == 0) return 1;
    }
    return 0;
}

int ai_provider_ready(const AIProvider *p, const char **motivo) {
    switch (p->transport) {
        case TRANSPORT_OLLAMA:
            if (!ollama_vivo()) {
                if (motivo) *motivo = "Ollama no responde en :11434";
                return 0;
            }
            return 1;

        case TRANSPORT_OPENAI:
            if (p->api_key_env && !getenv(p->api_key_env)) {
                if (motivo) *motivo = p->api_key_env;  // falta esta variable
                return 0;
            }
            return 1;

        case TRANSPORT_CLI:
            if (!binario_en_path(p->endpoint)) {
                if (motivo) *motivo = "no esta en el PATH";
                return 0;
            }
            return 1;
    }
    return 0;
}

void ai_provider_list(void) {
    printf("\nProveedores de IA:\n");
    for (int i = 0; i < ai_provider_count; i++) {
        const AIProvider *p      = &ai_providers[i];
        const char       *motivo = NULL;
        int               listo  = ai_provider_ready(p, &motivo);

        printf("  %d) %-9s %-22s [%s]  %s",
               i + 1, p->id, p->label, ai_transport_name(p->transport),
               listo ? "listo" : "NO");

        if (!listo && motivo) printf(" (%s)", motivo);
        if (i == activo)      printf("   <- activo");
        printf("\n");
    }
    printf("\n  <numero>             elegir del menu (ej: 3)\n");
    printf("  ai use <id>          elegir por nombre (ej: ai use claude)\n");
    printf("  ai <prompt>          mandar un prompt al activo\n\n");
}
