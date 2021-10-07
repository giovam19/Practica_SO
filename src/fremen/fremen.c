#include <stdio.h>
#include "ConfigFichero.h"

int main() {
    Configuracion datos;

	datos = leerFichero("config.dat");

	printf("%s\n", datos.ip);
    printf("%s\n", datos.directorio);
    
    printf("Hola Mundo\n");
    
    return 0;
}
