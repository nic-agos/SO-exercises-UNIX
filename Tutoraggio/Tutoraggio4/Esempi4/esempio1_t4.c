#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>


unsigned nthreads = 1;

double src_matrix_1[1024][1024] = {{ 0.0 }};
double src_matrix_2[1024][1024] = {{ 0.0 }};

double dst_matrix[1024][1024] = {{ 0.0 }};

/* A barrier object that avoids threads to
   continue their work untill all of them
   have not already reached it. */
pthread_barrier_t barrier;


void * matrix_multiplication(void *arg)
{
	int i;

	int row_idx;
	int col_idx;

	int thr_idx = *((int *)&arg);

	int str_idx = ((1024 / nthreads) * thr_idx);
	int end_idx = (thr_idx < (nthreads-1)) ? str_idx + ((1024 / nthreads) - 1) : 1023;

	/* Makes this thread waiting for the
	   arrival of all the others to the barrier. */
	pthread_barrier_wait(&barrier);

	for (row_idx=str_idx; row_idx<=end_idx; row_idx++)
		for (col_idx=0; col_idx<1024; col_idx++)
			for (i=0; i<1024; i++)
				dst_matrix[row_idx][col_idx] += (src_matrix_1[row_idx][i] * src_matrix_2[i][col_idx]);

	pthread_exit(NULL);
}


int main (int argc, char *argv[])
{
	int i, j;
	struct timeval start, end, elapsed;

	/* Initializes a random seed for the
	   pseudo-random number generator. */
	srand(time(NULL));

	for (i=0; i<1024; i++)
	{
		for (j=0; j<1024; j++)
		{
			/* Initializes both the source matrices
			   with random doubles that range from
			   -100.0 and +100.0. */
			src_matrix_1[i][j] = (((double) rand() / (double) RAND_MAX) * 200.0) - 100.0;
			src_matrix_2[i][j] = (((double) rand() / (double) RAND_MAX) * 200.0) - 100.0;
		}
	}

	if (argc > 1)
		nthreads = (unsigned) atoi(argv[1]);

	pthread_t threads[nthreads];

	/* Initializes a barrier for pthreads. It
	   allaws to make threads to start only
	   after all the setup phases are completed. */
	pthread_barrier_init(&barrier, NULL, nthreads + 1);

	for (i=0; i<nthreads; i++)
		if (pthread_create(&threads[i], NULL, matrix_multiplication, *((void **)&i)))
			threads[i] = (pthread_t) 0;

	/* Makes this thread waiting for the
	   arrival of all the others to the barrier. */
	pthread_barrier_wait(&barrier);

	gettimeofday(&start, NULL);

	for (i=0; i<nthreads; i++)
		if (threads[i])
			pthread_join(threads[i], NULL);

	gettimeofday(&end, NULL);

	/* Destoyes the barrier for later reuse (in case). */
	pthread_barrier_destroy(&barrier);

	elapsed.tv_sec = end.tv_sec - start.tv_sec;
	elapsed.tv_usec = end.tv_usec - start.tv_usec;

	printf("%u\n", (elapsed.tv_sec * 1000000) + elapsed.tv_usec);

	return 0;
}