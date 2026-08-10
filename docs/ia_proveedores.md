# Proveedores de IA

El plugin `ai` le pide a un modelo que devuelva PAED. Qué modelo, y cómo se le
habla, se elige **en runtime**: no hay que editar un header ni recompilar.

## Usarlo

```
vimmon> ai                       menú de proveedores
vimmon> 3                        elegir por número
vimmon> ai use claude            elegir por nombre
vimmon> ai hacé una nave espacial   mandar un prompt al activo
```

```
Proveedores de IA:
  1) llama     Llama 3.2 3B         [ollama]  NO (Ollama no responde en :11434)   <- activo
  2) qwen      Qwen2.5 Coder 7B     [ollama]  NO (Ollama no responde en :11434)
  3) claude    Claude Code          [cli]     listo
  4) kimi      Kimi (Moonshot)      [http]    NO (MOONSHOT_API_KEY)
  5) qwen-api  Qwen Max (DashScope) [http]    NO (DASHSCOPE_API_KEY)
```

El menú no adivina: consulta disponibilidad de verdad (ver más abajo).

## La idea: lo que cambia es el transporte

El **prompt es siempre el mismo** — las reglas de PAED no cambian según quién las
lea. Lo que cambia entre Claude, Kimi y Qwen es *cómo* se le habla: qué URL, qué
forma tiene el JSON y en qué campo viene la respuesta.

| Transporte | Cómo se pide | Dónde viene la respuesta |
|---|---|---|
| `TRANSPORT_OLLAMA` | `POST /api/generate` a localhost | `.response` |
| `TRANSPORT_OPENAI` | `POST /v1/chat/completions` + `Bearer` | `.choices[0].message.content` |
| `TRANSPORT_CLI`    | proceso local, prompt por stdin | stdout |

## Agregar un modelo

Una fila en la tabla de `plugins/ai/provider.c`. Nada más.

```c
{ "deepseek", "DeepSeek Chat", TRANSPORT_OPENAI,
  "https://api.deepseek.com/v1/chat/completions", "deepseek-chat",
  "DEEPSEEK_API_KEY" },
```

Cualquier modelo que hable el protocolo de OpenAI entra sin escribir código
nuevo. Los campos son: id (lo que tipeás), etiqueta del menú, transporte,
endpoint (URL o nombre del binario), modelo, y variable de entorno con la clave
(`NULL` si no necesita).

## Disponibilidad

`ai_provider_ready()` comprueba, no supone:

- **Ollama**: `GET localhost:11434/api/tags` con timeout de 800 ms. Corto a
  propósito: esto corre al listar el menú y no queremos que la consola se cuelgue.
- **HTTP**: que exista la variable de entorno con la API key.
- **CLI**: que el binario esté en el `PATH` (se recorre a mano con `access(X_OK)`,
  sin lanzar un proceso solo para preguntar esto).

## El prompt sale de escena.json

El catálogo de procedimientos **no está escrito en C**: `catalogo_escena()` lee
`paed/Frankly/data/escena.json`, la misma definición que usa el validador del
parser.

Antes estaba hardcodeado en el prompt y era una segunda fuente de verdad: cuando
divergieron, el modelo generó `ESFERA(tamano = ...)` y el parser lo rechazó
porque `ESFERA` usa `radio`. **Agregar un procedimiento a `escena.json` alcanza
para que la IA lo conozca.**

El prompt además está escrito para que **componga, no elija**: los procedimientos
son ladrillos. Si le pedís una nave espacial, la arma con varias piezas en un
mismo `grupo` (ver `docs/escena_paed.md`).

## Seguridad en el transporte CLI

El prompt viaja por **stdin**, nunca como argumento de la shell:

```c
int fd = mkstemp(plantilla);        // nombre único, generado por nosotros
// ... se escribe el prompt en el archivo ...
snprintf(cmd, sizeof(cmd), "%s -p < %s", p->endpoint, plantilla);
```

Si el prompt fuera parte de la línea de comandos, una comilla o un `;` que
escribas los interpretaría la shell. Lo único que entra al comando es el nombre
del temporal, que lo generamos nosotros.
