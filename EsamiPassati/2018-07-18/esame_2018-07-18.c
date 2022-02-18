/*
Implementare un programma che riceva in input tramite argv[] i pathname
associati ad N file, con N maggiore o uguale ad 1. Per ognuno di questi
file generi un processo che legga tutte le stringhe contenute in quel file
e le scriva in un'area di memoria condivisa con il processo padre. Si
supponga per semplicita' che lo spazio necessario a memorizzare le stringhe
di ognuno di tali file non ecceda 4KB.
Il processo padre dovra' attendere che tutti i figli abbiano scritto in
memoria il file a loro associato, e successivamente dovra' entrare in pausa
indefinita.
D'altro canto, ogni figlio dopo aver scritto il contenuto del file nell'area
di memoria condivisa con il padre entrera' in pausa indefinita.
L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso
WinAPI) in modo tale che quando il processo padre venga colpito da esso dovra'
stampare a terminale il contenuto corrente di tutte le aree di memoria
condivisa anche se queste non sono state completamente popolate dai processi
figli.
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

#define MAX_LENGTH 4096

void **shared_memory;
int num_processes;
int sem;
int fd;
char *segment;
int ret;

void thread_function(){

    FILE *file;

    struct sembuf oper;

    file = fdopen(fd, "r+");

    if(file == NULL){
        printf("Unable to open file in child process\n");
        exit(EXIT_FAILURE);
    }

    while(fscanf(file, "%s", segment) != EOF){
        printf("Read string: %s\n", segment);
        segment += strlen(segment) + 1;
    }

    oper.sem_flg = 0;
    oper.sem_op = 1;
    oper.sem_num = 0;

    /*segnalo al processo main che ho terminato*/
    if(semop(sem, &oper, 1) == -1){
        printf("Unable to signal semaphore\n");
        exit(EXIT_FAILURE);
    }

    while(1) pause();
}

void handler(int signo){

    printf("I'm the SIGINT handler\n");

    char *string;

    for(int i = 0; i < num_processes; i++){
        string = shared_memory[i];
        while(strcmp(string, "\0") != 0){
            printf("%s\n", string);
            string += strlen(string) + 1;
        }
    }
    
}

void main(int argc, char **argv)
{

    int i;
    struct sigaction sa;
    int ret;
    sigset_t set;
    struct sembuf oper;

    if (argc < 2)
    {
        printf("usage: command filename1 [filename2] .... [filenameN]\n");
        exit(EXIT_FAILURE);
    }

    num_processes = argc - 1;

    /*tento di aprire i file passati in input al main per verificare che esistano*/
    for (i = 0; i < num_processes; i++)
    {
        fd = open(argv[i+1], O_RDONLY);
        if (fd == -1)
        {
            printf("Unable to open file %s\n", argv[i]);
            exit(EXIT_FAILURE);
        }
    }

    shared_memory = malloc(sizeof(void *)*num_processes);
    if(shared_memory == NULL){
        printf("Unable to allocate space for the shared memory\n");
        exit(EXIT_FAILURE);
    }

    /*creo il semaforo che conterà quanti processi figli hanno eseguito la scrittura sulla propria zona di memoria*/
    sem = semget(IPC_PRIVATE, 1 , 0660);

    if (sem == -1)
    {
        printf("Unable to create semaphore\n");
        exit(EXIT_FAILURE);
    }

    /*inizializzo il semaforo con 0 token*/
    if (semctl(sem, 0, SETVAL, 0) == -1)
    {
        printf("Unable to initialize semaphore\n");
        exit(EXIT_FAILURE);
    }

    /*inserisoc tutti i segnali all'interno della maschera dei segnali*/
    sigfillset(&set);

    sa.sa_handler = handler;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_mask = set;

    if(sigaction(SIGINT, &sa, NULL) == -1){
        printf("Unable to set SIGINT event handler\n");
        exit(EXIT_FAILURE);

    }

    for (i = 0; i < num_processes; i++)
    {   
        printf("qui\n");
        shared_memory[i] = mmap(NULL, MAX_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
        if (shared_memory[i] == NULL)
        {
            printf("Unable to allocate memory\n");
            exit(EXIT_FAILURE);
        }

        printf("qui\n");

        segment = shared_memory[i];

        fd = open(argv[i+1], O_RDONLY);

        ret = fork();
        if(ret == -1){
            printf("Unable to fork process\n");
            exit(EXIT_FAILURE);
        }

        if(ret == 0){
            /*ignoro esplicitamente il segnale SIGINT il quale deve essere a carico del solo processo main*/
            signal(SIGINT, SIG_IGN);
            thread_function();

        }

    }

    oper.sem_num = 0;
    oper.sem_op = -num_processes;
    oper.sem_flg = 0;

redo:
    if(semop(sem, &oper,1) == -1){
        if(errno == EINTR){
            goto redo;
        }
        printf("Unable to wait childs\n");
        exit(EXIT_FAILURE);
    }
    printf("All childs terminates\n");

    while(1) pause();
    
}