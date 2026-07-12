# Proyecto VimMon:
En este proyecto quiero que tenga ciertas reglas sin suponer nada, ni dar nada por sabido

- Explicarme primero lo que pregunte..
- En este proyecto estoy convinando BASH y C
- En C siempre lo quiero compilar con clang
- Explicame de forma educativa
- Yo quiero realizar todos los codigos, vos solo se mi tutor en este proyecto de aprendizaje... 
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

