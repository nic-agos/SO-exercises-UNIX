/*
Scrivere un programma in C che prende
inizialmente una stringa da input (può contenere
anche spazi bianchi) e la salva in un buffer, per
poi fork-are un processo figlio che manda in
stampa la stessa stringa acquisita dal processo
padre.
In più è richiesto che il processo padre termini
solo dopo che il processo figlio ha terminato
(verificare che tale ordine è rispettato stampando i
PID dei processi).
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void main (int argc, char *argv[]){
    
    char *buff;

    pid_t child;
    int status;

    printf("Inserire una stringa: \n");
    scanf("%m[^\n]", &buff);

    printf("Ho letto la stringa: %s\n", buff);

    child = fork();

    if (child == 0){
        printf("Sono il processo figlio, il mio pid è: %d\n", getpid());
        printf("%s\n", buff);
        exit(0);
    }
    else{
        wait(&status);
        printf("Sono il processo padre, il mio pid è: %d\n", getpid());
        printf("%d", status);
        exit(0);
    }

}