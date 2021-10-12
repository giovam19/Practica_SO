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

// hacer funcion separa argumentos por espacios.

char *comandoToMinus(char *input) {
    char *comandoaux = (char *) malloc(sizeof(char));
    int i;

    for (i = 0; input[i] != ' ' && input[i] != '\0'; i++) {
        comandoaux = (char *) realloc(comandoaux, sizeof(char) * (i + 2));
        comandoaux[i] = tolower(input[i]);
    }

    comandoaux[i] = '\0';

    return comandoaux;
}


char **getArgumentos(char* str, int posicion){
    char** argumentos;
    int j = 0;
    int k = 0;
    argumentos = (char**)malloc(sizeof(char*));
    argumentos[0] = (char*)malloc(sizeof(char));
    
    for (int i = posicion; str[i] != '\0'; i++) {
        if(str[i] != ' '){
            argumentos[j] = (char *) realloc(argumentos[j], sizeof(char) * (k + 2));
            argumentos[j][k] = str[i];
            k++;
        } else{
            argumentos = (char**)realloc(argumentos,sizeof(char*)*(10));
            j++;
            k = 0;
        }
    }
	return argumentos;
}

void comandoLinux(char *comando, char *input) {
    char **argumento;
    int pid;
    argumento = getArgumentos(input,(int)sizeof(comando));
    pid = fork();

    if (pid == 0) {
        int status = execvp(comando, argumento);
        if (status == -1) {
            printf("error execvp\n");
        }
    } else {
        wait(NULL);
    }
}

void menuComandos(char *input) {
    
    char *comando = comandoToMinus(input);

    if (strcmp(comando, "login") == 0) {
        write(1, "login ok\n", 9);
    } else if (strcmp(comando, "search") == 0) {
        write(1, "search ok\n", 10);
    } else if (strcmp(comando, "send") == 0) {
        write(1, "send ok\n", 10);
    } else if (strcmp(comando, "photo") == 0) {
        write(1, "photo ok\n", 10);
    } else if (strcmp(comando, "logout") == 0) {
        write(1, "logout ok\n", 10);
    } else {
        write(1, "linux\n", 6);
        comandoLinux(comando,input);
    }
}

int main() {
    //Configuracion datos;
    char input[40];
    int n;
	//datos = leerFichero("config.dat");

    while(1) {
        bzero(input, strlen(input));
        n = read(0, input, 40);
        input[n-1] = '\0';
        menuComandos(input);
    }
    
    return 0;
}
