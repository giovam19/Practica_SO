all: fremen

fremen: fremen.o ConfigFichero.o
	gcc fremen.o ConfigFichero.o -o fremen -Wall -Wextra

fremen.o: fremen.c ConfigFichero.h
	gcc -c fremen.c -Wall -Wextra

ConfigFichero.o: ConfigFichero.c ConfigFichero.h
	gcc -c ConfigFichero.c -Wall -Wextra

cleano:
	rm *.o

clean:
	rm *.o fremen
