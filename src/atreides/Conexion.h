/***********************************************
*
* @Proposit: Libreria que contendra la estructura de los datos de conexion y ciertas funciones
* @Autor/s: giovanni.vecchies - josue.terrazas
* @Data creacio: 02/11/2021
* @Data ultima modificacio: 28/12/2021
*
************************************************/
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

void my_read_until(int fd, char end, char *string);

#endif