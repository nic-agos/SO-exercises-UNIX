/*
Scrivere un programma in C che prende
inizialmente N (a piacere) stringhe rappresentanti
N directory corrette, e che fork-a quindi N processi
che andranno a stampare ognuno il contenuto di
una directory differente.
Il processo padre termina dopo i processi figli (non
serve la verifica).
(Hint: è necessario utilizzare execlp)
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_PATHS 4

void main (int argc, char *argv[]){

    char paths[MAX_PATHS][256];

    pid_t child;
    int status;
    int forked = 0;

    char *buff;

    for (int i = 0; i <MAX_PATHS; i++){
        printf("Inserire path numero %d: ", i+1);
        scanf("%s[^\n]", paths[i]);
        printf("Ho letto il path: %s\n", paths[i]);
    }
    
    for (int i = 0; i <MAX_PATHS; i++){
        child = fork();
        if(child == -1){
            printf("impossibile eseguire la fork del processo %d\n", i);
        }else{
            forked = forked +1;
        }
        if (child == 0){
            printf("sono nel processo child\n");
            execlp("ls", "ls", paths[i], NULL);
            exit(0);
        }
    }

    for (int i = 0; i < forked; i++){
        wait(&status);
        printf("Codice di uscita: %d\n", status);
    }



}