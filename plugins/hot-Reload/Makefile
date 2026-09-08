CC       = clang
# -I. para que los .c de game/ encuentren platform.h, que vive un nivel arriba.
CFLAGS   = -Wall -Wextra -g -I.
DEPFLAGS = -MMD -MP

# Fuentes del juego: TODO lo que este en game/, en cualquier subcarpeta.
# No hay lista de exclusiones y no hace falta: la ubicacion es el criterio.
# Lo que va en lab/ (demos, pruebas, punteros) queda afuera solo, y agregar
# un lab/loQueSea.c no obliga a tocar este archivo ni watch.sh.
GAME_DIR   := game
GAME_SRCS  := $(shell find $(GAME_DIR) -name '*.c')
GAME_OBJS  := $(GAME_SRCS:.c=.o)
GAME_DEPS  := $(GAME_OBJS:.o=.d)

all: host game.so

# El host se compila una sola vez y no se toca mas mientras jugas.
# Depende de platform.h (lo incluye), NO del codigo del juego: si dependiera
# de eso, cada edicion relinkearia el host y perderias la partida.
host: host.c platform.h
	$(CC) $(CFLAGS) -o host host.c -ldl

# Cada .c del juego se compila a su .o por separado. -MMD -MP le pide a clang
# que anote, junto al .o, un .d con los headers que ESE .c uso de verdad.
# Ese .d se incluye mas abajo: asi Make sabe que si cambias un .h en cualquier
# parte, hay que recompilar el .o que lo usa, sin mantener la lista a mano.
%.o: %.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -fPIC -c $< -o $@

# Se linkean todos los .o del juego juntos. Se compila a un temporal y se
# mueve solo si salio bien: si compilaras directo sobre game.so, un error te
# deja un archivo trunco y el host muere con "file too short" en la proxima
# recarga.
#
# -lm: la biblioteca matematica (sqrtf y companiia). Sin esto compila igual,
# porque un .so admite simbolos sin resolver, y el error recien aparece cuando
# el host hace dlopen: "undefined symbol: sqrtf". Se linkea aca y no en el host
# para que game.so se banque solo sus dependencias.
game.so: $(GAME_OBJS)
	$(CC) $(CFLAGS) -shared -fPIC -o game.so.tmp $(GAME_OBJS) -lm && mv game.so.tmp game.so

# Recompila solo al guardar, como el plugin de Unity. Corre en otro panel.
# Vigila todo el arbol de game/, no un solo archivo.
watch: watch.sh
	./watch.sh

# Los .o/.d se borran con find y no con $(GAME_OBJS): asi tambien se limpian
# los que quedaron huerfanos de un .c que renombraste o borraste, que la
# variable ya no conoce.
clean:
	rm -f host game.so game.so.tmp game_live.so
	find $(GAME_DIR) \( -name '*.o' -o -name '*.d' \) -delete

# Incluye los .d generados en la build anterior. "-include" (con guion) no
# falla si todavia no existen, por ejemplo la primera vez que se compila.
-include $(GAME_DEPS)

.PHONY: all clean watch
