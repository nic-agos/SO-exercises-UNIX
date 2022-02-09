/*
Scrivere un programma per Windows/Unix che
permette al processo principale P di create un
nuovo thread T il cui percorso di esecuzione è
associato alla funzione “thread_function”.
Il processo principale P ed il nuovo thread T
dovranno stampare ad output una stringa che li
identifichi rispettando l’ordine T→P, senza
utilizzare
“WaitForSingleObject”/“pthread_join”,
ma sfruttando un concetto fondamentale che
accomuna tutti i threads di un determinato
processo.
*/

#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int shared;

void *thread_function(){

    printf("Child thread is executing\n");
    pthread_t id = pthread_self();
    pid_t pid = getpid();
    printf("Child pid: %d\n", pid);
    printf("Child thread id: %ld\n", id);
    sleep(5);
    printf("Child thread is terminating\n");
    fflush(stdout);
    shared = 0;
    pthread_exit((void *) 1);

}

void main (int argc, char *argv[]){

    pthread_t child;
    int result;

    shared = 1;

    printf("Parent thread is executing\n");
    fflush(stdout);

    result = pthread_create(&child, NULL, thread_function, NULL);
    
    if (result !=0){
        printf("Impossible to create child process\n");
        exit(0);
    }else{
        while(shared);

        printf("\nParent pid is executing\n");
        pid_t pid = getpid();
        printf("Parent pid: %d\n", pid);
        pthread_t id = pthread_self();
        printf("Parent thread id: %ld\n", id);
        printf("Parent process is terminating\n");
        exit(0);
    }
}