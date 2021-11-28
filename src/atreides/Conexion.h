#ifndef FICHERO_CONEXION_H
#define FICHERO_CONEXION_H

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
    char *ip;
    char *puerto;
    char *directorio;
} Conexion;

Conexion leerFichero(char *nombre);
char* read_until(int fd, char end);

#endif