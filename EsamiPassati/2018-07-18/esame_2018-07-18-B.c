/*
Implementare un programma che riceva in input tramite argv[] i pathname 
associati ad N file, con N maggiore o uguale ad 1. Per ognuno di questi
file generi un thread (quindi in totale saranno generati N nuovi thread 
concorrenti). 
Successivamente il main-thread acquisira' stringhe da standard input in 
un ciclo indefinito, ed ognuno degli N thread figli dovra' scrivere ogni
stringa acquisita dal main-thread nel file ad esso associato.
L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso 
WinAPI) in modo tale che quando uno qualsiasi dei thread dell'applicazione
venga colpito da esso dovra' stampare a terminale tutte le stringhe gia' 
immesse da standard-input e memorizzate nei file destinazione.
*/

#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SIZE 128

int num_processes;
char **files_name;
int ready;
int done;
char buff[128];

void *thread_function(void *arg){

    long int me = (long int)arg; 
    FILE *output_file;

    struct sembuf op;

    printf("I'm the child thread %d in charge of file %s\n", me, files_name[me]);

    output_file = fopen(files_name[me], "w+");

    if(output_file == NULL){
        printf("Unable to open file %s\n", files_name[me]);
        exit(EXIT_FAILURE);
    }

    op.sem_flg = 0;
    op.sem_num = me;

    while(1){
redo1:  
        /*attendo che il processo padre abbia consegnato dei dati per inizare l'elaborazione*/
        op.sem_op = -1;
        if(semop(done, &op, 1) == -1){
            if(errno == EINTR){
                goto redo1;
            }
            printf("Unable to wait Done semaphores for thread %d", me);
            exit(EXIT_FAILURE);
        }

        printf("I'm the child thread %d, reading the string %s from buff\n", me, buff);

        /*scrivo i dati ottenuti dal buffer condiviso sul relativo file che ho in gestione*/
        fprintf(output_file, "%s\n", buff);

        /*forzo la scrittura dei dati sul file di output*/
        fflush(output_file);

redo2:  
        op.sem_op = 1;
        if(semop(ready, &op, 1) == -1){
            if(errno == EINTR){
                goto redo2;
            }
            printf("Unable to signal Ready sempahore for thread %d", me);
            exit(EXIT_FAILURE);
        }

    }

}

void handler(){

    char cmd[128];

    sprintf(cmd, "cat %s\n", files_name[0]);
    system(cmd);

}

void main(int argc, char **argv){
    
    int ret;
    int i;
    pthread_t tid;
    struct sembuf op;

    if(argc < 2){
        printf("usage: prog pathname1 [pathname2] ... [pathnameN]\n");
        exit(EXIT_FAILURE);
    }

    num_processes = argc-1;

    files_name = argv+1;

    /*imposto il gestore per il segnale SIGINT*/
    signal(SIGINT, handler);

    /*alloco l'array semaforico Ready*/
    ready = semget(IPC_PRIVATE, num_processes, IPC_CREAT | 0666);

    if(ready == -1){
        printf("Unable to initialize Ready semaphore\n");
        exit(EXIT_FAILURE);
    }

    /*inizializzo a 1 tutti i semafori Ready*/
    for(i = 0; i < num_processes; i++){
        if(semctl(ready, i, SETVAL, 1) == -1){
            printf("Unable to initialize Ready semaphores\n");
            exit(EXIT_FAILURE);
        }
    }

    /*alloco l'array semaforico Done*/
    done = semget(IPC_PRIVATE, num_processes, IPC_CREAT | 0666);

    if(done == -1){
        printf("Unable to initialize Done semaphore\n");
        exit(EXIT_FAILURE);
    }

    /*inizializzo a 0 tutti i semafori Done*/
    for(i = 0; i < num_processes; i++){
        if(semctl(done, i, SETVAL, 0) == -1){
            printf("Unable to initialize Done semaphores\n");
            exit(EXIT_FAILURE);
        }
    }

    /*genero gli N thread*/
    for(i = 0; i < num_processes; i++){
        if(pthread_create(&tid, NULL, thread_function, (void *)i) != 0){
            printf("Unable to spawn threads\n");
            exit(EXIT_FAILURE);
        }
    }

    while(1){

         /*attendo che tutti i threads siano ready*/
        for(i=0; i<num_processes; i++){
            op.sem_op = -1;
            op.sem_flg = 0;
redo1:      

            op.sem_num = i;
            if(semop(ready, &op, 1) == -1){
                if(errno == EINTR){
                    goto redo1;
                }
                printf("Main thread - unable to wait Ready semaphore for thread %d\n", i);
                exit(EXIT_FAILURE);
            }
        }

read_again:

        if(scanf("%s", buff) == EOF){
            if(errno == EINTR){
                goto read_again;
            }
            printf("Unable to get string from terminal\n");
            exit(EXIT_FAILURE);
        }

        /*segnalo a tutti i thread che possono eseguire le loro operazioni*/
        for (i = 0; i<num_processes; i++){
            op.sem_op = 1;
            op.sem_num = i;
redo2:

            if(semop(done, &op, 1) == -1){
                if(errno == EINTR){
                    goto redo2;
                }
                printf("Main thread - unable to signal Done semaphore for thread %d\n", i);
                exit(EXIT_FAILURE);
            }
        }

    }

}