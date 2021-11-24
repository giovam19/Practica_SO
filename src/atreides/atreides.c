#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <strings.h>
#include <ctype.h>
#include "Conexion.h"

#define print(x) write(1, x, strlen(x))

int main(int argc, char* argv[]) {
    Conexion datos;

    if(argc != 2){
        print("Error. Numero de argumentos no es correcto!\n");
        exit(0);
    }

    datos = leerFichero(argv[1]);

    print("SERVIDOR ATREIDES\n");
    print(datos.ip);
    print(":");
    print(datos.puerto);
    print("\n");

    print("Llegit el fitxer de configuracio\n");
    print("Esperant connexions... \n\n");

}