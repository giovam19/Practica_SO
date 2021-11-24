#include "Conexion.h"

char* read_until(int fd, char end) {
    int i = 0, size;
    char c = '\0';
    char* string = (char*)malloc(sizeof(char));

    while (1) {
        size = read(fd, &c, sizeof(char));
        
        if (c != end && size > 0) {
            string = (char*)realloc(string, sizeof(char) * (i + 2));
            string[i++] = c;
        } else {
            break;
        }
    }

    string[i] = '\0';
    return string;
}

Conexion leerFichero(char *nombre) {
    Conexion datos;
    int fdFichero;

    fdFichero = open(nombre, O_RDONLY);

    if (fdFichero > 0) {
        datos.ip = read_until(fdFichero, '\n');
        
        datos.puerto = read_until(fdFichero, '\n');
        
        datos.directorio = read_until(fdFichero, '\0');

        close(fdFichero);
    } else {
        write(1, "Error Apertura fichero de configuracion\n", 40);
    }

    return datos;
}