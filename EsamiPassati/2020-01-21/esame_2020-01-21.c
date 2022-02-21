/*
Implementare una programma che ricevuto in input tramite argv[] una stringa S
esegua le seguenti attivita'.
Il main thread dovra' attivare due nuovi thread, che indichiamo con T1 e T2.
Successivamente il main thread dovra' leggere indefinitamente caratteri dallo 
standard input, a blocchi di 5 per volta, e dovra' rendere disponibili i byte 
letti a T1 e T2. 
Il thread T1 dovra' inserire di volta in volta i byte ricevuti dal main thread 
in coda ad un file di nome S_diretto, che dovra' essere creato. 
Il thread T2 dovra' inserirli invece nel file S_inverso, che dovra' anche esso 
essere creato, scrivendoli ogni volta come byte iniziali del file (ovvero in testa al 
file secondo uno schema a pila).

L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso
WinAPI) in modo tale che quando il processo venga colpito esso dovra' 
calcolare il numero dei byte che nei due file hanno la stessa posizione ma sono
tra loro diversi in termini di valore. Questa attivita' dovra' essere svolta attivando 
per ogni ricezione di segnale un apposito thread.

In caso non vi sia immissione di dati sullo standard input, l'applicazione dovra' 
utilizzare non piu' del 5% della capacita' di lavoro della CPU.
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

#define NUM_THREADS 2
#define SIZE 4096
#define NUM_CHARS 5

int ready_sem;
int done_sem;
FILE **output_files;
char buff[SIZE];
int written_bytes;
char *file_header_name;
int fd[2];

void *thread_function(void *arg){

    long me = (int)arg;
    char *filename;
    struct sembuf op;
    long current_size;
    char temp[NUM_CHARS];

    /*alloco spazio per contenere il nome del file*/
    filename = (char *)malloc(128);

    if(filename == NULL){
        printf("Unable to allocate space for ouput file's name\n");
        exit(EXIT_FAILURE);
    }

    /*costruisco il filename sulla base del thread*/
    sprintf(filename, "%s%s", file_header_name, me == 0 ? "_diretto" : "_inverso");

    printf("I'm thread %d, in charge of file %s\n", (me+1), filename);

    /*invoco la creazione del file destinazione*/
    fd[me] = open(filename, O_CREAT|O_RDWR|O_TRUNC, S_IRWXU|S_IRWXG|S_IRWXO);

    if(fd[me] == -1){
        printf("Unable to create file %s\n", filename);
        exit(EXIT_FAILURE);
    }
    
    while(1){

redo1:
        op.sem_flg = 0;
        op.sem_num = me;
        op.sem_op = -1;

        /*attendo che il processo main abbia terminato di acquisire dati da stdin*/
        if(semop(done_sem, &op, 1) == -1){
            if(errno == EINTR){
                goto redo1;
            }
            printf("Child thread - unable to wait Done semaphore for thread %d\n", me);
            exit(EXIT_FAILURE);
        }

            /*sono il thread T1 e sto operando sul file diretto*/
            write(fd[me], buff, (size_t)NUM_CHARS);

            /*forzo la scrittura dei dati sul file diretto*/
            fsync(fd[me]);

        if(me == 1){

            /*sono ilthread T2 e sto operando sul file inverso*/

            /*prelevo la posizione corrente del file pointer*/
            current_size = lseek(fd[me], 0, SEEK_CUR);

            /*posiziono il file pointer all'inizio del file*/
            lseek(fd[me], 0, SEEK_SET);

            while(current_size > 0){

                /*leggo la stringa in cima al file inverso*/
                read(fd[me], temp, (size_t)NUM_CHARS);

                /*mi posiziono sul file indietro di 5 caratteri per tornare al punto di inizio*/
                lseek(fd[me], -NUM_CHARS, SEEK_CUR);

                /*scrivo nella posizione in cima al file i nuovi dati*/
                write(fd[me], buff, (size_t)NUM_CHARS);

                /*copio sul buffer i caratteri precedentemente acquisiti sulla variabile temp*/
                memcpy(buff, temp, NUM_CHARS);

                /*decremento il valore dei bytes ancora da modificare*/
                current_size -= NUM_CHARS;
            }

            /*forzo la scrittura sul file inverso*/
            fsync(fd[me]);

        }
redo2:
        op.sem_flg = 0;
        op.sem_num = me;
        op.sem_op = 1;

        /*segnalo al processo parent di aver terminato la scrittura sul file*/
        if(semop(ready_sem, &op, 1) == -1){
            if(errno == EINTR){
                goto redo2;
            }
            printf("Child thread - unable to signal Ready semaphore for thread %d", me);
            exit(EXIT_FAILURE);
        }
    }

}

void handler(){

    int count = 0;
    int i;
    char char1[1];
    char char2[1];
    char *filename;

    printf("\nReceived CTRL+C\n");

    printf("Written bytes are %d\n", written_bytes);

    filename = malloc(128);

    for(i = 0; i<NUM_THREADS; i++){
        sprintf(filename, "%s%s", file_header_name, i == 0 ? "_diretto" : "_inverso");
        printf("filename %d is %s\n", i, filename);
        fd[i] = open(filename, O_RDONLY);
        if(fd[i] == -1){
            printf("Unable to open files\n");
            exit(EXIT_FAILURE);
        }
    }

    printf("file descriptor 1: %d\n", fd[0]);
    printf("file descriptor 2: %d\n", fd[1]);

    for(i = 0; i<written_bytes; i++){
        read(fd[0], char1, (size_t)1);
        read(fd[1], char2, (size_t)1);
        printf("c0[%d] = %s, c1[%d] = %s\n", i, char1, i, char2);
        if(char1 != char2){
            count ++;
        }
    }

    close(fd[0]);
    close(fd[1]);

    printf("Different bytes in same position are %d\n", count);
    exit(EXIT_SUCCESS);

}

void main(int argc, char *argv[]){

    int i;
    pthread_t tid;
    int ret;
    struct sembuf op;

    if(argc != 2){
        printf("usage: prog filename\n");
        fflush(stdout);
        exit(EXIT_FAILURE); 
    }

    file_header_name = argv[1];

    /*istanzio il semaforo Ready costituito da due elementi, uno per ogni thread*/
    ready_sem = semget(IPC_PRIVATE, 2, IPC_CREAT | 0666);

    if(ready_sem == -1){
        printf("Unable to create Ready semaphores\n");
        exit(EXIT_FAILURE);
    }

    /*inizializzo tutti gli elementi del semaforo Ready ad 1*/
    for(i=0; i<NUM_THREADS; i++){
        ret = semctl(ready_sem, i, SETVAL, 1);
        if(ret == -1){
            printf("Unable to initialize Ready semaphore\n");
            exit(EXIT_FAILURE);
        }
    }

    /*istanzio il semaforo Done costituito da due elementi, uno per ogni thread*/
    done_sem = semget(IPC_PRIVATE, 2, IPC_CREAT | 0666);

    if(done_sem == -1){
        printf("Unable to create Done semaphores\n");
        exit(EXIT_FAILURE);
    }

    //*inizializzo tutti gli elementi del semaforo Done a 0*/
    for(i=0; i<NUM_THREADS; i++){
        if(semctl(done_sem, i, SETVAL, 0)== -1){
            printf("Unable to initialize Done semaphore\n");
            exit(EXIT_FAILURE);
        }
    }

    /*genero i due thread*/
    for(i=0; i<NUM_THREADS; i++){
        ret = pthread_create(&tid, NULL, thread_function, (void*)i);

        if(ret != 0){
            printf("Unable to spawn threads\n");
            exit(EXIT_FAILURE);
        }
    }

    /*imposto il gestore per il segnale SIGINT*/
    signal(SIGINT, handler);

    written_bytes = 0;

    while(1){

        /*aspetto che entrambi i thread siano nello stato ready prima di iniziare a leggere caratteri da stdin*/
        for(i=0; i<NUM_THREADS; i++){

redo1:
            op.sem_flg = 0;
            op.sem_num = i;
            op.sem_op = -1;

            /*controllo se l'operazione di Wait è stata interrotta dall'arrivo di un segnale*/
            if(semop(ready_sem, &op, 1) == -1){
                if(errno == EINTR){
                    goto redo1;
                }
                printf("Main thread - unable to wait Ready semaphore for thread %d", i);
                exit(EXIT_FAILURE);
            }
        }

        for(i=0; i<=NUM_CHARS; i++){
            ret = scanf("%c", buff+i);
            if(ret == EOF){
                i--;
            }
        }

        written_bytes += 5;

        for(i=0; i<NUM_THREADS; i++){
redo2:
            op.sem_flg = 0;
            op.sem_num = i;
            op.sem_op = 1;

            /*controllo se l'operazione di Signal è stata interrotta dall'arrivo di un segnale*/
            if(semop(done_sem, &op, 1) == -1){
                if(errno == EINTR){
                    goto redo2;
                }
                printf("Main thread - unable to signal Done semaphore for thread %d\n", i);
                exit(EXIT_FAILURE);
            }

        }

    }

}