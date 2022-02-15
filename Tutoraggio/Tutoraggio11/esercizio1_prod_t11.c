/*
Scrivere due programmi in C (produttore &
consumatore) che una volta agganciati alla stessa
memoria condivisa, operano su di essa in maniera
esclusiva tramite l’utilizzo di semafori (System V).
Più precisamente il consumatore dovrà attendere il
completamento della scrittura sulla memoria da parte del
produttore, che a sua volta attenderà il consumatore
affinchè abbia effettivamente completato la lettura.
Il produttore prende il dato da stdin, mentre il
consumatore stampa il dato a stdout.
*/

#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/sem.h>

#define SHME_SIZE 4096

/*union da definire obbligatoriamente per utilizzare le operazioni sul semaforo*/
union semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
	struct seminfo *__buf;
};

int produttore(void *addr, int sem){

    /*dichiaro una struttura sembuf come un array perchè potrei poterne usare contemporaneamente
      più di una per eseguire più operazioni su uno stesso array semaforico*/
    struct sembuf buf[1];

    /*WAIT sulla seconda istanza del semaforo(sem_num = 1), realizzata tentando di decrementare
      il valore del semaforo (sem_op = -1)*/
    buf[0].sem_num = 1;
    buf[0].sem_op = -1;  
    buf[0].sem_flg = 0;

    if(semop(sem, buf, 1) == -1){
        printf("Unable to synchronize with the second semaphore");
        return 0;
    }

    printf("Insert a message for the consumer: ");
    if(scanf("%[^\n]", (char *)addr) == 0){
        *((char *)addr) = '\0';
    }
    /*faccio avanzare l'indicatore di posizione dello stream (stdin)*/ 
    getc(stdin);

    /*SIGNAL sulla prima istanza del semaforo(sem_num = 0), realizzata tentando di incrementare
      il valore del semaforo (sem_op = 1)*/
    buf[0].sem_num = 0;
    buf[0].sem_op = 1;
    buf[0].sem_flg = 0;

    if(semop(sem, buf, 1)== -1){
        printf("Unable to synchronize with the first semaphore");
        return 0;
    }

    return 1;

}

void main(int argc, char *argv[]){

    int sem;
    int shm;
    void *shm_addr;
    key_t shm_key, sem_key;
    int ret;

    sem_key = 2367;
    shm_key = 1345;

    union semun sem_arg;

    shm = shmget(shm_key, SHME_SIZE, IPC_CREAT | 0666);

    if (shm == -1){
        printf("Unable to create shared memory\n");
        exit(-1);
    }

    /*collego la zona di memoria condivisa con l'address space del processo, delegando al kernel
      la scelta della zona di memoria da usare*/
    shm_addr = shmat(shm, NULL, 0);

    if (shm_addr == (void *)-1)
    {
        printf("Unable to attach the shared memory to the process\n");
        /*elimino la memoria condivisa dal sistema*/
        shmctl(shm, IPC_RMID, NULL);
        exit(-1);
    }

    /*invoco la creazione di un array semaforico con 2 elementi*/
    sem = semget(sem_key, 2, IPC_CREAT | 0666);

    if (sem == -1){
        printf("Unable to create semaphores\n");
        /*elimino la memoria condivisa dal sistema*/
        shmctl(shm, IPC_RMID, NULL);
        /*scollego la memoria condivisa dall'address space del processo*/
		shmdt(shm_addr);
        exit(-1);
    }

    sem_arg.val = 0;

    /*imposto il primo semaforo, posizionato all'indice 0 dell'array semaforico, al valore di 0*/
    if(semctl(sem, 0, SETVAL, sem_arg) == -1){
        printf("Unable to set the initial value of the first semaphore\n");
        /*rimuovo il semaforo dal sistema, l'argomento semnum viene ignorato*/
        semctl(sem, -1, IPC_RMID, sem_arg);
        /*rimuovo la memoria condivisa dal sistema*/
		shmctl(shm, IPC_RMID, NULL);
        /*scollego la memoria condivisa dall'address space del processo*/
		shmdt(shm_addr);
        exit(-1);
    }

    sem_arg.val = 1;

    /*imposto il secondo semaforo, posizionato all'indice 1 dell'array semaforico, al valore di 1*/
    if(semctl(sem, 1, SETVAL, sem_arg) == -1){
        printf("Unable to set the initial value of the first semaphore\n");
        /*rimuovo il semaforo dal sistema, l'argomento semnum viene ignorato*/
        semctl(sem, -1, IPC_RMID, sem_arg);
        /*rimuovo la memoria condivisa dal sistema*/
		shmctl(shm, IPC_RMID, NULL);
        /*scollego la memoria condivisa dall'address space del processo*/
		shmdt(shm_addr);
        exit(-1);
    }

    do
    {
        ret = produttore(shm_addr, sem);
    } while (ret);
    
    printf("Producer execution ended\n");
    /*rimuovo il semaforo dal sistema, l'argomento semnum viene ignorato*/
    semctl(sem, -1, IPC_RMID, sem_arg);
    /*rimuovo la memoria condivisa dal sistema*/
    shmctl(shm, IPC_RMID, NULL);
    /*scollego la memoria condivisa dall'address space del processo*/
    shmdt(shm_addr);

    exit(0);


}