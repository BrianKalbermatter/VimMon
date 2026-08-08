#ifndef VIMMON_AI_PROVIDER_H
#define VIMMON_AI_PROVIDER_H

// Selector de proveedor de IA.
//
// La idea clave: el PROMPT es siempre el mismo (las reglas de PAED no cambian
// segun quien las lea). Lo unico que cambia entre Claude, Kimi y Qwen es COMO
// se le habla al modelo: que URL, que forma tiene el JSON, y en que campo viene
// la respuesta. Eso es el TRANSPORTE, y es lo unico que abstraemos aca.
//
// Agregar un modelo nuevo = una linea en la tabla de provider.c. Nada mas.

typedef enum {
    TRANSPORT_OLLAMA,  // HTTP local: POST /api/generate, respuesta en .response
    TRANSPORT_OPENAI,  // HTTP remoto: POST /v1/chat/completions + Bearer token,
                       // respuesta en .choices[0].message.content
    TRANSPORT_CLI,     // proceso local (Claude Code): prompt por stdin, respuesta
                       // por stdout. Sin red, sin API key.
} AITransport;

typedef struct {
    const char *id;           // como se escribe en la consola: "ai use qwen"
    const char *label;        // texto lindo para el menu
    AITransport transport;
    const char *endpoint;     // URL (HTTP) o nombre del binario (CLI)
    const char *model;        // id del modelo; "" si lo decide el CLI
    const char *api_key_env;  // variable de entorno con la clave; NULL si no hace falta
} AIProvider;

extern const AIProvider ai_providers[];
extern const int        ai_provider_count;

// Proveedor activo. Nunca devuelve NULL: arranca en el primero de la tabla.
const AIProvider *ai_provider_active(void);

// Cambia el activo por id ("qwen") o por numero de menu ("2").
// Devuelve 0 si lo encontro, -1 si no.
int ai_provider_use(const char *id_or_index);

// Imprime el menu marcando el activo y si cada uno esta listo para usarse.
void ai_provider_list(void);

// "listo" = tiene lo que necesita para funcionar ahora mismo:
//   OLLAMA -> el servidor local responde
//   OPENAI -> la variable de entorno con la API key existe
//   CLI    -> el binario esta en el PATH
// Devuelve 1 si esta listo, 0 si no. En motivo (si no es NULL) deja el porque.
int ai_provider_ready(const AIProvider *p, const char **motivo);

const char *ai_transport_name(AITransport t);

#endif // VIMMON_AI_PROVIDER_H
