/*
Scrivere due programmi server e client per ambiente
UNIX, tali per cui il processo-server utilizza una FIFO
predefinita (server_fifo) per leggere i messaggi scritti dal
processo-client.
Inoltre il processo-client è in grado di generare un
numero arbitrario (argomento del programma client) di
client-threads, ognuno dei quali utilizza una FIFO privata
(client_fifo_x) per leggere/scrivere in modo esclusivo
dal/al server.
A sua volta il processo-server delega un server-thread
apposito per comunicare con il client-thread che lo ha
richiesto per mezzo della FIFO di uso esclusivo di
quest’ultimo.
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define MAX_READ 32

void *server(void *arg){

    int fr;
    int fw;

    char *data_client;

    char *fifo_name = (char *)arg;


    if(open(fifo_name, data_client, MAX_READ)){
        printf("Unable to read from %s", fifo_name);
        exit(-1);
    }

    printf("Received message from %s, is %s", fifo_name, data_client);



}

void main(int argc, char *argv[]){

    const char *fifonames = "server_fifo";
    int fd;
    char *buff;
    size_t size;
    char *fifo_client_name;
    pthread_t thread;

    if(mkfifo(fifonames, S_IRUSR|S_IWUSR) && errno != EEXIST){
        printf("Unable to create server_fifo\n");
        exit(-1);
    }

    fd = open(fifonames, O_RDONLY);

    if (fd < 0){
        printf("Unable to open server_fifo\n");
        exit(-1);
    }

    while(1){

        while (read(fd, buff, (size_t) MAX_READ) > 0){

            fifo_client_name = strdup(buff);

            if (fifo_client_name == NULL){
                printf("Unable to duplicate FIFO descriptor");
                exit(-1);
            }

            thread = pthread_create(fifo_client_name, NULL, thread, (void *) fifo_client_name);

            if(thread != 0){
                printf("Unable to spawn server thread for %s\n", fifo_client_name);
                free(fifo_client_name);
                close(fd);
                exit(-1);
            }

        }

    }

    exit(0);

}