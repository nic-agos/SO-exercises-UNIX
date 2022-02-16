/*
Implementare una programma che riceva in input, tramite argv[], il nomi
di N file (con N maggiore o uguale a 1).
Per ogni nome di file F_i ricevuto input dovra' essere attivato un nuovo thread T_i.
Il main thread dovra' leggere indefinitamente stringhe dallo standard-input 
e dovra' rendere ogni stringa letta disponibile ad uno solo degli altri N thread
secondo uno schema circolare.
Ciascun thread T_i a sua volta, per ogni stringa letta dal main thread e resa a lui disponibile, 
dovra' scriverla su una nuova linea del file F_i. 

L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso
WinAPI) in modo tale che quando il processo venga colpito esso dovra' 
riversare su standard-output e su un apposito file chiamato "output-file" il 
contenuto di tutti i file F_i gestiti dall'applicazione ricostruendo esattamente 
la stessa sequenza di stringhe (ciascuna riportata su 
una linea diversa) che era stata immessa tramite lo standard-input.

In caso non vi sia immissione di dati sullo standard-input, l'applicazione dovra' utilizzare 
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

int num_threads;

char **files; /*array di puntatori a stringhe per contenere i nomi dei files che verranno passati in input al main*/
char **buffers; /*array di puntatori a stringhe dove memorizzare temporaneamente le stringhe da passare ad ogni thread*/
FILE **source_files; /*array di puntatori a stream su files*/
pthread_mutex_t *ready; /*array di puntatori a identificatori di mutex*/
pthread_mutex_t *done; /*array di puntatori a identificatori di mutex*/
FILE *output_file; /*puntatore per ospitare lo stream aperto sul file di output*/

void *thread_function(void *arg){

    long int me = (long int)arg;
    sigset_t set;
    FILE *target_file;

    printf("thread n. %d started up _ in charge of file %s\n", me, files[me]);
    fflush(stdout);

    /* w+ : Open for reading and writing. The file is created if it does not exist, otherwise it is truncated.  
            The stream is positioned at the beginning of the file.*/
    target_file = fopen(files[me],"w+");

    if(target_file == NULL){
        printf("file %s opening error\n", files[me]);
        exit(EXIT_FAILURE);
    }

    /*inserisco tutti i segnali all'interno della maschera set*/
    sigfillset(&set);

    /*blocco tutti i segnali in arrivo per il thread, senza salvare la vecchia signal mask*/
    sigprocmask(SIG_BLOCK, &set, NULL);

    while(1){

        if(pthread_mutex_lock(ready+me)){
            printf("Unable to lock Ready mutex in thread %d\n", me);
            exit(EXIT_FAILURE);
        }

        printf("thread n. %d got string %s\n", me, buffers[me]);

        /*scrivo sul file la nuova riga ahce ho ricevuto dal main thread*/
        fprintf(target_file, "%s\n", buffers[me]);

        /*forzo la scrittura dei dati sul file*/
        fflush(target_file);

        /*sblocco il thread Done per segnalare che ho terminato l'esecuzione dell'operazione*/
        if(pthread_mutex_unlock(done+me)){
            printf("Unable to unlock Done mutex in thread %d\n", me);
            exit(EXIT_FAILURE);
        }

    }

}

void printer(int dummy){

    int i;
    char *s;
    int ret;

    for(i = 0; i<num_threads; i++){
        source_files[i] = fopen(files[i], "r");
        if(source_files[i] == NULL){
            printf("Signal handler - unable to open file %s\n", source_files[i]);
            exit(EXIT_FAILURE);
        }
    }

    output_file = fopen("output-file", "w+");
    if(output_file == NULL){
        printf("Unable to open file %s\n", output_file);
        exit(EXIT_FAILURE);
    }

    i = 0;

    while(1){
        ret = fscanf(source_files[i], "%ms", &s);
        if(ret == EOF){
            /*sono arrivato alla fine del file */
            break;
        }
        /*stampo su terminale la stringa letta*/
        printf("%s", s);

        /*svuoto il buffer di stdout*/
        fflush(stdout);

        /*scrivo la stringa letta sul file di output*/
        fprintf(output_file,"%s\n", s);

        /*forzo la scrittura sul file di output*/
        fflush(output_file);

        /*libero la variabile s*/
        free(s);

        i = (i+1)%num_threads;
    }

}

int main(int argc, char **argv){

    int i;
    int ret;
    pthread_t tid;
    char *s;

    if(argc < 2){
        printf("Incorrect number of arguents\n");
        exit(EXIT_FAILURE);
    }

    files = argv + 1; /*per evitare in inserire argv[0] (nome dell'eseguibile) nell'array di stringhe */ 

    num_threads = argc - 1;

    /*alloco lo spazio necessario a contenere un buffer per ogni thread da attivare*/
    buffers = (char**)malloc(sizeof(char*)*num_threads);

    if(buffers == NULL){
        printf("Buffer pointers allocation failure\n");
        exit(EXIT_FAILURE);
    }

    /*alloco spazio per contenere i puntatori agli stream aperti su file*/
    source_files = (FILE **)malloc(sizeof(FILE *)*num_threads);

    if(source_files == NULL){
        printf("file pointers allocation failure\n");
        exit(EXIT_FAILURE);
    }

    /*alloco spazio per contenere i puntatori agli identificatori dei mutex*/
    ready = malloc(num_threads * sizeof(pthread_mutex_t));
    done = malloc(num_threads * sizeof(pthread_mutex_t));

    if (ready == NULL || done == NULL){
        printf("Mutex array allocation failure\n");
        exit(EXIT_FAILURE);
    }

    /*Associo i mutex all'i-esimo slot di ready e done ed eseguo il lock di tutty i mutex Ready*/
    for (i = 0; i < num_threads; i++){
        if(pthread_mutex_init(ready+i, NULL) != 0){
            printf("Ready mutex initialization failure\n");
            exit(EXIT_FAILURE);
        }
        if(pthread_mutex_init(done+i, NULL) != 0){
            printf("Done mutex initialization failure\n");
            exit(EXIT_FAILURE);
        }
        if(pthread_mutex_lock(ready+i)){
            printf("Unable to lock \n");
            exit(EXIT_FAILURE);
        }
    }

    /*genero N thread*/
    for (i = 0; i < num_threads; i++){
        ret = pthread_create(&tid, NULL, thread_function, (void *)i);
        if(ret != 0){
            printf("Unable to spawn thread n. %d", i);
            exit(EXIT_FAILURE);
        }
    }

    /*associo al segnal SIGINT la funzione di gestione printer*/
    signal(SIGINT, printer);

    i = 0;

    while(1){

read_again:
        ret = scanf("%ms", &s);
        /*se abbiamo un errore nella scanf questa ritorna EOF ed imposta errno con una macro che indica il tipo di errore,
          errno = EINTR quando l'eecuzione viene interrota dall'arrivo di un segnale*/
        if(ret == EOF && errno == EINTR){
            /*se vengo bloccato da un segnale devo rieseguire la scanf, l'unico segnale gestitio esplicitamente è SIGINT*/
            goto read_again;
        }
        printf("Read string (area is at address %p): %s\n", s, s);

redo_1: 
        /* provo ad eseguire il lock sul mutex i-esimo Done, quando riuscirò a prenderlo vuol dire che 
           l'i-esimo thread ha terinato la sua precedente esecuzione */
        if(pthread_mutex_lock(done+i)){
            if(errno == EINTR){
                goto redo_1;
            }
            printf("Main thread - Unable to lock mutex\n");
            exit(EXIT_FAILURE);
        }
        buffers[i] = s;

redo_2:
        /*provo ad eseguire l'unlock sul mutex i-esimo Ready, quando riuscirò a prenderlo vuol dire che il thread è in 
        stato di ready pronto per attendere la nuova stringa una volta arrivato il suo turno */
        if(pthread_mutex_unlock(ready+1)){
            if(errno == EINTR){
                goto redo_2;
            }
            printf("Main thread - Unable to unlock mutex\n");
            exit(EXIT_FAILURE);
        }
        i = (i+1)%num_threads;
    }

    return 0;
}