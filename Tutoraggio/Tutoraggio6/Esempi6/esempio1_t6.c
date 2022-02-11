#include <unistd.h>
#include <pthread.h>
#include <stdio.h>


/*
 * GCC può compilare il vostro programma a differenti livelli di
 * ottimizzazione del codice.
 *
 * E' possibile specificare tale livello a tempo di compilazione
 * passando al comando "gcc" l'opzione di ottimizzazione "-O"
 * seguita dal valore numerico del livello desiderato (sono in
 * tutto 4: 0, 1, 2 e 3).
 *
 * Ref: https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html
 *
 * E' solito che un programmatore voglia compilare il proprio
 * programma con il massimo livello di ottimizzazione prima di
 * andare in deploy.
 *
 * Per certi livelli di ottimizzazione però, funzionalmente a certe
 * operazioni di ottimizzazione che il compilatore intraprende, una
 * analisi statica del codice potrebbe indurre il compilatore a
 * dedurre che il valore di una variabile non cambia mai perché
 * nessuna operazione di scrittura su di essa viene rilevata sul
 * flusso di controllo (sequenza di istruzioni sullo stesso path
 * di esecuzione) analizzato, e decidere dunque di generare un codice
 * più performante che utilizza "stale values" mantenuti localmente
 * sullo stack o su registri, o decidere addirittura di prunare
 * completamente la porzione di codice di test di un predicato che
 * ha rilevato staticamente non cambiare mai il valore di veridicità.
 *
 * Compilate il seguente codice con il massimo livello di
 * ottimizzazione "-O3" come segue ed eseguite:
 *
 *     gcc -O3 -o test esempio1.c -pthread
 *
 * Ricompilate poi definendo anche USE_VOLATILE come segue
 * ed eseguite:
 *
 *     gcc -DUSE_VOLATILE -O3 -o test esempio1.c -pthread
 */


#ifdef USE_VOLATILE
volatile unsigned long long shared_variable = 1;
#else
unsigned long long shared_variable = 1;
#endif


void *thread_function(void *arg)
{
	sleep(4);

	shared_variable = 0;

	pthread_exit(NULL);
}


int main()
{
	int result;
	pthread_t thread;
	void *exit_status;

	if ((result = pthread_create(&thread, NULL, thread_function, NULL)))
	{
		return 1;
	}
	else
	{
		unsigned long long local_variable = 0;

		while (shared_variable)
		{
			local_variable += shared_variable;
		}

		printf("Local value has advanced of %u steps\n", local_variable);
		
		pthread_join(thread, &exit_status);
	}
	return 0;
}