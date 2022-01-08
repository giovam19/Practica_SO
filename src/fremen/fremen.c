/*
    giovanni.vecchies
    josue.terrazas
*/

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/poll.h>
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
#include <netdb.h>
#include <dirent.h>
#include <pthread.h>
#include "ConfigFichero.h"
#include "semaphore_v2.h"

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
int num_argumentos, socketFD, logged;
pthread_t removeThread, pollThread;
semaphore semImage, semLogged, semSocket;

Trama fillTrama(char *buffer) {
    Trama trama;
    int i, j;

    bzero(&trama.origen, 16);
    bzero(&trama.data, 241);

    for (i = 0; i < 15; i++) {
        trama.origen[i] = buffer[i];
    }

    trama.tipo = buffer[i];

    j = 0;
    for (i++; i < 256; i++) {
        trama.data[j] = buffer[i];
        j++;
    }

    return trama;
}

char *getArgumentos(char* str){
    int j, f;
    //char *token;

    if (argumentos != NULL) {
        for (int i = 0; i < num_argumentos; i++) {
            free(argumentos[i]);
        }
        free(argumentos);
        argumentos = NULL;
    }
    
    num_argumentos = 0;

    for(int i = 0; str[i] != '\0'; i++){
        if((str[i] != ' ' && str[i+1] == ' ' )|| (str[i] != ' ' && str[i+1] == '\0')){
            num_argumentos++;
        }
    }

    argumentos = (char**) malloc(sizeof(char*) * (num_argumentos+1));
    
    for(int i = 0; i < num_argumentos; i++){
        argumentos[i] = (char*) malloc(sizeof(char));
    }
    
    j=0;
    f=0;
    for(int i = 0; str[i] != '\0'; i++) {
        if (str[i] != ' ') {
            argumentos[j][f] = str[i];
            f++;
            argumentos[j] = (char *) realloc(argumentos[j], sizeof(char) * (f + 1));
        }
                
        if((str[i] != ' ' && str[i+1] == ' ' ) || (str[i] != ' ' && str[i+1] == '\0')){
            argumentos[j][f] = '\0';
            j++;
            f = 0;
        }
    }

    return argumentos[0];
}

void createTramaSend(char *trama, char tipo, char *data) {
    int i, j;
    char origen[15] = "FREMEN\0\0\0\0\0\0\0\0\0";

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

void getMD5Sum(char *photoName, char checksum[33]) {
    int fd[2];
    int n;
    char buffer[100];

    if (pipe(fd)==-1){
        print("Error en crear pipe\n");
        exit(-1);
    }

    n=fork();
    switch (n){
        case -1:// Si hi ha error el podem tractar
            break;
        case 0: //ESTEM AL FILL
            close(fd[0]);
            dup2(fd[1], 1);
            dup2(fd[1], 2);
            execlp("md5sum", "md5sum", photoName, NULL);
            break;
        default://ESTEM AL PARE
            wait(NULL);
            close(fd[1]);
            read(fd[0], buffer, 32);
            close(fd[0]);

            for (int i = 0; i < 32; i++) {
                checksum[i] = buffer[i];
            }
            checksum[32] = '\0';

            break;
    }
}

void deleteImages() {
    DIR *folder;
    struct dirent *entry;
    int files = 0;
    char **args;

    args = (char **) malloc(sizeof(char *) * 4);

    args[0] = (char *) malloc(sizeof(char) * (strlen("rm")+1));
    args[1] = (char *) malloc(sizeof(char) * (strlen("./fremen/xxx.jpg")+1));
    args[2] = NULL;

    strcpy(args[0], "rm");

    folder = opendir("./fremen/");
    if(folder == NULL)
    {
        print("Unable to read directory\n");
    }

    while( (entry=readdir(folder)) ){
        files++;
        if (files > 2) {
            sprintf(args[1], "./fremen/%s", entry->d_name);
            comandoLinux("rm", args);
        }
    }

    closedir(folder);

    free(args[0]);
    free(args[1]);
    free(args);
}

void *pollingFunct() {
    int i;
    char buf[256], data[240];
    struct pollfd pfds[2];

    bzero(&data, 240);
    while(1) {
        pfds[0].fd = 0;
        pfds[0].events = POLLIN;
        SEM_wait(&semSocket);
        pfds[1].fd = socketFD;
        pfds[1].events = POLLIN;
        SEM_signal(&semSocket);

        poll(pfds, 2, -1);
        
        if (pfds[1].revents & POLLIN) {
            SEM_wait(&semSocket);
            createTramaSend(buf, 'G', data);
            write(socketFD, buf, 256);
            i = read(socketFD, buf, 256);
            if (i != 256) {
                close(socketFD);
                socketFD = -1;
                if (user.nombre != NULL) {
                    free(user.nombre);
                    user.nombre = NULL;
                }
                if (user.c_postal != NULL) {
                    free(user.c_postal);
                    user.c_postal = NULL;
                }
                if (user.id != NULL) {
                    free(user.id);
                    user.id = NULL;
                }
                SEM_wait(&semLogged);
                logged = 0;
                SEM_signal(&semLogged);
                print("Perdida de conexion con Atreides.\n");
                SEM_signal(&semSocket);
                break;
            } else {
                SEM_signal(&semSocket);
            }
        }
    }

    //terminar el thread;
    pthread_cancel(pthread_self());
    pthread_detach(pthread_self());

    return NULL;
}

void loginAtreides() {
    struct sockaddr_in cliente;
    char buffer[256], data[240];
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


    user.nombre = (char *) malloc(sizeof(char) * (strlen(argumentos[1])+1));
    user.c_postal = (char *) malloc(sizeof(char) * (strlen(argumentos[2])+1));

    strcpy(user.nombre, argumentos[1]);
    strcpy(user.c_postal, argumentos[2]);
    
    bzero(&data, 240);
    sprintf(data, "%s*%s", argumentos[1], argumentos[2]);

    createTramaSend(buffer, 'C', data);
    write(socketFD, buffer, 256);

    bzero(&buffer, sizeof(buffer));
    read(socketFD, buffer, 256);

    trama = fillTrama(buffer);

    if (trama.tipo == 'O') {
        char buffer2[300];
        sprintf(buffer2, "Benvingut %s. Tens ID %s\n", user.nombre, trama.data);
        print(buffer2);
        print("Ara estàs connectat a Atreides.\n");
        user.id = (char *) malloc(sizeof(char) * (strlen(trama.data)+1));
        strcpy(user.id, trama.data);
        logged = 1;
        //iniciar polling
        int controlPoll = pthread_create(&pollThread, NULL, pollingFunct, NULL);
        if (controlPoll < 0) {
            print("Error creando Thread polling\n");
        }

    } else {
        print("Error en Login.\n");
    }
}

void searchInServer() {
    char buffer[256], data[240];
    Trama trama;

    bzero(&data, 240);
    sprintf(data, "%s*%s*%s", user.nombre, user.id, argumentos[1]);
    createTramaSend(buffer, 'S', data);
    write(socketFD, buffer, 256);

    bzero(&buffer, 256);
    read(socketFD, buffer, 256);

    trama = fillTrama(buffer);

    if (trama.tipo == 'L') {
        char *dataAux, *nombreAux, *idAux;
        int i, j, num_personas;
        
        dataAux = (char*) malloc(sizeof(char) * 242);
        nombreAux = (char*) malloc(sizeof(char));
        idAux = (char*) malloc(sizeof(char));
        i = 0;
       
        for(j = 0; trama.data[j] != '*'; j++){
            dataAux[j] = trama.data[j];
        }
        dataAux[j] = '\0';
        
        num_personas = atoi(dataAux);
        sprintf(dataAux, "Hi ha %d persones humanes a %s.\n", num_personas, argumentos[1]);
        print(dataAux);
        
        for(int k = 0; k < num_personas; k++) {
            i = 0;

            for(j++; trama.data[j] != '*' && trama.data[j] != '\0'; j++){
                nombreAux[i] = trama.data[j];
                i++;
                nombreAux = (char*) realloc(nombreAux, sizeof(char) * (i+1));
            }
            nombreAux[i] = '\0';
            i = 0;
            for(j++; trama.data[j] != '*' && trama.data[j] != '\0'; j++){
                idAux[i] = trama.data[j];
                i++;
                idAux = (char*) realloc(idAux, sizeof(char) * (i+1));
            }
            idAux[i] = '\0';
            sprintf(dataAux, "%s %s\n", idAux, nombreAux);
            print(dataAux);
            if(trama.data[j] == '\0' && k+1 < num_personas){
                bzero(&buffer, 256);
                read(socketFD, buffer, 256);

                trama = fillTrama(buffer);
                j = -1;
                i = 0;
            }
        }

        free(nombreAux);
        free(idAux);
        free(dataAux);

    } else {
        print("Error en Search.\n");
    }
}

void sendImage() {
    char checksum5[33], buffer[256], data[240], imagen[100];
    int fdImg, size, vueltas, resto;
    struct stat st;
    Trama trama;

    sprintf(imagen, "%s/%s", datos.directorio, argumentos[1]);

    fdImg = open(imagen, O_RDONLY);

    if (fdImg < 0) {
        print("Error abriendo Imagen\n");
        return;
    }

    //calculamos md5sum
    bzero(&checksum5, 32);
    getMD5Sum(imagen, checksum5);

    //calculamos mida del archivo
    fstat(fdImg, &st);
    size = st.st_size;
    vueltas = size / 240;
    resto = size - (vueltas * 240);

    bzero(&data, 240);
    sprintf(data, "%s*%d*%s", argumentos[1], size, checksum5);
    createTramaSend(buffer, 'F', data);
    write(socketFD, buffer, 256);

    //enviar datos imagen
    for (int i = 0; i < vueltas; i++) {
        bzero(&buffer, 240);
        bzero(&data, 240);
        read(fdImg, data, 240);
        createTramaSend(buffer, 'D', data);
        write(socketFD, buffer, 256);
    }

    bzero(&buffer, 240);
    bzero(&data, 240);
    read(fdImg, data, resto);
    createTramaSend(buffer, 'D', data);
    write(socketFD, buffer, 256);

    close(fdImg);

    bzero(&buffer, sizeof(buffer));
    read(socketFD, buffer, 256);

    trama = fillTrama(buffer);
    
    if (trama.tipo == 'I') {
        print("Foto enviada amb exit a Atreides.\n");
    } else if (trama.tipo == 'R') {
        print("Image KO.\n");
    }
}

void getPhoto() {
    char buffer[256], data[240];
    Trama trama;

    bzero(&data, 240);
    sprintf(data, "%s", argumentos[1]);
    createTramaSend(buffer, 'P', data);
    write(socketFD, buffer, 256);

    //leer
    bzero(&buffer, sizeof(buffer));
    read(socketFD, buffer, 256);

    trama = fillTrama(buffer);

    if (strcmp(trama.data, "FILE NOT FOUND") == 0) {
        //not found
        print("No hi ha foto registrada.\n");
    } else {
        //found
        int i, j, fdImg, size, vueltas, resto;
        char foto[31], checksum[33], thisChecksum[33], imagen[100];
        
        bzero(&foto, 31);
        j = 0;
        for (i = 0; trama.data[i] != '*'; i++) {
            foto[j] = trama.data[i];
            j++;
        }

        bzero(&checksum, 33);
        j = 0;
        for (i++; trama.data[i] != '*'; i++) {
            checksum[j] = trama.data[i];
            j++;
        }
        size = atoi(checksum);

        bzero(&checksum, 33);
        j = 0;
        for (i++; j < 32; i++) {
            checksum[j] = trama.data[i];
            j++;
        }

        bzero(&imagen, 100);
        sprintf(imagen, "%s/%s", datos.directorio, foto);

        fdImg = open(imagen, O_CREAT|O_WRONLY|O_TRUNC, 00777);
        if (fdImg < 0) {
            print("Error creando la imagen\n");
        } else {
            vueltas = size / 240;
            resto = size - (vueltas * 240);
            for (int i = 0; i < vueltas; i++) {
                read(socketFD, buffer, 256);
                trama = fillTrama(buffer);
                if (trama.tipo == 'D') {
                    write(fdImg, trama.data, 240);
                    bzero(&buffer, sizeof(buffer));
                }
            }
            read(socketFD, buffer, 256);
            trama = fillTrama(buffer);
            if (trama.tipo == 'D') {
                write(fdImg, trama.data, resto);
                bzero(&buffer, sizeof(buffer));
            }

            close(fdImg);

            getMD5Sum(imagen, thisChecksum);

            if (strcmp(checksum, thisChecksum) == 0) {
                bzero(&data, 240);
                sprintf(data, "IMAGE OK");
                createTramaSend(buffer, 'I', data);
                write(socketFD, buffer, 256);
            } else {
                bzero(&data, 240);
                sprintf(data, "IMAGE KO");
                createTramaSend(buffer, 'R', data);
                write(socketFD, buffer, 256);
            }
            
            print("Foto descarregada\n");
        }

    }
}

void freeAllMemory() {
        for (int i = 0; i < num_argumentos; i++) {
            if (argumentos[i] != NULL)
                free(argumentos[i]);
        }

        if (argumentos != NULL)
            free(argumentos);
        if (datos.ip != NULL)
            free(datos.ip);
        if (datos.directorio != NULL)
            free(datos.directorio);
        if (user.nombre != NULL)
            free(user.nombre);
        if (user.c_postal != NULL)
            free(user.c_postal);
        if (user.id != NULL)
            free(user.id);

        pthread_cancel(removeThread);
        pthread_join(removeThread, NULL);
        pthread_detach(removeThread);

        pthread_cancel(pollThread);
        pthread_join(pollThread, NULL);
        pthread_detach(pollThread);

		SEM_destructor(&semImage);
		SEM_destructor(&semSocket);
		SEM_destructor(&semLogged);
}

void logoutServer() {

    if (socketFD > 0) {
        char buffer[256], data[240];


        bzero(&data, 240);
        sprintf(data, "%s*%s", user.nombre, user.c_postal);
        createTramaSend(buffer, 'Q', data);
        write(socketFD, buffer, 256);
        close(socketFD);
    }
}

void signalHandler(int signum) {
    if (signum == SIGINT) {
        logoutServer();

        //vaciar memoria
        freeAllMemory();

        print("Saliendo Fremen.\n");

        signal(SIGINT, SIG_DFL);
        raise(SIGINT);
    }
}

void menuComandos(char *str) {

    char *input = getArgumentos(str);

    SEM_wait(&semLogged);
    if (strcasecmp(input, "login") == 0) { //Login
        if (num_argumentos - 1 < 2) {
            print("Comanda KO. Falta parametres\n");
        } else if (num_argumentos - 1 > 2) {
            print("Comanda KO. Massa parametres\n");
        } else {
            if (logged == 0) {
                SEM_wait(&semSocket);
                loginAtreides();
                SEM_signal(&semSocket);
            } else {
                print("Login ya hecho.\n");
            }
        }
    } else if (strcasecmp(input, "search") == 0) { //Search
        if (num_argumentos - 1 < 1) {
            print("Comanda KO. Falta parametres\n");
        } else if (num_argumentos - 1 > 1) {
            print("Comanda KO. Massa parametres\n");
        } else {
            if (logged == 1) {
                SEM_wait(&semSocket);
                searchInServer();
                SEM_signal(&semSocket);
            } else {
                print("Es necesario hacer login!\n");
            }
        }
    } else if (strcasecmp(input, "send") == 0) { //Send
        if (num_argumentos - 1 < 1) {
            print("Comanda KO. Falta parametres\n");
        } else if (num_argumentos - 1 > 1) {
            print("Comanda KO. Massa parametres\n");
        } else {
            if (logged == 1) {
                SEM_wait(&semSocket);
                SEM_wait(&semImage);
                sendImage();
                SEM_signal(&semImage);
                SEM_signal(&semSocket);
            } else {
                print("Es necesario hacer login!\n");
            }
        }
    } else if (strcasecmp(input, "photo") == 0) { //Photo
        if (num_argumentos - 1 < 1) {
            print("Comanda KO. Falta parametres\n");
        } else if (num_argumentos - 1 > 1) {
            print("Comanda KO. Massa parametres\n");
        } else {
            if (logged == 1) {
                SEM_wait(&semSocket);
                SEM_wait(&semImage);
                getPhoto();
                SEM_signal(&semImage);
                SEM_signal(&semSocket);
            } else {
                print("Es necesario hacer login!\n");
            }
        }
    } else if (strcasecmp(input, "logout") == 0) { //Logout
        if (num_argumentos - 1 > 0) {
            print("Comanda KO. Massa parametres\n");
        } else { //Linux
            SEM_wait(&semSocket);
            logoutServer();
            SEM_signal(&semSocket);
            freeAllMemory();
            print("Desconnectat d’Atreides. Dew!\n");
            exit(0);
        }
    } else {
        num_argumentos++;
        argumentos = (char**) realloc(argumentos, sizeof(char*) * (num_argumentos+1));
        argumentos[num_argumentos-1] = (char *) NULL;
        comandoLinux(input, argumentos);
    }
    SEM_signal(&semLogged);
    
}

void *removeImages() {
    while(1) {
        sleep(datos.tiempo);
        SEM_wait(&semImage);
        deleteImages();
        SEM_signal(&semImage);
    }

    return NULL;
}

int main(int argc, char* argv[]) {
    char input[40];
    int n;
    
    logged = 0;
    argumentos = NULL;
    num_argumentos = 0;
    socketFD = -1;

    user.nombre = NULL;
    user.c_postal = NULL;
    user.id = NULL;

    if(argc != 2){
        print("Error. Numero de argumentos no es correcto!\n");
        exit(0);
    }

	datos = leerFichero(argv[1]);
    datos.directorio[strlen(datos.directorio)-2] = '\0';

    signal(SIGINT, signalHandler);
    int controlAlarm = pthread_create(&removeThread, NULL, removeImages, NULL);
    if (controlAlarm < 0) {
        print("Error creando Thread alarma\n");
        exit(0);
    }

    SEM_constructor(&semImage);
    SEM_constructor(&semSocket);
    SEM_constructor(&semLogged);

	SEM_init(&semImage, 1);
	SEM_init(&semLogged, 1);
	SEM_init(&semSocket, 1);

    print("Benvingut a Fremen\n");

    while(1) {
        bzero(input, 40);
        print("$ ");
        n = read(0, input, 40);
        input[n-1] = '\0';
        menuComandos(input);
    }
    
    return 0;
}
