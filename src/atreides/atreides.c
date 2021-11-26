#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <pthread.h>
#include <netdb.h>
#include "Conexion.h"

#define print(x) write(1, x, strlen(x))

void *prueba(void *arg) {
    int *num = (int *) num;
    printf("Thread del hilo numero: %d\n", *num);

    return (void *) arg;
}

int main(int argc, char* argv[]) {
    Conexion datos;
    struct sockaddr_in servidor;
    pthread_t *threads;
    int servidorFD, totalFremens;

    totalFremens = 0; //inicializar de otra manera

    if(argc != 2){
        print("Error. Numero de argumentos no es correcto!\n");
        exit(0);
    }

    datos = leerFichero(argv[1]);

    servidorFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (servidorFD < 0) {
        print("Error creant el socket\n");
        exit(0);
    }

    bzero(&servidor, sizeof(servidor));
    servidor.sin_family = AF_INET;
    servidor.sin_port = htons(atoi(datos.puerto));
    servidor.sin_addr.s_addr = inet_addr(datos.ip);

    if(bind(servidorFD, (struct sockaddr*) &servidor, sizeof(servidor)) < 0){
        print("Error fent el bind\n");
        exit(0);
    }

    if(listen(servidorFD, 10) < 0){
        print("Error fent el listen\n");
        exit(0);
    }
	
    socklen_t len = sizeof(servidor);

    print("SERVIDOR ATREIDES\n");
    print("Llegit el fitxer de configuracio\n");
    print("Esperant connexions... \n\n");

    while(1) {
        int newFremen = accept(servidorFD, (void *)&servidor, &len);

        if (newFremen < 0) {
            print("Error accept new fremen\n");
        } else {
            if(totalFremens == 0){
                threads = (pthread_t *)malloc(sizeof(pthread_t));
                //threadStruct = (ThreadStruct *)malloc(sizeof(ThreadStruct));
                //fdClients = (int *)malloc(sizeof(int));
            } else {
                threads = (pthread_t *)realloc(threads, sizeof(pthread_t) * (totalFremens + 1));
                //threadStruct = (ThreadStruct *)realloc(threadStruct, sizeof(ThreadStruct) * (totalClients + 1));
                //fdClients = (int *)realloc(fdClients, sizeof(int) * (totalClients + 1));
            }

			int controlThread = pthread_create(&threads[totalFremens], NULL, prueba, &totalFremens);

            if(controlThread < 0){
                print("Error Thread\n");
            }
            totalFremens++;
        }
    }
}