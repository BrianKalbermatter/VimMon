#include "ai.h"
#include "../../cjson/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

// ── Buffer para acumular la respuesta HTTP ────────────────────
typedef struct {
    char  *data;
    size_t len;
} CurlBuf;

static size_t curl_write_cb(char *ptr, size_t size, size_t nmemb, void *userdata) {
    CurlBuf *buf = (CurlBuf *)userdata;
    size_t total = size * nmemb;
    char  *tmp   = realloc(buf->data, buf->len + total + 1);
    if (!tmp) return 0;
    buf->data = tmp;
    memcpy(buf->data + buf->len, ptr, total);
    buf->len += total;
    buf->data[buf->len] = '\0';
    return total;
}

// ── Leer scene.paed completo ──────────────────────────────────
static char *read_scene(void) {
    FILE *f = fopen(SCENE_PATH, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    char *buf = malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, (size_t)sz, f);
    buf[sz] = '\0';
    fclose(f);
    return buf;
}

// ── POST a Ollama, escribe PAED delta en out ──────────────────
static int ollama_request(const char *user_prompt, char *out, int out_size) {
    char *scene = read_scene();

    char full_prompt[8192];
    snprintf(full_prompt, sizeof(full_prompt),
        "Sos un motor de escenas 3D. El usuario te pide cambios y vos respondés "
        "SOLO con instrucciones PAED válidas, sin explicaciones ni comentarios.\n\n"
        "Reglas del lenguaje PAED:\n"
        "- Una instrucción por línea, todo en minúsculas\n"
        "- Entidades nuevas: cubo/esfera/plano/luz con nombre=<id> y sus parámetros\n"
        "- Modificar existente: mover/rotar/escalar/color <id> con sus parámetros\n"
        "- Comportamientos: girar/oscilar <id>\n"
        "- Si el objeto ya existe en la escena, nunca lo repitas completo\n\n"
        "Escena actual:\n%s\n\n"
        "Usuario: %s\n\nPAED delta:",
        scene ? scene : "# vacía", user_prompt);

    free(scene);

    // Body JSON para Ollama
    cJSON *body = cJSON_CreateObject();
    cJSON_AddStringToObject(body, "model",  AI_MODEL);
    cJSON_AddStringToObject(body, "prompt", full_prompt);
    cJSON_AddFalseToObject(body,  "stream");
    char *body_str = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);

    CURL    *curl    = curl_easy_init();
    CurlBuf  resp   = { NULL, 0 };
    int      result = -1;

    if (!curl) { free(body_str); return -1; }


    // Esto no se para que es todavia
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL,           AI_ENDPOINT);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,    body_str);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,    headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &resp);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,       120L);

    CURLcode rc = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(body_str);

    if (rc != CURLE_OK) {
        fprintf(stderr, "[ai] curl: %s\n", curl_easy_strerror(rc));
        free(resp.data);
        return -1;
    }

    cJSON *json     = cJSON_Parse(resp.data);
    free(resp.data);

    if (!json) { fprintf(stderr, "[ai] JSON parse error\n"); return -1; }

    cJSON *response = cJSON_GetObjectItemCaseSensitive(json, "response");
    if (cJSON_IsString(response)) {
        strncpy(out, response->valuestring, (size_t)out_size - 1);
        out[out_size - 1] = '\0';
        result = 0;
    } else {
        fprintf(stderr, "[ai] campo 'response' no encontrado\n");
    }

    cJSON_Delete(json);
    return result;
}

// ── Plugin callbacks ──────────────────────────────────────────
static int ai_init(void) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    printf("[ai] iniciado — modelo: %s\n", AI_MODEL);
    return 0;
}

static void ai_shutdown(void) {
    curl_global_cleanup();
    printf("[ai] apagado\n");
}

static void ai_tick(float delta) { (void)delta; }

static void ai_on_event(Event *e) {
    if (e->type != EVENT_AI_REQUEST) return;

    AIRequest *req = (AIRequest *)e->data;
    printf("[ai] procesando: \"%s\"\n", req->prompt);

    static AIResponse resp;
    memset(&resp, 0, sizeof(resp));

    if (ollama_request(req->prompt, resp.paed_delta, AI_RESPONSE_MAX) != 0) {
        printf("[ai] error al contactar Ollama\n");
        return;
    }

    printf("[ai] PAED delta generado:\n--- PAED ---\n%s\n------------\n",
           resp.paed_delta);

    bus_send(EVENT_AI_RESPONSE, &resp, sizeof(resp));
}

Plugin ai_plugin = {
    .name      = "ai",
    .version   = "0.1",
    .init      = ai_init,
    .shutdown  = ai_shutdown,
    .tick      = ai_tick,
    .on_event  = ai_on_event,
};
