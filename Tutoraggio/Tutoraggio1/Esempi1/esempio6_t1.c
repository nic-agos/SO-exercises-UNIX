#include <stdlib.h>
#include <stdio.h>

int main (int argc, char *argv[])
{
	int i;
	char *e_ptr;

	if (argc < 2)
		printf("Nessun decimale passato in ingresso.\n"
			"Il formato corretto è:  ./test  int#1  [int#2  ...]\n");

	/* Un buffer sufficientemente ampio da
	   contenere tutti gli interi passati
	   in ingresso dall'utente. */
	long int input[argc-1];

	/* "strtol(const char *nptr, char **endptr, int base)"
	   converte la stringa puntata da "nptr" a intero
	   e la ritorna come risultato. "base" indica quale
	   base utilizzare per la conversione. "*endptr" viene
	   inizializzato all'indirizzo del primo carattere non
	   valido in caso la formattazione non sia corretta. */
	for (i=0; i<argc-1; i++)
	{
		input[i] = strtol(argv[i+1], &e_ptr, 16);

		if (e_ptr[0] != '\0')
		{
			printf("Carattere non valido: %c\n", e_ptr[0]);
			e_ptr = NULL;
		}
	}

	for (i=0; i<argc-1; i++)
		printf("int#%d: %li\n", i+1, input[i]);

	return 0;
}