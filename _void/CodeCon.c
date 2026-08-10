//Primero lo pensare en pseudocodigo
/*---
  "El Almacén de Cajas"

  Tienes un almacén representado como una cuadrícula. Hay un robot (@), cajas (O), paredes (#) y espacios vacíos (.).

  ########
  #..O.O.#
  ##@.O..#
  #...O..#
  #.#.O..#
  #...O..#
  #......#
  ########

  El robot recibe una secuencia de movimientos: ^ (arriba), v (abajo), < (izquierda), > (derecha).

  Reglas:
  - El robot se mueve en la dirección indicada.
  - Si hay una caja en esa dirección, el robot la empuja (si hay espacio libre detrás).
  - Si hay una pared (o no hay espacio), el robot no se mueve.

  Input:
  <^^>>>vv<v>>v<<

  Parte 1: Después de todos los movimientos, calcula la suma de coordenadas GPS de todas las cajas.
  GPS de una caja = 100 * fila + columna

  Parte 2 (harder): ¿Y si las cajas tuvieran el doble de ancho ([])? Reescala el mapa y repite.

  ---
  Pista de solución

  # Estructura básica
  def mover(grid, robot, direccion):
      dr, dc = direccion
      r, c = robot
      nr, nc = r + dr, c + dc

      if grid[nr][nc] == '#':
          return robot  # no se mueve

      if grid[nr][nc] == 'O':
          # intentar empujar la caja
          ...

      return (nr, nc)

  ---
 *
 *
 *
 *
 * PSEUDOCODIGO
 * 
 * ACCION juegoDelRobotAtrapado ES
 *      
 *
 * FUNCION mover (tabla, robot, direccion ) ES
 *      
 *
 * FIN_FUNCION
 *
 * FIN_ACCION
 *
 * */



