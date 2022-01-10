/***********************************************
*
* @Proposit: Codigo fuente para el funcionamiento del servidor
* @Autor/s: giovanni.vecchies - josue.terrazas
* @Data creacio: 02/11/2021
* @Data ultima modificacio: 10/01/2022
*
************************************************/

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
#define MAX_NOMBRE 50

typedef struct {
    char origen[16];
    char tipo;
    char data[241];
} Trama;

typedef struct {
    int id;
    char nombre[MAX_NOMBRE];
    char c_postal[MAX_NOMBRE];
} Usuario;

Conexion datos;
char **actualUsers;
int num_actuals, connectedFremens, *fdClients, servidorFD;
pthread_t *threads;
pthread_mutex_t images_mtx = PTHREAD_MUTEX_INITIALIZER,
                localUsers_mtx = PTHREAD_MUTEX_INITIALIZER,
                fdclients_mtx = PTHREAD_MUTEX_INITIALIZER,
                lista_mtx = PTHREAD_MUTEX_INITIALIZER;

/***********************************************
*
* @Finalitat: Abrir archivo de texto que contendra el historico de todos los users que se han conectado y añadir uno nuevo
* @Parametres: in: usuario = datos del usuario nuevo a añadir.
* @Retorn: retorna el id asignado el nuevo usuario.
*
************************************************/
int guardaUsuario(char usuario[240]){
    char *buffer, *id;
    int num_usuarios, id_user = -1;
    int i;
    int fdUsuarios, registrado = 0;

    fdUsuarios = open("lista.txt", O_CREAT|O_RDWR, 00777);
    buffer = read_until(fdUsuarios, '\n');
    num_usuarios = atoi(buffer);

    free(buffer);

    for(i = 0; i <= num_usuarios; i++) {
        buffer = read_until(fdUsuarios, '&');
        id = read_until(fdUsuarios,'\n');
        if(strcmp(buffer,usuario) == 0) {
            id_user = atoi(id);
            registrado = 1;
            free(buffer);
            free(id);
            break;
        }
        free(buffer);
        free(id);
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

/***********************************************
*
* @Finalitat: Buscar en el archivo txt de usuarios las personas que coincidan con el codigo postal indicado.
* @Parametres: in: codigoPostal = el codigo con el que se realizara la busqueda.
*              out: num_personas = el numero total de personas encontradas.
* @Retorn: Una cadena con las personas encontradas.
*
************************************************/
char* searchUsers(char* codigoPostal, int* num_personas) {
    int fdUsuarios;
    char *buffer, *c_postal, *id, *data, *dataFinal;
    int num_usuarios = 0, sizeData = 1, f;

    f = 0;
    *num_personas = 0;
    data = (char*)malloc(sizeof(char)+1);

    fdUsuarios = open("lista.txt", O_RDONLY);
    buffer = read_until(fdUsuarios, '\n');
    num_usuarios = atoi(buffer);
    free(buffer);
    
    for(int i = 0; i < num_usuarios; i++){
        buffer = read_until(fdUsuarios, '*');
        c_postal = read_until(fdUsuarios, '&');
        id = read_until(fdUsuarios, '\n');
        if(strcmp(c_postal, codigoPostal) == 0) {
            *num_personas = (*num_personas) + 1;
            buffer = (char*)realloc(buffer, sizeof(char)*(strlen(buffer)+ strlen(id)+3));
            strcat(buffer,"*");
            strcat(buffer,id);
            strcat(buffer,"*");
            sizeData = sizeData + strlen(buffer) + strlen(id);
            data = (char*)realloc(data, sizeof(char) * sizeData);
            if(f == 0){
                strcpy(data, buffer);
            } else {
                strcat(data, buffer);
            }
            f++;
        }
        free(buffer);
        free(c_postal);
        free(id);
    }
    if (*num_personas > 0) {
        buffer = (char *) malloc(sizeof(char) * 10);
        sprintf(buffer, "%d*", *num_personas);
        dataFinal = (char*)malloc(sizeof(char)*(strlen(data) + strlen(buffer) +1));
        strcpy(dataFinal, buffer);
        strcat(dataFinal, data);
        dataFinal[strlen(dataFinal)-1] = '\0';
        free(buffer);
    } else {
        dataFinal = (char*) malloc(sizeof(char) * 3);
        dataFinal[0] = '0';
        dataFinal[1] = '*';
        dataFinal[2] = '\0';
    }

    free(data);
    close(fdUsuarios);

    return dataFinal;
}

/***********************************************
*
* @Finalitat: Obtener el MD5SUM de la imagen indicada.
* @Parametres: in: photoName = el nombre de la imagen que se desea obtener la informacion.
*              out: checksum = el valor del MD5SUM de la imagen.
* @Retorn: --
*
************************************************/
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

/***********************************************
*
* @Finalitat: Creador de la trama que se enviara al cliente.
* @Parametres: out: trama = cadena que contendra la trama a enviar al cliente.
*              in: tipo = indicara el tipo de comando que sera la trama.
*              in: data = indicara la informacion importante de la trama.
* @Retorn: --
*
************************************************/
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

/***********************************************
*
* @Finalitat: Separara la trama recibida en tres partes, origen, tipo y data.
* @Parametres: in: buffer = contendra la trama a separar.
* @Retorn: Retorna la estructura Trama donde tendra la informacion ya separada.
*
************************************************/
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

/***********************************************
*
* @Finalitat: Cada vez que se cree un thread se lanzara con esta funcion que sera el controlador de todas las peticiones que envie el cliente al servidor.
* @Parametres: in: arg = sera el file descriptor con el que este thread realizara la comunicacion con el cliente.
* @Retorn: --
*
************************************************/
void *clientController(void *arg) {
    int *clienteFD = (int *) arg;
    char buffer[256], data[240];
    Trama trama;
    Usuario user;

    while(1) {
        int size = read(*clienteFD, buffer, 256);
        if (size <= 0) {
            break;
        }

        trama = fillTrama(buffer);
        bzero(&buffer, sizeof(buffer));
        
        if (trama.tipo == 'C') {
            //login
            int i, j;
            pthread_mutex_lock(&lista_mtx);
            user.id = guardaUsuario(trama.data);
            pthread_mutex_unlock(&lista_mtx);

            bzero(&data, 240);
            sprintf(data, "%d", user.id);
            createTramaSend(buffer, 'O', data);
            write(*clienteFD, buffer, 256);

            j = 0;
            for (i = 0; trama.data[i] != '*'; i++) {
                user.nombre[j] = trama.data[i];
                j++;
            }
            user.nombre[j] = '\0';

            j = 0;
            for (i++; trama.data[i] != '\0'; i++) {
                user.c_postal[j] = trama.data[i];
                j++;
            }
            user.c_postal[j] = '\0';

            sprintf(buffer, "Rebut login %s %s\n", user.nombre, user.c_postal);
            print(buffer);
            sprintf(buffer, "Assignat a ID %d.\n", user.id);
            print(buffer);
            print("Enviada resposta\n\n");

            //exclusion mutua
            pthread_mutex_lock(&localUsers_mtx);
            actualUsers[num_actuals] = (char *) malloc(sizeof(char) * (strlen(trama.data)+1));
            strcpy(actualUsers[num_actuals], trama.data);
            num_actuals++;
            actualUsers = (char **) realloc(actualUsers, sizeof(char *) * (num_actuals + 1));
            pthread_mutex_unlock(&localUsers_mtx);

        } else if (trama.tipo == 'S') {
            //search
            int num_personas, j, i;
            char *nombreAux, *idAux, *dataAux;
            char *auxC = (char *) malloc(sizeof(char));
            Usuario userAux;
            
            j = 0;
            for(i = 0; trama.data[i] != '*'; i++){
            //Copiamos el nombre del usuario
                userAux.nombre[j] = trama.data[i];
                j++;    
            }
            userAux.nombre[j] = '\0';
            j = 0;

            for(i++; trama.data[i] != '*'; i++){
                //Copiamos el id del usuario
                auxC[j] = trama.data[i];
                j++;
                auxC = (char *) realloc(auxC, sizeof(char) * (j+1));

            }
            auxC[j] = '\0';
            userAux.id = atoi(auxC);
            free(auxC);

            j = 0;
            for(i++; trama.data[i] != '\0'; i++){
                //Copiamos el codigo postal
                userAux.c_postal[j] = trama.data[i];
                j++;
            }
            userAux.c_postal[j] = '\0';
            
            auxC = (char *) malloc(sizeof(char)*(30+strlen(userAux.c_postal)+strlen(userAux.nombre)));
            sprintf(auxC, "Rebut search %s de %s %d\n", userAux.c_postal, userAux.nombre, userAux.id);
            print(auxC);
            pthread_mutex_lock(&lista_mtx);
            dataAux = searchUsers(userAux.c_postal, &num_personas);
            pthread_mutex_unlock(&lista_mtx);
            print("Feta la cerca\n");
            free(auxC);
            auxC = (char *) malloc(sizeof(char)*(38+strlen(userAux.c_postal)));
            sprintf(auxC, "Hi ha %d persones humanes a %s\n", num_personas, userAux.c_postal);
            print(auxC);
            free(auxC);

            
            i = 0;

            if (num_personas > 0) {
                for(j = 0; dataAux[j] != '*'; j++){}

                for(int k = 0; k < num_personas; k++) {
                    nombreAux = (char*) malloc(sizeof(char));
                    idAux = (char*) malloc(sizeof(char));
                    i = 0;
                    for(j++; dataAux[j] != '*' && dataAux[j] != '\0'; j++){
                        nombreAux[i] = dataAux[j];
                        i++;
                        nombreAux = (char *) realloc(nombreAux, sizeof(char) * (i+1));   

                    }
                    nombreAux[i] = '\0';
                    i = 0;

                    for(j++; dataAux[j] != '*' && dataAux[j] != '\0'; j++){
                        idAux[i] = dataAux[j];
                        i++;
                        idAux = (char *) realloc(idAux, sizeof(char) * (i+1));
                    }
                    idAux[i] = '\0';
                    print(idAux);
                    print(" ");
                    print(nombreAux);
                    print("\n");
                    free(nombreAux);
                    free(idAux); 
                }

                bzero(&buffer, sizeof(buffer));
                bzero(&data, sizeof(data));
                
                char aux[240];
                int datos = 0;
                j = 0;
                for(i = 0; dataAux[i] != '*'; i++){
                    aux[i] = dataAux[i];
                    j++;
                }
                aux[i] = dataAux[i];
                aux[i+1] = '\0';
                strcpy(data,aux);
                j = 0;
                i++;
                while(dataAux[i] != '\0') {    
                    j = 0;
            
                    for(; datos != 2 && dataAux[i] != '\0'; i++){
                        if(dataAux[i] == '*') datos++;
                        aux[j] = dataAux[i];
                        j++;
                    }
                    datos = 0;
                    aux[j] = '\0';

                    if(strlen(aux) + strlen(data) < 240){
                        strcat(data, aux);
                    
                    } else {
                        data[strlen(data)-1] = '\0';
                        createTramaSend(buffer, 'L', data);
                        write(*clienteFD, buffer, 256);
                        bzero(&data, sizeof(data));
                        bzero(&buffer, sizeof(buffer));
                        strcpy(data,aux);
                    }
                    bzero(&aux, sizeof(aux));
                }
                
                createTramaSend(buffer, 'L', data);
                write(*clienteFD, buffer, 256);
                print("Enviada resposta\n\n");
                bzero(&data, sizeof(data));
                bzero(&buffer, sizeof(buffer));

            } else {
                bzero(&data, 240);
                sprintf(data, "0*");
                createTramaSend(buffer, 'L', data);
                write(*clienteFD, buffer, 256);
                print("Enviada resposta\n\n");
                bzero(&data, sizeof(data));
                bzero(&buffer, sizeof(buffer));
            }
            
            
            if (dataAux != NULL)
                free(dataAux);

        } else if (trama.tipo == 'F') {
            //send
            int i, j, fdImg, size, vueltas, resto;
            char foto[30], checksum[33], thisChecksum[33], nomFoto[10];

            bzero(&foto, 30);
            bzero(&checksum, 33);

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

            bzero(&nomFoto, 10);
            sprintf(nomFoto, "%d.jpg", user.id);

            bzero(&buffer, 256);
            sprintf(buffer, "Rebut send %s de %s %d\n", foto, user.nombre, user.id);
            print(buffer);

            pthread_mutex_lock(&images_mtx);
            fdImg = open(nomFoto, O_CREAT|O_WRONLY|O_TRUNC, 00777);
            if (fdImg < 0) {
                print("Error creando la imagen\n");
                pthread_mutex_unlock(&images_mtx);
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
                pthread_mutex_unlock(&images_mtx);
            }
        } else if (trama.tipo == 'P') {
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

            pthread_mutex_lock(&images_mtx);
            int imgFD = open(nombre, O_RDONLY);

            if (imgFD < 0) {
                //file not found
                print("No hi ha foto registrada.\n");
                bzero(&data, 240);
                sprintf(data, "FILE NOT FOUND");
                createTramaSend(buffer, 'F', data);
                write(*clienteFD, buffer, 256);
                print("Enviada resposta\n\n");
                pthread_mutex_unlock(&images_mtx);
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
                pthread_mutex_unlock(&images_mtx);
            }

        } else if (trama.tipo == 'Q') {
            //logout
            pthread_mutex_lock(&localUsers_mtx);
            for (int i = 0; i < num_actuals; i++) {
                if (strcmp(actualUsers[i], trama.data) == 0) {
                    free(actualUsers[i]);
                    num_actuals--;
                    break;
                }
            }
            pthread_mutex_unlock(&localUsers_mtx);

            sprintf(buffer, "Rebut logout de  %s %s\n", user.nombre, user.c_postal);
            print(buffer);
            print("Desconnectat d’Atreides.\n");
            print("Esperant connexions...\n\n");

            break;
        } else {
            //error
            bzero(&data, 240);
            sprintf(data, "ERROR");
            createTramaSend(buffer, 'E', data);
            write(*clienteFD, buffer, 256);
        }

    }

    close(*clienteFD);
    pthread_mutex_lock(&fdclients_mtx);
    for (int i = 0; i < connectedFremens; i++) {
        if (fdClients[i] == *clienteFD) {
            fdClients[i] = -1;
            break;
        }
    }
    pthread_mutex_unlock(&fdclients_mtx);

    pthread_cancel(pthread_self());
    pthread_detach(pthread_self());

    return NULL;
}

/***********************************************
*
* @Finalitat: Esta funcion reprogramara los signals.
* @Parametres: in: signum = indicara el valor del signal que se disparo.
* @Retorn: --
*
************************************************/
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

        for (int i = 0; i < num_actuals; i++) {
            if (actualUsers[i] != NULL) {
                free(actualUsers[i]);
            }
        }

        if (actualUsers != NULL)
            free(actualUsers);
        if (fdClients != NULL)
            free(fdClients);
        if (threads != NULL)
            free(threads);
        if (datos.ip != NULL)
            free(datos.ip);
        if (datos.puerto != NULL)
            free(datos.puerto);
        if (datos.directorio != NULL)
            free(datos.directorio);
        
        close(servidorFD);

        signal(SIGINT, SIG_DFL);
        raise(SIGINT);
    }
}

/***********************************************
*
* @Finalitat: Main de la ejecucion.
* @Parametres: in: argc = numero de parametros recibidos en la ejecucion.
*              in: argv = cadena de Strings que tendra los parametros.
* @Retorn: --
*
************************************************/
int main(int argc, char* argv[]) {
    struct sockaddr_in servidor;

    connectedFremens = 0;
    num_actuals = 0;

    signal(SIGINT, signalHandler);

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
            } else {
                connectedFremens++;
            }
        }
    }
}