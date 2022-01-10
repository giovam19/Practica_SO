/***********************************************
*
* @Proposit: Contendra la funciones desarrolladas de la libreria
* @Autor/s: giovanni.vecchies - josue.terrazas
* @Data creacio: 15/10/2021
* @Data ultima modificacio: 15/01/2021
*
************************************************/
#include "ConfigFichero.h"

/***********************************************
*
* @Finalitat: Se encargara de leer hasta cierto caracter de un file descriptor.
* @Parametres: in: fd = file descriptor de donde se realizara la lectura.
*              in: end = el caracter que marcara hasta donde se realizara la lectura.
* @Retorn: Retornara la cadena leida.
*
************************************************/
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

/***********************************************
*
* @Finalitat: Leera el fichero de configuracion y rellenara con esa informacion la estructura de Conexion.
* @Parametres: in: nombre = nombre del archivo de configuracion.
* @Retorn: Variable de tipo Conexion con los datos leidos.
*
************************************************/
Configuracion leerFichero(char *nombre) {
    Configuracion datos;
    int fdFichero;

    datos.ip = NULL;
    datos.directorio = NULL;

    fdFichero = open(nombre, O_RDONLY);

    if (fdFichero > 0) {
        char *data = read_until(fdFichero, '\n');
        datos.tiempo = atoi(data);
        free(data);

        datos.ip = read_until(fdFichero, '\n');
        
        data = read_until(fdFichero, '\n');
        datos.puerto = atoi(data);
        free(data);

        datos.directorio = read_until(fdFichero, '\0');

        close(fdFichero);
    } else {
        write(1, "Error Apertura fichero de configuracion\n", 40);
    }

    return datos;
}
