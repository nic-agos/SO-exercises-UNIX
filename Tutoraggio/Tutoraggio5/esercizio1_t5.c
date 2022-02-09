/*
Scrivere un programma per Windows/Unix che
una volta mandato in esecuzione sia in grado di
creare ed attendere la terminazione di N threads
in sequenza:
(create(T0) → join(T0) → create(T1) → join(T1) → …)
in maniera tale che ogni thread Ti sia affine alla
CPU-core (i % #CPU).
*/

/*compile with gcc esercizio1_t5.c -lpthread -o esercizio1_t5.out*/


#define _GNU_SOURCE
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <sched.h>
 

#define CPU_NUM 4

void *thread_function(void *args){

    char *resp;

    int id = (intptr_t) args;

    printf("Just spawn a new thread n.%d\n", id);

    sleep(3);

    resp = "All right";

    pthread_exit((void *) (intptr_t) id);
    
}

int main(int argc, char *argv[]){

    pthread_t thread;
    void *status;
    pthread_attr_t attr;
    int stack_size;
    cpu_set_t cpus;

    stack_size = 8388608;

    for(int i = 0; i<CPU_NUM; i++){

        /*inizializza una struttura attr*/
        if(pthread_attr_init(&attr)){
            printf("Unable to initialize attr structure\n");
            return 0;
        }

        /*imposto la dimensione dello stack*/
        if (pthread_attr_setstacksize(&attr, stack_size)) {
			printf("Warning. Unable to set thread stack size.\n");
		}

        /*azzero tutti i campi del set di cpu*/
        CPU_ZERO(&cpus);

        /*aggiungo la CPU i-esima al set*/
        CPU_SET(i, &cpus);

        if (pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus)) {
			printf("Warning. Unable to set thread affinity.\n");
		}

        if(pthread_create(&thread, &attr, thread_function, (void *) (intptr_t) i)){
            printf("Unable to create thread n. %d", i);
            return 0;
        }

        if(pthread_attr_destroy(&attr)){
            printf("Unable to destroy thread's attribute structure");
        }

        if(pthread_join(thread, &status)){
            printf("Unable to join thread n. %d", i);
            return 0;
        }

        printf("Created and joined thread n. %d, exit code was %ld\n", i, (intptr_t) status);

        // free(status);

        sleep(2);

    }
     
    printf("Execution terminated\n");
    return 0;
    
}