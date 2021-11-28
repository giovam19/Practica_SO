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
#include <signal.h>
#include <ctype.h>
#include <netdb.h>
#include "ConfigFichero.h"

#define print(x) write(1, x, strlen(x))

typedef struct {
    char origen[16];
    char tipo;
    char data[241];
} Trama;

typedef struct {
    char *id;
    char *nombre;
    char *c_postal;
} Usuario;

Configuracion datos;
Usuario user;
char **argumentos;
int num_argumentos;
int socketFD;

Trama fillTrama(char *buffer) {
    Trama trama;
    int i, j;

    bzero(&trama.origen, sizeof(trama.origen));
    bzero(&trama.data, sizeof(trama.data));

    for (i = 0; buffer[i] != '$'; i++) {
        trama.origen[i] = buffer[i];
    }

    for (i++; buffer[i] != '$'; i++) {
        trama.tipo = buffer[i];
    }
    j = 0;
    for (i++; buffer[i] != '\0'; i++) {
        trama.data[j] = buffer[i];
        j++;
    }

    return trama;
}

void getArgumentos(char* str){
    int j = 0;
    char *token;

    if (argumentos != NULL) {
        for (int i = 0; i < num_argumentos; i++) {
            free(argumentos[i]);
        }
        free(argumentos);
    }
    
    num_argumentos = 0;

    for(int i = 0; str[i] != '\0'; i++){
        if((str[i] != ' ' && str[i+1] == ' ' )|| (str[i] != ' ' && str[i+1] == '\0')){
            num_argumentos++;
            
        }
    }

    argumentos = (char**) malloc(sizeof(char*) * (num_argumentos+1));
    
    /* get the first token */
    for(int i = 0; i < num_argumentos; i++){
        argumentos[i] = (char*) malloc(sizeof(char));
    }
    
    token = strtok(str, " ");
    strcpy(argumentos[0], token);
    strcpy(str, argumentos[0]);
    
    /* walk through other tokens */
    while( token != NULL) {
        j++;
        token = strtok(NULL, " ");
        if(token != NULL) {
            strcpy(argumentos[j], token);
        }    
    }

    free(token);
}

void comandoLinux(char *comando, char **args) {
    int pid;
    int status;

    pid = fork();
    switch (pid) {
        case -1:
            print("Error ejecutando comando\n");
            break;
        case 0: //Fill
            status = execvp(comando, args);
            if (status < 0) {
                print("Error comando linux\n");
                exit(-1);
            }
            break;
        default: //Pare
            wait(NULL);
            break;
    }
}

void loginAtreides() {
    struct sockaddr_in cliente;
    char buffer[256];
    Trama trama;

    socketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(socketFD < 0){
        print("Error creant el socket\n");
        return;
    }

    bzero(&cliente, sizeof(cliente));
    cliente.sin_family = AF_INET;
    cliente.sin_port = htons(datos.puerto);
    cliente.sin_addr.s_addr = inet_addr(datos.ip);

    if(inet_pton(AF_INET, datos.ip, &cliente.sin_addr) < 0){
        print("Error configurant IP\n");
        return;
    }

    if(connect(socketFD, (struct sockaddr*) &cliente, sizeof(cliente)) < 0){
        print("Error fent el connect\n");
        return;
    }

    user.nombre = (char *) realloc(user.nombre, sizeof(char) * strlen(argumentos[1]));
    user.c_postal = (char *) realloc(user.c_postal, sizeof(char) * strlen(argumentos[2]));

    strcpy(user.nombre, argumentos[1]);
    strcpy(user.c_postal, argumentos[2]);

    sprintf(buffer, "FREMEN$C$%s*%s", argumentos[1], argumentos[2]);
    write(socketFD, buffer, strlen(buffer));

    bzero(&buffer, sizeof(buffer));
    read(socketFD, buffer, 256);

    trama = fillTrama(buffer);

    if (trama.tipo == 'O') {
        char buffer2[300];
        sprintf(buffer2, "Benvingut %s. Tens ID %s\n", argumentos[1], trama.data);
        print(buffer2);
        print("Ara estàs connectat a Atreides.\n");
        user.id = (char *) malloc(sizeof(char) * strlen(trama.data));
        strcpy(user.id, trama.data);
    } else {
        print("Error en Login.\n");
    }
}

void searchInServer() {
    char buffer[256];

    sprintf(buffer, "FREMEN$S$%s*%s*%s", user.nombre, user.id, argumentos[1]);
    //write(socketFD, buffer, strlen(buffer));
}

void logoutServer() {
    char buffer[256];

    sprintf(buffer, "FREMEN$Q$%s*%s", user.nombre, user.c_postal);
    write(socketFD, buffer, strlen(buffer));

    close(socketFD);
}

void signalHandler(int signum) {
    if (signum == SIGINT) {
        logoutServer();
        //vaciar memoria
        for (int i = 0; i < num_argumentos; i++) {
            free(argumentos[i]);
        }
        free(argumentos);
        free(datos.ip);
        free(datos.directorio);

        print("Saliendo Fremen.\n");

        signal(SIGINT, SIG_DFL);
        raise(SIGINT);
    }
}

void menuComandos(char *input) {

    getArgumentos(input);

    if (strcasecmp(input, "login") == 0) { //Login
        if (num_argumentos - 1 < 2) {
            print("Comanda KO. Falta parametres\n");
        } else if (num_argumentos - 1 > 2) {
            print("Comanda KO. Massa parametres\n");
        } else {
            loginAtreides();
        }
    } else if (strcasecmp(input, "search") == 0) { //Search
        if (num_argumentos - 1 < 1) {
            print("Comanda KO. Falta parametres\n");
        } else if (num_argumentos - 1 > 1) {
            print("Comanda KO. Massa parametres\n");
        } else {
            searchInServer();
        }
    } else if (strcasecmp(input, "send") == 0) { //Send
        if (num_argumentos - 1 < 1) {
            print("Comanda KO. Falta parametres\n");
        } else if (num_argumentos - 1 > 1) {
            print("Comanda KO. Massa parametres\n");
        } else {
            print("Comanda OK\n");
        }
    } else if (strcasecmp(input, "photo") == 0) { //Photo
        if (num_argumentos - 1 < 1) {
            print("Comanda KO. Falta parametres\n");
        } else if (num_argumentos - 1 > 1) {
            print("Comanda KO. Massa parametres\n");
        } else {
            print("Comanda OK\n");
        }
    } else if (strcasecmp(input, "logout") == 0) { //Logout
        if (num_argumentos - 1 > 0) {
            print("Comanda KO. Massa parametres\n");
        } else { //Linux
            logoutServer();
            print("Desconnectat d’Atreides. Dew!\n");
            exit(0);
        }
    } else {
        comandoLinux(input, argumentos);
    }
    
}

int main(int argc, char* argv[]) {
    char input[40];
    int n;

    argumentos = NULL;
    num_argumentos = 0;

    user.nombre = (char *) malloc(sizeof(char));
    user.c_postal = (char *) malloc(sizeof(char));

    if(argc != 2){
        print("Error. Numero de argumentos no es correcto!\n");
        exit(0);
    }

	datos = leerFichero(argv[1]);
    signal(SIGINT, signalHandler);

    print("Benvingut a Fremen\n");
    print(datos.ip);
    print("\n");

    while(1) {
        bzero(input, strlen(input));
        print("$ ");
        n = read(0, input, 40);
        input[n-1] = '\0';
        menuComandos(input);
    }
    
    return 0;
}
