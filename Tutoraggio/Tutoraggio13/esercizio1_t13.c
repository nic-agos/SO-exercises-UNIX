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
#include <sys/types.h>
#include <signal.h>

pid_t pid1;
pid_t pid2;

void handler_sigusr1(int sig){

    printf("Process %d received signal %d\n", pid1, sig);
    sleep(1);
    /*invio il segnale SIGUSR2 al processo child */
    kill(pid2, SIGUSR2);

}

void handler_sigusr2(int sig){

    printf("Process %d received signal %d\n", pid2, sig);
    sleep(1);
    /*invio il segnale SIGUSR1 al processo main */
    kill(pid1, SIGUSR1);

}

void main(int argc, char *argv[]){

    sigset_t set1;
    sigset_t old_set1;

    sigset_t set2;
    sigset_t old_set2;

    struct sigaction act1;
    struct sigaction act2;
    struct sigaction old_act1;
    struct sigaction old_act2;

    memset((void *)&act1, 0, sizeof(struct sigaction));
    memset((void *)&act2, 0, sizeof(struct sigaction));

    pid1 = getpid();

    printf("Parent pid: %d\n", pid1);

    /*inserisco nella maschera set1 tutti i segnali*/
    if(sigfillset(&set1) == -1){
        printf("Unable to fill signal set\n");
        exit(-1);
    }

    /*blocco tutti i segnali per il processo main finchè non siamo pronti a gestirli*/
    if(sigprocmask(SIG_BLOCK, &set1, &old_set1) == -1){
        printf("Unable to block all the signal for the main thread\n");
        exit(-1);

    }

    /*blocco tutti i segnali durante l'esecuzione di handler_sigusr1 ed imposto il gestore*/
    act1.sa_mask = set1;
    act1.sa_handler = handler_sigusr1;
    
    /*imposto il gestore del segnale per SIGUSR1*/
    if(sigaction(SIGUSR1, &act1, &old_act1 ) == -1){
        printf("Unable to set the handler for SIGUSR1\n");
        exit(-1);
    }

    /*sblocco SIGINT e SIGUSR1 per il main thread*/
    if(sigemptyset(&set1) == -1){
        printf("Unable to empty the signal set\n");
        exit(-1);
    }

    if(sigaddset(&set1, SIGINT) == -1){
        printf("Unable to add SIGINT to the signal set\n");
        exit(-1);
    }

    if(sigaddset(&set1, SIGUSR1) == -1){
        printf("Unable to add SIGUSR1 to the signal set\n");
        exit(-1);
    }

    if(sigprocmask(SIG_UNBLOCK, &set1, NULL) == -1){
        printf("Unable to unlock SIGINT and SIGUSR1 for main thread\n");
        exit(-1);
    }


    
    if((pid2 = fork()) == -1){
        printf("Unable to fork process\n");
        exit(-1);
    }

    if(pid2 == 0){
        /*sono nel processo figlio*/

        pid2 = getpid();

        printf("Child pid: %d\n", pid2);

        /*inserisco nella maschera set2 tutti i segnali*/
        if(sigfillset(&set2) == -1){
            printf("Unable to fill signal set\n");
            exit(-1);
        }

        /*blocco tutti i segnali durante l'esecuzione di handler_sigusr2 ed imposto il gestore*/
        act2.sa_mask = set2;
        act2.sa_handler = handler_sigusr2;

        /*imposto il gestore del segnale per SIGUSR1*/
        if(sigaction(SIGUSR2, &act2, &old_act2) == -1){
            printf("Unable to set the handler for SIGUSR1\n");
            exit(-1);
        }

        /*sblocco SIGINT e SIGUSR2 per il processo child*/
        if(sigemptyset(&set2) == -1){
            printf("Unable to empty the signal set\n");
            exit(-1);
        }

        if(sigaddset(&set2, SIGINT) == -1){
            printf("Unable to add SIGINT to the signal set\n");
            exit(-1);
        }

        if(sigaddset(&set2, SIGUSR2) == -1){
            printf("Unable to add SIGUSR2 to the signal set\n");
            exit(-1);
        }

        if(sigprocmask(SIG_UNBLOCK, &set2, NULL) == -1){
            printf("Unable to unlock SIGINT and SIGUSR2 for child process\n");
            exit(-1);
        }

        while (raise(SIGUSR1)) ;

		while (1)
		{
			pause();
		}

        
    }else{
        /*sono nel processo padre*/

        while (1)
		{
			pause();
		}

    }

    /* Restores both the old action for SIGUSR1 signal and 
       the old signal mask for this process.  */
	if (sigaction(SIGUSR1, &old_act1, NULL)) exit(-1);
	if (sigprocmask(SIG_SETMASK, &old_set1, NULL)) exit(-1);

    /* Restores both the old action for SIGUSR2 signal and 
       the old signal mask for this process.  */
    if (sigaction(SIGUSR2, &old_act2, NULL)) exit(-1);
	if (sigprocmask(SIG_SETMASK, &old_set2, NULL)) exit(-1);

	exit(0);

}