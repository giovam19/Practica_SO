/***********************************************
*
* @Proposit: Codigo fuente para el funcionamiento del Harkonen
* @Autor/s: giovanni.vecchies - josue.terrazas
* @Data creacio: 05/01/2022
* @Data ultima modificacio: 08/01/2022
*
************************************************/
#define _GNU_SOURCE
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <ctype.h>

#define print(x) write(1, x, strlen(x))

/***********************************************
*
* @Finalitat: Se encargara de ejecutar un scrip que retornara un PID aleatorio de un fremen activo para finalizar su ejecucion.
* @Parametres: --
* @Retorn: --
*
************************************************/
int getRandomPID() {
    int fd[2];
    int n, pid;
    char buffer[50];

    pid = -1;
    
    if (pipe(fd)==-1){
        print("Error en crear pipe\n");
        exit(-1);
    }

    print("Scanning pids...\n");

    n=fork();
    switch (n){
        case -1:// Si hi ha error el podem tractar
            break;
        case 0: //ESTEM AL FILL
            close(fd[0]);
            dup2(fd[1], 1);
            execlp("getRandom.sh", "getRandom.sh", NULL);
            break;
        default://ESTEM AL PARE
            wait(NULL);
            close(fd[1]);
            bzero(&buffer, 10);
            read(fd[0], buffer, 8);
            close(fd[0]);

            if (strlen(buffer) > 1){
                pid = atoi(buffer);
                sprintf(buffer, "killing pid %d\n\n", pid);
                print(buffer);
            }

            break;
    }

    return pid;
}

/***********************************************
*
* @Finalitat: Main de la ejecucion.
* @Parametres: in: argc = numero de parametros recibidos en la ejecucion.
*              in: argv = cadena de Strings que tendra los parametros.
* @Retorn: --
*
************************************************/
int main(int argc, char *argv[]) {
    int pid, time;
    
    if (argc != 2) {
        print("Faltan argumentos\n");
    } else {
        print("Starting Harkonen...\n\n");
        time = atoi(argv[1]);
        
        while(1) {
            sleep(time);
            pid = getRandomPID();
            if (pid > 0) {
                kill(pid, SIGKILL);
            }
        }
    }
}