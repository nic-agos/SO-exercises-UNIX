#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>


static const char *filename = "file-to-reverse.txt";

/*
 * Questa versione inverte tutto il contenuto di un file leggendo e scrivendo i
 * bytes in prima ed ultima posizione per mezzo delle system-calls "read" e "write"
 * che operano su un descrittore ottenuto all'atto dell'apertura del file.
 *
 * Iterativamente i puntatori all'interno del file dei bytes da invertire
 * vengono invcrementati e decrementati fino ad ottenere un file con il
 * contenuto originale completamente invertito.
 */

int main()
{
	int fd;

	off_t end;
	off_t start;

	char c1, c2;

	if ((fd = open(filename, O_RDWR)) == -1) {
		printf("Unable to open file %s\n", filename);
		exit(-1);
	}
	
	if ((end = lseek(fd, 0, SEEK_END)) == -1) {
		printf("Unable to seek at the end of file %s\n", filename);
		exit(-1);
	}

	end = end - 1;

	if ((start = lseek(fd, 0, SEEK_SET)) == -1) {
		printf("Unable to seek at the start of file %s\n", filename);
		exit(-1);
	}

	while (start < end)
	{
		if (read(fd, &c1, 1) == -1) {
			printf("Unable to read at the offset %d of file %s\n", start, filename);
			exit(-1);
		}

		if (lseek(fd, end, SEEK_SET) == -1) {
			printf("Unable to seek at the offset %d of file %s\n", end, filename);
			exit(-1);
		}

		if (read(fd, &c2, 1) == -1) {
			printf("Unable to read at the offset %d of file %s\n", end, filename);
			exit(-1);
		}

		if (lseek(fd, end, SEEK_SET) == -1) {
			printf("Unable to seek at the offset %d of file %s\n", start, filename);
			exit(-1);
		}

		if (write(fd, &c1, 1) == -1) {
			printf("Unable to write at the offset %d of file %s\n", end, filename);
			exit(-1);
		}

		if (lseek(fd, start, SEEK_SET) == -1) {
			printf("Unable to seek at the offset %d of file %s\n", start, filename);
			exit(-1);
		}

		if (write(fd, &c2, 1) == -1) {
			printf("Unable to write at the offset %d of file %s\n", start, filename);
			exit(-1);
		}

		start += 1;
		end -= 1;
	}

	close(fd);

	exit(0);
}