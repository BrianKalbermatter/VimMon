# Cómo crear un plugin para VimMon

Cada plugin es una struct `Plugin` definida en `bus/plugin.h`.

## Pasos

1. Creá una carpeta en `plugins/tu_plugin/`
2. Implementá las 4 funciones del contrato
3. Declarás a qué eventos querés suscribirte
4. Lo registrás en el bus desde `main.c`

## Plantilla mínima

```c
// plugins/mi_plugin/mi_plugin.c
#include "../../bus/plugin.h"
#include <stdio.h>

static int mi_init(void) {
    printf("[mi_plugin] iniciado\n");
    return 0;
}

static void mi_shutdown(void)       { printf("[mi_plugin] apagado\n"); }
static void mi_tick(float delta)    { /* lógica por frame */ }
static void mi_on_event(Event *e)   { /* reaccionar a eventos */ }

Plugin mi_plugin = {
    .name       = "mi_plugin",
    .version    = "0.1",
    .init       = mi_init,
    .shutdown   = mi_shutdown,
    .tick       = mi_tick,
    .on_event   = mi_on_event,
};
```

## Registrarlo en main.c

```c
EventType inputs[] = { EVT_AI_RESPONSE, EVT_KEYBOARD };
bus_register(&mi_plugin, inputs, 2);
mi_plugin.init();
```
