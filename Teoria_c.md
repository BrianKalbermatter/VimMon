# STRINGS
En C, las cadenas son arreglos de caracteres que terminan con un caracter especial \0 (llamado "null terminator" o terminador nulo). Este caracter le dice a las funciones "aqui termina la cadena".

## Ejemplo:
Cuando escribo:
char nombre[] = "Hola";

En memoria se ve asi:
índice:   [0]  [1]  [2]  [3]  [4]
valor:    'H'  'o'  'l'  'a'  '\0'

El \0 ocupa una posicion extra. Por eso "   Hola" tiene 4 letras pero ocupa 5 bytes.
Como funciona strcat
strcat(destino, fuente)hace esto:

1. Busca el \0 en destino
2. Empieza a copiar fuente desde esa posicion
3. Pone un \0 al final

El problema que a veces se presenta es:
```bash
    char final[50]; // Sin inicializar!
```
En memoria tienes basura aleatoria:
[g] [&] [�] [�] [t] [\0] [?] [?] ...  ← basura
                    ↑
              strcat encuentra el \0 aquí

strcat encontró un \0 aleatorio en posición 5, y puso "Brian" después de la basura.

Con inicialización

char final[50] = "";

[\0] [?] [?] [?] ...  ← vacío, listo para usar
 ↑
strcat empieza aquí

Ahora strcat encuentra el \0 en posición 0 y concatena desde el inicio.              
