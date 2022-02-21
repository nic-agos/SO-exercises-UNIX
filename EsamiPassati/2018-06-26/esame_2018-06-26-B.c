/*
Implementare un programma che riceva in input tramite argv[2] un numero
intero N maggiore o uguale ad 1 (espresso come una stringa di cifre 
decimali), e generi N nuovi processi. Ciascuno di questi leggera' in modo 
continuativo un valore intero da standard input, e lo comunichera' al
processo padre tramite memoria condivisa. Il processo padre scrivera' ogni
nuovo valore intero ricevuto su di un file, come sequenza di cifre decimali. 
I valori scritti su file devono essere separati dal carattere ' ' (blank).
Il pathname del file di output deve essere comunicato all'applicazione 
tramite argv[1].
L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso
WinAPI) in modo tale che se il processo padre venga colpito il contenuto
del file di output venga interamente riversato su standard-output.
Nel caso in cui non vi sia immissione in input, l'applicazione non deve 
consumare piu' del 5% della capacita' di lavoro della CPU.
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

#define MAX_LENGTH 1024

int num_processes;
char *file_name;
FILE *output_file;
int ready;
int done;
int *values;

void run(){

    pid_t me = getpid();
    struct sembuf op;
    int ret;

    printf("I'm the process %d, just started\n", me);

    /*ignoro esplicitamente il segnale SIGINT che dovrà essere gestito solo dal processo padre*/
    signal(SIGINT, SIG_IGN);

    while(1){
redo1:
        op.sem_flg = 0;
        op.sem_num = 1;
        op.sem_op = -1;

        /*tento di eseguire una wait sul semaforo Ready, se riesco a prenderlo allora il main thread 
          avrà terminato la sua esecuzione e potrò inizare ad acquisire valori da stdin*/
        if(semop(ready, &op, 1) == -1){
            if(errno == EINTR){
                goto redo1;
            }
            printf("Unable to wait Ready semaphore\n");
            exit(EXIT_FAILURE);
        }

        /*acquisisco da stdin un valore intero e lo scrivo sulla variabile condivisa*/
        ret = scanf("%d", values);

        if(ret == 0){
            printf("Unable to read an integer value from stdin\n");
            exit(EXIT_FAILURE);
        }

        printf("Child process %d, just readed value %d\n", me, *values);

redo2:

        op.sem_flg = 0;
        op.sem_num = 0;
        op.sem_op = 1;

        /*eseguo una signal sul semaforo Done, per indicare al main thread che è stato acquisito un nuovo valore*/
        if(semop(ready, &op, 1) == -1){
            if(errno == EINTR){
                goto redo2;
            }
            printf("Unable to signal Done semaphore\n");
            exit(EXIT_FAILURE);
        }

    }

}

void handler(){

    char cmd[128];

    sprintf(cmd, "cat %s\n", file_name);

    system(cmd);

}

void main(int argc, char **argv){

    int i;
    int key = 1234;
    int ret; 
    pid_t pid;
    struct sembuf op;

    if(argc != 3){
        printf("usage: prog pathname num_threads\n");
        exit(EXIT_FAILURE);
    }

    /*converto in intero il numero di thread passato come stringa al main*/
    num_processes = atoi(argv[2]);

    if(num_processes < 1){
        printf("num_threads parameter must be greater than or equal to 1\n");
        exit(EXIT_FAILURE);
    }

    file_name = argv[1];

    /*apro il file di output, verrà creato se non esistente e troncato se già esistente*/
    output_file = fopen(file_name, "w+");

    if(output_file == NULL){
        printf("Unable to open/create target file\n");
        exit(EXIT_FAILURE);
    }

    /*invoco il mapping della zona di memoria da condividere con i processi*/
    values = mmap(NULL, MAX_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);

    if(values == NULL){
        printf("Unable to map shared memory\n");
        exit(EXIT_FAILURE);
    }

    /*invoco la creazione di un array di 2 semafori*/
    ready = semget(key, 2, IPC_CREAT | 0666);

    if(ready == -1){
        printf("Unable to initialize sempahores\n");
        exit(EXIT_FAILURE);
    }

    /*inizializzo il semaforo Done (in posizione 0) a 0*/
    ret = semctl(ready, 0, SETVAL, 0);

    if(ret == -1){
        printf("Unable to initialize first semaphore\n");
        exit(EXIT_FAILURE);
    }

    /*inizializzo il semaforo Ready (in posizione 1) a 1*/
    ret = semctl(ready, 1, SETVAL, 1);

     if(ret == -1){
        printf("Unable to initialize second semaphore\n");
        exit(EXIT_FAILURE);
    }

    /*imposto la funzione di gestione per il segnale SIGINT del main thread*/
    signal(SIGINT, handler);   

    printf("Spawning %d processes\n", num_processes);

    /*invoco la creazione dei processi*/
    for(i=0; i<num_processes; i++){

        pid = fork();

        if(pid == -1){
            printf("Unable to fork process\n");
            exit(EXIT_FAILURE);
        }

        if(pid == 0){
            run();
        }
    }
    
    while(1){
redo1:
        op.sem_flg = 0;
        op.sem_num = 0;
        op.sem_op = -1;

        /*attendo che il semaforo Done venga sbloccato da qualche thread, 
          vorrà dire che è stato acquisito un nuovo intero da stdin*/
        if(semop(ready, &op, 1) == -1){
            if(errno == EINTR){
                goto redo1;
            }
            printf("Unable to wait Done semaphore\n");
            exit(EXIT_FAILURE);
        }

        printf("Main process - found value %d\n", *values);

redo2:  
        /*scrivo il valore letto dalla memoria condivisa sul file*/
        ret = fprintf(output_file, "%d ", *values);

        if(ret == -1){
            if(errno == EINTR){
                goto redo2;
            }
            printf("Main process - unable to write on the output file\n");
            exit(EXIT_FAILURE);
        }
        /*forzo la scrittura sul file così da avere il file sempre aggiornato*/
        fflush(output_file);
redo3:
        op.sem_flg = 0;
        op.sem_num = 1;
        op.sem_op = 1;

        /*segnalo ai thread di essere di nuovo prontfo per ricevere un nuovo valore*/
        if(semop(ready, &op, 1) == -1){
            if(errno == EINTR){
                goto redo3;
            }
            printf("Main process - unable to signal Ready semaphore\n");
            exit(EXIT_FAILURE);
        }
    }

    /*mi metto in attesa di un qualunque segnale, sto gestendo esplicitamente solo SIGINT*/
    pause();

}