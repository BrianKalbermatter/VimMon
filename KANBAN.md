---

kanban-plugin: board

---

## Backlog

- [ ] Subir `PAED_VAL_MAX` (128 bytes) o hacer que el texto de `ESCRIBIR` no tenga ese tope: hoy un marco decorativo Unicode de 40 simbolos ya no entra #fase2
- [ ] Parser PAED: una instrucción no puede partirse en dos líneas — el parser lee línea por línea. Un `ESCRIBIR` largo hay que dejarlo en una sola #fase2
- [ ] Parser PAED: `SEGUN` — CONFLICTO a decidir antes de implementar #fase2
- [ ] Parser PAED: `REPETIR`/`HASTA` — cero apariciones reales en el corpus, solo declaradas en `sintaxis.json`. Confirmar que existan antes de implementarlas #fase2
- [ ] Evaluador: guardar un árbol de la expresión en vez de re-parsear el texto en CADA vuelta del bucle. Hoy es simple y correcto, pero un `MIENTRAS` largo paga el costo en cada iteración #fase2
- [ ] Evaluador: usar el `AMBIENTE` para chequear tipos. Hoy se parsea pero no se usa: asignarle un texto a algo declarado `ENTERO` no da error #fase2
- [ ] Evaluador: los arreglos no chequean el TIPO declarado. `A: ARREGLO[1..5] DE ENTERO` acepta que le metan un texto en `A[2]`, igual que pasa con los escalares #fase2
- [ ] Evaluador: `NFDS`/`FDS` necesitan SECUENCIAS, que el intérprete no tiene. Hoy avisan en vez de inventar un valor #fase2
- [ ] Avisar cuando se usa `==`: ya está resuelto que NO existe en AED (`TEORIA_COMPLETA.txt:324` define `=`, y la wiki lo marca como error de escritura arrastrado, 91 usos). Hoy se acepta callado para no romper los archivos; debería avisar sin frenar la ejecución #fase2
- [ ] Confirmar contra la cátedra: `-2 ** 2` da 4 porque la tabla de prioridad pone los unarios ARRIBA de la potencia. En casi todos los lenguajes da -4. ¿Es lo que quiere AED? #fase2
- [ ] Parser PAED: `FUNCION`/`PROCEDIMIENTO` anidados dentro de `AMBIENTE` #fase2
- [ ] Parser PAED: nombre de `ACCION` con espacios (`ACCION Ejercicio de Parcial ES`) #fase2
- [ ] Parser PAED: declaración múltiple `A,B,SUMA: entero`. Es la forma del único ejemplo con autoridad de cátedra (`AED_2021_UnI.pdf:10`) y hoy da "nombre de variable invalido" #fase2
- [ ] Parser PAED: `VARIABLES` como sub-sección de `AMBIENTE`. Aparece en `AED_2021_UnI.pdf:10` y en ninguna otra fuente. Antes de implementar, confirmar si es obligatoria #fase2
- [ ] Decidir el `;`: la cátedra lo usa como SEPARADOR (la última sentencia no lo lleva) y el parser lo exige como terminador. Cambiarlo rompe todos los `.paed` del repo — decidir antes de tocar #fase2
- [ ] `escena.json` todavia vive en `paed/Frankly/data/`, que es el repo de PAED, cuando la escena 3D es de VimMon. Anda porque `paed_syntax_load_lib` busca en el directorio de datos de PAED, pero la libreria de un host deberia poder vivir del lado del host #fase2
- [ ] Dos copias de cJSON en el arbol: `cjson/` (VimMon, para el plugin de IA) y `paed/lang/vendor/cjson/` (PAED). Linkea bien porque el archivo estatico solo aporta lo que falta, pero depende del ORDEN en el comando de link. Unificar cuando PAED exponga su cJSON o cuando VimMon deje de necesitarlo #fase2
- [ ] `LEER` en la ventana SDL: hoy solo `paedrun` engancha una entrada (`interp_set_entrada`), asi que un `LEER` dentro del renderer avisa que no tiene datos. Necesita una cola alimentada por el plugin de input, NO un `fgets`: bloquear ahi congela el game loop #fase3
- [ ] Interprete PAED: `ARR`/`AVZ`/`CREAR`/`CERRAR` (hoy parsean pero no ejecutan). Van despues del tipo `SECUENCIA`, porque sin secuencias no tienen sobre que operar #fase2
- [ ] **`HV` (High Value) — constante del lenguaje, no una variable.** El usuario escribe `HV` y vale un numero mas grande que cualquier clave posible. Hoy no existe: `SI (x <> HV)` da "la variable 'HV' no tiene valor todavia". No esta en `sintaxis.json` ni en `expr.c` #fase2
      **Para que sirve** (`wiki.txt:2399-2450`, `OnlySintaxis.md:248-296`): es el centinela de la MEZCLA INCLUYENTE de archivos. Cuando un archivo se agota, su clave pasa a `HV`; como `HV` es mas grande que cualquier clave real, ese archivo pierde siempre la comparacion `reg1.clave <= reg2.clave` y el ciclo sigue vaciando el otro solo. Sin `HV` hacen falta TRES ciclos (uno principal y dos residuales); con `HV`, uno solo (`wiki.txt:2447`).
      **Como implementarlo**: junto a `V`/`F` en `primario()` de `expr.c`, que ya resuelve los literales logicos sin pasar por el entorno. Ese es el lugar: `HV` no se declara, no se asigna y no ocupa una entrada de variable. Agregarlo tambien a `sintaxis.json` (categoria propia o junto a `booleanos`) para que el resaltador lo pinte como constante y no como variable.
      **A decidir**: que valor exacto. `DBL_MAX` lo hace incomparable de verdad pero se imprime feo; un `999999999` es legible y alcanza para las claves de los ejercicios. Y si `HV` distingue mayusculas (`hv`, `Hv`) — las palabras clave no distinguen, y esto es una constante del lenguaje, asi que deberia seguir la misma regla #fase2
- [ ] `LV` (Low Value): CERO apariciones en wiki, OnlySintaxis y TEORIA_COMPLETA. No inventarlo: si algun dia aparece en un parcial, ahi se agrega #fase2
- [ ] Archivos en disco de verdad: `ABRIR`/`LEER`/`ESCRIBIR`/`CERRAR` con handle, modo (lectura/escritura/actualizacion) y flag de fin. Hoy la forma se DISTINGUE y se valida, pero ninguna toca el disco #fase2
- [ ] Decidir la sintaxis de `ABRIR`: la wiki escribe `ABRIR(arch, lectura)` y la catedra `Abrir E/(arch)` (`TEORIA_COMPLETA.txt:1104`). Son incompatibles entre si #fase2
- [ ] El TERCER `LEER`: acceso indexado por clave (`reg.clave := x; LEER(arch, reg)`, `wiki.txt:2764`) NO avanza, busca. Hoy se marca igual que el secuencial; distinguirlos necesita saber si el archivo es indexado #fase2
- [ ] `GRABAR`/`REGRABAR`/`BORRAR` de archivos indexados (`TEORIA_COMPLETA.txt:1112-1114`) #fase2
- [ ] La rama de `ARREGLO` en `parse_decl` no exige espacio despues de la palabra, asi que un tipo llamado `ARREGLOS` entraria por ahi. La de `ARCHIVO` si lo exige #fase2
- [ ] Corroborar contra la wiki: `RETORNAR`, `TRUNC`, `ABSO`, `REDOND` — 0 apariciones en los apuntes #fase2
- [ ] Conectar EVENT_KEYBOARD/EVENT_MOUSE a un consumidor real (hoy solo hay debug prints en `plugins/input/input.c`) #fase3
- [ ] Recortar contra el plano cercano: hoy `scene_view.c` acota las coordenadas de las esquinas en vez de generar vértices en el borde, y un objeto que abraza la cámara (el suelo) queda mal #fase3
- [ ] Z-buffer por píxel: hoy el orden del pintor usa la profundidad del CENTRO, así que dos cuerpos que se cruzan se ordenan mal #fase3
- [ ] Caras del cubo con sombreado: hoy se rellena el rectángulo que contiene las 8 esquinas, no las 6 caras #fase3
- [ ] `LUZ` que ilumine de verdad: hoy solo se dibuja una cruz donde está #fase3
- [ ] Split screen: panel izquierdo (AI chat) + panel derecho (viewport) #fase3
- [ ] `plugins/raycaster/map.c` — mapa como grilla 2D hardcodeada #fase3b (no arrancar hasta tener renderer.h + put_pixel de FASE 3)
- [ ] `plugins/raycaster/ray.c` — DDA: un rayo por columna, paredes sólidas de color #fase3b
- [ ] Input por el bus: mover/rotar jugador + colisión contra la grilla #fase3b
- [ ] Texturas en paredes (BMP con SDL o stb_image) #fase3b
- [ ] Z-buffer por columna + sprites billboard (enemigo 2D estilo boomer shooter) #fase3b
- [ ] Test: caminar un mapa con un enemigo sprite que te mira #fase3b
- [ ] `plugins/monitor/monitor.c` — leer /proc/meminfo y /proc/stat #fase4
- [ ] Mostrar RAM, CPU, plugins activos en un panel #fase4
- [ ] Registrar en el bus (EVT_MONITOR_TICK) #fase4
- [ ] Mover/adaptar `paed/` como plugin en `plugins/ide/` #fase5
- [ ] Conectar al bus: teclado, render, AI #fase5
- [ ] Editor PAED integrado al split screen (hoy `edit` abre PseudoGames en su propia ventana; falta el split dentro de VimMon) #fase5
- [ ] `kernel/boot/entry.asm` — Multiboot2 + request framebuffer #fase6
- [ ] `kernel/drivers/fb.c` — framebuffer VESA #fase6
- [ ] `kernel/drivers/font.c` — PSF font embebida #fase6
- [ ] `kernel/kernel/main.c` — kmain() #fase6
- [ ] `kernel/arch/x86_64/gdt.c` — GDT #fase6
- [ ] `kernel/arch/x86_64/idt.c` — IDT #fase6
- [ ] `kernel/mm/pmm.c` — bitmap allocator #fase6
- [ ] `kernel/mm/heap.c` — kmalloc/kfree #fase6
- [ ] `kernel/drivers/keyboard.c` — PS/2 IRQ1 #fase6
- [ ] `kernel/drivers/timer.c` — PIT 100Hz #fase6
- [ ] Boot en QEMU: VimMon arranca sin Linux #fase6
- [ ] Instalar vulkan-headers + vulkan-validation-layers + vulkan-dzn #futura (no arrancar hasta cerrar FASE 3 — mismo `renderer.h`, el framebuffer SDL2 queda como fallback)
- [ ] `plugins/renderer/vulkan_init.c` — instancia + device + swap chain #futura
- [ ] `plugins/renderer/vulkan_backend.c` — dibujar triángulo (hola Vulkan) #futura
- [ ] `plugins/renderer/vulkan_backend.c` — dibujar cubo desde PAED #futura
- [ ] Switch de backend en runtime: framebuffer ↔ Vulkan #futura


## En progreso

- [ ] Aprendiendo C
- [ ] Crear el primer juego en `game/game.c` (usando el motor 2D)


## Hecho

- [x] **PAED se instala sin compilar: release `v0.1.0` con el binario armado.** `curl -L .../releases/latest/download/paed-linux-x86_64.tar.gz | tar xz` y `./instalar.sh`. 41 KB comprimido, 120 KB descomprimido. Verificado bajandolo con curl como un desconocido, instalandolo en un prefijo ajeno y corriendo un programa con `LEER` desde `/tmp` #fase2
- [x] El binario es REUBICABLE: `paed_datadir()` pregunta "¿donde estoy?" con `/proc/self/exe` y mira `<ahi>/../share/paed`. Sin eso, un binario compilado para `/usr/local` y descomprimido en `~/.local` busca sus datos donde no estan — que es el mismo bug del `PREFIX` pero del lado del que lo baja #fase2
- [x] El workflow corre los 19 tests ANTES de publicar y ademas descomprime el paquete en otra carpeta y lo ejecuta desde ahi. Un release roto es peor que ninguno: el que lo baja arranca creyendo que el problema es suyo #fase2
- [x] Bug del workflow: fallaba en 0 segundos porque el heredoc de las notas empezaba en columna 0 y eso CIERRA el bloque `run: |` de YAML. No hay forma de tenerlo adentro — indentado se le cuela la indentacion al texto publicado, sin indentar rompe el archivo. Las notas se fueron a `.github/notas-release.md` con `--notes-file` #fase2
- [x] `AlgebraRectas/` salio del repo del lenguaje a `programas/` de VimMon: es un programa ESCRITO EN PAED, no parte de PAED. Un lenguaje no lleva adentro los programas de sus usuarios #fase2
- [x] Un commit que mezclaba el movimiento de AlgebraRectas con toda la maquina de release se partio en dos antes de que quedara asi para siempre. El mensaje hablaba solo del movimiento y escondia 198 lineas de otra cosa #fase2
- [x] **`ParcialSimulado_01.paed` pide los datos con `LEER`.** Era la razon de ser de todo lo de hoy: un parcial se resuelve como en la catedra, cargando los datos, en vez de editar el archivo y volver a correrlo #fase2
- [x] El ejercicio 5 recupero su enunciado ORIGINAL: carga notas hasta el centinela `-1`, sin saber cuantas son. Con datos fijos era imposible y estaba reemplazado por un arreglo de tamanio conocido con un `PARA`. Ahora es un `MIENTRAS` con el patron de siempre — leer antes de entrar, y que la ultima instruccion del ciclo lea el siguiente #fase2
- [x] Dos guardas que el enunciado no pide pero el algoritmo si: `Y (cant < 12)` en el ciclo (el arreglo llega hasta 12) y `SI (cant > 0)` antes del promedio (dividir por cant sin preguntar es dividir por cero si nadie cargo notas) #fase2
- [x] El bloque `ENTRADA` con los datos de ejemplo va al final del `.paed`, con el comando para mandarlos por stdin. El patron del `sed` va ANCLADO al principio de linea: sin eso matchea la propia linea del comentario que lo explica, y el primer dato que entra es texto de la documentacion #fase2
- [x] **Los tres repos publicados y enganchados como submodulos**: `paed` y `PseudoGames` viven adentro de VimMon pero los versiona su propio git. VimMon guarda UNA entrada por cada uno (`mode=160000`, un puntero al commit), no sus archivos. Verificado con `git clone --recurse-submodules` limpio: llegan solos, compilan y pasan 19/19 #fase5
- [x] **Trampa del submodulo: `git submodule update` deja el repo en HEAD DESACOPLADO.** Commitear ahi deja el commit sin rama y a un `gc` de perderse. Hay que entrar y hacer `git checkout master` antes de trabajar — o verificar con `git -C paed branch --show-current`, que en desacoplado responde vacio #fase5
- [x] Trabajar en un submodulo son DOS commits: uno adentro (el cambio) y otro en VimMon (mover el puntero). Si falta el segundo, el que clone VimMon se lleva la version vieja aunque el otro repo este al dia #fase5
- [x] El verificador de soluciones del editor llamaba a `./Frankly/paed` (el interprete de bash, por ruta relativa) y quedo apuntando a la nada con la separacion. Ahora llama a `paed` por PATH — el de C, el que se sigue desarrollando #fase5
- [x] Al verificador se le agrego `< /dev/null`: el interprete en C ejecuta `LEER` de verdad, y `popen` le pasa al hijo el stdin del EDITOR. Una solucion con `LEER` habria dejado al interprete esperando que alguien tipee en el stdin de una ventana SDL que nadie mira — colgado y sin decir por que. Con la redireccion falla en 5ms con "la entrada se termino", que es lo correcto #fase5
- [x] **TRES repos, no uno.** `paed` es el lenguaje, `PseudoGames` es el editor y VimMon es el OS. La dependencia va en UNA sola direccion: VimMon lanza el editor, el editor usa el lenguaje, y el lenguaje no sabe que existe ninguno de los dos. Quien quiera solo PAED ya no se lleva assets, niveles y SDL2 de arrastre #fase5
- [x] El editor es una APP del OS y no una parte de el: `plugins/editor/editor.c` lanza `PseudoGames/aed` por defecto pero acepta `VIMMON_IDE=/usr/bin/vim`. Cambiar de editor no toca una linea de VimMon #fase5
- [x] El workflow que arma el `.exe` para Windows colgaba de la raiz de VimMon aunque empaqueta el EDITOR. Se fue con el, a `PseudoGames/.github/workflows/` #fase5
- [x] Auditado contra `origin/aed_pseudo` (la rama del editor, del 2026-05-25) que no se perdiera nada: los 4 archivos de `C/` ya viven en `src/C/` de VimMon, `Frankly/tests/secuencia.sh` estaba VACIO, y el workflow de Windows se recupero. Nada mas quedaba solo en esa rama #fase5
- [x] **PAED es un proyecto aparte.** Vive en `paed/`, tiene su propio git, su propio Makefile y su propio `make install`. VimMon lo consume como libreria (`-Ipaed/lang/include` + `paed/build/libpaed.a`), que es una relacion de UNA sola direccion: PAED no sabe que VimMon existe #fase2
- [x] Lo que hacia imposible instalarlo era que la definicion del lenguaje estaba clavada a `paed/Frankly/data/sintaxis.json`, una ruta del repo donde nacio. Ahora `paed_datadir()` la BUSCA en runtime — `$PAED_HOME`, la ruta de instalacion compilada adentro del binario, y despues el repo — igual que Python con `PYTHONHOME` #fase2
- [x] `escena.json` dejo de cargarse solo por una ruta fija: es una libreria de VimMon, asi que la pide VimMon con `paed_syntax_load_lib("escena")`. El binario `paed` acepta `--lib` para VALIDAR programas que la usan (ejecutarlos necesita el host que implemente esos procedimientos) #fase2
- [x] Los mensajes de error nombran ARCHIVOS y no rutas (`no esta ni en sintaxis.json ni en escena.json`): la ruta depende de donde se instalo, y el mismo programa no puede dar mensajes distintos en dos maquinas #fase2
- [x] Bug encontrado al probar la instalacion de verdad: `make install PREFIX=~/.local` copiaba un binario que seguia buscando en `/usr/local/share/paed`. Los `.o` ya estaban compilados con el `DATADIR` viejo y make no puede notarlo solo — mira fechas de ARCHIVOS, y una flag de linea de comandos no es un archivo. Se resolvio con un sello que guarda la ruta y del que dependen los objetos #fase2
- [x] Verificado instalando en `~/.local` y corriendo un `.paed` con `LEER` parado en `/tmp`, sin el repo a la vista #fase2
- [x] **La escena 3D salio del interprete, ahora en el CODIGO y no solo en el papel.** `escena.json` decia desde el dia uno "NO es parte del lenguaje PAED", pero `CUBO`, `MOVER`, `GIRAR` y otros nueve vivian adentro de `interpreter.c` y `interp_exec` recibia el `SceneState`. Un lenguaje que se quiere instalar aparte no puede traer cubos adentro #fase2
- [x] `paed_register_proc(nombre, fn, ud)` — el host ANOTA sus procedimientos y el interprete no los conoce. Misma idea que `bus_register` y que `interp_set_entrada`: el nucleo no conoce a sus extensiones. Los 12 de escena se mudaron a `plugins/ide/escena.{c,h}`, del lado de VimMon #fase2
- [x] Los procedimientos del lenguaje se resuelven ANTES que el registro: un host no puede tapar el `LEER` de AED con uno propio. Extiende, no redefine #fase2
- [x] Registrar reemplaza por nombre en vez de acumular, y hay que registrar justo ANTES de ejecutar: el registro guarda un puntero al estado, y `scene_view` arma su escena nueva en la pila. Sin eso, el siguiente que ejecutara escribiria en una pila que ya no existe #fase2
- [x] Verificado que `build/paedrun` (que no engancha la escena) corre el lenguaje entero y avisa `'CUBO' lo reconoce el parser pero no lo implementa nadie`, mientras que `vimmon` con `scene` sigue armando los 14 cuerpos igual que antes #fase2
- [x] Se cayo el flag `--escena` de `paedrun`: mostraba una escena que el runner del lenguaje ya no tiene #fase2
- [x] **`LEER` de consola EJECUTA.** Un destino por linea, y el destino puede ser `x`, `A[i]` o `p.campo` — los mismos tres lugares donde escribe una asignacion, asi que los dos caminos comparten `guardar_valor` en vez de duplicar el chequeo de campos y de limites #fase2
- [x] De donde salen los datos lo decide el HOST, no el interprete: `interp_set_entrada(fn, ud)` es el puerto y `paedrun` engancha stdin. El interprete corre dentro del game loop, asi que si abriera stdin por su cuenta congelaria la ventana entera esperando que alguien tipee en una terminal que quiza ni esta a la vista #fase2
- [x] El TIPO del dato leido lo decide el dato, no la declaracion: si la linea ENTERA es un numero, es numero; si no, es texto. `12abc` es texto y no 12, porque un numero a medias esconderia el error del que cargo el dato. Cuando el AMBIENTE chequee tipos, esto cambia #fase2
- [x] Una linea por destino y NO un token por destino: partir por espacios haria que "Juan Perez" llenara dos destinos con medio nombre cada uno #fase2
- [x] Al dato leido se le sacan los espacios de los dos lados, y el `\r` cuenta como espacio: un archivo de datos guardado en Windows termina cada linea con `\r\n`, y ese `\r` invisible convertiria el numero 12 en el texto "12\r". El bug seria mudo y la culpa se la llevaria el interprete #fase2
- [x] Bloque `// ── ENTRADA` en el `.paed`, que `correr.sh` le pasa por stdin al runner. Mismo criterio que la salida esperada: un test es UN archivo. stdin viene SIEMPRE de ese bloque, aunque este vacio, para que un test sin datos falle con "la entrada se termino" en vez de colgar la corrida esperando que alguien tipee #fase2
- [x] `leer_errores.paed` fija el TEXTO de los cinco errores: destino que no es destino, `LEER()` sin destinos, indice fuera de rango, campo que el registro no declara, y entrada agotada. Un LEER que falla callado es peor que uno que no existe #fase2
- [x] Limpiar directorios → _void #fase0
- [x] Crear estructura de carpetas del OS #fase0
- [x] `bus/plugin.h` — contrato del bus: `Plugin`, `EventType` (con `EVENT_MOUSE` y centinela `EVENT_COUNT`), `PLUGIN_MAX` #fase0
- [x] `bus/bus.c` — bus real: `bus_init`, `bus_register` (un plugin puede suscribirse a varios `EventType`), `bus_send`, `bus_unregister` (swap-con-el-ultimo), `bus_shutdown` (deduplicado) #fase0
- [x] Loop principal en `main.c` con manejo de `SIGINT` (Ctrl+C apaga limpio, no mata el proceso) #fase0
- [x] `plugins/input/input.c` — plugin de teclado + mouse: modo raw de terminal (termios), lectura no bloqueante, reporte SGR de mouse (click + posición), restaura la terminal al apagar #fase0
- [x] `paed/Frankly/docs/PAED.md` + `data/sintaxis.json` — spec y definicion formal de PAED, fuente unica de verdad #fase0
- [x] PAED.md v4.0 absorbe entero `docs/paed_spec.md` (v2.0), que se borra: una sola spec, no dos. Se separan **catedra** / **decidido** / **implementado**, porque habia decisiones documentadas que el parser no cumplia (keywords case-insensitive, `;` como separador) #fase0
- [x] `edit` abre **PseudoGames entero** — el IDE completo con su menu, niveles, wiki, pomodoro y editor adentro. Es una opcion del OS como `engine` o `ai`, no un programa que se corre por afuera #fase5
- [x] `plugins/editor/editor.c` escucha `EVENT_EDITOR_OPEN` y lanza `paed/aed`. main.c no sabe QUE programa es ni donde vive: el dia que haya otro, cambia el plugin y nada mas #fase5
- [x] Si `paed/aed` no esta compilado, el plugin corre `make -C paed` solo. La primera vez que alguien escribe `edit` no tiene por que saber que habia que compilar a mano #fase5
- [x] PseudoGames corre parado en `paed/`, porque carga `assets/`, `data/` y `saves/` con rutas relativas. `VIMMON_IDE` permite cambiar el programa sin recompilar VimMon #fase5
- [x] `ParcialSimulado_01.paed` — parcial de 5 ejercicios REALES de la catedra (guia 1.1.5.1/2/3 + condicion de alumno + BOSS de notas), corriendo entero con `paedrun`. Usa arreglos, registros, `PARA`, `SI/SINO` anidados y potencia con exponente 0.5 para la raiz #fase2
- [x] Bug de diagnostico: un texto de mas de `PAED_VAL_MAX` (128) bytes se truncaba en silencio y el error que salia era "falta la comilla de cierre", que manda a buscar un problema inexistente. Ahora dice cuantos bytes ocupa y cual es el maximo. El limite es en BYTES: una linea de guiones Unicode gasta 3 bytes por guion #fase2
- [x] Los `.esperado` se eliminaron: cada `.paed` declara su salida al final, en un bloque `// ── SALIDA ESPERADA`. Un test es UN archivo, y la referencia queda al lado del codigo que la produce. Se saco tambien `ACTUALIZAR=1`: si un test falla, el bloque se corrige a mano leyendo el diff, que es lo que evita "arreglar" el test en vez del bug #fase2
- [x] `LEER`/`ESCRIBIR` de consola y de archivo DISTINGUIDOS — se siguen escribiendo igual, y el parser decide cual es mirando si el primer argumento se declaro `ARCHIVO DE X`. No alcanza con contar argumentos: `LEER(clave, cod_mov)` es consola y tiene dos, igual que la de archivo (`wiki.txt:2761` vs `:2765`, en el MISMO algoritmo) #fase2
- [x] `arch: ARCHIVO DE <tipo>;` se declara y se valida. Un REGISTRO no puede tener un ARCHIVO adentro: vive en memoria #fase2
- [x] La decision se toma POR INSTRUCCION: un programa con tres archivos arriba y varios `LEER` de consola abajo no se pisa. Verificado en `archivos_formas.paed` #fase2
- [x] Bug: `ESCRIBIR(arch, reg)` entraba al camino de consola y evaluaba `arch` como expresion, asi que decia "la variable 'arch' no tiene valor todavia" — mandaba a mirar al lugar equivocado #fase2
- [x] Bug: `FDA(arch)` evaluaba el argumento ANTES de saber que funcion era, asi que el error era el del argumento y el nombre de la funcion no aparecia. Ahora las funciones sin soporte se resuelven antes de mirar sus argumentos #fase2
- [x] `FDA`/`NFDA` reconocidas en `expr.c` con mensaje propio. Antes decian "funcion desconocida", como si no existieran en el lenguaje: existen, falta implementarlas #fase2
- [x] REGISTRO implementado — `vector2 = REGISTRO ... FIN_REGISTRO` en el AMBIENTE, y `pori.vx` como destino y dentro de expresiones. Es el `struct` de C con otro nombre #fase2
- [x] `AlgebraRectas/recta.paed` corre ENTERO: de 10 errores a 0. Verificado a mano que el calculo da bien — P0=(0,0), D=(1,-4), dominio [-4,4] produce el segmento (-4,16) a (4,-16), que es la recta y=-4x #fase2
- [x] Los registros se APLANAN: `pori` de tipo vector2 se guarda como las variables "pori.vx" y "pori.vy". El Entorno no sabe nada de registros. El precio es que no se puede asignar un registro entero (`p1 := p2`), que no aparece en el corpus #fase2
- [x] Un campo que el registro NO declara se rechaza (`'p' no tiene un campo 'vz'`). Sin ese chequeo el registro no serviria de nada: como los campos se aplanan, `p.vz` naceria solo en su primera asignacion igual que un escalar #fase2
- [x] Keywords case-insensitive RESUELTO — `accion`, `MiEnTrAs` y `FiN_sI` parsean igual que en mayusculas. Alcanza a palabras clave, tipos, nombres de procedimiento y claves de parametro. Los IDENTIFICADORES siguen distinguiendo: `total` y `Total` son dos variables. Regla: lo que define el lenguaje no distingue, lo que nombras vos si #fase2
- [x] Verificado que ninguna variable del corpus real choca solo por mayusculas, asi que mantener los identificadores case-sensitive no rompe ningun `.paed` existente #fase2
- [x] Bug encontrado al hacerlo: `FIN_MIENTRAS` y `FIN_PARA` se distinguian por `linea[4] == 'P'`. Con `fin_para` en minuscula eso es `'p'` y el bucle se cerraba como si fuera un `FIN_MIENTRAS` #fase2
- [x] Cierre de la `ACCION` RESUELTO — era el ultimo punto BLOQUEANTE de la spec. Se aceptan `FIN_ACCION` y `FINACCION`: las dos son una sola palabra, cuestan un `strcmp` y ningun lookahead. `FACCION` se rechaza (abreviar FIN a F deja el cierre incompleto) y `FIN ACCION` con espacio tambien, aunque sea la forma de la catedra: partida en dos obliga a mirar la palabra siguiente. Las tres formas rechazadas igual CIERRAN el bloque, para no cascar un error por cada linea que venga despues #fase2
- [x] Verificado de primera mano `AED_2021_UnI.pdf` pagina 10: la catedra efectivamente escribe `FIN ACCION` con espacio. La cita que venia arrastrada de `paed_spec.md` era correcta — es una captura de Sublime Text 2 dentro del apunte, no un BNF formal #fase2
- [x] `docs/plugin_spec.md` — cómo crear plugins #fase0
- [x] `KANBAN.md` — este archivo #fase0
- [x] `Makefile` raíz — compila bus + plugins juntos #fase0
- [x] `main.c` raíz — entry point, inicializa bus y plugins #fase0
- [x] `plugins/ai/ai.h` — interfaz del plugin AI #fase1
- [x] `plugins/ai/ai.c` — HTTP client a Ollama con libcurl #fase1
- [x] `plugins/ai/ai.c` — parser de respuesta JSON (cJSON) #fase1
- [x] `plugins/ai/ai.c` — armar el prompt con scene.paed como contexto #fase1
- [x] `plugins/ai/ai.c` — registrar en el bus (EVT_AI_REQUEST → EVT_AI_RESPONSE) #fase1
- [x] Test: mandar "creá un cubo rojo" → recibir PAED delta en terminal #fase1
- [x] `plugins/ide/paed/parser.h` + `parser.c` — leer scene.paed línea por línea #fase2
- [x] `plugins/ide/paed/interpreter.h` + `interpreter.c` — ejecutar comandos PAED #fase2
- [x] `plugins/ide/scene.paed` — archivo de estado de la escena #fase2
- [x] Integrar: AI response → append a scene.paed → re-parsear #fase2
- [x] Test: scene.paed con un cubo → interprete lo lee sin crash #fase2
- [x] PAED = pseudocodigo AED puro: `data/sintaxis.json` solo tiene lo corroborado contra `wiki.txt`, `TEORIA_COMPLETA.txt` y los `.paed` de la catedra #fase2
- [x] La escena 3D salio del lenguaje: es una libreria aparte en `data/escena.json`, se carga ademas de `sintaxis.json` #fase2
- [x] `paed/Frankly/tools/generar.sh` — genera `paed.tmLanguage.json` y `core/palabras.sh` desde `sintaxis.json` (se acabo copiar keywords a mano) #fase2
- [x] `plugins/ide/parser.c` — parser de PAED real (ACCION/AMBIENTE/PROCESO) que reporta errores con archivo:linea y NUNCA ignora en silencio #fase2
- [x] Pila de bloques en el parser: `SI`/`SINO`/`FIN_SI` y `MIENTRAS`/`FIN_MIENTRAS` anidados. Una variable no puede representar un CAMINO de anidamiento, y es pila porque los bloques cierran en orden inverso al que se abren #fase2
- [x] Saltos parcheados al cerrar el bloque (backpatching): cada `SI`/`MIENTRAS` guarda a dónde ir, igual que el bytecode. `instrs[]` sigue siendo plano #fase2
- [x] Asignación `:=` con destino simple: se parsea y se guarda la expresión cruda (todavía sin evaluador) #fase2
- [x] Errores de bloque que citan la línea de APERTURA: "FIN_SI cierra un MIENTRAS abierto en la linea 3", "falta FIN_MIENTRAS: el MIENTRAS de la linea 3 quedo abierto" #fase2
- [x] Test: `ejercicio2_1_11.paed` (MIENTRAS > MIENTRAS > SI/SINO) parsea entero y cada `FIN_MIENTRAS` vuelve al suyo #fase2
- [x] `plugins/ide/expr.{h,c}` — evaluador de expresiones por descenso recursivo. La prioridad NO es una tabla de números: es el orden en que las funciones se llaman entre sí (`TEORIA_COMPLETA.txt:361-371`) #fase2
- [x] Cortocircuito en `Y` y `O`, que la teoría exige textualmente ("En AND, si el primer operando es Falso, el segundo no se evalúa"). Cambia el comportamiento, no solo la velocidad #fase2
- [x] Tabla de variables (`Entorno`) sin malloc, y `ESCRIBIR` que EVALÚA sus argumentos: antes `ESCRIBIR(cont_pal)` imprimía el nombre en vez del valor #fase2
- [x] `interp_exec` sigue los saltos con un índice en vez de recorrer el array: es un contador de programa. `SI`/`SINO`/`MIENTRAS`/`PARA` se ejecutan de verdad #fase2
- [x] Guarda de bucle infinito (2M pasos): el intérprete corre DENTRO del game loop, así que un programa colgado colgaba la ventana entera #fase2
- [x] Bug: `parse_instruction` buscaba el `=` de `clave = valor` con `strchr`, sin respetar comillas, y `ESCRIBIR("a = b")` quedaba destrozado #fase2
- [x] Test: programa con `PARA`, `PARA` en reversa, `MIENTRAS` acumulador, `SI/SINO` y `PARA` anidado corre entero y da los valores correctos #fase2
- [x] `build/paedrun` — arnés que corre un `.paed` en la terminal, sin SDL ni bus. El intérprete vive dentro del game loop, así que probar el lenguaje era abrir la ventana y mirar; un test que hay que mirar no es un test #fase2
- [x] `make test` — corre todos los `.paed` de `paed/Frankly/tests/`. Agregar un test es dejar el archivo: no hay lista que mantener a mano #fase2
- [x] Evaluador: `ARREGLO[desde..hasta] DE <tipo>` en el `AMBIENTE`, `A[i]` en expresiones y como destino. El índice es una expresión completa, así que `A[(izq+der) DIV 2]` sale gratis. Los límites se chequean en runtime: `indice 4 fuera de rango: 'A' va de 5 a 9` #fase2
- [x] Algoritmos reales corriendo: búsqueda lineal, búsqueda binaria, burbuja con `PARA` anidado, Euclides, primos, Fibonacci, factorial #fase2
- [x] Bug: `falla(c, "%s", c->env->error)` pasaba el buffer de error como argumento de un `vsnprintf` que escribe en ESE MISMO buffer. Aliasing: el mensaje llegaba vacío (`error: ` pelado). Se copia antes de pasarlo #fase2
- [x] Bug: `stdout` con buffer y `stderr` sin él descolocaban el orden de la salida al mandarla a una tubería, y los errores aparecían antes de líneas impresas primero. `setvbuf` en `paedrun` #fase2
- [x] `PARA <var> := <desde> HASTA <hasta>[; <paso>] HACER` / `FIN_PARA`. El paso es OPCIONAL y por defecto 1, corroborado en `TEORIA_COMPLETA.txt:565-571` ("Si el incremento es distinto de 1, debe indicarse"). En reversa se usa paso negativo, no una palabra tipo `downto`. `recta.paed:47` decía `a` en vez de `HASTA` y se corrigió #fase2
- [x] SDL2 ya instalado (`sdl2-compat`, headers en `/usr/include/SDL2/`) — card "Instalar libsdl2-dev" satisfecha #fase3
- [x] `plugins/renderer/renderer.h` — interfaz abstracta: vtable de punteros a función, sin tipos SDL (backend intercambiable) #fase3
- [x] `plugins/renderer/sdl_fb.c` — backend framebuffer: ventana SDL2 + textura streaming ARGB8888, framebuffer privado (`static uint32_t *pixels`) #fase3
- [x] Primitivas a mano: `put_pixel` (con guard de límites), `fill_rect`, `draw_line` (Bresenham, solo enteros) #fase3
- [x] `engine/engine.{h,c}` — motor 2D de entidades: `Entity` con callbacks `update`/`draw`, `World` (pool estático), game loop dueño del tiempo (`dt`, ~60fps), colisión AABB (`entity_overlaps`) #fase3
- [x] `game/game.{h,c}` — seam del juego del usuario: `game_setup(World*)` donde vive el juego (starter: cuadrado movible con flechas/WASD) #fase3
- [x] `examples/hello_entity.c` — plantilla ejecutable (rectángulo que rebota) #fase3
- [x] Consola de comandos en `main.c` (`engine`/`scene`/`ai`/`help`/`quit`); "engine" publica `EVENT_RENDER_FRAME` → el renderer lanza el motor con `game_setup` #fase3
- [x] `plugins/renderer/renderer.c` — puente bus↔motor: `on_event(EVENT_RENDER_FRAME)` abre el motor y le cede el loop #fase3
- [x] `plugins/ide/scene_view.{h,c}` — el `SceneState` de PAED se dibuja en el motor: base de cámara (derecha/arriba/adelante con producto vectorial), perspectiva dividiendo por la profundidad, orden del pintor. Se monta como UNA entidad del pool, así `engine.c` no se tocó #fase3
- [x] Recarga en caliente de la ESCENA: `scene_view.c` vigila el mtime (en nanosegundos) de `scene.paed` cada ~0.5s y re-parsea sola. Si el archivo nuevo tiene errores, conserva la última escena buena #fase3
- [x] Renombrar el `Entity` de PAED a `Cuerpo`: chocaba con el `Entity` del motor, que son cosas distintas #fase3
- [x] Test: `scene.paed` con cubo → aparece en pantalla. Verificado con un `Renderer` falso que anota las llamadas: cubo de 1×1×1 a distancia 5 → rect de 115px centrado; a distancia 10 → 55px #fase3




%% kanban:settings
```
{"kanban-plugin":"board","show-checkboxes":true,"tag-colors":[],"move-tags":true}
```
%%