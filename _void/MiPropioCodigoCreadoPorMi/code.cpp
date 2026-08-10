/*  Problema: "El Cifrador de Mensajes"

  Descripcion

  Eres el ingeniero de seguridad de una empresa. Debes implementar un sistema de cifrado modular donde diferentes
  estrategias de cifrado se aplican secuencialmente a un mensaje.

  Tu sistema recibe un mensaje y una lista de operaciones. Cada operacion aplica una estrategia distinta. Al final,
  imprime el mensaje resultante y su longitud.

  Estrategias disponibles

  ┌────────┬──────────┬───────────────────────────────────────────────────────────────┐
  │ Codigo │  Nombre  │                          Descripcion                          │
  ├────────┼──────────┼───────────────────────────────────────────────────────────────┤
  │ ROT    │ Rotacion │ Desplaza cada letra N posiciones en el alfabeto (solo letras) │
  ├────────┼──────────┼───────────────────────────────────────────────────────────────┤
  │ REV    │ Reverso  │ Invierte el string completo                                   │
  ├────────┼──────────┼───────────────────────────────────────────────────────────────┤
  │ DUP    │ Duplicar │ Duplica cada caracter ("ab" → "aabb")                         │
  ├────────┼──────────┼───────────────────────────────────────────────────────────────┤
  │ REM    │ Remover  │ Elimina todas las vocales del mensaje                         │
  └────────┴──────────┴───────────────────────────────────────────────────────────────┘

  Entrada

  Linea 1: El mensaje inicial (sin espacios)
  Linea 2: Q (numero de operaciones, 1 <= Q <= 20)
  Siguientes Q lineas: CODIGO [arg]

  Salida

  Mensaje final
  Longitud: N

  Ejemplo

  Entrada:
  HelloWorld
  3
  ROT 3
  REV
  REM

  Salida:
  guriKhoor -> despues ROT3: KhoorZruog -> REV: goruZrooK -> REM: grZrK
  */


