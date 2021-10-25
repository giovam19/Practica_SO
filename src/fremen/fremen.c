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

char *comandoToMinus(char *input) {
    char *comandoaux = (char *) malloc(sizeof(char));
    int i;
    int j = 0;
    int k = 0;
    while(input[j] == ' ' && input[j] != '\0'){
        j++;
    }
    if(input[j] == '\0'){
        j = 0;
    }
    for (i = j; input[i] != ' ' && input[i] != '\0'; i++) {
        comandoaux = (char *) realloc(comandoaux, sizeof(char) * (i + 2));
        comandoaux[k] = tolower(input[i]);
        k++;
    }
    comandoaux[k] = '\0';
    return comandoaux;
}


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
    char *comando;
    char **argumentos; 
    
    comando = comandoToMinus(input);
    argumentos = getArgumentos(input, &num_argumentos);

    if (strcmp(comando, "login") == 0) { //Login
        if (num_argumentos < 2) {
            print("Comanda KO. Falta parametres\n");
        } else if (num_argumentos > 2) {
            print("Comanda KO. Massa parametres\n");
        } else {
            print("Comanda OK\n");
        }
    } else if (strcmp(comando, "search") == 0) { //Search
        if (num_argumentos < 1) {
            print("Comanda KO. Falta parametres\n");
        } else if (num_argumentos > 1) {
            print("Comanda KO. Massa parametres\n");
        } else {
            print("Comanda OK\n");
        }
    } else if (strcmp(comando, "send") == 0) { //Send
        if (num_argumentos < 1) {
            print("Comanda KO. Falta parametres\n");
        } else if (num_argumentos > 1) {
            print("Comanda KO. Massa parametres\n");
        } else {
            print("Comanda OK\n");
        }
    } else if (strcmp(comando, "photo") == 0) { //Photo
        if (num_argumentos < 1) {
            print("Comanda KO. Falta parametres\n");
        } else if (num_argumentos > 1) {
            print("Comanda KO. Massa parametres\n");
        } else {
            print("Comanda OK\n");
        }
    } else if (strcmp(comando, "logout") == 0) { //Logout
        if (num_argumentos > 0) {
            print("Comanda KO. Massa parametres\n");
        } else { //Linux
            print("Comanda OK\n");
        }
    } else {
        comandoLinux(comando, argumentos);
    }

    free(comando);
    for (int i = 0; i < num_argumentos+1; i++) {
        free(argumentos[i]);
    }
    free(argumentos);
}

int main() {
    //Configuracion datos;
    char input[40];
    int n;
	//datos = leerFichero("config.dat");
    //revisar lectura de datos

    print("Benvingut a Fremen\n");

    while(1) {
        bzero(input, strlen(input));
        print("$ ");
        n = read(0, input, 40);
        input[n-1] = '\0';
        menuComandos(input);
    }
    
    return 0;
}
