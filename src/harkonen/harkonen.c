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