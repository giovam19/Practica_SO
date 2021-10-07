#ifndef FICHERO_CONFIG_H
#define FICHERO_CONFIG_H

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <errno.h>

typedef struct {
    int tiempo;
    char *ip;
    int puerto;
    char *directorio;
} Configuracion;

Configuracion leerFichero(char *nombre);

#endif