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

int consumatore(void *addr, int sem){

    /*dichiaro una struttura sembuf come un array perchè potrei poterne usare contemporaneamente
      più di una per eseguire più operazioni su uno stesso array semaforico*/
    struct sembuf buf[1];

    /*WAIT sulla prima istanza del semaforo(sem_num = 0), realizzata tentando di decrementare
      il valore del semaforo (sem_op = -1)*/

    buf[0].sem_num = 0;
    buf[0].sem_op = -1;
    buf[0].sem_flg = 0;

    if(semop(sem, buf, 1)== -1){
        printf("Unable to synchronize with the first semaphore");
        return 0;
    }

    printf("Message from the produces: %s\n", (char *)addr);

    /*SIGNAL sulla seconda istanza del semaforo(sem_num = 1), realizzata tentando di incrementare
      il valore del semaforo (sem_op = 1)*/
    buf[0].sem_num = 1;
    buf[0].sem_op = 1;
    buf[0].sem_flg = 0;

    if(semop(sem, buf, 1) == -1){
        printf("Unable to synchronize with the first semaphore");
        return 0;
    }

    return 1;

}

void main(int argc, char*argv[]){

    int sem;
    int shm;
    void *shm_addr;
    key_t shm_key, sem_key;

    sem_key = 2367;
    shm_key = 1345;

    int ret;

    /*cerco di ottenere il descrittore alla memoria condivisa creata dal prduttore*/
    shm = shmget(shm_key, SHME_SIZE, 0);

    if(shm == -1){
        printf("Unable to find the shared memory\n");
        exit(-1);
    }

    shm_addr = shmat(shm, NULL, 0);

    if(shm_addr == (void *)-1){
        printf("Unable to attach the shared memory to the process\n");
        exit(-1);
    }

    /*cerco di ottenere l'array semaforico istanziato dal produttore*/
    sem = semget(sem_key, 2, 0);

    if(sem == -1){
        printf("Unable to find the semaphores\n");
        /*scollego la memoria condivisa*/
        shmdt(shm_addr);
        exit(-1);
    }

    do
    {
        ret = consumatore(shm_addr, sem);
    } while (ret);

    shmdt(shm_addr);

    printf("Consumer execution ended\n");
    exit(0);
    






}