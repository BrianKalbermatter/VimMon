#include "../../bus/plugin.h"
#include <stdio.h>

static int  monitor_init(void)          { printf("[monitor] iniciado\n"); return 0; }
static void monitor_shutdown(void)      { printf("[monitor] apagado\n"); }
static void monitor_tick(float delta)   { (void)delta; }
static void monitor_on_event(Event *e)  { (void)e; }

Plugin monitor_plugin = {
    .name      = "monitor",
    .version   = "0.1",
    .init      = monitor_init,
    .shutdown  = monitor_shutdown,
    .tick      = monitor_tick,
    .on_event  = monitor_on_event,
};
