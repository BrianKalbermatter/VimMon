# Librería `escena` — v1.0

**Esto NO es parte del lenguaje PAED.** Es una librería de VimMon que agrega
procedimientos propios, igual que `stdlib/matematica.sh` agrega funciones al
intérprete bash. El pseudocódigo AED de la cátedra no conoce ninguno de estos
nombres.

Definición formal: [`../data/escena.json`](../data/escena.json).
El parser la carga **además** de `sintaxis.json`. Si el archivo no está, PAED
sigue andando como AED puro.

---

## Convención propia: argumentos con nombre

AED llama posicional (`AVZ(sec, ventana)`). Esta librería usa argumentos con
nombre:

```
PROCEDIMIENTO(clave = valor, clave = valor);
```

Es una convención **de la librería**, no una regla del lenguaje. La razón: una
entidad de escena se referencia siempre igual, la cree o la modifique, y con
diez parámetros opcionales el orden posicional es inmanejable.

Los vectores van entre paréntesis: `posicion = (0,2,5)`. Los paréntesis le
permiten al parser distinguir la coma que separa las componentes del vector de
la coma que separa los argumentos.

## Crear entidades

```
CUBO  (nombre = <id>, posicion = (x,y,z), color = #hex, tamano = (x,y,z));
ESFERA(nombre = <id>, posicion = (x,y,z), color = #hex, radio  = n);
PLANO (nombre = <id>, posicion = (x,y,z), color = #hex, tamano = (x,y,z));
LUZ   (nombre = <id>, posicion = (x,y,z), tipo = puntual|dir, intensidad = 0.0-1.0);
```

## Modificar

```
MOVER  (nombre = <id>, posicion = (x,y,z));
ROTAR  (nombre = <id>, eje = x|y|z, angulo = grados);
ESCALAR(nombre = <id>, factor = n);
COLOR  (nombre = <id>, color = #hex);
```

## Comportamientos

```
GIRAR  (nombre = <id>, eje = x|y|z, velocidad = n);
OSCILAR(nombre = <id>, amplitud = n, frecuencia = n);
```

## Global

```
CAMARA(posicion = (x,y,z), mirar = (x,y,z));
FONDO (color = #hex);
```

## Ejemplo

```paed
ACCION escena ES
    AMBIENTE
        cubo1: CUBO;
    PROCESO
        FONDO (color = #1a1a2e);
        CAMARA(posicion = (0,2,5), mirar = (0,0,0));
        CUBO  (nombre = cubo1, posicion = (0,0,0), color = #ff0000, tamano = (1,1,1));
FIN_ACCION
```

`CUBO` como tipo en `AMBIENTE` es también de la librería: el parser guarda el
tipo como texto sin validarlo, así que no choca con los tipos de AED.

## Reglas para la IA

La IA genera SOLO instrucciones, sin el envoltorio `ACCION`/`PROCESO`/
`FIN_ACCION`. El plugin `ide` las inserta antes de `FIN_ACCION`.

- Si la entidad ya existe → `MOVER`, `COLOR`, `ESCALAR`, `ROTAR`.
- Si es nueva → el procedimiento de creación completo.
- Nunca repetir lo que ya está en la escena.
