/*
Scrivere un programma che una volta aperto un
file F in scrittura, genera altri N processi figli,
ognuno dei quali scrive su file F una stringa che lo
contraddistingue dagli altri processi (e.g. il suo
pid) per W volte.
Ognuno dei processi scrittori deve poter lavorare
in maniera esclusiva sull’intero file F, senza
interferire con gli altri scrittori (per poter ottenere
questo comportamento avrete bisogno di fcntl
che potete consultare sul man ed utilizzare quindi
il comando più adatto per il vosto scopo).
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/wait.h>

#define N_PROC 5
#define W 3

static const char *filename = "test_file.txt";

void main(int argc, char *argv[]){

    pid_t pid;
    int fd;
    int status;
    struct flock fl;
    pid_t mypid;
    char buff[10];
    size_t char_write;
    int len;

    fd = open(filename, O_CREAT|O_RDWR|O_TRUNC, S_IRWXU|S_IRWXG|S_IRWXO);

    if (fd == -1){
        printf("Unable to open file\n");
        exit(-1);
    }
    
    for (int i = 0; i < N_PROC; i++){

        pid = fork();

        if(pid == -1){
            printf("Unable to spawn process n. %d\n", i);
            exit(-1);

        }else if(pid == 0){

            mypid = getpid();

            /*tarsformo il pid in una stringa*/
            len = sprintf(buff, "%d\n", (int) mypid);

            /*
			 * Advisory Record Locking
			 * -----------------------
			 * Linux implementa un sistema UNIX recod lock per files
			 * come previsto dalle specifiche POSIX.
			 *
			 * Permette di definire ed utilizzare dei locks per un
			 * sotto-insieme di records del file o tutto il file.
			 *
			 * l_type = FWRLCK : richiede un write-lock sul file.
			 * l_type = F_UNLCK : richiede il rilascio del lock.
			 *
			 * l_whnce = SEEK_SET, l_start = 0, l_len = 0 : il lock
			 * è definito per tutti i records che vanno dall'inizio
			 * del file alla sua fine, indipendentemente da quanto
			 * questo possa crescere.
			 *
			 * F_SETLKW : richiede di settare il lock e, in caso fosse
			 * già acquisito, attende il suo rilascio.
			 */

			fl.l_type = F_WRLCK;
			fl.l_whence = SEEK_SET;
			fl.l_start = 0;
			fl.l_len = 0;
			fl.l_pid = 0;

            if (fcntl(fd, F_SETLKW, &fl) == -1) {  
				printf("Unable to lock file %s\n", filename);
				exit(-1);
			}

            for (int j = 0; j < W; j++){

                char_write = write(fd, buff, (size_t)len);

                if (char_write == -1){
                    printf("Unable to write on the file\n");
                    exit(-1);
                }
            }

            fl.l_type = F_UNLCK;

            if (fcntl(fd, F_SETLKW, &fl) == -1) {  
				printf("Unable to unlock file %s\n", filename);
				exit(-1);
			}

            close(fd);
			exit(0);

        }
    }

    for (int k = 0; k < W; k++) {
		if (wait(&status) == -1) {
			printf("Wait has failed\n");
            exit(-1);
		}
	}

    close(fd);
    exit(0);
}