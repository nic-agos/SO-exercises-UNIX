/*
Implementare una programma che riceva in input, tramite argv[], un insieme di
stringhe S_1 ..... S_n con n maggiore o uguale ad 1. 
Per ogni stringa S_i dovra' essere attivato un thread T_i.
Il main thread dovra' leggere indefinitamente stringhe dallo standard-input.
Ogni stringa letta dovra' essere resa disponibile al thread T_1 che dovra' 
eliminare dalla stringa ogni carattere presente in S_1, sostituendolo con il 
carattere 'spazio'.
Successivamente T_1 rendera' la stringa modificata disponibile a T_2 che dovra' 
eseguire la stessa operazione considerando i caratteri in S_2, e poi la passera' 
a T_3 (che fara' la stessa operazione considerando i caratteri in S_3) e cosi' 
via fino a T_n. 
T_n, una volta completata la sua operazione sulla stringa ricevuta da T_n-1, dovra'
passare la stringa ad un ulteriore thread che chiameremo OUTPUT il quale dovra' 
stampare la stringa ricevuta su un file di output dal nome output.txt.
Si noti che i thread lavorano secondo uno schema pipeline, sono ammesse quindi 
operazioni concorrenti su differenti stringhe lette dal main thread dallo 
standard-input.

L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso
WinAPI) in modo tale che quando il processo venga colpito esso dovra' 
stampare il contenuto corrente del file output.txt su standard-output.

In caso non vi sia immissione di dati sullo standard-input, l'applicazione 
dovra' utilizzare non piu' del 5% della capacita' di lavoro della CPU.
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

int num_processes;
FILE *output_file;
char **strings;
pthread_mutex_t *ready;
pthread_mutex_t *done;
char **buffers;


void printer(int dummy){
    printf("\n");
    system("cat output.txt");
}

void *thread_function(void *arg){
    
    long int me = (long int)arg;
    sigset_t set;
    int i, j;

    /*controllo quale thread sono in base all'indice*/
    if(me < num_processes -1){
        printf("Thread %d started up - in charge of string: %s\n", me, strings[me]);
    }
    else{
        printf("Thread %d started up - in charge of the output file\n", me);
    }
    fflush(stdout);

    /*inserisco tutti i segnali all'interno della maschera*/
    sigfillset(&set);

    /*blocco tutti i segnali durante l'eseuczione delle operazioni del thread,
      senza voler ottenre la vecchia maschera dei segnali per il thread*/
    sigprocmask(SIG_BLOCK, &set, NULL);
    
    while(1){

        /*tento di otteere il lock sul semaforo Ready dell'i-esimo thread, 
          se ci riesco vuol dire che questo è pronto per eseguire*/
        if(pthread_mutex_lock(ready+me)){
            printf("Unable to lock Ready mutex of thread %d\n", me);
            exit(EXIT_FAILURE);
        }

        printf("Thread %d got string %s\n", me, buffers[me]);

        /*controllo se sono l'output thread*/
        if(me == num_processes - 1){
            
            printf("Writing string \"%s\" on the output file\n", buffers[me]);
            /*scrivo sul file la stringa ottenuta dopo le N elaborazioni*/
            fprintf(output_file, "%s\n", buffers[me]);

            /*forzo la scrittura sul file*/
            fflush(output_file);

        }else{
            /*sostituisco i caratteri della stringa passata da stdin che si trovano 
              all'interno dell'i-esima stringa passata come paramentro al main*/
            for(i = 0; i < strlen(buffers[me]); i++){
                for(j = 0; j < strlen(strings[me]); j++){
                    if(*(buffers[me]+i) == *(strings[me]+j)){
                        *(buffers[me]+i) = ' ';
                    }
                }
            }

            /*provo ad eseguire il lock sul mutex Done relativo al thread successivo per capire se questo ha 
              terminato la sua esecuzione*/
            if(pthread_mutex_lock(done+me+1)){
                printf("Unable to lock next Done mutex of thread %d\n", me+1);
                exit(EXIT_FAILURE);
            }

            /*se riesco ad ottenere il lock sul mutex Done relativo al prossimo thread allora posso 
              passargli la nuova stringa che dovrà computare*/

            buffers[me+1] = buffers[me];

            /*segnalo al thread successivo che può inizare la sua computazione perchè gli ho consegnato la nuova stringa da elaborare */
            if(pthread_mutex_unlock(ready+me+1)){
                printf("Unable to unlock next Ready mutex of thread %d\n", me+1);
                exit(EXIT_FAILURE);
            }
        }

        /*segnalo agli altri thread di aver completato la mia esecuzione*/
        if(pthread_mutex_unlock(done+me)){
            printf("Unable to unlock Done mutex of thread %d\n", me);
            exit(EXIT_FAILURE);
        }

    }
    
}

int main(int argc, char **argv){

    int i;
    int ret;
    pthread_t tid;
    char *string;

    if(argc <2){
        printf("Incorrect number of parameter\n");
        exit(EXIT_FAILURE);
    }

    /*num_threads include nel conto anche il thread di output, quindi in totale N+1 threads*/
    num_processes = argc;

    /*copio in strings i parametri passati al main eccetto il primo (nome del programma*/
    strings = argv + 1;

    /*apro uno stream verso il file di output, 
      con "w+" il files viene creato se non esiste o troncato se gia esistente, ed aperto in scrittura */
    output_file = fopen("output.txt", "w+");

    if(output_file == NULL){
        printf("Unablw to open output file\n");
        exit(EXIT_FAILURE);
    }

    /*alloco spazio per i puntatori agli identificatori dei semafori*/
    ready = malloc(sizeof(pthread_mutex_t)*num_processes);
    done = malloc(sizeof(pthread_mutex_t)*num_processes);

    if(ready == NULL || done == NULL){
        printf("unable to allocate mutex array\n");
        exit(EXIT_FAILURE);
    }

    /*alloco spazio per i buffers che conterranno le stringhe durante l'elaborazione*/
    buffers = (char**)malloc(sizeof(char*)*num_processes);

    if(buffers == NULL){
        printf("Unable to allocate buffers\n");
        exit(EXIT_FAILURE);
    }

    for(i=0; i<num_processes; i++){

        /*inizializzo l'array di mutex Ready*/
        if(pthread_mutex_init(ready+i, NULL) != 0){
            printf("Unable to initialize Ready mutex\n");
            exit(EXIT_FAILURE);
        }

        /*inizializzo l'array di mutex Ready*/
        if(pthread_mutex_init(done+i, NULL) != 0){
            printf("Unable to initialize Done mutex\n");
            exit(EXIT_FAILURE);
        }

        /*tento di eseguire il lock sul semaforo ready*/
        if(pthread_mutex_lock(ready+i) != 0){
            printf("Unable to lock Ready mutex\n");
            exit(EXIT_FAILURE);
        }
    }

    /*genero gli N+1 thread*/
    for(i = 0; i < num_processes; i++){

        ret = pthread_create(&tid, NULL, thread_function, (void*)i);

        if(ret != 0){
            printf("Unable to spawn threads\n");
            exit(EXIT_FAILURE);
        }
    }

    signal(SIGINT, printer);

    while(1){

read_again:
        ret = scanf("%ms", &string);

        /*controllo se la scanf è stata interrotta dall'arrivo di un segnale*/
        if(ret == EOF && errno == EINTR){
            goto read_again;
        }
        printf("Input string (memory address %p) is : %s\n", string, string);

redo1:
        /*provo a prendere il lock sul mutex Done associato al primo thread, se ci riesco vuol dire che ha 
        terminato la sua precedente esecuzione e può ricevere la nuova stringa da elaborare */
        if(pthread_mutex_lock(done)){
            if(errno == EINTR){
                goto redo1;
            }
            printf("Main thread - error locking Done mutex for first thread\n");
            exit(EXIT_FAILURE);
        }
        buffers[0] = string;

redo2: 
        /*provo ad eseguire l'unlock sul mutex Ready, quando riuscirò a prenderlo vuol dire che il thread è in 
        stato di ready pronto per attendere la nuova stringa */
        if(pthread_mutex_unlock(ready)){
            if(errno == EINTR){
                goto redo2;
            }
            printf("Main thread - error locking Ready mutex for first thread\n");
            exit(EXIT_FAILURE);
        }


    }
    
    return 0;
}