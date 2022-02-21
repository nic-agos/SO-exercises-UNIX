/*
Implementare un'applicazione che riceva in input tramite argv[] il 
nome di un file F ed una stringa indicante un valore numerico N maggiore
o uguale ad 1.
L'applicazione, una volta lanciata dovra' creare il file F ed attivare 
N thread. Inoltre, l'applicazione dovra' anche attivare un processo 
figlio, in cui vengano attivati altri N thread. 
I due processi che risulteranno attivi verranno per comodita' identificati
come A (il padre) e B (il figlio) nella successiva descrizione.

Ciascun thread del processo A leggera' stringhe da standard input. 
Ogni stringa letta dovra' essere comunicata al corrispettivo thread 
del processo B tramite memoria condivisa, e questo la scrivera' su una 
nuova linea del file F. Per semplicita' si assuma che ogni stringa non
ecceda la taglia di 4KB. 

L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso
WinAPI) in modo tale che quando il processo A venga colpito esso dovra' 
inviare la stessa segnalazione verso il processo B. Se invece ad essere 
colpito e' il processo B, questo dovra' riversare su standard output il 
contenuto corrente del file F.

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

#define MAX_LENGHT 4096

FILE *output_file;
int ready, done;
char **shared_memory;
pid_t child;
long num_threads;
char *file_name;

void *child_thread(void *arg){
    
    long me = (long)arg;
    int ret;
    struct sembuf oper;

    printf("I'm the child thread %d\n", me);

    oper.sem_num = me;
    oper.sem_flg = 0;

    while(1){

oper1:
        oper.sem_op = -1;

        /*controllo se il relativo thread parent mi ha sbloccato, cosa che accade se mi ha consegnato una stringa*/
        ret = semop(done, &oper,1);

        if (ret == -1 && errno != EINTR){
			printf("Unable to lock Done semaphore in child thread %d", me);
            exit(EXIT_FAILURE);
		}
		if (ret == -1) goto oper1;

        printf("I'm the child thread %d, reading the string %s", me, shared_memory[me]);

        /*scrivo sul file di output la stringa ricevuta dal relativo thread parent*/
        fprintf(output_file, "%s\n", shared_memory[me]);

        /*forzo la scrittura della stringa sul file*/
        fflush(output_file);

oper2:
        /*segnalo al relativo thread parent di aver terminato la mia esecuzione*/
        oper.sem_op = 1;

        ret = semop(ready, &oper, 1);

        if (ret == -1 && errno != EINTR){
			printf("Unable to lock Done semaphore in child thread %d", me);
            exit(EXIT_FAILURE);
		}
		if (ret == -1) goto oper2;

    }
    return NULL;

}

void *parent_thread(void *arg){

    long me = (long)arg;
    int ret;
    struct sembuf oper;

    printf("I'm the parent thread %d\n", me);

    oper.sem_num = me;
    oper.sem_flg = 0;

    while(1){

        oper.sem_op = -1;

oper1:
        /*tento di ottenere il lock sul semaforo ready per capire 
          se posso leggere una nuova string da stdint*/
        ret = semop(ready, &oper, 1);

        if (ret == -1 && errno != EINTR){
			printf("Unable to lock Ready semaphore in parent thread %d", me);
            exit(EXIT_FAILURE);
		}
		if (ret == -1) goto oper1;

oper2:
        /*inserisco la stringa prelevata da stdin nella i-esima entry del buffer condiviso*/
        ret = scanf("%s", shared_memory[me]);

        if(ret == EOF && errno != EINTR){
            printf("Unable to read from the terminal\n");
            exit(EXIT_FAILURE);
        }
        if (ret == -1) goto oper2;
        
        printf("I'm the parent thread %d, writing the string %s on the shared memory\n", me, shared_memory[me]);

        oper.sem_op = 1;

oper3:  
        /*segnalo al relativo thread figlio che ho consegnato dei dati per lui*/
        ret = semop(done, &oper, 1);

        if(ret == -1 && errno != EINTR){
            printf("Unable to unlock Done semaphore in parent thread %d\n", me);
            exit(EXIT_FAILURE);
        }
        if (ret == -1) goto oper2;

    }
    return NULL;
}

void child_handler(int dummy){

    char cmd[1024];
    
    sprintf(cmd, "cat %s\n", file_name);
    system(cmd);

}

void parent_handler(int signo){

    printf("I'm the parent handler, sending signal %d to child process identified by pid: %d\n", signo, child);
    kill(child, signo);

}


int main(int argc, char **argv){

    pthread_t tid;
    long i;
    int ret;


    if(argc != 3){
        printf("Usage: prog filename NumThreads\n");
        exit(EXIT_FAILURE);
    }

    /*converto in long la stringa indicante il numero dei threads passata al main*/
    num_threads = strtol(argv[2], NULL, 10);

    if(num_threads < 1){
        printf("NumThreads must be greater than 1");
        exit(EXIT_FAILURE);
    }

    file_name = argv[1];

    /*invoco la creazione del file di output, se non esiste già verrà 
      creato mentre se già esistente viene troncato*/
    output_file = fopen(argv[1], "w+");

    if(output_file == NULL){
        printf("Unable to create output file\n");
        exit(EXIT_FAILURE);
    }

    /*alloco spazio per i buffer di memoria condivisa*/
    shared_memory = malloc(sizeof(char *)*num_threads);

    if(shared_memory == NULL){
        printf("Unable to allocate the shared memory\n");
        exit(EXIT_FAILURE);
    }

    /*eseguo il mapping di ogni entry della zona di memoria condivisa*/
    for(i = 0; i<num_threads; i++){

        shared_memory[i] = (char*)mmap(NULL, MAX_LENGHT, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS , 0, 0);
        
        if(shared_memory[i] == NULL){
            printf("Unable to map memory\n");
            exit(EXIT_FAILURE);
        }
    }

    /*istanzio l'array semaforico da N elementi */
    ready = semget(IPC_PRIVATE, num_threads, IPC_CREAT | 0666);

    if(ready == -1){
        printf("Unable to initialize Ready semaphores\n");
        exit(EXIT_FAILURE);
    }   

    /*inizializzo tutti i semafori a 1*/
    for(i = 0; i < num_threads; i++){
        ret = semctl(ready, i, SETVAL, 1);
        if(ret == -1){
            printf("Unable to set Ready semaphores\n");
        }
    }

    /*istanzio l'array semaforico Done*/
    done = semget(IPC_PRIVATE, num_threads, IPC_CREAT | 0666);

    if(done == -1){
        printf("Unable to initialize Done semaphores\n");
        exit(EXIT_FAILURE);
    }

    /*inizializzo i semafori Done a 0*/
    for(i = 0; i < num_threads; i++){
        ret = semctl(done, i, SETVAL, 0);
        if(ret == -1){
            printf("Unable to set Done semaphores\n");
            exit(EXIT_FAILURE);
        }
    }

    /*eseguo la fork del processo main*/
    child = fork();

    if(child == -1){
        printf("Unable to fork main process\n");
        exit(EXIT_FAILURE);
    }
    
    if(child == 0){
        /*sono nel processo figlio*/
        
        /*imposto la gestione del segnale SIGINT per il child process*/
        signal(SIGINT, child_handler);

        for(i = 0; i < num_threads; i++){
            if(pthread_create(&tid, NULL, child_thread, (void *)i) == -1){
                printf("Unable to spwan threads in child process\n");
                exit(EXIT_FAILURE);
            }
        }

    }
    else{
        /*sono nel processo padre*/

        /*imposto la gestione del segnale SIGINT per il main process*/
        signal(SIGINT, parent_handler);

        for(i = 0; i < num_threads; i++){
            if(pthread_create(&tid, NULL, parent_thread, (void *)i) == -1){
                printf("Unable to spwan threads in main process\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    /*mi metto in attesa di un qualsiasi segnale*/
    while(1){
        pause();
    }  

}