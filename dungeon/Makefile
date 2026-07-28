CC      = clang
CFLAGS  = -Wall -Wextra -g

all: host game.so

# El host se compila una sola vez y no se toca mas mientras jugas.
# Depende de platform.h (lo incluye), NO de dungeon.c: si dependiera del
# codigo del juego, cada edicion relinkearia el host y perderias la partida.
host: host.c platform.h
	$(CC) $(CFLAGS) -o host host.c -ldl

# Esto es lo que recompilas en caliente, desde otra terminal: `make game.so`
# platform.h esta como prerequisito para que un cambio en el contrato
# rebuildee la lib y no te quede desincronizada del host.
#
# Se compila a un temporal y se mueve solo si salio bien. Si compilaras directo
# sobre game.so, un error de compilacion te deja un archivo trunco y el host
# muere con "file too short" en la proxima recarga.
game.so: dungeon.c platform.h
	$(CC) $(CFLAGS) -shared -fPIC -o game.so.tmp dungeon.c && mv game.so.tmp game.so

# Modulo de demo: lo minimo que cumple el contrato, para ver el hot-reload
# andando. Genera el mismo game.so, pero desde demo.c en lugar de dungeon.c,
# asi tu codigo queda intacto.
demo: demo.c platform.h
	$(CC) $(CFLAGS) -shared -fPIC -o game.so.tmp demo.c && mv game.so.tmp game.so

# Recompila solo al guardar, como el plugin de Unity. Corre en otro panel.
watch: watch.sh
	./watch.sh $(FUENTE)

clean:
	rm -f host game.so game.so.tmp game_live.so

.PHONY: all clean demo watch
