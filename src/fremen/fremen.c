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
#include <ctype.h>
#include "ConfigFichero.h"

#define print(x) write(1, x, strlen(x))


char **getArgumentos(char* str, int *numArgs){
    int j = 0;
    int num_argumentos = 0;

    for(int i = 0; str[i] != '\0'; i++){
        if((str[i] != ' ' && str[i+1] == ' ' )|| (str[i] != ' ' && str[i+1] == '\0')){
            num_argumentos++;
        }
    }
    
    *numArgs = num_argumentos - 1;

    char *token;
    char **argumentos = (char**)malloc(sizeof(char*)*(num_argumentos+1));
    /* get the first token */
    for(int i = 0; i < num_argumentos; i++){
        argumentos[i] = (char*)malloc(sizeof(char)+1);
    }
    
    
    token= strtok(str, " ");
    argumentos[0] = token;
    /* walk through other tokens */
    while( token != NULL) {
        j++;
        token = strtok(NULL, " ");
        if(token != NULL) {
            argumentos[j] = token;
        }    
    }
	return argumentos;
}

void comandoLinux(char *comando, char **argumentos) {
    int pid;
    
    pid = fork();
    switch (pid) {
        case -1:
            print("Error ejecutando comando\n");
            break;
        case 0: //Fill
            execvp(comando, argumentos);
            break;
        default: //Pare
            wait(NULL);
            break;
    }
}



void menuComandos(char *input) {
    int num_argumentos;
    char **argumentos; 
    

    
    argumentos = getArgumentos(input, &num_argumentos);

    if (strcasecmp(input, "login") == 0) { //Login
        if (num_argumentos < 2) {
            print("Comanda KO. Falta parametres\n");
        } else if (num_argumentos > 2) {
            print("Comanda KO. Massa parametres\n");
        } else {
            print("Comanda OK\n");
        }
    } else if (strcasecmp(input, "search") == 0) { //Search
        if (num_argumentos < 1) {
            print("Comanda KO. Falta parametres\n");
        } else if (num_argumentos > 1) {
            print("Comanda KO. Massa parametres\n");
        } else {
            print("Comanda OK\n");
        }
    } else if (strcasecmp(input, "send") == 0) { //Send
        if (num_argumentos < 1) {
            print("Comanda KO. Falta parametres\n");
        } else if (num_argumentos > 1) {
            print("Comanda KO. Massa parametres\n");
        } else {
            print("Comanda OK\n");
        }
    } else if (strcasecmp(input, "photo") == 0) { //Photo
        if (num_argumentos < 1) {
            print("Comanda KO. Falta parametres\n");
        } else if (num_argumentos > 1) {
            print("Comanda KO. Massa parametres\n");
        } else {
            print("Comanda OK\n");
        }
    } else if (strcasecmp(input, "logout") == 0) { //Logout
        if (num_argumentos > 0) {
            print("Comanda KO. Massa parametres\n");
        } else { //Linux
            print("Comanda OK\n");
            printf("Desconnectat d’Atreides. Dew!\n");
            exit(0);
        }
    } else {
        comandoLinux(input, argumentos);
    }
    //free(input);
    /*for (int i = 0; i < num_argumentos+1; i++) {
        print(argumentos[i]);
        print("\n");
        free(argumentos[i]);
    }*/
    //free(argumentos);
    
}

int main(int argc, char* argv[]) {
    Configuracion datos;
    char input[40], buffer[100];
    int n;

    if(argc != 2){
        print("Error. Numero de argumentos no es correcto!\n");
        exit(0);
    }

	datos = leerFichero(argv[1]);


    print("Benvingut a Fremen\n");
    //revisar lectura de datos
    sprintf(buffer, "IP: %s\n", datos.ip);
    print(buffer);

    while(1) {
        bzero(input, strlen(input));
        print("$ ");
        n = read(0, input, 40);
        input[n-1] = '\0';
        menuComandos(input);
    }
    
    
    return 0;
}
