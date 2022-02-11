#include <stdlib.h>
#include <stdio.h>


static const char *filename = "file-to-reverse.txt";

/*
 * Questa versione inverte tutto il contenuto di un file sfruttando la bufferizzazione
 * interna alla libreria "stdio", sia per la lettura che per la scrittura, ed utilizzando
 * le apposite API previste per operare con gli stream di I/O.
 *
 * Questo approccio ci solleva dalla necessità di dover utilizzare buffer locali nel nostro
 * programma, come pure dal dover operare ri-ordinamenti intermedi.
 */

int main()
{
	FILE *file;

	long end;
	long start;

	char c1, c2;

	if ((file = fopen(filename, "r+")) == NULL) {
		printf("Unable to open file %s\n", filename);
		exit(-1);
	}

	if (fseek(file, 0, SEEK_END) == -1) {
		printf("Unable to seek at the end of file %s\n", filename);
		exit(-1);
	} else if ((end = ftell(file)) == -1) {
		printf("Unable to ask for the position of file %s\n", filename);
		exit(-1);
	}

	end = end - 1;

	if (fseek(file, 0, SEEK_SET) == -1) {
		printf("Unable to seek at the start of file %s\n", filename);
		exit(-1);
	} else if ((start = ftell(file)) == -1) {
		printf("Unable to ask for the position of file %s\n", filename);
		exit(-1);
	}

	while (start < end)
	{
		if (fscanf(file, "%c", &c1) == EOF) {
			printf("Unable to read at the offset %d of file %s\n", start, filename);
			exit(-1);
		}

		if (fseek(file, end, SEEK_SET) == -1) {
			printf("Unable to seek at the offset %d of file %s\n", end, filename);
			exit(-1);
		}

		if (fscanf(file, "%c", &c2) == EOF) {
			printf("Unable to read at the offset %d of file %s\n", end, filename);
			exit(-1);
		}

		if (fseek(file, end, SEEK_SET) == -1) {
			printf("Unable to seek at the offset %d of file %s\n", start, filename);
			exit(-1);
		}

		if (fprintf(file, "%c", c1) < 0) {
			printf("Unable to write at the offset %d of file %s\n", end, filename);
			exit(-1);
		}

		if (fseek(file, start, SEEK_SET) == -1) {
			printf("Unable to seek at the offset %d of file %s\n", start, filename);
			exit(-1);
		}

		if (fprintf(file, "%c", c2) < 0) {
			printf("Unable to write at the offset %d of file %s\n", start, filename);
			exit(-1);
		}

		start += 1;
		end -= 1;
	}

	fclose(file);

	exit(0);
}