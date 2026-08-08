# El evaluador de expresiones de PAED

El parser guarda las condiciones y las asignaciones **crudas**, como texto.
Sabe que `MIENTRAS (cont < 4) HACER` es un bucle y a dónde saltar, pero no sabe
si hay que entrar. Eso lo decide `plugins/ide/expr.c`.

```
"cont < 4"  --expr_eval-->  Valor{tipo=LOGICO, logico=1}
```

## Probarlo

```
build/vimmon
vimmon> scene
```

O directamente sobre un `.paed` cualquiera, con `interp_exec`. Un programa que
hoy corre entero:

```paed
ACCION tabla ES
    PROCESO
        PARA i := 1 HASTA 5 HACER
            ESCRIBIR("7 x ", i, " = ", 7 * i);
        FIN_PARA

        suma := 0;
        n := 1;
        MIENTRAS (n <= 10) HACER
            suma := suma + n;
            n := n + 1;
        FIN_MIENTRAS
        ESCRIBIR("1..10 suman ", suma);

        SI (suma = 55) ENTONCES
            ESCRIBIR("Gauss tenia razon");
        FIN_SI
FIN_ACCION
```

## Descenso recursivo: la prioridad son las llamadas

Una función por nivel de prioridad, y cada una llama a la de **mayor**
prioridad que ella. La tabla sale de `TEORIA_COMPLETA.txt:361-371`:

| Prioridad | Operadores | Función |
|---|---|---|
| 9 (menor) | `O` `OR` | `eval_o` |
| 8 | `Y` `AND` | `eval_y` |
| 7 | `=` `<>` | `igualdad` |
| 6 | `<` `<=` `>` `>=` | `relacional` |
| 4 y 5 | `+` `-` (y concatenación) | `suma` |
| 3 | `*` `/` `DIV` `MOD` | `producto` |
| 2 | `**` | `potencia` |
| 1 (mayor) | `+` `-` `NO` unarios | `unario` |
| — | literales, variables, funciones, `( )` | `primario` |

**No hay tabla de números en el código.** La prioridad *es* el orden de las
llamadas: cuando `suma()` pide su operando derecho, llama a `producto()`, que
ya se comió el `3 * 4`. Por eso `2 + 3 * 4` da 14 y no 20.

La potencia es asociativa a **derecha**, así que se llama a sí misma en el lado
derecho: `2 ** 3 ** 2` es `2 ** (3 ** 2)` = 512, no `(2 ** 3) ** 2` = 64.

### Ojo con el menos unario

Por la tabla de AED, los unarios tienen **más** prioridad que `**`. Entonces
`-2 ** 2` da **4**, no -4. En la mayoría de los lenguajes da -4, porque ahí la
potencia liga más fuerte que el signo. Acá se siguió la tabla de la cátedra.

## Cortocircuito: no es una optimización

`TEORIA_COMPLETA.txt` lo dice textual:

> *"En AND, si el primer operando es Falso, el segundo no se evalúa."*

Eso **cambia el comportamiento**, no solo la velocidad. En
`i > 0 Y A[i] = 3`, si no cortara, evaluar `A[i]` con `i` inválido sería un
error de verdad. `eval_y` y `eval_o` recorren el lado derecho para avanzar el
cursor, pero con el contexto marcado como fallado para que no ejecute ni
reporte nada de ahí.

## Mirar antes de consumir

Dos trampas resueltas leyendo sin avanzar el cursor:

- Si `<` se probara antes que `<=`, la expresión `a <= b` se leería como
  `a < (= b)`.
- `<>` pertenece a otro nivel (`igualdad`), así que `relacional` tiene que
  dejar pasar un `<` cuando lo sigue un `>`.

Lo mismo con `*` y `**`.

## Tipos

| Tipo | Cuándo |
|---|---|
| `VAL_NUM` | enteros y reales; al evaluar no se distinguen |
| `VAL_TEXTO` | `"hola"` y `'a'` |
| `VAL_LOGICO` | `V` / `F`, y el resultado de toda comparación |

Las comparaciones entre textos usan **ASCII**, como manda la teoría:
`'A' < 'K'` es verdadero y `'MARIA' < 'JUAN'` es falso.

El `+` también concatena (prioridad 5 de la tabla). Se decide por el tipo: si
alguno de los dos lados es texto, se pegan.

## El intérprete sigue los saltos

`interp_exec` dejó de recorrer el array de punta a punta. Ahora lleva un
índice y lo mueve, que es **literalmente un contador de programa**:

```c
case PAED_SI:            i = condicion(...) ? i + 1 : in->salto;  break;
case PAED_SINO:          i = in->salto;                           break;
case PAED_MIENTRAS:      i = condicion(...) ? i + 1 : in->salto;  break;
case PAED_FIN_MIENTRAS:  i = in->salto;                           break;
```

Llegar a un `SINO` **ejecutando** significa que terminó la rama verdadera, así
que hay que saltearse la rama del `SINO`.

### El PARA y su marca

Un `PARA` inicializa su variable la primera vez, pero no en cada vuelta — y el
`FIN_PARA` salta de vuelta al `PARA`. Por eso hay un `para_activo[]` **por
instrucción**: con una sola bandera, dos `PARA` anidados se pisarían. Al salir
del bucle la marca se apaga, para que un `PARA` adentro de otro vuelva a
arrancar en la siguiente vuelta del externo.

El corte usa el signo del paso: con paso positivo termina al pasarse del final,
con paso negativo al bajar de él. Sin eso, un `PARA` en reversa no terminaría
nunca.

## Guarda de bucle infinito

`PAED_MAX_PASOS` (2.000.000) corta y avisa. **Esto no es opcional**: el
intérprete corre dentro del game loop, así que un programa colgado cuelga la
ventana entera y hay que matar el proceso sin saber por qué.

## Lo que todavía no hace

- **No hay tipos declarados.** El `AMBIENTE` se parsea pero no se usa para
  chequear: asignarle un texto a algo declarado `ENTERO` no da error.
- **Se re-parsea en cada evaluación.** La condición de un `MIENTRAS` se lee de
  texto en cada vuelta. Es simple y correcto, pero un bucle largo paga ese
  costo. La solución es guardar un árbol una sola vez.
- **`NFDS` y `FDS` no funcionan**: preguntan por secuencias, que el intérprete
  no tiene. Se avisa en vez de inventar un valor.
- **No hay arreglos ni campos de registro**: `A[i]` y `pori.vx` no se evalúan.
- **`==` se acepta como sinónimo de `=`** porque aparece en los ejercicios,
  aunque la teoría solo define `=`. Falta confirmar si es válido.
