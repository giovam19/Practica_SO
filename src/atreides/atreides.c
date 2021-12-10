/*
    giovanni.vecchies
    josue.terrazas
*/

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
#include <signal.h>
#include <pthread.h>
#include <netdb.h>
#include "Conexion.h"

#define print(x) write(1, x, strlen(x))

typedef struct {
    char origen[16];
    char tipo;
    char data[241];
} Trama;

typedef struct {
    int id;
    char *nombre;
    char *c_postal;
} Usuario;

char **actualUsers;
int num_actuals, actual_size;

int guardaUsuario(char* usuario){
    char* buffer;
    char* id;
    int num_usuarios, id_user = -1;
    int i;
    int fdUsuarios, registrado = 0;
    fdUsuarios = open("lista.txt",O_RDWR);
    buffer = read_until(fdUsuarios, '\n');
    num_usuarios = atoi(buffer);

    for(i = 0; i <= num_usuarios; i++) {

        buffer = read_until(fdUsuarios, '&');
        id = read_until(fdUsuarios,'\n');
        if(strcmp(buffer,usuario) == 0) {
            id_user = atoi(id);
            registrado = 1;
            break;
        }
    }
    if(registrado == 0){
        char buffer3[100];
        id_user = num_usuarios+1;
        sprintf(buffer3,"%s&%d\n",usuario,id_user);
        write(fdUsuarios, buffer3, strlen(buffer3));
        close(fdUsuarios);
        fdUsuarios = open("lista.txt",O_RDWR);
        num_usuarios++;
        char buffer2[5];
        sprintf(buffer2,"%d\n",num_usuarios);
        write(fdUsuarios,buffer2, strlen(buffer2));

    }

    close(fdUsuarios);
    return id_user;
}

void createTramaSend(char *trama, char tipo, char *data) {
    int i, j;
    char origen[15] = "ATREIDES\0\0\0\0\0\0\0";

    j = 0;
    for (i = 0; i < 15; i++) {
        trama[i] = origen[j];
        j++;
    }

    trama[15] = tipo;

    j = 0;
    for (i = 16; i < 256; i++) {
        trama[i] = data[j];
        j++;
    }

}

void *clientController(void *arg) {
    int *clienteFD = (int *) arg;
    int i, j, logged;
    char buffer[256];
    Trama trama;
    Usuario user;

    logged = 0;

    while(1) {
        int size = read(*clienteFD, buffer, 256);
        if (size == 0) {
            break;
        }

        for (i = 0; i < 15; i++) {
            trama.origen[i] = buffer[i];
        }

        trama.tipo = buffer[i];

        j = 0;
        for (i++; i < 256; i++) {
            trama.data[j] = buffer[i];
            j++;
        }

        bzero(&buffer, sizeof(buffer));
        
        if (trama.tipo == 'C' && logged == 0) {
            //login
            char data[240];

            logged = 1;
            user.id = guardaUsuario(trama.data);

            bzero(&data, 240);
            sprintf(data, "%d", user.id);
            createTramaSend(buffer, 'O', data);
            write(*clienteFD, buffer, 265);

            char *aux = (char *) malloc(sizeof(char) * strlen(trama.data));
            strcpy(aux, trama.data);

            char *token = strtok(aux, "*");

            user.nombre = (char *) malloc(sizeof(char) * strlen(token));
            user.c_postal = (char *) malloc(sizeof(char));
            strcpy(user.nombre, token);

            while(token != NULL) {
                token = strtok(NULL, "*");
                if (token != NULL) {
                    user.c_postal = (char *) realloc(user.c_postal, sizeof(char) * strlen(token));
                    strcpy(user.c_postal, token);
                }
            }

            free(token);

            sprintf(buffer, "Rebut login %s %s\n", user.nombre, user.c_postal);
            print(buffer);
            sprintf(buffer, "Assignat a ID %d.\n", user.id);
            print(buffer);
            print("Enviada resposta\n\n");

            actualUsers[num_actuals] = (char *) malloc(sizeof(char) * strlen(trama.data));
            strcpy(actualUsers[num_actuals], trama.data);
            num_actuals++;
            if (num_actuals >= actual_size) {
                actualUsers = (char **) realloc(actualUsers, sizeof(char *) * num_actuals);
                actual_size = num_actuals;
            }
        } else if (trama.tipo == 'S' && logged == 1) {
            //search 
        } else if (trama.tipo == 'Q' && logged == 1) {
            //logout
            for (int i = 0; i < actual_size; i++) {
                if (strcmp(actualUsers[i], trama.data) == 0) {
                    bzero(&actualUsers[i], sizeof(actualUsers[i]));
                    num_actuals--;
                    break;
                }
            }

            sprintf(buffer, "Rebut logout de  %s %s\n", user.nombre, user.c_postal);
            print(buffer);
            print("Desconnectat d’Atreides.\n");
            print("Esperant connexions...\n\n");

            break;
        } else {
            sprintf(buffer, "ATREIDES$E$ERROR");
            write(*clienteFD, buffer, strlen(buffer));
        }

    }

    close(*clienteFD);

    return NULL;
}

void signalHandler(int signum) {
    if (signum == SIGINT) {
        //vaciar memoria

        signal(SIGINT, SIG_DFL);
        raise(SIGINT);
    }
    if (signum == SIGUSR1) {
        // kill thread;
        pthread_exit(NULL);
    }
}

int main(int argc, char* argv[]) {
    Conexion datos;
    struct sockaddr_in servidor;
    pthread_t *threads;
    int servidorFD, totalFremens, *fdClients;

    totalFremens = 0; //inicializar de otra manera
    num_actuals = 0;
    actual_size = 0;

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

    actualUsers = (char **) malloc(sizeof(char *));
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
                fdClients = (int *)malloc(sizeof(int));
            } else {
                threads = (pthread_t *)realloc(threads, sizeof(pthread_t) * (totalFremens + 1));
                //threadStruct = (ThreadStruct *)realloc(threadStruct, sizeof(ThreadStruct) * (totalClients + 1));
                fdClients = (int *)realloc(fdClients, sizeof(int) * (totalFremens + 1));
            }
            fdClients[totalFremens] = newFremen;

			int controlThread = pthread_create(&threads[totalFremens], NULL, clientController, (void *)&fdClients[totalFremens]);

            if(controlThread < 0){
                print("Error Thread\n");
            }
            totalFremens++;
        }
    }
}