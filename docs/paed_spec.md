# PAED — Game Description Language v1.0

Lenguaje declarativo para describir escenas de videojuego.
Diseñado para ser generado por IA y ejecutado por el intérprete.

## Reglas

- Una instrucción por línea
- `#` inicia un comentario
- Todo en minúsculas
- Parámetros con `clave=valor`
- `nombre=` es obligatorio en entidades nuevas
- Coordenadas: `x,y,z` sin espacios

---

## Entidades

```
cubo    nombre=<id> posicion=<x,y,z> color=<#hex> tamaño=<x,y,z>
esfera  nombre=<id> posicion=<x,y,z> color=<#hex> radio=<r>
plano   nombre=<id> posicion=<x,y,z> color=<#hex> tamaño=<x,y>
luz     nombre=<id> posicion=<x,y,z> tipo=<puntual|dir> intensidad=<0.0-1.0>
```

## Modificaciones

```
mover   <id> a=<x,y,z>
rotar   <id> eje=<x|y|z> angulo=<grados>
escalar <id> factor=<n>
color   <id> valor=<#hex>
```

## Comportamientos

```
girar   <id> eje=<x|y|z> velocidad=<n>
oscilar <id> amplitud=<n> frecuencia=<n>
```

## Escena global

```
camara posicion=<x,y,z> mirar=<x,y,z>
fondo  color=<#hex>
```

---

## Ejemplo completo

```paed
# escena inicial
fondo color=#1a1a2e
camara posicion=0,2,5 mirar=0,0,0

cubo   nombre=cuerpo posicion=0,0,0 color=#e63946 tamaño=1,1,1
esfera nombre=ojo_izq posicion=-0.3,0.4,0.5 color=#ffffff radio=0.15
esfera nombre=ojo_der posicion=0.3,0.4,0.5  color=#ffffff radio=0.15

girar cuerpo eje=y velocidad=0.5
```

---

## Reglas para la IA

Cuando el usuario pide algo, la IA genera SOLO el delta:
- Si el objeto ya existe en scene.paed → usar `mover`, `color`, etc.
- Si es nuevo → usar la instrucción de entidad completa
- Nunca repetir lo que ya está en scene.paed
