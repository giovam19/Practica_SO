/*
    giovanni.vecchies
    josue.terrazas
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <strings.h>
#include <signal.h>
#include <ctype.h>
#include "ConfigFichero.h"

#define print(x) write(1, x, strlen(x))

char **argumentos;
int num_argumentos;

typedef struct {
    char origen[15];
    char type;
    char data[240];
} Mensaje;

void signalHandler(int signum) {
    if (signum == SIGINT) {
        //vaciar memoria
        for (int i = 0; i < num_argumentos; i++) {
            free(argumentos[i]);
        }
        free(argumentos);

        signal(SIGINT, SIG_DFL);
        raise(SIGINT);
    }
}

void getArgumentos(char* str){
    int j = 0;
    char *token;

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
    
    token= strtok(str, " ");
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



void menuComandos(char *input) {

    getArgumentos(input);

    if (strcasecmp(input, "login") == 0) { //Login
        if (num_argumentos - 1 < 2) {
            print("Comanda KO. Falta parametres\n");
        } else if (num_argumentos - 1 > 2) {
            print("Comanda KO. Massa parametres\n");
        } else {
            print("Comanda OK\n");
        }
    } else if (strcasecmp(input, "search") == 0) { //Search
        if (num_argumentos - 1 < 1) {
            print("Comanda KO. Falta parametres\n");
        } else if (num_argumentos - 1 > 1) {
            print("Comanda KO. Massa parametres\n");
        } else {
            print("Comanda OK\n");
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
            print("Comanda OK\n");
            print("Desconnectat d’Atreides. Dew!\n");
            exit(0);
        }
    } else {
        comandoLinux(input, argumentos);
    }
    
}

int main(int argc, char* argv[]) {
    Configuracion datos;
    char input[40];
    int n;

    argumentos = NULL;
    num_argumentos = 0;

    if(argc != 2){
        print("Error. Numero de argumentos no es correcto!\n");
        exit(0);
    }

	datos = leerFichero(argv[1]);
    signal(SIGINT, signalHandler);

    print("Benvingut a Fremen\n");
    print(datos.ip);

    while(1) {
        bzero(input, strlen(input));
        print("$ ");
        n = read(0, input, 40);
        input[n-1] = '\0';
        menuComandos(input);
    }
    
    return 0;
}
