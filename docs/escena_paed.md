# Cómo se dibuja una escena PAED

`scene.paed` describe una escena en 3D. La ventana dibuja píxeles en 2D.
El puente entre las dos cosas es `plugins/ide/scene_view.c`.

```
scene.paed  --parser-->  PAEDProgram  --interprete-->  SceneState  --scene_view-->  píxeles
```

## Probarlo

```
build/vimmon
vimmon> engine
```

Con la ventana abierta, editá `plugins/ide/scene.paed` en otra terminal y guardá:
la escena se redibuja sola. También sirve pedirle a la IA (`ai hacé un árbol`),
porque el plugin `ide` escribe el mismo archivo.

Se cierra con la X o con ESC.

## Una entidad para toda la escena

El motor (`engine/engine.h`) ya llamaba `update` y `draw` por cada entidad viva.
No hizo falta tocarlo: la escena PAED entera se monta como **una sola** entidad
del pool.

```c
Entity *e = world_spawn(w);
e->state  = &g_view;      // SceneState + ruta + mtime
e->update = view_update;  // vigila el archivo
e->draw   = view_draw;    // proyecta y dibuja
```

Para volver a montar el juego de `game/game.c`, en `plugins/renderer/renderer.c`
se cambia `scene_view_mount(w, PAED_SCENE_PATH)` por `game_setup(w)`.

## La proyección, en tres pasos

**1. Base de cámara.** Tres vectores perpendiculares entre sí, sacados con
producto vectorial a partir de dónde está la cámara y hacia dónde mira
(`CAMARA(posicion = ..., mirar = ...)`).

```c
c.ade = normalizar(cam_target - cam_pos);   // adelante
c.der = normalizar(cruz(c.ade, arriba));    // derecha
c.arr = cruz(c.der, c.ade);                 // arriba real
```

**2. Perspectiva.** Todo el 3D se reduce a **dividir por la profundidad**: lo
lejano se achica porque `z` es más grande.

```c
sx = centro_x + x * foco / z;
sy = centro_y - y * foco / z;   // la y de pantalla crece hacia ABAJO
```

`foco = alto/2 / tan(FOV/2)`, con FOV vertical de 60°.

**3. Orden del pintor.** No hay z-buffer, así que **el orden de dibujo es la
profundidad**: se ordenan los cuerpos de lejos a cerca y se pintan en ese orden.
El que se pinta último tapa a los anteriores.

### Dos guardas que no son opcionales

- Si no hay `CAMARA` en el archivo, `cam_pos` y `cam_target` quedan los dos en
  cero: la resta da el vector nulo, que no tiene dirección. Se mira a `-z`.
- Si la cámara mira casi en vertical, "adelante" y "arriba" son paralelos y el
  producto vectorial da cero: no habría un "derecha". Se cambia el vector de
  referencia.

Sin esas guardas aparecen `NaN` y la escena desaparece sin un solo error.

## Cómo se dibuja cada cuerpo

| `kind`  | Cómo se dibuja |
|---------|----------------|
| `cubo`  | Se proyectan las 8 esquinas (ya rotadas por `ROTAR`) y se rellena el rectángulo que las contiene |
| `plano` | Igual que el cubo |
| `esfera`| Círculo relleno por barridos horizontales, radio `radio * foco / profundidad` |
| `luz`   | Una cruz. No ilumina nada todavía: marca dónde está |

El fondo lo manda `FONDO(color = ...)`, no el motor.

## Recarga en caliente

`view_update` mira el `mtime` de `scene.paed` cada 30 frames (~medio segundo a
60fps). Si cambió, re-parsea y reemplaza la escena. Es la misma idea que usa
`hot-Reload/host.c` para `game.so`, pero vigilando una descripción de escena:
acá no hay código que recargar.

**Se usan nanosegundos, no `st_mtime`.** `st_mtime` cuenta segundos enteros: si
guardás dos veces dentro del mismo segundo, la segunda no se detecta y te quedás
mirando una escena vieja sin entender por qué. Se compara
`st_mtim.tv_sec * 1e9 + st_mtim.tv_nsec`.

**Si el archivo nuevo tiene errores, se conserva la última escena buena** y los
errores salen por consola con archivo:línea. Quedarte con la pantalla en negro
porque te comiste un `;` sería el peor comportamiento posible mientras editás.

## Grupos

Las cuatro primitivas aceptan `grupo = <id>`. Las piezas de un mismo grupo se
mueven, rotan y escalan juntas, como un cuerpo rígido:

```paed
CUBO(nombre = tronco, posicion = (0,0,0), color = #6b4423, tamano = (0.4,2,0.4), grupo = arbol);
ESFERA(nombre = copa, posicion = (0,1.7,0), color = #2e7d32, radio = 0.95, grupo = arbol);
MOVER(nombre = arbol, posicion = (10,0,0));   -- se mueve el árbol entero
```

La pieza le gana al grupo: si existe un cuerpo con ese id exacto, la operación
va a ese cuerpo. Así podés tocar `copa` sin mover el árbol.

Sobre un grupo, `MOVER` **traslada** (calcula el centro y arrastra a todos con el
mismo delta). Ponerle la misma posición a cada pieza derrumbaría el árbol en un
punto.

## Verificar sin pantalla

Si no hay display, se puede comprobar la proyección con números: se implementa
el vtable de `Renderer` con funciones que **anotan** las llamadas en vez de
dibujar, se llama a `scene_view_mount` y se invoca `e->draw` a mano.

Un cubo de 1×1×1 a distancia 5, con la pantalla en 800×600, tiene que dar un
rectángulo de 115×115 centrado en (400,300). A distancia 10, de 55×55: al
duplicar la distancia, el tamaño se parte al medio.

## Lo que todavía no hace

- **No recorta contra el plano cercano.** Un cuerpo grande que abraza la cámara
  (el suelo) tiene esquinas atrás y otras a profundidad casi cero: dividir por
  eso da coordenadas de decenas de miles. Hoy se acotan a unas pantallas. Lo
  correcto es recortar el polígono generando vértices nuevos en el borde.
- **No hay z-buffer.** El orden del pintor usa la profundidad del *centro*, así
  que dos cuerpos que se cruzan se pueden ordenar mal.
- **Las cajas no tienen caras.** Se rellena el rectángulo que contiene las 8
  esquinas, no las 6 caras con sombreado.
- **`LUZ` no ilumina.**
