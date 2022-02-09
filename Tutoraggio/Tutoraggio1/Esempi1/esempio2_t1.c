// Lanciate il programma ed inserite una
// stringa a piacere (anche con spazio).

#include <stdio.h>

int main()
{
	int c;
	char buff[1024];

	while (1)
	{
		// Questa formattazione indica di
		// consumare tutti i caratteri
		// (spazi inclusi) da stdin che
		// non sono il terminatore di linea.
		scanf("%[^\n]", buff);
		// Consuma il terminatore di linea.
		c = getchar();
		printf("%s\n", buff);
	}

	return 0;
}