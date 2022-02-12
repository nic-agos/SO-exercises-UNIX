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


void main(int argc, char *argv[]){

    const char *fifonames = "server_fifo";
    int fd[2];
    int rescode;

    if (argc != 2){
        printf("Invalid parameters number\n");
        exit(-1);
    }

    if(mkfifo(fifonames, S_IRUSR|S_IWUSR) == -1){
        printf("Unable to create server_fifo\n");
        exit(-1);
    }

    





}