#include <stdio.h>
#include "FicheroConfig.h"

int main(int argc, char const *argv[]) {
    Configuracion datos;

	datos = leerFichero("config.dat");

	printf("%s\n", datos.ip);
    printf("%s\n", datos.directorio);
    
    printf("Hola Mundo\n");
    
    return 0;
}
