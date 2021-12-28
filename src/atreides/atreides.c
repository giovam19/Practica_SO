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
int num_actuals, actual_size, connectedFremens, *fdClients;
pthread_t *threads;

int guardaUsuario(char* usuario){
    char* buffer;
    char* id;
    int num_usuarios, id_user = -1;
    int i;
    int fdUsuarios, registrado = 0;
    fdUsuarios = open("lista.txt", O_CREAT|O_RDWR, 00777);
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

char* searchUsers(char* codigoPostal, int* num_personas) {
    int fdUsuarios;
    char* buffer, *c_postal, *id, *data, *dataFinal;
    int num_usuarios = 0, sizeData = 1;

    *num_personas = 0;
    data = (char*)malloc(sizeof(char));
    fdUsuarios = open("lista.txt", O_RDONLY);
    buffer = read_until(fdUsuarios, '\n');
    num_usuarios = atoi(buffer);

    for(int i = 0; i < num_usuarios; i++){
        buffer = read_until(fdUsuarios, '*');
        c_postal = read_until(fdUsuarios, '&');
        id = read_until(fdUsuarios, '\n');
        if(strcmp(c_postal, codigoPostal) == 0) {
            *num_personas = (*num_personas) + 1;
            strcat(buffer, "*");
            strcat(buffer,id);
            strcat(buffer,"*");
            sizeData = sizeData + strlen(buffer) + strlen(id);
            data = (char*)realloc(data, sizeof(char) * sizeData);
           strcat(data, buffer);
        }
    }
    sprintf(buffer, "%d*", *num_personas);
    dataFinal = (char*)malloc(sizeof(char)*strlen(data) + strlen(buffer) +1);
    strcpy(dataFinal, buffer);
    strcat(dataFinal, data);
    dataFinal[strlen(dataFinal)-1] = '\0';
    free(data);
    close(fdUsuarios);

    return dataFinal;
}

void getMD5Sum(char photoName[10], char checksum[33]) {
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

Trama fillTrama(char *buffer) {
    Trama trama;
    int i, j;

    bzero(&trama.origen, sizeof(trama.origen));
    bzero(&trama.data, sizeof(trama.data));

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

void *clientController(void *arg) {
    int *clienteFD = (int *) arg;
    int logged;
    char buffer[256], data[240];
    Trama trama;
    Usuario user;

    logged = 0;

    while(1) {
        int size = read(*clienteFD, buffer, 256);
        if (size <= 0) {
            break;
        }

        trama = fillTrama(buffer);
        bzero(&buffer, sizeof(buffer));
        
        if (trama.tipo == 'C' && logged == 0) {
            //login
            logged = 1;
            user.id = guardaUsuario(trama.data);

            bzero(&data, 240);
            sprintf(data, "%d", user.id);
            createTramaSend(buffer, 'O', data);
            write(*clienteFD, buffer, 256);

            char *aux = (char *) malloc(sizeof(char) * (strlen(trama.data)+1));
            strcpy(aux, trama.data);


            //cambiar el strtok
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

            actualUsers[num_actuals] = (char *) malloc(sizeof(char) * (strlen(trama.data)+1));
            strcpy(actualUsers[num_actuals], trama.data);
            num_actuals++;
            if (num_actuals >= actual_size) {
                actualUsers = (char **) realloc(actualUsers, sizeof(char *) * num_actuals);
                actual_size = num_actuals;
            }
        } else if (trama.tipo == 'S' && logged == 1) {
            //search
            int num_personas, j, i;
            char* nombreAux, *idAux, *dataAux;
            char* auxC = (char *) malloc(sizeof(char));;
            Usuario userAux;

            j = 0;
            userAux.nombre = (char *) malloc(sizeof(char));
            userAux.c_postal = (char *) malloc(sizeof(char));

            for(i = 0; trama.data[i] != '*'; i++){
            //Copiamos el nombre del usuario
                userAux.nombre[j] = trama.data[i];
                j++;    
            }
            j = 0;
            for(i++; trama.data[i] != '*'; i++){
                //Copiamos el id del usuario
                auxC[j] = trama.data[i];
                j++;
            }
            userAux.id = atoi(auxC);
            j = 0;
            for(i++; trama.data[i] != '\0'; i++){
                //Copiamos el codigo postal
                userAux.c_postal[j] = trama.data[i];
                j++;
            }
            
            bzero(&auxC,strlen(auxC));
            sprintf(auxC, "Rebut search %s de %s %d\n", userAux.c_postal, userAux.nombre, userAux.id);
            print(auxC);
            dataAux = searchUsers(userAux.c_postal, &num_personas);
            print("Feta la cerca\n");
            sprintf(auxC, "Hi ha %d persones humanes a %s\n", num_personas, userAux.c_postal);
            print(auxC);
            
            nombreAux = (char*) malloc(sizeof(char));
            idAux = (char*) malloc(sizeof(char));
            i = 0;

            if (num_personas > 0) {
                for(j = 0; dataAux[j] != '*'; j++){}

                for(int k = 0; k < num_personas; k++) {
                    i = 0;
                    for(j++; dataAux[j] != '*' && dataAux[j] != '\0'; j++){
                        nombreAux[i] = dataAux[j];
                        i++;
                    }
                    nombreAux[i] = '\0';
                    i = 0;
                    for(j++; dataAux[j] != '*' && dataAux[j] != '\0'; j++){
                        idAux[i] = dataAux[j];
                        i++;
                    }
                    
                    sprintf(auxC, "%s %s\n", idAux, nombreAux);
                    print(auxC);
                }
            }

            bzero(&buffer, sizeof(buffer));
            createTramaSend(buffer, 'L', dataAux);
            write(*clienteFD, buffer, 256);
            print("Enviada resposta\n");
        } else if (trama.tipo == 'F' && logged == 1) {
            //send
            int i, j, fdImg, size, vueltas, resto;
            char foto[30], checksum[33], thisChecksum[33], nomFoto[10];

            bzero(&foto, 30);
            bzero(&checksum, 33);

            //usar mutex para los ficheros

            j = 0;
            for (i = 0; trama.data[i] != '*'; i++) {
                foto[j] = trama.data[i];
                j++;
            }

            j = 0;
            for (i++; trama.data[i] != '*'; i++) {
                checksum[j] = trama.data[i];
                j++;
            }
            size = atoi(checksum);

            bzero(&checksum, 32);
            j = 0;
            for (i++; j < 32; i++) {
                checksum[j] = trama.data[i];
                j++;
            }

            bzero(&nomFoto, sizeof(nomFoto));
            sprintf(nomFoto, "%d.jpg", user.id);

            bzero(&buffer, sizeof(buffer));
            sprintf(buffer, "Rebut send %s de %s %d\n", foto, user.nombre, user.id);
            print(buffer);

            fdImg = open(nomFoto, O_CREAT|O_WRONLY|O_TRUNC, 00777);
            if (fdImg < 0) {
                print("Error creando la imagen\n");
            } else {
                vueltas = size / 240;
                resto = size - (vueltas * 240);
                
                for (int i = 0; i < vueltas; i++) {
                    read(*clienteFD, buffer, 256);
                    trama = fillTrama(buffer);
                    if (trama.tipo == 'D') {
                        write(fdImg, trama.data, 240);
                        bzero(&buffer, sizeof(buffer));
                    }
                }

                read(*clienteFD, buffer, 256);
                trama = fillTrama(buffer);
                if (trama.tipo == 'D') {
                    write(fdImg, trama.data, resto);
                    bzero(&buffer, sizeof(buffer));
                }

                close(fdImg);

                getMD5Sum(nomFoto, thisChecksum);

                //hacer if de md5sum
                if (strcmp(checksum, thisChecksum) == 0) {
                    sprintf(buffer, "Guardada com %s\n", nomFoto);
                    print(buffer);

                    bzero(&data, 240);
                    sprintf(data, "IMAGE OK");
                    createTramaSend(buffer, 'I', data);
                    write(*clienteFD, buffer, 256);

                    print("Esperant connexions...\n\n");
                } else {
                    bzero(&data, 240);
                    sprintf(data, "IMAGE KO");
                    createTramaSend(buffer, 'R', data);
                    write(*clienteFD, buffer, 256);
                }
            }
        } else if (trama.tipo == 'P' && logged == 1) {
            //photo
            char checksum[33], nombre[30], dataAux[5];
            struct stat st;
            int size, vueltas, resto;

            bzero(&dataAux, 5);
            for (int d = 0; trama.data[d] != '\0'; d++) {
                dataAux[d] = trama.data[d];
            }

            bzero(&nombre, 30);
            sprintf(nombre, "%s.jpg", dataAux);

            bzero(&buffer, 256);
            sprintf(buffer, "Rebut photo %s de %s %d\n", dataAux, user.nombre, user.id);
            print(buffer);

            int imgFD = open(nombre, O_RDONLY);

            if (imgFD < 0) {
                //file not found
                print("No hi ha foto registrada.\n");
                bzero(&data, 240);
                sprintf(data, "FILE NOT FOUND");
                createTramaSend(buffer, 'F', data);
                write(*clienteFD, buffer, 256);
                print("Enviada resposta\nEsperant connexions...\n\n");
            } else {
                bzero(&buffer, 256);
                sprintf(buffer, "Enviament %s\n", nombre);
                print(buffer);

                //calculamos md5sum
                bzero(&checksum, 33);
                getMD5Sum(nombre, checksum);

                //calculamos mida del archivo
                fstat(imgFD, &st);
                size = st.st_size;
                vueltas = size / 240;
                resto = size - (vueltas * 240);

                bzero(&data, 240);
                sprintf(data, "%s*%d*%s", nombre, size, checksum);
                createTramaSend(buffer, 'F', data);
                write(*clienteFD, buffer, 256);

                //enviar datos imagen
                for (int i = 0; i < vueltas; i++) {
                    bzero(&buffer, 240);
                    bzero(&data, 240);
                    read(imgFD, data, 240);
                    createTramaSend(buffer, 'D', data);
                    write(*clienteFD, buffer, 256);
                }

                bzero(&buffer, 240);
                bzero(&data, 240);
                read(imgFD, data, resto);
                createTramaSend(buffer, 'D', data);
                write(*clienteFD, buffer, 256);

                close(imgFD);

                bzero(&buffer, sizeof(buffer));
                read(*clienteFD, buffer, 256);

                trama = fillTrama(buffer);
                
                if (trama.tipo == 'I') {
                    print("Enviada resposta\nEsperant connexions...\n\n");
                } else if (trama.tipo == 'R') {
                    print("Image KO.\n");
                }
            }

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
            bzero(&data, 240);
            sprintf(data, "ERROR");
            createTramaSend(buffer, 'E', data);
            write(*clienteFD, buffer, 256);
        }

    }

    //exclusion mutua
    for (int i = 0; i < connectedFremens; i++) {
        if (*clienteFD == fdClients[i]) {
            fdClients[i] = -1;
            break;
        }
    }

    close(*clienteFD);
    pthread_cancel(pthread_self());
    pthread_detach(pthread_self());

    return NULL;
}

void signalHandler(int signum) {
    if (signum == SIGINT) {
        //vaciar memoria
        for (int i = 0; i < connectedFremens; i++) {
            if (fdClients[i] != -1) {
                pthread_cancel(threads[i]);
                pthread_join(threads[i], NULL);
                pthread_detach(threads[i]);
                close(fdClients[i]);
            }
        }

        free(fdClients);
        free(threads);

        signal(SIGINT, SIG_DFL);
        raise(SIGINT);
    }
}

int main(int argc, char* argv[]) {
    Conexion datos;
    struct sockaddr_in servidor;
    int servidorFD;

    connectedFremens = 0; //inicializar de otra manera
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
            if(connectedFremens == 0){
                threads = (pthread_t *)malloc(sizeof(pthread_t));
                fdClients = (int *)malloc(sizeof(int));
            } else {
                threads = (pthread_t *)realloc(threads, sizeof(pthread_t) * (connectedFremens + 1));
                fdClients = (int *)realloc(fdClients, sizeof(int) * (connectedFremens + 1));
            }
            fdClients[connectedFremens] = newFremen;

			int controlThread = pthread_create(&threads[connectedFremens], NULL, clientController, (void *)&fdClients[connectedFremens]);

            if(controlThread < 0){
                print("Error Thread\n");
            }
            connectedFremens++;
        }
    }
}