#include <stdlib.h>
#include <stdio.h>

int main (int argc, char *argv[])
{
	int i;

	if (argc < 2)
		printf("Nessun decimale passato in ingresso.\n"
			"Il formato corretto è:  ./test  int#1  [int#2  ...]\n");

	/* Un buffer sufficientemente ampio da
	   contenere tutti gli interi passati
	   in ingresso dall'utente. */
	int input[argc-1];

	/* "int atoi(const char *nptr)" converte
	   la stringa puntata da "nptr" a intero
	   e la ritorna come risultato, ma non
	   notifica la possibilità che il formato
	   della stringa inserita non sia corretto. */
	for (i=0; i<argc-1; i++)
		input[i] = atoi(argv[i+1]);

	for (i=0; i<argc-1; i++)
		printf("int#%d: %d\n", i+1, input[i]);

	return 0;
}