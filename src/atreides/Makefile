all: atreides

atreides: atreides.o Conexion.o
	gcc atreides.o Conexion.o -o atreides -Wall -Wextra

atreides.o: atreides.c Conexion.h
	gcc -c atreides.c -Wall -Wextra

Conexion.o: Conexion.c Conexion.h
	gcc -c Conexion.c -Wall -Wextra

clean:
	rm *.o atreides