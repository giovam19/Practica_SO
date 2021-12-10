all: Atreides

Atreides: atreides.o Conexion.o
	gcc atreides.o Conexion.o -o Atreides -Wall -Wextra -lpthread

atreides.o: atreides.c Conexion.h
	gcc -c atreides.c -Wall -Wextra -lpthread

Conexion.o: Conexion.c Conexion.h
	gcc -c Conexion.c -Wall -Wextra

clean:
	rm *.o Atreides
