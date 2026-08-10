# PAED — Especificación del lenguaje v3.0

**PAED es el pseudocódigo AED de la cátedra. Nada más que eso.**

Este es el único documento de sintaxis del lenguaje. La definición formal está en
[`../data/sintaxis.json`](../data/sintaxis.json): el resaltador de Neovim y el
parser en C la leen en runtime, y `tools/generar.sh` genera el resto desde ahí.

Todo lo que está acá está corroborado contra tus apuntes:
`PseudoGames/data/wiki.txt`, `PseudoGames/solutions/AED_Teoria/TEORIA_COMPLETA.txt`
y tus propios ejercicios en `AprendiendoPseudo/`. **Nada inventado.**

Los procedimientos de escena 3D de VimMon **no son parte del lenguaje**: son una
librería aparte, ver [`ESCENA.md`](ESCENA.md).

---

## 1. Estructura de un programa

```paed
ACCION nombre_algoritmo ES
    AMBIENTE
        variable: TIPO;
    PROCESO
        instrucciones;
FIN_ACCION
```

- Palabras clave en MAYÚSCULAS, identificadores en minúsculas.
- Comentarios con `//` hasta el fin de línea.
- Declaraciones e instrucciones terminan en `;`.
- Las palabras de bloque (`SI`, `MIENTRAS`, `FIN_SI`, …) **no** llevan `;`.

## 2. Tipos de datos

| Tipo | Ejemplo en tus apuntes |
|---|---|
| `ENTERO` | `cont_pal: ENTERO;` |
| `REAL` | `tini: REAL;` |
| `BOOLEANO` | |
| `CARACTER` / `CARACTERES` | `SECUENCIA DE CARACTERES` |
| `AN(n)` | `a: AN(5);` |
| `N(n)` | |
| `SECUENCIA DE <tipo>` | `sec1: SECUENCIA DE CARACTER;` |
| `VENTANA DE <tipo>` | `recorrido: VENTANA DE CARACTER;` |
| `SECUENCIA DE SALIDA` | `secSalida: SECUENCIA DE SALIDA;` |
| `ARREGLO[d..h] DE <tipo>` | `A: ARREGLO[1..10] DE ENTERO;` |
| `REGISTRO`, `CONSTANTE` | |

Los límites del arreglo los elige el programador y **no arrancan en 0**:
`ARREGLO[1..10]` va del 1 al 10, y `ARREGLO[5..9]` del 5 al 9.

## 3. Asignación y operadores

```paed
cont_pal := 0;
cont_pal := cont_pal + 1;
```

Corroborado contra `TEORIA_COMPLETA.txt:307-371`:

| Grupo | Operadores |
|---|---|
| Asignación | `:=` |
| Aritméticos | `+` `-` `*` `/` `MOD` `DIV` `**` |
| Relacionales | `=` `<>` `<` `<=` `>` `>=` |
| Lógicos | `Y` (`AND`) · `O` (`OR`) · `NO` |
| Literales lógicos | `V` `F` |

`MOD` y `DIV` son operadores **infijos** (`a MOD b`), no funciones.
`**` es la potencia — **no** `^`.

**`==` no existe en AED.** La igualdad es `=`. Tus `.paed` lo usan 91 veces:
es un error de escritura arrastrado, no del lenguaje.

Prioridad, de mayor a menor: `+ - NO` · `**` · `* / DIV MOD` · `+ -` ·
`+` (concatenación) · `< <= > >=` · `= <>` · `Y` · `O`.

## 4. Estructuras de control

```paed
SI (condicion) ENTONCES
    ...
SINO
    ...
FIN_SI

MIENTRAS (condicion) HACER
    ...
FIN_MIENTRAS

PARA ... HACER
    ...
FIN_PARA

REPETIR
    ...
HASTA (condicion)

SEGUN variable HACER
    valor: ...;
FIN_SEGUN
```

## 5. Procedimientos y funciones

Las llamadas en AED son **posicionales**:

```paed
ESCRIBIR("La cantidad de palabras es:", cont_pal);
LEER(variable);
ARR(secCaracter);
AVZ(secCaracter, venCaracter);
CREAR(secSalida);
CERRAR(sec1, recorrido);
```

Funciones usadas dentro de expresiones: `NFDS`, `FDS`, `MOD`, `DIV`,
`TRUNC`, `ABSO`, `REDOND`.

Definidas por el usuario, dentro de `AMBIENTE`:

```paed
FUNCION car_a_num(c: caracter) ES: ENTERO
    PROCESO
        ...
FIN_FUNCION
```

## 6. Errores

El parser **nunca ignora en silencio**. Formato clang:

```
archivo.paed:12: error: falta ';' al final de la instruccion
```

Si hay al menos un error, no se ejecuta nada. Análisis primero, ejecución
después.

## 7. Estado real del parser en C

Esto es lo que `plugins/ide/parser.c` entiende **hoy**. El resto se reconoce y
se reporta como no implementado, con número de línea — nunca se ignora.

| Construcción | Estado |
|---|---|
| `ACCION` / `AMBIENTE` / `PROCESO` / `FIN_ACCION` | ✅ |
| Declaraciones `nombre: TIPO;` | ✅ (el tipo se guarda como texto, no se valida) |
| Comentarios `//`, incluso dentro de strings | ✅ |
| Llamadas posicionales `PROC(a, b);` | ✅ |
| `;` obligatorio, error con línea | ✅ |
| Asignación `:=` | ✅ |
| `SI` / `SINO` / `FIN_SI`, anidados | ✅ |
| `MIENTRAS` / `FIN_MIENTRAS`, anidados | ✅ |
| `PARA ... HASTA ... [; paso] HACER`, incluso en reversa | ✅ |
| Expresiones y operadores de comparación | ✅ |
| `ARREGLO[desde..hasta] DE <tipo>`, en expresión y como destino | ✅ |
| Nombre de `ACCION` **con espacios** (`ACCION Ejercicio de Parcial ES`) | ❌ |
| `REPETIR` / `SEGUN` | ❌ |
| `FUNCION` / `PROCEDIMIENTO` anidados en `AMBIENTE` | ❌ |
| `REGISTRO` (`pori.vx`) / `ARCHIVO` | ❌ |
| `SECUENCIA`, y por lo tanto `NFDS` / `FDS` | ❌ |

Las estructuras de control usan una **pila** de bloques, no una máquina de
estados plana: una variable sola no puede representar un CAMINO de anidamiento,
y es pila porque los bloques cierran en el orden inverso al que se abren.

Los arreglos chequean el índice contra los límites declarados. Es la diferencia
con C: ahí `A[99]` sobre un arreglo de 10 pisa memoria ajena en silencio, y acá
corta diciendo cuál fue el índice y cuáles eran los límites.

### Cómo se prueba

`build/paedrun` corre un `.paed` en la terminal, sin abrir la ventana:

```bash
make paedrun
build/paedrun paed/Frankly/tests/busqueda_binaria.paed
```

`make test` corre todos los `.paed` de `paed/Frankly/tests/` y compara la
salida contra el `.esperado` de al lado. Agregar un test es dejar los dos
archivos ahí: no hay ninguna lista que mantener a mano.

## 8. Sin corroborar

Estas palabras están en `sintaxis.json` pero **no aparecen** en `wiki.txt`, en
`TEORIA_COMPLETA.txt` ni en ningún `.paed` tuyo. Vienen de la versión original
del archivo. Revisarlas contra los apuntes antes de darlas por buenas:

- `RETORNAR`
- `TRUNC`, `ABSO`, `REDOND` (sí están implementadas en `stdlib/matematica.sh`)
