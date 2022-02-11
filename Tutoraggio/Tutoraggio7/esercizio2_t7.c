/*
Scrivere un programma in ambiente UNIX che,
utilizzando le funzioni messe a disposizione dalla
libreria stdio:
• apre gli streams per un file sorgente e per un
file destinazione (può essere creato all’atto
dell’apertura);
• legge il contenuto dal file sorgente;
• filtra tutte e sole le parole di lunghezza
superiore a 6 caratteri;
• scrive quest’ultime nel file di destinazione.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/wait.h>

static const char *rfilename = "file-to-analyze.txt";
static const char *wfilename = "file-word-length.txt";

void main(int argc, char *argv[]){

    FILE *rfilepointer;
    FILE *wfilepointer;
    char *line;
    char *token;
    int written;

    int wfd;

    wfd = open(wfilename, O_CREAT|O_RDWR, S_IRWXU|S_IRWXG|S_IRWXO);

    if (wfd == -1){
        printf("Unable to create destination file");
        exit(-1);
    }

    close(wfd);

    
    rfilepointer = fopen(rfilename, "r");
    if(rfilepointer == NULL){
        printf("Unable to create a stream to the source file");
        exit(-1);
    }

    wfilepointer = fopen(wfilename, "w");
    if(wfilepointer == NULL){
        printf("Unable to create a stream to the destination file");
        exit(-1);
    }
    while(fscanf(rfilepointer, "%m[^\n]", &line) != EOF){
    
        fgetc(rfilepointer);
        token = strtok(line, " ");

        while(token != NULL){
            
            if(strlen(token) > 6){
                written = fprintf(wfilepointer, "%s ", token);
                if (written < 0){
                    printf("Unable to write the word on the destination file\n");
                    exit(-1);
                }
            }
            token = strtok(NULL, " ");
        }

        if(fprintf(wfilepointer, "\n" ) < 0){
            printf("Unable to write the last symbol on the destiantion file\n");
            exit(-1);
        }

        /*
		 * Utilizziamo la funzione di libreria "fflush" per
		 * forzare il flush di tutti i dati bufferizzati a
		 * lato user-space sul buffer-cache in kernel-space.
		 */
		fflush(wfilepointer);

		free(line);
	}

	/*
	 * In maniera analoga a quanto visto con "fflush", la system call
	 * "fsync" richiede in modo bloccante il trasferimento del contenuto
	 * del buffer-cache a lato kernel-space sul device sottostante (e.g.,
	 * hard disk). Al ritorno abbiamo reale garanzia che i dati siano
	 * stati storati su memoria permanente.
	 *
	 * La funzione "fileno" restituisce il descrittore di file associato
	 * alla stream di I/O passato come argomento.
	 */
	fsync(fileno(wfilepointer));

	fclose(wfilepointer);
	fclose(rfilepointer);

    printf("Execution ended \n");

    exit(0);

}