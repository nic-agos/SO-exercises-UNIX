#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>


#define BUFF_SIZE 64


static const char *filename = "file-to-reverse.txt";

/*
 * Questa versione inverte tutto il contenuto di un file leggendo e scrivendo un set di
 * bytes per volta all'inizio e alla fine del file per mezzo delle system-calls "read"
 * e "write" che operano su un descrittore ottenuto all'atto dell'apertura del file.
 *
 * Iterativamente i puntatori all'interno del file dei set di bytes da invertire
 * vengono incrementati e decrementati fino ad ottenere un file con il
 * contenuto originale completamente invertito.
 *
 * Inoltre, per ottenere un risultato corretto, il contenuto letto in dei buffer
 * locali deve essere invertito a sua volta prima di essere riscritto interamente
 * nella posizione complementare.
 */

int main()
{
	int fd;

	int i, j;

	off_t end;
	off_t start;
	off_t middle;

	char c;
	char c1[BUFF_SIZE];
	char c2[BUFF_SIZE];

	size_t bytes;

	if ((fd = open(filename, O_RDWR)) == -1) {
		printf("Unable to open file %s\n", filename);
		exit(-1);
	}

	if ((end = lseek(fd, 0, SEEK_END)) == -1) {
		printf("Unable to seek at the end of file %s\n", filename);
		exit(-1);
	}

	middle = end / 2;
	bytes = (BUFF_SIZE <= middle) ? BUFF_SIZE : middle;
	end = end - bytes;

	if ((start = lseek(fd, 0, SEEK_SET)) == -1) {
		printf("Unable to seek at the start of file %s\n", filename);
		exit(-1);
	}

	while (bytes)
	{
		if (read(fd, c1, bytes) == -1) {
			printf("Unable to read at the offset %d of file %s\n", start, filename);
			exit(-1);
		}

		i = 0;
		j = bytes - 1;
		while (i < j)
		{
			c = c1[i];
			c1[i++] = c1[j];
			c1[j--] = c;
		}

		if (lseek(fd, end, SEEK_SET) == -1) {
			printf("Unable to seek at the offset %d of file %s\n", end, filename);
			exit(-1);
		}

		if (read(fd, c2, bytes) == -1) {
			printf("Unable to read at the offset %d of file %s\n", end, filename);
			exit(-1);
		}

		i = 0;
		j = bytes - 1;
		while (i < j)
		{
			c = c2[i];
			c2[i++] = c2[j];
			c2[j--] = c;
		}

		if (lseek(fd, end, SEEK_SET) == -1) {
			printf("Unable to seek at the offset %d of file %s\n", start, filename);
			exit(-1);
		}

		if (write(fd, c1, bytes) == -1) {
			printf("Unable to write at the offset %d of file %s\n", end, filename);
			exit(-1);
		}

		if (lseek(fd, start, SEEK_SET) == -1) {
			printf("Unable to seek at the offset %d of file %s\n", start, filename);
			exit(-1);
		}

		if (write(fd, c2, bytes) == -1) {
			printf("Unable to write at the offset %d of file %s\n", start, filename);
			exit(-1);
		}

		start += bytes;
		bytes = ((start + BUFF_SIZE) <= middle) ? BUFF_SIZE : middle - start;
		end -= bytes;
	}

	close(fd);

	exit(0);
}