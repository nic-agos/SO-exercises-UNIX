#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#define Error_(x) { \
    puts(x); \
    exit(1); \
}

#define Warning_(x) { \
    puts(x); \
}

#define DATA_SIZE 30

int main(int argc, char *argv[])
{
    char messaggio[DATA_SIZE];
    int  pid, status, fd[2];
    int ret;

    /*
     * Crea una nuova Pipe e ritorna due descrittori
     * di file, uno per la lettura ed uno per la
     * scrittura. La pipe è utilizzate per creare un
     * canale di comunicazione tra processi relazionati.
     */
    ret = pipe(fd);
    if (ret == -1)
        Error_("Errore nella chiamata pipe");

    pid = fork();
    if (pid == -1)
        Error_("Errore nella fork");


    if (pid == 0)
    {
        /*
         * Chiusura del canale di scrittura per
         * prevenire possibili deadlocks.
         */
        close(fd[1]);
        
        /*
         * Un processo che legge dalla pipe è bloccato
         * in attesa della disponibilità del dato.
         *
         * Solo quando lo scrittore chiuderà il suo
         * descrittore allora il processo lettore verrà
         * sbloccato per leggere EOF e tornare zero
         * bytes letti.
         */
        while(read(fd[0], messaggio, DATA_SIZE) > 0)
            printf("Letto il Messaggio: %s", messaggio);
        
        close(fd[0]);
    }
    else
    {
        /*
         * Chiusura del canale di lettura per
         * prevenire possibili deadlocks.
         */
        close(fd[0]);

        Warning_("Digitare il testo da trasferire (quit per terminare): ");

        do
        {
            fgets(messaggio, DATA_SIZE, stdin);

            /*
             * Un processo che prova a scrivere su una pipe
             * piena rimane bloccato fino a quando non vi
             * sarà sufficiente spazio per accogliere il
             * prossimo dato.
             *
             * Tale comportamento può essere modificato
             * impostando O_NONBLOCK tramite fcntl con il
             * comando F_SETFL, ed in caso la pipe sarà piena
             * la write tornerà errore.
             */
            write(fd[1], messaggio, DATA_SIZE);

            printf("Scritto il Messaggio: %s", messaggio);
        }
        while(strcmp(messaggio,"quit\n") != 0);

        close(fd[1]);

        wait(&status);
    }
}