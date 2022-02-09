/*
Scrivere un programma per Unix che sia in grado
di generare due Thread T1 e T2, tali per cui:
-T1 chiede all’utente di inserire una messaggio da
tastiera
-T2 scrive il messaggio ottenuto dall’utente a
schermo
con la condizione che non si possono utilizzare
variabili globali.
*/

/* compile with gcc esercizio2.c -lpthread -o esercizio2.out*/

#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>


void *thread1_function(void *args){

    char *buff;

    printf("Insert a string: ");
    scanf("%m[^\n]", &buff);

    pthread_exit((void *)buff);

}

void *thread2_function(void *args){
    
    char *buff;

    buff = (char *)args;
    
    if(buff == NULL){
        printf("Unable to read message from T1");
        return (void *)1;
    }

    printf("Ho letto: %s", buff);
    pthread_exit((char *)0);

}

int main(int argc, char *argv[]){

    pthread_t child;
    void *exitcode;
    void *buff;


    if(pthread_create(&child, NULL, thread1_function, NULL)){
        printf("Impossible to create T1");
        return 1;
    }

    if(pthread_join(child, &buff)){
        printf("Impossible to join T1\n");
        return 1;
    }

    

    if(pthread_create(&child, NULL, thread2_function, buff)){
        printf("Impossible to create T2");
        return 0;
    }

    if(pthread_join(child, &exitcode)){
        printf("Unable to join T2");
        return 1;
    }

    free(buff);
    return 0;

    

}