# PAED — Especificación del lenguaje v2.0

Lenguaje imperativo de VimMon. Dialecto de pseudocódigo AED de la cátedra.

---

## 0. Unificación

Hasta hoy convivían **dos lenguajes llamados PAED**, sin relación entre sí:

| Versión | Ubicación | Paradigma | Estado |
|---|---|---|---|
| PAED-escena | `plugins/ide/` | Declarativo (`cubo nombre=x posicion=0,0,0`) | **DISCONTINUADO** — archivado en `_void/docs/paed_spec_v1.md` |
| PAED-pseudocódigo | `PseudoGames/` | Imperativo (`ACCION`/`SI`/`MIENTRAS`) | **ES PAED v2** |

**A partir de esta versión existe un solo PAED: el dialecto AED.** Las primitivas de escena del lenguaje viejo (`cubo`, `esfera`, `mover`, `girar`) pasan a ser funciones nativas invocables desde PAED, no sintaxis.

## 0.1 Fuente de verdad — SOLO DOS

La sintaxis de PAED **no se inventa y no se deduce del código existente**. Únicamente estas dos fuentes definen el lenguaje:

| # | Fuente | Ruta | Por qué |
|---|---|---|---|
| 1 | **Material de cátedra** | `PseudoGames/solutions/AED_Teoria/*.pdf` | Oficial, elaborado por los docentes |
| 2 | **La wiki** | `PseudoGames/data/wiki.txt`, `PseudoGames/data/OnlySintaxis.md` | Escrita por Brian verificando contra la cátedra |

Nada más es autoridad. Punto.

### 0.2 Lo que NO define la sintaxis

Todo lo siguiente es **implementación o notas personales**. Puede contener errores, y de hecho los contiene:

| Fuente | Estado | Error conocido |
|---|---|---|
| `Frankly/paed` (715 líneas bash) | Implementación | Acepta `PARA ... a ...` y comillas dobles — ninguna es de cátedra |
| `Frankly/DOC.txt` | Notas de aprendizaje | Fija `FIN_ACCION`, forma no confirmada por la cátedra |
| `PseudoGames/AlgebraRectas/recta.paed` | Ejercicio propio | **Sintaxis incorrecta** (ver §8) |
| `Frankly/data/sintaxis.json` | Lista de keywords | Incompleta, sin consumidor |
| `Frankly/syntaxes/paed.tmLanguage.json` | Resaltador | Incompleta, comillas dobles |
| `PseudoGames/src/editorText.c:134-163` | Resaltador | Incompleta, desincronizada |
| `~/apuntes/AED/**/*.md` | Práctica personal | Formas mezcladas (`finaccion`, `FinAccion`) |

**Regla dura:** si una de estas contradice a la cátedra o a la wiki, **la fuente está mal y se corrige** — no se adapta la spec para acomodarla.

Corolario importante: **v2 NO tiene contrato de compatibilidad con Frankly.** Si Frankly acepta algo que la cátedra no define, v2 lo rechaza y Frankly es el que está mal.

### 0.3 Qué se conserva de Frankly

Solo decisiones de **arquitectura**, que no son sintaxis y por lo tanto no requieren autoridad de cátedra:

| Convención | Evidencia | Se conserva en |
|---|---|---|
| Análisis completo antes de ejecutar | `DOC.txt:8-15` | Arquitectura general |
| Formato de error `archivo:línea: error: msg` | `paed:96,147,155,163` | Fase F (diagnósticos) |
| Pila para validar anidamiento | `paed:110-195` | Fase E (control de flujo) |

**Regla de trazabilidad:** ninguna construcción entra a esta spec sin cita a línea de la cátedra o de la wiki.

---

## 1. Alcance por fase

Este documento describe el lenguaje completo, pero la implementación es por etapas.

| Construcción | Fase | Estado |
|---|---|---|
| `ACCION`/`AMBIENTE`/`PROCESO` | D | v2 |
| Declaraciones y tipos escalares | D | v2 |
| Asignación `:=`, expresiones | C–D | v2 |
| `ESCRIBIR`, `LEER` (consola) | D | v2 |
| `SI`/`SINO`, `MIENTRAS`, `PARA` | E | v2 |
| `REGISTRO` y acceso a campos | D | v2 |
| Arreglos `a[i]` | D | v2 |
| Funciones nativas del motor | G | v2 |
| `FUNCION`, `PROCEDIMIENTO`, `SUBACCION` | H | diferido |
| Recursividad | H | diferido |
| Archivos (`ABRIR`, `FDA`, ...) | I | diferido |
| Secuencias (`SECUENCIA`, `VENTANA`, `AVZ`, `FDS`) | I | diferido |
| Punteros, nodos, listas | J | diferido |

Diferido significa **documentado y reservado**, no descartado: sus keywords ya ocupan lugar en el enum de tokens para no romper el ABI después.

---

## 2. Inventario de tokens

### 2.1 Keywords — estructura

`ACCION` `ES` `FIN_ACCION` `AMBIENTE` `PROCESO`
`FUNCION` `FIN_FUNCION` `PROCEDIMIENTO` `FIN_PROCEDIMIENTO`
`SUBACCION` `FIN_SUBACCION` `RETORNAR`

> `OnlySintaxis.md:9-14` (ACCION/AMBIENTE/PROCESO), `:58-64` (FUNCION), `:74-79` (PROCEDIMIENTO), `:177-183` (SUBACCION)

### 2.2 Keywords — control de flujo

`SI` `ENTONCES` `SINO` `FIN_SI`
`MIENTRAS` `HACER` `FIN_MIENTRAS`
`PARA` `FIN_PARA`
`REPETIR` `HASTA`
`SEGUN`

> `OnlySintaxis.md:189-199` (SI anidado), `:164-167` (MIENTRAS), `:124-127` (PARA)

### 2.3 Keywords — tipos

`ENTERO` `REAL` `CARACTER` `BOOLEANO`
`AN` `N`
`ARREGLO` `DE` `SECUENCIA` `VENTANA` `CONSTANTE` `VAR`
`REGISTRO` `FIN_REGISTRO`
`ARCHIVO` `PUNTERO`

> `OnlySintaxis.md:19-24` (escalares, `AN(20)`, `N(5)`), `:100-103` (REGISTRO/FIN_REGISTRO), `:121` (arreglo), `:143` (archivo), `:502` (puntero), `wiki.txt:750-751` (SECUENCIA DE CARACTERES, VENTANA DE CARACTER)

### 2.4 Keywords — operadores lógicos

`Y` `O` `NO`

> `OnlySintaxis.md:227` (`NFDA(arch1) Y NFDA(arch2)`), `:260` (`O`), `:427` (`MIENTRAS NO Bandera`)

### 2.5 Keywords — operadores aritméticos con nombre

`MOD` `DIV` `TRUNC` `ABSO` `REDOND`

> `OnlySintaxis.md:379` (`(iz + de) DIV 2`)

### 2.6 Keywords — entrada/salida y archivos

`LEER` `ESCRIBIR` `ABRIR` `CERRAR` `BORRAR`
`FDA` `NFDA` `FDS` `NFDS` `AVZ` `ARR`
`NUEVO` `DISPONER`

> `OnlySintaxis.md:46,48` (LEER/ESCRIBIR consola), `:148-158` (ABRIR/CERRAR), `:341` (BORRAR), `:508-510` (NUEVO/DISPONER)

**FDA y FDS NO son lo mismo** y el resaltador actual los confunde:

| Token | Significado | Fuente |
|---|---|---|
| `FDA` / `NFDA` | Fin De **Archivo** | `wiki.txt:1905-1907` |
| `FDS` / `NFDS` | Fin De **Secuencia** | `wiki.txt:1003,1009` |

### 2.7 Literales

| Token | Forma | Fuente |
|---|---|---|
| `NUMERO` | `23`, `1.5`, `-4` | `OnlySintaxis.md:29`, `recta.paed:39` |
| `CADENA` | `"Hola"` | `wiki.txt` (comillas dobles) |
| `CARACTER_LIT` | `'M'`, `'A'` | `OnlySintaxis.md:337,371` |
| `VERDADERO` | `Verdadero` | `OnlySintaxis.md:426`, `wiki.txt:131` |
| `FALSO` | `Falso` | `OnlySintaxis.md:428` |
| `NIL` | `nil` | `OnlySintaxis.md:503` |
| `IDENTIFICADOR` | `pori`, `t_label_x`, `a` | `recta.paed:8-26` |

### 2.8 Operadores y delimitadores

```
Asignación     :=                        OnlySintaxis.md:29
Comparación    =   <>   <   <=   >   >=  OnlySintaxis.md:460, :189, :380
Aritméticos    +   -   *   /             OnlySintaxis.md:30-31, :463
Puntero        *                         OnlySintaxis.md:514  (prefijo, deref)
Delimitadores  (  )  [  ]  ,  ;  :  .    OnlySintaxis.md:125, :114
Rango          ..                        OnlySintaxis.md:121  (arreglo[1..30])
```

### 2.9 Especiales

`EOF` — fin de archivo · `ERROR` — token inválido, lleva el mensaje de diagnóstico

---

## 3. Decisiones de diseño

Las cinco ambigüedades del lenguaje, resueltas. **Sujetas a revisión contra la cátedra.**

### 3.1 Sensibilidad a mayúsculas: keywords case-insensitive

La wiki usa `ARREGLO` (`wiki.txt:1791`) y `arreglo[` (`wiki.txt:1798`) para lo mismo. También `puntero a ENTERO` (`OnlySintaxis.md:502`) y `archivo de TIPO` (`:143`) en minúscula, mientras el resaltador las lista en mayúscula.

**Decisión:** el lexer normaliza a mayúsculas **solo para buscar en la tabla de keywords**. Los identificadores conservan su capitalización original.

Consecuencia: `MIENTRAS` y `mientras` son el mismo token. `Verdadero`, `VERDADERO` y `verdadero` también.

### 3.2 El separador del `PARA` es `HASTA` — RESUELTO 2026-08-07

```
PARA i := 1 HASTA n HACER
    ...
FIN_PARA
```

**Decisión: `HASTA`.** Es la forma de la cátedra (10 apariciones en `~/apuntes/AED`, cero de `a`) y **elimina el problema de raíz**: `HASTA` es keyword normal, ya existe en `tmLanguage.json:43` y `Frankly/data/sintaxis.json:11`, y nadie llama `hasta` a una variable.

La alternativa descartada (`a`, como en `recta.paed:47` y `OnlySintaxis.md:124`) habría obligado a implementar **keyword contextual**: emitir `a` como `IDENTIFICADOR` y hacer que el parser la aceptara por posición, porque `a: ENTERO;` es una declaración válida (`OnlySintaxis.md:19`). Ese caso especial ya no hace falta.

#### `HASTA` cumple dos roles

| Rol | Forma | Posición |
|---|---|---|
| Separador de rango | `PARA i := 1 HASTA n HACER` | dentro del encabezado del `PARA` |
| Cierre de ciclo post-test | `REPETIR ... HASTA cond` | a nivel de sentencia |

**No hay ambigüedad**: las posiciones gramaticales son distintas, así que un parser de descenso recursivo las distingue sin lookahead extra. Un solo `TOKEN_HASTA`.

Compatibilidad con Frankly verificada: `Frankly/paed:62` despacha sobre la **primera palabra** de la línea (`case $1`). La línea `PARA i := 1 HASTA n HACER` entra por `PARA)` (`paed:115`), nunca por `HASTA)` (`paed:184`). **No rompe nada.**

#### Pendiente relacionado

`de` en `arreglo[1..30] de Alumno` (`OnlySintaxis.md:121`) sigue siendo keyword contextual mientras se escriba en minúscula. No está resuelto.

### 3.3 `AN(20)` y `N(5)` — tipos, no llamadas

`OnlySintaxis.md:22-23`. El paréntesis pertenece al **tipo**, no a una invocación.

**Decisión:** `AN` y `N` son keywords. La regla gramatical de tipo consume `AN` `(` NUMERO `)`. El lexer no hace nada especial — la desambiguación es del parser, porque en PAED un tipo nunca aparece donde se espera una expresión.

### 3.4 El `=` tiene tres roles

| Rol | Ejemplo | Fuente |
|---|---|---|
| Comparación | `SI n = 0 ENTONCES` | `OnlySintaxis.md:460` |
| Constante | `MAX = 100;` | `OnlySintaxis.md:24` |
| Tipo registro | `Nodo = REGISTRO` | `OnlySintaxis.md:100` |

**Decisión:** un solo token `IGUAL`. El parser desambigua por contexto — dentro de `AMBIENTE` es declaración, dentro de una expresión es comparación. No hace falta lookahead en el lexer.

### 3.5 Un token por keyword

**Decisión:** `TOKEN_SI`, `TOKEN_MIENTRAS`, ... uno por cada una. No un `TOKEN_KEYWORD` genérico con subtipo.

Motivo: el parser hace `switch (t.type)` directo, sin leer un segundo campo. Y un typo (`TOKEN_MIENTAS`) lo caza el compilador de C, no el runtime. El costo es un enum de ~70 entradas, que sigue siendo un `int`.

### 3.6 El `.` decimal contra el `.` de campo

`1.5` es un número; `pori.vx` (`recta.paed:60`) es acceso a campo. El mismo carácter.

**Decisión:** se resuelve **dentro del escaneo de número**. Si el lexer ya venía consumiendo dígitos y encuentra `.` **seguido de un dígito**, lo absorbe como parte decimal. En cualquier otro caso `.` es un token propio.

Requiere **un carácter de lookahead después del punto**. Caso a testear: `arreglo[1..30]` — tras el `1` viene `.` y después otro `.`, que no es dígito, así que el número corta y salen dos tokens `PUNTO` (o un `RANGO` si se decide token propio).

---

## 4. Gramática — construcciones en alcance v2

```ebnf
programa    = accion ;
accion      = "ACCION" IDENT "ES" [ ambiente ] proceso "FIN_ACCION" ;

ambiente    = "AMBIENTE" { registro | constante | declaracion } ;
registro    = IDENT "=" "REGISTRO" { campo } "FIN_REGISTRO" ;
campo       = IDENT ":" tipo ";" ;
constante   = IDENT "=" literal ";" ;
declaracion = IDENT ":" tipo ";" ;

tipo        = "ENTERO" | "REAL" | "CARACTER" | "BOOLEANO"
            | "AN" "(" NUMERO ")"
            | "N"  "(" NUMERO ")"
            | "ARREGLO" "[" NUMERO ".." NUMERO "]" "de" tipo
            | IDENT ;

proceso     = "PROCESO" { sentencia } ;
sentencia   = asignacion | si | mientras | para | escribir | leer ;

asignacion  = lvalue ":=" expr ";" ;
lvalue      = IDENT { "." IDENT | "[" expr "]" } ;

si          = "SI" expr "ENTONCES" { sentencia }
              [ "SINO" { sentencia } ] "FIN_SI" ;
mientras    = "MIENTRAS" expr "HACER" { sentencia } "FIN_MIENTRAS" ;
para        = "PARA" IDENT ":=" expr "HASTA" expr "HACER"
              { sentencia } "FIN_PARA" ;

escribir    = "ESCRIBIR" "(" [ expr { "," expr } ] ")" [ ";" ] ;
leer        = "LEER" "(" lvalue { "," lvalue } ")" [ ";" ] ;
```

El `;` de `ESCRIBIR`/`LEER` es opcional: la wiki es inconsistente — `OnlySintaxis.md:46,48` los escribe sin `;`, `:115` con `;`. Ver §7.

## 4.1 Precedencia de expresiones

De menor a mayor ligadura. Esta tabla **es** el parser de Pratt.

| Nivel | Operadores | Asociatividad |
|---|---|---|
| 1 | `O` | izquierda |
| 2 | `Y` | izquierda |
| 3 | `NO` | prefija |
| 4 | `=` `<>` `<` `<=` `>` `>=` | izquierda |
| 5 | `+` `-` | izquierda |
| 6 | `*` `/` `MOD` `DIV` | izquierda |
| 7 | `-` unario, `*` deref | prefija |
| 8 | `.` campo, `[` índice, `(` llamada | postfija |
| 9 | NUMERO, CADENA, CARACTER_LIT, `Verdadero`, `Falso`, `nil`, IDENT, `(` expr `)` | — |

Casos reales que el parser debe resolver:

Todas las citas de esta tabla son de la wiki. **Las de `recta.paed` fueron removidas** el 2026-08-07 al perder ese archivo su autoridad (§8.1).

| Expresión | Fuente | Qué ejercita |
|---|---|---|
| `c := (a - b) / 2` | `OnlySintaxis.md:31` | paréntesis + división |
| `n * factorial(n - 1)` | `OnlySintaxis.md:463` | `*` liga más que la llamada; argumento con expresión |
| `cen := (iz + de) DIV 2` | `OnlySintaxis.md:379` | `DIV` como operador binario |
| `r.clave3 <> Reg3` | `OnlySintaxis.md:189` | `<>` con acceso a campo |
| `a[j+1] := a[j]` | `OnlySintaxis.md:402` | índice con expresión adentro |
| `(i < N) Y A[i] <> x` | `OnlySintaxis.md:365` | `Y` liga menos que comparación |
| `(reg1.clave <> HV) O (reg2.clave <> HV)` | `OnlySintaxis.md:260` | `O` en el nivel más bajo |
| `SUMA := A + B` | `AED_2021_UnI.pdf:10` | **única expresión con autoridad de cátedra directa** |

> **Menos unario sin fuente confirmada.** El nivel 7 de la tabla lo incluye, pero el único ejemplo que teníamos era `recta.paed:39` (`dommin := -4`), ya descartado. La wiki muestra negativos en prosa (`OnlySintaxis.md:19` "pueden ser negativos y positivos") pero no en una expresión. **Verificar antes de implementar el nivel 7.**

---

## 5. Reglas léxicas

- **Comentarios:** `//` hasta fin de línea (`OnlySintaxis.md:36`). El PAED declarativo usaba `#`; ya no es válido.
- **Identificadores:** letra o `_` inicial, luego letras, dígitos o `_`. Ejemplos reales: `t_label_x`, `mi_primer_algoritmo` (`OnlySintaxis.md:41`).
- **Keywords:** búsqueda case-insensitive (§3.1).
- **Números:** `[0-9]+` con parte decimal opcional `.` `[0-9]+` (§3.6).
- **Cadenas:** `"` ... `"`. Sin escapes por ahora — la wiki no los usa.
- **Caracteres:** `'` un carácter `'`.
- **UTF-8:** todas las keywords son ASCII puro. Caracteres no-ASCII (`ñ`, tildes) solo pueden aparecer dentro de cadenas y comentarios. Esto evita el problema de `interpreter.c:61`, que comparaba la clave `"tamaño"` byte a byte.
- **Espacios y saltos de línea:** no significativos. La indentación es estética.

---

## 6. Funciones nativas del motor — Fase G

Reemplazan la sintaxis declarativa de v1. Se invocan como cualquier llamada de PAED.

| Firma | Reemplaza a | Puente |
|---|---|---|
| `crear_cubo(x, y, color): ENTERO` | `cubo nombre= posicion= color=` | `world_spawn` — `engine/engine.h:61` |
| `mover(id, dx, dy)` | `mover <id> a=` | `Entity.x/.y` — `engine/engine.h:29` |
| `color(id, valor)` | `color <id> valor=` | `Entity.color` — `engine/engine.h:32` |
| `tecla(cod): BOOLEANO` | — (no existía) | `Renderer.key_down` — `plugins/renderer/renderer.h:57` |
| `fondo(color)` | `fondo color=` | `World.clear_color` — `engine/engine.h:49` |

Ninguna de estas es sintaxis: son identificadores resueltos en una tabla de nativas. El lenguaje no las conoce.

---

## 7. Pendiente de revisión con la cátedra

Reclasificado el 2026-08-07 bajo la regla de §0.1: **solo cátedra y wiki son autoridad.** Muchos de los "conflictos" anteriores no lo eran — eran bugs de herramientas sin autoridad, y no bloquean el diseño del lenguaje.

### 7.A — BLOQUEANTE: cátedra y wiki se contradicen

Un solo punto, y es el único que frena la Fase A.

| # | Punto | Cátedra dice | Wiki dice |
|---|---|---|---|
| 12 | **Cierre de bloque** | `FIN ACCION` **con espacio** — `AED_2021_UnI.pdf:10` | `FIN_ACCION` **con guión bajo** — `OnlySintaxis.md:14` |

Costo en el lexer:

- **Guión bajo** (`FIN_ACCION`): una keyword, un token. Cero trabajo extra.
- **Espacio** (`FIN ACCION`): **dos tokens**. Obliga a lookahead en el parser o caso especial en el lexer. Es la única forma que cuesta código.

Las otras formas (`FinAccion`, `finaccion`) venían de `~/apuntes/AED`, que **ya no es autoridad** — son notas de práctica. Quedan descartadas.

Falta contrastar con los PDF no leídos (§7.2). Si `AED_2018_UnI_B.pdf` o el resto usan guión bajo, el conflicto se cierra a favor de la wiki.

### 7.B — Resueltos por la cátedra

| # | Punto | Resolución | Fuente |
|---|---|---|---|
| 5 | Comillas | `'...'` **comilla simple** | `AED_2021_UnI.pdf:10` |
| 11 | Separador del `PARA` | **`HASTA`** | wiki + cátedra, decidido 2026-08-07 |
| 15 | Declaración múltiple | `A,B,SUMA: entero` válido | `AED_2021_UnI.pdf:10` |
| 6 | `;` | **Separador**, no terminador — la última sentencia no lo lleva | `AED_2021_UnI.pdf:10` |

### 7.C — Abiertos, sin bloquear (sintaxis de fases diferidas)

| # | Punto | Evidencia | Fase |
|---|---|---|---|
| 14 | ¿`VARIABLES` es obligatoria dentro de `AMBIENTE`? | `AED_2021_UnI.pdf:10` la usa; `OnlySintaxis.md:10` no | D |
| 7 | `ES` con dos significados: cierre de `ACCION` y modo de parámetro | `OnlySintaxis.md:9` vs `:91` | H |
| 8 | `..` de rango: ¿token propio o dos `PUNTO`? | `OnlySintaxis.md:121` | D |
| 9 | `*p.Dato`: ¿`(*p).Dato` o `*(p.Dato)`? | `OnlySintaxis.md:546` | J |
| 10 | `EN` sin definición — probablemente prosa | `OnlySintaxis.md:331` | — |

### 7.D — NO son conflictos de la spec: bugs de herramientas

Sin autoridad, no afectan el diseño del lenguaje. Se arreglan aparte, cuando convenga.

| Herramienta | Le falta |
|---|---|
| `Frankly/syntaxes/paed.tmLanguage.json` | `FIN_REGISTRO`, `SUBACCION`, `FDA`, `NFDA`, `Verdadero`, `Falso`, `nil`; usa comillas dobles |
| `Frankly/data/sintaxis.json` | ídem; además quedó sin consumidor |
| `PseudoGames/src/editorText.c:134-163` | ídem, desincronizada de las otras dos |
| `Frankly/paed` | Acepta `PARA ... a ...` y comillas dobles — **es permisivo de más** |
| Neovim | Sin resaltador desde que se removió `syntax/paed.lua` |

Descartados por falta de autoridad: `DESDE ... HASTA` (comentarios), `MIENTRAS(cond) HACER` (solo Frankly), `;` obligatorio (solo `core/validacion.sh:23`).

Contra-evidencia a considerar: `Frankly/DOC.txt:25` fija `FIN_ACCION` con guión bajo en la definición formal, y `Frankly/paed:92` solo matchea esa forma. Si v2 debe seguir corriendo los `.paed` de Frankly sin tocarlos, el guión bajo es obligatorio — pero eso no impide aceptar además las otras.

### 7.1 Evidencia de primera mano de la cátedra

`AED_2021_UnI.pdf`, página 10 — ejemplo canónico de pseudocódigo publicado por los docentes:

```
ACCION SUMA ES
AMBIENTE
    VARIABLES
        A,B,SUMA: entero
PROCESO
    ESCRIBIR('Ingrese 2 números');
    LEER(A,B);
    SUMA := A + B;
    ESCRIBIR('El resultado es', SUMA)
FIN ACCION
```

Seis hechos que este ejemplo establece, y que ninguna otra fuente del repo registraba:

| # | Hecho | Efecto en la spec |
|---|---|---|
| A | **`FIN ACCION` se escribe con ESPACIO** | **Cuarta forma** de cierre. Ni `FIN_ACCION` (repo) ni `FinAccion` (apuntes). Ver §7.12 |
| B | **`VARIABLES` es sub-sección de `AMBIENTE`** | Keyword nueva, ausente de `tmLanguage.json` y de `OnlySintaxis.md` |
| C | **Declaración múltiple: `A,B,SUMA: entero`** | La gramática de §4 solo admite un identificador por declaración. **Hay que corregirla** |
| D | **Los tipos van en minúscula (`entero`)** | Refuerza la decisión §3.1 (keywords case-insensitive) |
| E | **Las cadenas usan COMILLA SIMPLE**: `'Ingrese 2 números'` | **Resuelve el conflicto §7.5**: `'...'` es cadena, no solo carácter |
| F | **La última sentencia no lleva `;`** | El `;` es **separador**, no terminador. Afina §7.6 |

Corrección de la gramática por el hecho C:

```ebnf
declaracion = IDENT { "," IDENT } ":" tipo [ ";" ] ;
ambiente    = "AMBIENTE" [ "VARIABLES" ] { registro | constante | declaracion } ;
```

> El PDF es una captura de Sublime Text dentro del material, no un BNF formal. Es la mejor evidencia disponible, pero conviene contrastarla con `AED_2018_UnI_B.pdf` y las resoluciones de parcial antes de congelar la decisión.

### 7.2 Fuentes de la cátedra sin explotar

Material disponible que puede resolver los 13 puntos, aún no leído:

| Ruta | Formato |
|---|---|
| `~/apuntes/AED/Teoria/*.pdf` (6 archivos: Subacciones, Indexados, UnI, UnII, Registros-Archivos) | PDF |
| `~/apuntes/AED/Teoria/TEMAS_7-10_Registros_Archivos.md` (429 líneas) | Markdown |
| `~/apuntes/AED/patronesAED.md` (59 líneas) | Markdown |
| `~/apuntes/AED/Teoria/Parcial1.adoc` | AsciiDoc |
| `~/apuntes/AED/Simulacros/` — resoluciones de parcial | varios |
| `~/apuntes/AED/Teoria/EjerciciosParaVerSintaxis/` — 3 repos externos de sintaxis AED | varios |

**Los PDF de la cátedra son la autoridad final** ante cualquier discrepancia entre wiki, repo y apuntes.

---

## 8. Corpus de test

**Los `.paed` existentes NO sirven como corpus.** Fueron escritos contra Frankly, no contra la cátedra, y contienen sintaxis inválida.

### 8.1 `recta.paed` está mal — no es test de integración

`PseudoGames/AlgebraRectas/recta.paed` corre en Frankly (verificado: imprime la salida completa sin errores), pero eso **no lo hace correcto** — solo prueba que Frankly es permisivo.

Errores confirmados contra la cátedra:

| Línea | Escrito | Correcto | Fuente |
|---|---|---|---|
| `:47` | `PARA coord := -4 a 4 HACER` | `PARA coord := -4 HASTA 4 HACER` | wiki + cátedra |
| `:29-37` | `t_eje_x := "EJE_X";` (comillas dobles) | `'EJE_X'` (comillas simples) | `AED_2021_UnI.pdf:10` |
| `:2` | `AMBIENTE` sin `VARIABLES` | `AMBIENTE` / `VARIABLES` | `AED_2021_UnI.pdf:10` |

> Por qué Frankly lo acepta: `paed:437-439` lee el `PARA` **por posición** (`$2`, `$4`, `$6`) y **nunca examina el separador** (`$5`). Verificado empíricamente — `PARA i := 1 HASTA 3 HACER` produce `1 2 3` idéntico. El separador le da igual.

### 8.2 El corpus hay que construirlo

No existe hoy ningún `.paed` verificado contra la cátedra. **Antes de la Fase D hay que escribir uno**, derivado del único ejemplo oficial disponible (`AED_2021_UnI.pdf:10`):

```
ACCION SUMA ES
AMBIENTE
    VARIABLES
        A,B,SUMA: entero
PROCESO
    ESCRIBIR('Ingrese 2 números');
    LEER(A,B);
    SUMA := A + B;
    ESCRIBIR('El resultado es', SUMA)
FIN ACCION
```

Ese es el **único** fragmento de PAED en todo el proyecto con autoridad de cátedra confirmada. Todo test de integración parte de ahí.

| Archivo | Estado |
|---|---|
| `PseudoGames/Frankly/tests/hola_mundo.paed` | Sin verificar contra cátedra — revisar antes de usar |
| `PseudoGames/AlgebraRectas/recta.paed` | **Inválido**, ver §8.1 |
| corpus de cátedra | **NO EXISTE — hay que crearlo** |

**Regla de oro:** ninguna construcción entra a esta spec sin un `.paed` que la ejercite y corra. v1 se rompió justamente por eso — `_void/docs/paed_spec_v1.md:29` documentaba `mover <id> a=<x,y,z>` mientras `plugins/ide/interpreter.c:69-70` leía `nombre=` y `posicion=`, y `escalar`/`girar`/`oscilar` quedaron documentados pero nunca implementados.
