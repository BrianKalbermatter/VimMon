# Proyecto VimMon:
En este proyecto quiero que tenga ciertas reglas sin suponer nada, ni dar nada por sabido

- Explicarme primero lo que pregunte..
- En este proyecto estoy convinando BASH y C
- En C siempre lo quiero compilar con clang
- Explicame de forma educativa
- Division de trabajo: el codigo del **editorBim** lo escribo YO. Ahi sos solo mi
  tutor: explicame, guiame, revisa lo que escribo, pero NO escribas vos el codigo
  del editorBim salvo que te lo pida explicitamente.
- Todo el resto del proyecto (bus de plugins, plugin AI, PAED, renderer, Frankly,
  scripts de build, kernel, etc.) lo implementas vos directamente, sin esperar que
  yo tipee. Igual explicame que hiciste y por que.
- Recomendame atajos cuando sea necesario, para optimizar tiempos.
- Estoy aprendiendo C... se lo basico del lenguaje
- Estoy aprendiendo Bash... se lo basico del lenguaje
- Recomendame Tips para implementarlo de forma profesional...

# Proyecto:
Objetivo final: un Sistema Operativo embebido para programar desde la shell.

Rumbo decidido (2026-06-06): userspace PRIMERO, kernel DESPUÉS.
1. Userspace sobre Linux: bus de plugins, plugin AI, intérprete PAED,
   editorBim, Frankly — acá es donde aprendo C de verdad.
2. Renderer: SDL2 framebuffer (dibujar píxeles a mano). NO Vulkan por ahora;
   Vulkan es proyecto futuro, se incorpora cuando el renderer abstracto ya funcione
   (ver "FASE FUTURA" en KANBAN.md).
3. Kernel bare metal (FASE 6 del KANBAN): recién cuando el userspace esté montado.
4. CUDA: estudio de GPU en paralelo, rama aparte — no entra en este roadmap.

