# Tutorial de Algoritmos y Estructura de Datos

---

## Unidad 1 — El hardware que ejecuta tu código

Antes de escribir una sola línea de código, hay que entender **dónde** corre ese código y **cómo** lo maneja la máquina.

### El procesador (CPU)
Es el cerebro de la computadora. Ejecuta instrucciones una por una, a velocidades altísimas. Cuando escribís un algoritmo, básicamente le estás dando órdenes al procesador en un lenguaje que él pueda entender.

### Memoria interna — la RAM
La RAM es la memoria de trabajo. Cuando ejecutás un programa, todo lo que ese programa necesita (variables, resultados parciales, instrucciones) se carga en la RAM. Es rápida, pero es temporal: cuando el programa termina o apagás la computadora, todo lo que había ahí desaparece.

Pensala como el escritorio de tu cuarto: es el espacio donde trabajás, pero no guardás las cosas ahí para siempre.

### Memoria externa — el disco (HDD/SSD)
El disco es donde se guardan los archivos de forma permanente. Cuando guardás un documento o instalás un programa, va al disco. Es más lenta que la RAM, pero persiste aunque apagues todo.

Siguiendo la analogía: el disco es el cajón donde guardás las cosas cuando terminás de trabajar.

### ¿Dónde vive una variable?
Cuando en tu algoritmo escribís en el ambiente `salario : ENTERO;`, lo que hacés es reservar un espacio en la **RAM** con el nombre `salario` y guardar ahí el valor `que despues le daras en una asignacion dentro del proceso`. Ese espacio, que creaste en el ambiente, existe solo mientras el programa corre.

### ¿Y los archivos?
Los archivos (datos que guardás en disco) son otra cosa. Están en la memoria externa y hay que pedirle explícitamente al programa que los abra, los lea o los escriba.

---

## Unidad 2 — Las 4 áreas del análisis previo

**Antes de escribir pseudocódigo, siempre hay que pensar.** Este análisis se hace a mano, en papel si hace falta, y siempre en este orden:

### 1. Entrada
¿Qué datos le llegan al algoritmo desde afuera? ¿Qué le pide el usuario? ¿Qué archivos necesita?

> Ejemplo: Para calcular el salario de un empleado, la entrada es la cantidad de horas trabajadas y el valor de la hora.

### 2. Salida
¿Qué resultado tiene que mostrar o producir el algoritmo?

> Ejemplo: El salario total calculado, mostrado por pantalla.

### 3. Ambiente
¿Qué variables internas vamos a necesitar? ¿Qué espacio en la RAM vamos a reservar?

> Ejemplo: `horas`, `valorHora`, `salario`.

### 4. Proceso
¿Cómo transformamos la entrada en la salida? Acá describís los pasos del algoritmo.

> Ejemplo: `salario := horas * valorHora`

**Este orden no es opcional.** Si lo saltás, vas a terminar escribiendo algoritmos que no tienen sentido o que te falta algo a mitad de camino.

---

## Unidad 3 — Los lenguajes de programación y el pseudocódigo

### Cada lenguaje tiene su lugar
No existe un lenguaje que sea "el mejor". Cada uno está pensado para resolver algo específico:

- Para hacer páginas web existen los navegadores, HTML, JavaScript, frameworks, APIs.
- Para sistemas de bajo nivel (cerca del hardware) hay C, Assembly.
- Para ciencia de datos, Python.
- Para aplicaciones móviles, Swift o Kotlin.

No tiene sentido programar una página web desde el procesador directamente. Para eso existen las herramientas adecuadas, incluyendo hoy en día la IA, que te ayuda a pensar y a construir lo que imaginás.

### La creatividad importa
Esta área premia a quienes se animan a crear. Si te esforzás en aprender y en construir, la recompensa existe y es real.

### El pseudocódigo
El pseudocódigo es un lenguaje conceptual. No lo ejecuta ninguna computadora directamente, pero te permite diseñar cualquier algoritmo de la vida real: cálculos, búsquedas, ordenamientos, recursividad, complejidad.

**Los algoritmos se aprenden usándolos.** No hay un libro que te prepare para todo. La experiencia de aplicarlos en el día a día es lo que te lleva a entenderlos de verdad.

---

## Unidad 4 — Tipos de datos

```
Tipos de Datos
├── Simples
│   ├── Numéricos
│   │   ├── Enteros       (1, 2, -5, 100)
│   │   └── Reales        (3.14, -0.5, 1000.99)
│   ├── Alfanuméricos
│   │   └── Caracteres / Cadena de texto   ("Hola", 'A')
│   │       ⚠️  En pseudocódigo NO existe la cadena de texto como tal
│   │           porque no se puede concatenar. Usarlo así en la
│   │           práctica es incorrecto para las actividades.
│   └── Booleanos
│       └── Solo dos valores posibles: Verdadero (V) o Falso (F)
└── Estructurados
    (se verán más adelante en la materia)
```

### Los booleanos y el procesador
Todo en la computadora, en el fondo, es un booleano. El procesador solo entiende Verdadero o Falso, 1 o 0. Cuando escribís un bucle o una condición, lo que está pasando adentro es que el procesador evalúa algo y obtiene V o F. De ahí parte toda la lógica.

### Palabras reservadas
Son palabras que el lenguaje ya usa para sus propias instrucciones. No podés usarlas como nombres de variables porque generaría error de sintaxis. Ejemplos en pseudocódigo: `SI`, `MIENTRAS`, `PARA`, `FIN`, `INICIO`, etc.

### Funciones comunes en pseudocódigo

**ESCRIBIR** — muestra algo por pantalla:
```
ESCRIBIR("Bienvenido al sistema");
```
- Todo el contenido va entre comillas dobles `""`.
- Si querés que algo aparezca literalmente entre comillas en pantalla, usás comillas simples `''` adentro:
```
ESCRIBIR("El nombre es 'Juan'");
```
- Mezclar mal las comillas = error de sintaxis. Es incorrecto.

**LEER** — recibe un valor del usuario y lo guarda en una variable:
```
LEER(salario);
```
- Lo que hace por detrás: reserva un espacio en la RAM con el nombre de la variable y guarda ahí lo que el usuario escriba.
- A partir de ese momento podés usar esa variable en el resto del algoritmo.

---

## Unidad 5 — Acumuladores

Un acumulador es una variable que **se modifica a sí misma** con cada operación. La idea es que va "acumulando" un valor a medida que el algoritmo avanza.

### Ejemplo paso a paso

```
Salario := 10000        // Reservo la variable y le asigno 10000
Salario := Salario + 3  // Ahora Salario vale 10003
Salario := Salario / 2  // Ahora Salario vale 5001.5
```

Lo importante: **el valor final depende completamente del valor inicial.**

Si arrancás con 10000 y después sumás 3 y dividís por 2, el resultado (5001.5) es más chico que el valor original (10000). El acumulador no "recuerda" que antes era 10000, simplemente opera sobre lo que tiene en ese momento.

Esto es fundamental para entender bucles, donde el acumulador se va modificando en cada iteración.

---

## Unidad 6 — Acciones

```
Acciones
├── Simples
│   ├── Elementales
│   │   └── Instrucciones básicas como ESCRIBIR o LEER
│   └── Asignaciones
│       ├── Puras         → Le das un valor fijo a una variable
│       │   Ejemplo:  Salario := 10000
│       └── De expresión  → El valor viene de una operación
│           Ejemplo:  Salario := Salario + 3
└── Complejas
    ├── Con nombres
    │   └── Procedimientos o funciones que tienen un nombre
    │       y se pueden llamar desde otras partes del algoritmo
    ├── Condiciones
    │   └── SI (condición) ENTONCES ... FIN_SI
    │       Evalúan un booleano y ejecutan algo según V o F
    └── Repetitivas / Bucles
        └── MIENTRAS, PARA, REPETIR
            Ejecutan un bloque de código mientras una condición
            sea Verdadera. Cada iteración evalúa un booleano.
```


### Las condiciones y los bucles son booleanos
Cada vez que escribís un `SI` o un `MIENTRAS`, el algoritmo evalúa una condición que da V o F. Si es Verdadero, entra al bloque. Si es Falso, lo saltea o termina el bucle. No hay más opciones. El procesador no entiende "tal vez".


Etapa 2:
