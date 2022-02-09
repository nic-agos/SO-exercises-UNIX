// Lanciate il programma ed inserite una
// stringa a piacere (anche con spazio)
// e di lunghezza variabile.

#include <stdio.h>
#include <stdlib.h>

int main()
{
	char *buff;

	// Questa formattazione indica che
	// dovrà essere la funzione scanf
	// ad allocare spazio sufficiente
	// a contenere la stringa passata
	// da stdin.
	scanf("%m[^\n]", &buff);
	printf("%s\n", buff);

	// Il programmatore deve liberare
	// lo spazio precedentemente
	// allocato.
	free(buff);

	return 0;
}