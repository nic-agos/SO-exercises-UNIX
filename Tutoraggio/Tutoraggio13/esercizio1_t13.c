/*
Scrivere un programma in C in ambiente UNIX
che permette a due processi P1 e P2 di scrivere a
standard output (stdout) in STRETTA ALTERNANZA.
La coordinazione tra i due processi P1 e P2 deve
essere effettuata per mezzo di appositi segnali
(e.g. SIGUSR1) al fine di comunicare al processo
concorrente la terminazione della propria attività e
dare ad esso la possibilità di effettuare la sua
propria.
*/

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

void main(int argc, char *argv[]){

    

}