#include "ai.h"
#include "provider.h"
#include "../ide/parser.h"   // PAED_ESCENA_PATH: una sola ruta para toda la librería
#include "../../cjson/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
    size_t leidos = fread(buf, 1, (size_t)sz, f);
    buf[leidos] = '\0';
    fclose(f);
    return buf;
}

// ── Leer un archivo entero a memoria ──────────────────────────
static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    char *buf = malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t leidos = fread(buf, 1, (size_t)sz, f);
    buf[leidos] = '\0';
    fclose(f);
    return buf;
}

// ── El catalogo NO se escribe a mano: se deriva de escena.json ─
// Antes esta lista estaba hardcodeada en el prompt, y era una segunda fuente
// de verdad: el dia que escena.json cambiaba, el prompt mentia y el modelo
// generaba PAED que el parser rechazaba. Ahora leemos la MISMA definicion que
// usa el validador, asi que agregar un procedimiento a escena.json alcanza
// para que la IA lo conozca. Cero C que tocar.
static void catalogo_escena(char *out, size_t out_size) {
    out[0] = '\0';
    size_t usado = 0;

    char *raw = read_file(PAED_ESCENA_PATH);
    if (!raw) return;

    cJSON *json  = cJSON_Parse(raw);
    free(raw);
    if (!json) return;

    cJSON *procs = cJSON_GetObjectItemCaseSensitive(json, "procedimientos");
    cJSON *proc  = NULL;

    cJSON_ArrayForEach(proc, procs) {
        cJSON *nombre = cJSON_GetObjectItemCaseSensitive(proc, "nombre");
        cJSON *efecto = cJSON_GetObjectItemCaseSensitive(proc, "efecto");
        cJSON *params = cJSON_GetObjectItemCaseSensitive(proc, "params");
        if (!cJSON_IsString(nombre)) continue;

        char linea[512];
        int  n = snprintf(linea, sizeof(linea), "    %s(", nombre->valuestring);

        cJSON *par = NULL;
        int    primero = 1;
        cJSON_ArrayForEach(par, params) {
            cJSON *pn = cJSON_GetObjectItemCaseSensitive(par, "nombre");
            cJSON *pt = cJSON_GetObjectItemCaseSensitive(par, "tipo");
            cJSON *pr = cJSON_GetObjectItemCaseSensitive(par, "requerido");
            if (!cJSON_IsString(pn)) continue;

            n += snprintf(linea + n, sizeof(linea) - (size_t)n, "%s%s: %s%s",
                          primero ? "" : ", ",
                          pn->valuestring,
                          cJSON_IsString(pt) ? pt->valuestring : "?",
                          cJSON_IsTrue(pr) ? "*" : "");
            primero = 0;
            if (n >= (int)sizeof(linea) - 1) break;
        }
        n += snprintf(linea + n, sizeof(linea) - (size_t)n, ")   [%s]\n",
                      cJSON_IsString(efecto) ? efecto->valuestring : "");

        if (usado + (size_t)n >= out_size) break;
        memcpy(out + usado, linea, (size_t)n);
        usado += (size_t)n;
        out[usado] = '\0';
    }

    cJSON_Delete(json);
}

// ── El prompt: uno solo, igual para todos los proveedores ─────
// Las reglas de PAED no cambian segun quien las lea. Por eso esto vive fuera
// de los transportes: si mañana el prompt mejora, mejora para los cinco.
static void build_prompt(const char *user_prompt, char *out, size_t out_size) {
    char *scene = read_scene();

    static char catalogo[4096];
    catalogo_escena(catalogo, sizeof(catalogo));

    snprintf(out, out_size,
        "Sos un motor de escenas 3D. El usuario te pide cambios y vos respondés "
        "SOLO con instrucciones PAED válidas, sin explicaciones ni comentarios.\n\n"
        "Reglas del lenguaje PAED:\n"
        "- Una instrucción por línea, terminada en ';'\n"
        "- Palabras clave en MAYÚSCULAS, identificadores en minúsculas\n"
        "- Toda llamada es PROCEDIMIENTO(clave = valor, clave = valor);\n"
        "- NO existe la forma posicional: la entidad siempre se referencia con nombre = <id>\n"
        "- Los identificadores son ASCII: letras a-z, dígitos y '_'. Sin tildes ni ñ\n"
        "  (escribí 'aleron', no 'alerón'; 'tamano' como parámetro ya acepta 'tamaño')\n"
        "- Los vectores van entre paréntesis: posicion = (0,2,5)\n"
        "- NO escribas ACCION, AMBIENTE, PROCESO ni FIN_ACCION: solo las instrucciones\n"
        "- Si el objeto ya existe en la escena, nunca lo repitas completo\n\n"
        "ESTO ES LO IMPORTANTE — CONSTRUÍ, NO ELIJAS:\n"
        "Los procedimientos de abajo son LADRILLOS, no un catálogo de cosas que\n"
        "podés pedir. Si te piden una nave espacial, un árbol, un castillo o un\n"
        "perro, NO digas que no existe: armalo con varias piezas colocadas en el\n"
        "espacio, y ponelas todas en el mismo grupo = <id>. Todas las piezas de\n"
        "un grupo se mueven, rotan y escalan juntas como un solo objeto.\n"
        "Usá tantas piezas como haga falta para que se reconozca la forma.\n\n"
        "Procedimientos disponibles (* = obligatorio):\n%s\n"
        "Usá EXACTAMENTE esos parámetros: cada figura tiene los suyos y no se mezclan.\n\n"
        "Ejemplo — \"hacé una nave espacial\":\n"
        "CUBO(nombre = fuselaje, posicion = (0,0,0), color = #c0c0c0, tamano = (0.6,0.5,2.4), grupo = nave);\n"
        "CUBO(nombre = ala_izq, posicion = (-1.1,0,-0.3), color = #8b0000, tamano = (1.6,0.1,0.7), grupo = nave);\n"
        "CUBO(nombre = ala_der, posicion = (1.1,0,-0.3), color = #8b0000, tamano = (1.6,0.1,0.7), grupo = nave);\n"
        "ESFERA(nombre = cabina, posicion = (0,0.35,0.7), color = #4cc9f0, radio = 0.32, grupo = nave);\n"
        "CUBO(nombre = motor_izq, posicion = (-0.45,0,-1.3), color = #333333, tamano = (0.25,0.25,0.5), grupo = nave);\n"
        "CUBO(nombre = motor_der, posicion = (0.45,0,-1.3), color = #333333, tamano = (0.25,0.25,0.5), grupo = nave);\n\n"
        "Después, MOVER(nombre = nave, ...) mueve la nave entera.\n\n"
        "Escena actual:\n%s\n\n"
        "Usuario: %s\n\nPAED delta:",
        catalogo, scene ? scene : "# vacía", user_prompt);

    free(scene);
}

// ── Helper HTTP: POST de un JSON, devuelve el cuerpo crudo ────
// Devuelve malloc'd (el que llama lo libera) o NULL si fallo.
static char *http_post_json(const char *url, const char *body,
                            const char *bearer, long timeout_s) {
    CURL *curl = curl_easy_init();
    if (!curl) return NULL;

    CurlBuf resp = { NULL, 0 };

    // curl NO manda estas cabeceras solo: sin Content-Type, el servidor recibe
    // los bytes pero no sabe que son JSON y responde 400.
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    char auth[512];
    if (bearer) {
        snprintf(auth, sizeof(auth), "Authorization: Bearer %s", bearer);
        headers = curl_slist_append(headers, auth);
    }

    curl_easy_setopt(curl, CURLOPT_URL,           url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,    body);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,    headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &resp);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,       timeout_s);

    CURLcode rc = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK) {
        fprintf(stderr, "[ai] curl: %s\n", curl_easy_strerror(rc));
        free(resp.data);
        return NULL;
    }
    return resp.data;
}

// ── Transporte 1: Ollama (/api/generate) ──────────────────────
static int transport_ollama(const AIProvider *p, const char *prompt,
                            char *out, int out_size) {
    cJSON *body = cJSON_CreateObject();
    cJSON_AddStringToObject(body, "model",  p->model);
    cJSON_AddStringToObject(body, "prompt", prompt);
    cJSON_AddFalseToObject(body,  "stream");
    char *body_str = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);

    char *raw = http_post_json(p->endpoint, body_str, NULL, 120L);
    free(body_str);
    if (!raw) return -1;

    cJSON *json = cJSON_Parse(raw);
    free(raw);
    if (!json) { fprintf(stderr, "[ai] JSON parse error\n"); return -1; }

    int     result   = -1;
    cJSON  *response = cJSON_GetObjectItemCaseSensitive(json, "response");
    if (cJSON_IsString(response)) {
        snprintf(out, (size_t)out_size, "%s", response->valuestring);
        result = 0;
    } else {
        fprintf(stderr, "[ai] campo 'response' no encontrado\n");
    }

    cJSON_Delete(json);
    return result;
}

// ── Transporte 2: API estilo OpenAI (Kimi, Qwen, etc.) ────────
// Mismo prompt, otra forma: en vez de un campo "prompt" plano, va una lista de
// mensajes con rol, y la respuesta viene anidada en choices[0].message.content.
static int transport_openai(const AIProvider *p, const char *prompt,
                            char *out, int out_size) {
    const char *key = p->api_key_env ? getenv(p->api_key_env) : NULL;
    if (p->api_key_env && !key) {
        fprintf(stderr, "[ai] falta la variable de entorno %s\n", p->api_key_env);
        return -1;
    }

    cJSON *msg = cJSON_CreateObject();
    cJSON_AddStringToObject(msg, "role",    "user");
    cJSON_AddStringToObject(msg, "content", prompt);

    cJSON *messages = cJSON_CreateArray();
    cJSON_AddItemToArray(messages, msg);

    cJSON *body = cJSON_CreateObject();
    cJSON_AddStringToObject(body, "model", p->model);
    cJSON_AddItemToObject(body,   "messages", messages);
    cJSON_AddBoolToObject(body,   "stream", 0);

    char *body_str = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);

    char *raw = http_post_json(p->endpoint, body_str, key, 120L);
    free(body_str);
    if (!raw) return -1;

    cJSON *json = cJSON_Parse(raw);
    free(raw);
    if (!json) { fprintf(stderr, "[ai] JSON parse error\n"); return -1; }

    int    result  = -1;
    cJSON *choices = cJSON_GetObjectItemCaseSensitive(json, "choices");
    cJSON *first   = cJSON_IsArray(choices) ? cJSON_GetArrayItem(choices, 0) : NULL;
    cJSON *message = first ? cJSON_GetObjectItemCaseSensitive(first, "message") : NULL;
    cJSON *content = message ? cJSON_GetObjectItemCaseSensitive(message, "content") : NULL;

    if (cJSON_IsString(content)) {
        snprintf(out, (size_t)out_size, "%s", content->valuestring);
        result = 0;
    } else {
        // Si la API devolvio un error, viene en .error.message y decirlo ayuda
        // mucho mas que un "no encontrado" generico.
        cJSON *err = cJSON_GetObjectItemCaseSensitive(json, "error");
        cJSON *m   = err ? cJSON_GetObjectItemCaseSensitive(err, "message") : NULL;
        fprintf(stderr, "[ai] respuesta sin contenido%s%s\n",
                cJSON_IsString(m) ? ": " : "",
                cJSON_IsString(m) ? m->valuestring : "");
    }

    cJSON_Delete(json);
    return result;
}

// ── Transporte 3: CLI local (Claude Code) ─────────────────────
// Claude Code no es una API HTTP: es un binario. Le pasamos el prompt por
// STDIN, no como argumento. Eso no es un detalle de estilo: si el prompt fuera
// parte de la linea de comandos, cualquier comilla o ';' que escriba el usuario
// terminaria siendo interpretado por la shell. Por stdin, el texto es solo texto.
static int transport_cli(const AIProvider *p, const char *prompt,
                         char *out, int out_size) {
    char plantilla[] = "/tmp/vimmon_prompt_XXXXXX";
    int  fd = mkstemp(plantilla);   // crea el archivo con nombre unico y seguro
    if (fd < 0) { perror("[ai] mkstemp"); return -1; }

    FILE *tmp = fdopen(fd, "w");
    if (!tmp) { perror("[ai] fdopen"); close(fd); unlink(plantilla); return -1; }
    fputs(prompt, tmp);
    fclose(tmp);

    // El unico texto que entra al comando es el nombre del temporal, que lo
    // generamos nosotros. El prompt del usuario nunca toca la shell.
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "%s -p < %s", p->endpoint, plantilla);

    FILE *pipe = popen(cmd, "r");
    if (!pipe) { perror("[ai] popen"); unlink(plantilla); return -1; }

    int  usado = 0;
    char linea[1024];
    while (fgets(linea, sizeof(linea), pipe)) {
        int libre = out_size - 1 - usado;
        if (libre <= 0) break;
        int n = snprintf(out + usado, (size_t)libre, "%s", linea);
        usado += (n < libre) ? n : libre;
    }
    out[usado] = '\0';

    int estado = pclose(pipe);
    unlink(plantilla);

    if (estado != 0) {
        fprintf(stderr, "[ai] '%s' termino con estado %d\n", p->endpoint, estado);
        return -1;
    }
    if (usado == 0) {
        fprintf(stderr, "[ai] '%s' no devolvio nada\n", p->endpoint);
        return -1;
    }
    return 0;
}

// ── Despacho: arma el prompt y elige el transporte ────────────
static int ai_generate(const char *user_prompt, char *out, int out_size) {
    const AIProvider *p = ai_provider_active();

    static char full_prompt[8192];
    build_prompt(user_prompt, full_prompt, sizeof(full_prompt));

    switch (p->transport) {
        case TRANSPORT_OLLAMA: return transport_ollama(p, full_prompt, out, out_size);
        case TRANSPORT_OPENAI: return transport_openai(p, full_prompt, out, out_size);
        case TRANSPORT_CLI:    return transport_cli(p, full_prompt, out, out_size);
    }
    return -1;
}

// ── Plugin callbacks ──────────────────────────────────────────
static int ai_init(void) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    const AIProvider *p = ai_provider_active();
    printf("[ai] iniciado — proveedor: %s (%s)  'ai' para ver el menu\n",
           p->label, ai_transport_name(p->transport));
    return 0;
}

static void ai_shutdown(void) {
    curl_global_cleanup();
    printf("[ai] apagado\n");
}

static void ai_tick(float delta) { (void)delta; }

static void ai_on_event(Event *e) {
    if (e->type != EVENT_AI_REQUEST) return;

    AIRequest        *req = (AIRequest *)e->data;
    const AIProvider *p   = ai_provider_active();

    printf("[ai] %s <- \"%s\"\n", p->id, req->prompt);

    static AIResponse resp;
    memset(&resp, 0, sizeof(resp));

    if (ai_generate(req->prompt, resp.paed_delta, AI_RESPONSE_MAX) != 0) {
        printf("[ai] error con el proveedor '%s'\n", p->id);
        return;
    }

    printf("[ai] PAED delta generado:\n--- PAED ---\n%s\n------------\n",
           resp.paed_delta);

    bus_send(EVENT_AI_RESPONSE, &resp, sizeof(resp));
}

Plugin ai_plugin = {
    .name      = "ai",
    .version   = "0.2",
    .init      = ai_init,
    .shutdown  = ai_shutdown,
    .tick      = ai_tick,
    .on_event  = ai_on_event,
};
