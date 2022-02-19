/*
Implementare un programma che riceva in input tramite argv[1] un numero
intero N maggiore o uguale ad 1 (espresso come una stringa di cifre 
decimali), e generi N nuovi thread. Ciascuno di questi, a turno, dovra'
inserire in una propria lista basata su memoria dinamica un record
strutturato come segue:

typedef struct _data{
	int val;
	struct _data* next;
} data; 

I record vengono generati e popolati dal main thread, il quale rimane
in attesa indefinita di valori interi da standard input. Ad ogni nuovo
valore letto avverra' la generazione di un nuovo record, che verra'
inserito da uno degli N thread nella sua lista. 
L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso
WinAPI) in modo tale che quando il processo venga colpito esso dovra' 
stampare a terminale il contenuto corrente di tutte le liste (ovvero 
i valori interi presenti nei record correntemente registrati nelle liste
di tutti gli N thread). 
*/

#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct _data{
	int val;
	struct _data* next;
} data; 

int num_threads;
data *lists;
int ready_sem;
int done_sem;
int val;

void *thread_function(void *arg){

	long int me = (long int)arg;
	struct sembuf op;
	data *aux;

	printf("I'm thread %d, just spawning\n", me);

	

	while(1){
		/*alloco spazio per il nuovo record*/
		aux = malloc(sizeof(data));

		if(aux == NULL){
			printf("Unable to allocate space for new record in thread %d\n", me);
			exit(EXIT_FAILURE);
		}
redo1:
		op.sem_flg = 0;
		op.sem_num = me;
		op.sem_op = -1;

		if(semop(done_sem, &op, 1) == -1){
			if(errno == EINTR){
				goto redo1;
			}
			printf("Unable to wait Done semaphore for thread %d", me);
			exit(EXIT_FAILURE);
		}
		/*aggiungo alla lista relativa al thread il nuovo elemento*/
		aux->val = val;
		aux->next = lists[me].next;
		lists[me].next = aux;

		printf("New record for list %d has value = %d and next = %p\n", me, aux->val, aux->next);

redo2:
		op.sem_flg = 0;
		op.sem_num = me;
		op.sem_op = 1;

		if(semop(ready_sem, &op, 1) == -1){
			if(errno == EINTR){
				goto redo2;
			}
			printf("Unable to signal Ready semaphore for thread %d", me);
			exit(EXIT_FAILURE);
		}
	}

}

void handler(int signo, siginfo_t *a, void *b){

	int i;
	int j;
	int val;
	data aux;
	
	printf("\n");

	for(i=0; i<num_threads; i++){
		j=0;
		aux = lists[i];
		while(aux.next){
			val = aux.next->val;
			printf("Value for element %d in list %d is: %d\n", j, i, val);
			j = j+1;
			aux = *(aux.next);
		}
		printf("\n");
	}

}


void main(int argc, char **argv){

	int i;
	int ret;
	pthread_t tid;
	struct sigaction sa;
	sigset_t set;
	struct sembuf op;

	if(argc != 2){
		printf("usage: prog num_threads\n");
		exit(EXIT_FAILURE);
	}

	/*converto in intero il valore di num_thread ricevuto come stringa dal main*/
	num_threads = atoi(argv[1]);

	if(num_threads < 1){
		printf("Error! num_threads must be an integer greater than or equal 1\n");
		exit(EXIT_FAILURE);
	}

	/*alloco lo spazio necessario a contenere una lista per ogni thread*/
	lists = malloc(sizeof(data)*num_threads);

	if(lists == NULL){
		printf("Unable to allocate space for data buffer\n");
		exit(EXIT_FAILURE);
	}

	/*inizializzo il primo elemento di ogni lista*/
	for(i = 0; i< num_threads; i++){
		lists[i].val = -1;
		lists[i].next = NULL;
	}

	/*istanzio l'array semaforico Ready*/
	ready_sem = semget(IPC_PRIVATE, num_threads, IPC_CREAT | 0666);

	if(ready_sem == -1){
		printf("Unable to create Ready semaphores\n");
		exit(EXIT_FAILURE);
	}

	/*inizializzo a 1 tutti i semafori dell'array Ready*/
	for(i=0; i<num_threads; i++){
		ret = semctl(ready_sem, i, SETVAL, 1);
		if(ret == -1){
			printf("Unable to initialize Ready semaphores\n");
			exit(EXIT_FAILURE);
		}
	}

	/*istanzio l'array semaforico Done*/
	done_sem = semget(IPC_PRIVATE, num_threads, IPC_CREAT | 0666);

	if(done_sem == -1){
		printf("Unable to create Done semaphores\n");
		exit(EXIT_FAILURE);
	}

	/*inizializzo a 0 tutti i semafori dell'array Ready*/
	for(i=0; i<num_threads; i++){
		ret = semctl(done_sem, i, SETVAL, 0);
		if(ret == -1){
			printf("Unable to initialize Done semaphores\n");
			exit(EXIT_FAILURE);
		}
	}

	/*inserisco tutti i segnali nel set*/
	sigfillset(&set);

	
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = handler;
	sa.sa_mask = set;
	sa.sa_flags = 0;

	/*Imposto il gestore per il segnale SIGINT per il main thread,
	  durante l'esecuzione dell'handler verranno bloccati tutti i segnali*/
	ret = sigaction(SIGINT, &sa, NULL);

	if(ret == -1){
		printf("Unable to set SIGINT handler for main thread\n");
		exit(EXIT_FAILURE);
	}

	/*genero gli N threads*/
	for(i = 0; i<num_threads; i++){

		ret = pthread_create(&tid, NULL, thread_function, (void*)i);
		if(ret != 0){
			printf("Unable to spawn threads\n");
			exit(EXIT_FAILURE);
		}
	}

	i = 0;

	while(1){
redo1:	
		op.sem_flg = 0;
		op.sem_num = i;
		op.sem_op = -1;
		if(semop(ready_sem, &op, 1) == -1){
			if(errno == EINTR){
				goto redo1;
			}
			printf("Unable to wait Ready semaphore for thread %d", i);
			exit(EXIT_FAILURE);
		}
redo2:

		ret = scanf("%d", &val);
		if(ret == EOF){
			if(errno == EINTR){
				goto redo2;
			}
			printf("Unable to get an integer from stdin\n");
		}

redo3:
		op.sem_flg = 0;
		op.sem_num = i;
		op.sem_op = 1;

		if(semop(done_sem, &op, 1) == -1){
			if(errno == EINTR){
				goto redo3;
			}
			printf("Unable to signal Done semaphore for thread %d", i);
			exit(EXIT_FAILURE);
		}

		i = (i+1)%num_threads;
	}

}
