/*
Implementare una programma che riceva in input, tramite argv[], il nome
di un file F ed N stringhe S_1 .. S_N (con N maggiore o uguale
ad 1.
Per ogni stringa S_i dovra' essere attivato un nuovo thread T_i, che fungera'
da gestore della stringa S_i.
Il main thread dovra' leggere indefinitamente stringhe dallo standard-input. 
Ogni nuova stringa letta dovra' essere comunicata a tutti i thread T_1 .. T_N
tramite un buffer condiviso, e ciascun thread T_i dovra' verificare se tale
stringa sia uguale alla stringa S_i da lui gestita. In caso positivo, ogni
carattere della stringa immessa dovra' essere sostituito dal carattere '*'.
Dopo che i thread T_1 .. T_N hanno analizzato la stringa, ed eventualmente questa
sia stata modificata, il main thread dovra' scrivere tale stringa (modificata o non)
su una nuova linea del file F. 
In altre parole, la sequenza di stringhe provenienti dallo standard-input dovra' 
essere riportata su file F in una forma 'epurata'  delle stringhe S1 .. SN, 
che verranno sostituite da strighe  della stessa lunghezza costituite esclusivamente
da sequenze del carattere '*'.
Inoltre, qualora gia' esistente, il file F dovra' essere troncato (o rigenerato) 
all'atto del lancio dell'applicazione.

L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso
WinAPI) in modo tale che quando il processo venga colpito esso dovra' 
riversare su standard-output il contenuto corrente del file F.

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

char *file_name;
int num_processes;
FILE *output_file;
char **strings;
char buff[4096];

sem_t *write_sem;
sem_t *read_sem;

void printer(){

    char cmd[4096];
    printf("\n");
    sprintf(cmd, "cat %s", file_name);
    system(cmd);
    int ret;

}

void *thread_function(void *arg){

    long int me = (long int)arg;
    int ret;
    int i;

    printf("I'm the thread %d in charge of string %s\n", me, strings[me]);

    while(1){
step1:
        ret = sem_wait(&write_sem[me]);
        if(ret != 0 && errno == EINTR){
            goto step1;
        }
        /*controllo se la stringa in input dal terminale coincide con la stringa a me assegnata*/
        if(strcmp(buff, strings[me]) == 0){
            printf("Thread %d string match found\n", me);

            /*sostituisco la stringa presente in buf con un solo asterisco per poi concatenare i restanti asterischi*/
            strcpy(buff, "*");
            for(i =0; i<(strlen(strings[me])-1); i++){
                strcat(buff, "*");
            }

        }
step2:
        ret = sem_post(&read_sem[me]);
        if(ret != 0 && errno == EINTR){
            goto step2;
        }

    }

}

int main(int argc, char **argv){

    int i;
    struct sigaction sa;
    sigset_t set;
    pthread_t pid;
    char *s;
    int ret;
    int ret2;

    if(argc < 3){
        printf("Usage: prog filename string1 ... [stringN]\n");
        exit(EXIT_FAILURE);
    }

    strings = argv + 2;

    file_name = argv[1];

    num_processes = argc - 2;

    /*invoco l'apertura del file di output in scrittura, se non presente verrà creato e se presente viene troncato*/
    output_file = fopen(file_name, "w+");

    if(output_file == NULL){
        printf("Unable to open output file\n");
        exit(EXIT_FAILURE);
    }

    /*alloco lo spazio necessario a contentere i puntatori ai semafori*/
    write_sem = malloc(sizeof(sem_t)*num_processes);
    read_sem = malloc(sizeof(sem_t)*num_processes);

    if(write_sem == NULL || read_sem == NULL){
        printf("Unable to allocate semaphores array \n");
        exit(EXIT_FAILURE);
    }

    for(i = 0; i < num_processes; i++){

        /*inizializzo i semafori Read con valore 0 e l'opzione di condivisione*/
        if(sem_init(&write_sem[i], 0, 0) != 0){
            printf("Unable to initialize read semaphores\n");
            exit(EXIT_FAILURE);
        }

        /*inizializzo i semafori Write con valore 1 e l'opzione di condivisione*/
        if(sem_init(&read_sem[i], 0, 1) != 0){
            printf("Unable to initialize write semaphores\n");
            exit(EXIT_FAILURE);
        }
    }

    /*svuoto il set di tutti i segnali*/
    sigemptyset(&set);
    sa.sa_handler = printer;
    sa.sa_mask = set;
    sa.sa_flags = SA_SIGINFO;

    /*imposto la gestione del segnale SIGINT per il thread main*/
    if(sigaction(SIGINT, &sa, NULL) != 0){
        printf("Unable to set SIGINT event information\n");
        exit(EXIT_FAILURE);
    }

    /*creo gli N threads*/
    for(i=0; i< num_processes; i++){
        if(pthread_create(&pid, NULL, thread_function, (void *)i) != 0){
            printf("Unable to spawn threads\n");
            exit(EXIT_FAILURE);
        }
    }
    

    /*inizializzo il buffer con il carattere vuoto*/
    strcpy(buff, "\0");

    /*aggiorno lo stream sul file in lettura/scrittura*/
    output_file = fopen(file_name, "r+");

    if(output_file == NULL){
        printf("Unable to open the outut file for the read/write\n");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        /*attendo che tutti i thread siano pronti a leggere*/
        for(i = 0; i < num_processes; i++){
redo1:  
            
            if(sem_wait(&read_sem[i]) != 0){
                if(errno == EINTR){
                    goto redo1;
                }
                printf("Unable to wait Read semaphore for thread %d\n", i);
                exit(EXIT_FAILURE);
            }
    
        }
        
        /*controllo se nel buffer è presente un qualcosa diverso dal carattere vuoto, se si vuol dire che 
          gli N thread hanno eseguito una coputazione quindi devo srivere su file*/
        if(strcmp(buff, "\0") !=0){
            fprintf(output_file, "%s\n", buff);
            fflush(output_file);
        }

        /*rendo disponibile la stringa presa in input a tutti i thread*/
        while(scanf("%s", buff) <= 0);

        /*segnalo a tutti i thred la possibilità di lavorare*/
        for(i = 0; i < num_processes; i++){
redo2:
            if(sem_post(&write_sem[i]) != 0){
                if(errno == EINTR){
                    goto redo1;
                }
                printf("Unable to signal Write semaphore for thread %d\n", i);
                exit(EXIT_FAILURE);
            }
            
        }

    }
    return 0;
}