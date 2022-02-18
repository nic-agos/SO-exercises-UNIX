/*
Implementare un programma che riceva in input tramite argv[] i pathname
associati ad N file (F1 ... FN), con N maggiore o uguale ad 1.
Per ognuno di questi file generi un thread che gestira' il contenuto del file.
Dopo aver creato gli N file ed i rispettivi N thread, il main thread dovra'
leggere indefinitamente la sequenza di byte provenienti dallo standard-input.
Ogni 5 nuovi byte letti, questi dovranno essere scritti da uno degli N thread
nel rispettivo file. La consegna dei 5 byte da parte del main thread
dovra' avvenire secondo uno schema round-robin, per cui i primi 5 byte
dovranno essere consegnati al thread in carico di gestire F1, i secondi 5
byte al thread in carico di gestire il F2 e cosi' via secondo uno schema
circolare.

L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso
WinAPI) in modo tale che quando il processo venga colpito esso dovra',
a partire dai dati correntemente memorizzati nei file F1 ... FN, ripresentare
sullo standard-output la medesima sequenza di byte di input originariamente
letta dal main thread dallo standard-input.

Qualora non vi sia immissione di input, l'applicazione dovra' utilizzare
non piu' del 5% della capacita' di lavoro della CPU.
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

char **paths;
sem_t *ready;
sem_t *done;
int num_processes;
char buff[5];
FILE **target_files;

void printer(){

}

void *thread_function(void *arg){

    long int me = (long int)arg;
    int ret;

    printf("I'm the thread %d in charge of file %s\n", me, paths[me]);

    while(1){
step1:
        ret = sem_wait(&ready[me]);
        if(ret == -1){
            if(errno == EINTR){
                goto step1;
            }
            printf("Unable to wait Done semaphore for thread %d\n", me);
        }

        /*scrivo i dati sul file*/
        if(fprintf(target_files[me], "%s\n", buff)<0){
            printf("Unable to write on the file %s\n", paths[me]);
            exit(EXIT_FAILURE);
        }

        /*forzo la scrittura immediata dei dati sul file*/
        fflush(target_files[me]);

step2: 
        ret = sem_post(&done[me]);
        if(ret != 0){
            if(errno == EINTR){
                goto step2;
            }
            printf("Unable to signal ready semaphore for thread %d\n", me);
        }

    }
}

int main(int argc, char **argv){

    int i;
    int j;
    int ret;
    pthread_t pid;
    sigset_t set;
    struct sigaction sa;

    if(argc < 2){
        printf("Incorrect number of parameters\n");
        exit(EXIT_FAILURE);
    }

    /*salvo nella variabile globale paths i percorsi dei file*/
    paths = argv + 1;

    num_processes = argc - 1;

    ready = malloc(sizeof(sem_t)*num_processes);
    done = malloc(sizeof(sem_t)*num_processes);

    if(ready == NULL || done == NULL){
        printf("Unable to allocate space for semaphores\n");
        exit(EXIT_FAILURE);
    }

    /*alloco lo spazio necessario a mantenere i puntatori ai file*/
    target_files = (FILE**)malloc(sizeof(FILE*)*num_processes);
    if(target_files == NULL){
        printf("Unable to allocate space for stream's pointers\n");
        exit(EXIT_FAILURE);
    }    

    /*creo i file destinazione*/
    for(i = 0; i < num_processes; i++){

        target_files[i] = fopen(paths[i], "w+");

        if(target_files[i] == NULL){
            printf("Unable to create files\n");
            exit(EXIT_FAILURE);
        }
    }

    /*inizializzo i semafori*/
    for(i = 0; i < num_processes; i++){
        
        if(sem_init(&ready[i], 0, 1) == -1){
            printf("Unable to initialize Ready semaphores\n");
            exit(EXIT_FAILURE);
        }

        if(sem_init(&done[i], 0, 0) == -1){
            printf("Unable to set Done sempahores\n");
            exit(EXIT_FAILURE);
        }

    }

    /*creo gli N threads*/
    for(i = 0; i < num_processes; i++){

        ret = pthread_create(&pid, NULL, thread_function, (void*)i);

        if(ret != 0){
            printf("Unbale to spawn threads\n");
            exit(EXIT_FAILURE);
        }
    }

    /*svuoto il set di segnali*/
    sigemptyset(&set);

    sa.sa_flags = SA_SIGINFO;
    sa.sa_handler = printer;
    sa.sa_mask = set;

    if(sigaction(SIGINT, &sa, NULL) == -1){
        printf("Unable to set SIGINT handler for main thread\n");
        exit(EXIT_FAILURE);
    }

    i = 0;

    while(1){
redo1:
        ret = sem_wait(&done[i]);

        if(ret != 0){
            if(errno == EINTR){
                goto redo1;
            }
            printf("Unable to lock Ready semaphore of thread %d", i);
            exit(EXIT_FAILURE);
        }

        for(j = 0; j < 5; j++){
            buff[j] = getchar();
        }

redo2:
        ret = sem_post(&ready[i]);

        if(ret != 0){
            if(errno == EINTR){
                goto redo2;
            }
            printf("Unable to signal Done semaphore of thread %d", i);
            exit(EXIT_FAILURE);
        }
        /*implemento la logica di round-robin*/
        i = (i+1)%num_processes;
    }

    return 0;
}