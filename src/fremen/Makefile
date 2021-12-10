all: Fremen

Fremen: fremen.o ConfigFichero.o
	gcc fremen.o ConfigFichero.o -o Fremen -Wall -Wextra

fremen.o: fremen.c ConfigFichero.h
	gcc -c fremen.c -Wall -Wextra

ConfigFichero.o: ConfigFichero.c ConfigFichero.h
	gcc -c ConfigFichero.c -Wall -Wextra

cleano:
	rm *.o

clean:
	rm *.o Fremen
