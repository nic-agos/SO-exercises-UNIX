// Lanciate il programma ed inserite una
// stringa a piacere di lunghezza variabile.

#include <stdio.h>
#include <stdlib.h>

int main()
{
	char buff[128];

	// Questa formattazione indica che
	// la funzione scanf consumerà
	// esattamente, e non più di, 3
	// caratteri da standard input.
	scanf("%3c", buff);
	printf("%s\n", buff);

	// Questa formattazione indica che
	// la funzione scanf consumerà
	// esattamente, e non più di, 3
	// caratteri da standard input.
	scanf("%10c", buff);
	printf("%s\n", buff);

	// Vi sarete accorti come la funzione
	// scanf non termini qualora nel buffer
	// di input non vi siano sufficenti
	// caratteri a coprire la richiesta.

	// Inoltre, ogni carattere in eccesso
	// che rimane appeso nel buffer viene
	// ad essere consumato nelle successive
	// invocazioni di scanf.

	// Provare queste due sequenze di caratteri:
	//
	// 1. abcdef
	// 2. ghijklmnop
	//
	// Verificare che restituisca:
	//
	// 1. abc
	// 2. def      (anche il carattere '\n' viene consumato)
	//    ghijkl

	return 0;
}