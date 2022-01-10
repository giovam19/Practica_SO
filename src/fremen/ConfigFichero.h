/***********************************************
*
* @Proposit: Libreria que contendra la estructura de los datos de conexion
* @Autor/s: giovanni.vecchies - josue.terrazas
* @Data creacio: 15/10/2021
* @Data ultima modificacio: 15/01/2021
*
************************************************/
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