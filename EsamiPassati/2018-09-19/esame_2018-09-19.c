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


/* questa versione prevede che siano passati in input stringhe di non più di 5 caratteri alla volta*/

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

#define NUM_CHARS 5

sem_t *sem;
int num_threads;
char buff[NUM_CHARS];
char oth_buff[NUM_CHARS];
FILE **target_files;
FILE **recostruction_files;

void printer(){

    int res;
    int turn; 
    int i;

    printf("\nreceived CTRL+C - reconstructing the original stream\n");
    fflush(stdout);

    for(i= 1; i<num_threads; i++){

        /*muovo il file pointer all'inizio di ogni file */
        if(fseek(recostruction_files[i], 0, SEEK_SET) == -1){
            printf("Cannot position at the begin of file\n");
            fflush(stdout);
        }
    }

    turn = 0;

    while(1){

        res = fscanf(recostruction_files[turn+1], "%s", &oth_buff);
        if(res == EOF){
            if(turn == num_threads){
                printf("Recostruction done\n");
                exit(EXIT_SUCCESS);
            }
            goto next;
        }

        printf("%s ", oth_buff);
        fflush(stdout);
next:
        turn = (turn +1)%num_threads;
    }

}

void *thread_function(void *arg){

    long int me = (long int)arg;
    int ret;

    printf("I'm the thread %d, just spawn \n", me);

    while(1){
step1:
        /*attendo che il processo main consegni dati per questo thread*/
        ret = sem_wait(&sem[me]);

        if(ret == -1){
            if(errno == EINTR){
                goto step1;
            }
            printf("Unable to wait Done semaphore for thread %d\n", me);
            fflush(stdout);
            exit(EXIT_FAILURE);
        }
        
        printf("Child thread %d - reading string %s from buff\n", me, buff);

        /*scrivo i dati del buffer sul file*/
        if(fprintf(target_files[me], "%s\n", buff)<0){
            printf("Unable to write on the file\n");
            exit(EXIT_FAILURE);
        }

        /*forzo la scrittura immediata dei dati sul file*/
        fflush(target_files[me]);

step2: 
        /*segnalo al processo main di aver completato l'elaborazione*/
        ret = sem_post(&sem[0]);

        if(ret != 0){
            if(errno == EINTR){
                goto step2;
            }
            printf("Unable to signal ready semaphore for thread %d\n", me);
        }

        printf("Thread %d, just write %d more bytes on file\n", me, NUM_CHARS);
        fflush(stdout);

    }
}

int main(int argc, char **argv){

    int i;
    int j;
    int ret;
    pthread_t pid;
    sigset_t set;
    struct sigaction sa;
    int turn;
    char *string;

    if(argc < 2){
        printf("usage: prog file_name1 [file_name2] ... [file_nameN]\n");
        exit(EXIT_FAILURE);
    }

    num_threads = argc - 1;

    /*alloco lo spazio necessario a mantenere i puntatori ai file*/
    target_files = (FILE**)malloc(sizeof(FILE*)*(argc));

    if(target_files == NULL){
        printf("Unable to allocate space for stream's pointers\n");
        exit(EXIT_FAILURE);
    }

    /*alloco lo spazio necessario a mantenere i puntatori ai file per la ricostruzione*/
    recostruction_files = (FILE**)malloc(sizeof(FILE*)*(argc));

    if(recostruction_files == NULL){
        printf("Unable to allocate space for stream's pointers\n");
        exit(EXIT_FAILURE);
    }  

    target_files[0] = NULL;

    /*creo i file destinazione*/
    for(i = 1; i < argc; i++){

        /*apro il file in lettura e scrittura, se esiste già viene troncato altrimenti viene creato*/
        target_files[i] = fopen(argv[i], "w+");

        /*apro uno stream di sola lettura sul file appena creato*/
        recostruction_files[i] = fopen(argv[i], "r");

        if(target_files[i] == NULL || recostruction_files[i] == NULL){
            printf("Main thread - unable to create destination files\n");
            exit(EXIT_FAILURE);
        }
    }

    /*alloco lo spazio necessario a contenere i puntatori ai semafori*/
    sem = malloc(sizeof(sem_t)*(argc));

    if(sem == NULL){
        printf("Unable to allocate space for semaphores\n");
        exit(EXIT_FAILURE);
    }

    /*inizializzo il primo seaforo a 1, il primo semaforo viene utilizzato per segnalare al 
      main che il child thread avente il turno ha terminato l'elaborazione. Ne viene usato uno per tutti i thread*/
    if(sem_init(&sem[0], 0, 1) == -1){
        printf("Unable to initialize semaphores\n");
        exit(EXIT_FAILURE);
    }

    /*inizializzo i restanti semafori a 0*/
    for(i = 1; i < argc; i++){
        
        if(sem_init(&sem[i], 0, 0) == -1){
            printf("Unable to initialize semaphores\n");
            exit(EXIT_FAILURE);
        }

    }

    /*creo gli N threads*/
    for(i = 1; i < argc; i++){

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

    /*imposto la funzione di handler per il segnale SIGINT*/
    if(sigaction(SIGINT, &sa, NULL) == -1){
        printf("Unable to set SIGINT handler for main thread\n");
        exit(EXIT_FAILURE);
    }

    turn = 0;

    while(1){
redo1:  
        /*aspetto che il thread che ha il turno in quel momento abbia terminato la sua esecuzione*/
        ret = sem_wait(&sem[0]);

        if(ret != 0){
            if(errno == EINTR){
                goto redo1;
            }
            printf("Unable to lock Ready semaphore of thread %d", turn);
            exit(EXIT_FAILURE);
        }

        /*prelevo i dati in input e li scrivo nel buffer*/
        scanf("%ms", &string);

        if(strlen(string) <= 5){
            strcpy(buff, string);
        }
        else{
            printf("You must insert string of max 5 char\n");
            exit(EXIT_FAILURE);
        }

redo2:
        /*segnalo al thread avente il turno che può operare*/
        ret = sem_post(&sem[turn+1]);

        if(ret != 0){
            if(errno == EINTR){
                goto redo2;
            }
            printf("Unable to signal Done semaphore of thread %d", turn);
            exit(EXIT_FAILURE);
        }

        /*implemento la logica di round-robin*/
        turn = (turn+1)%num_threads;
    }

    return 0;
}