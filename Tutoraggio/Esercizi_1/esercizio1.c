// Lanciate il programma ed inserite una
// stringa a piacere (anche con spazio).

#include <stdio.h>

int main()
{
	char buff[1024];

	while (1)
	{
		// Se la stringa include spazi, le
		// sotto-stringhe successive alla prima
		// verranno consumate agli step successivi
		// del ciclo, e quindi stampati su linee
		// differenti.
		scanf("%s", buff);
		printf("%s\n", buff);
	}

	return 0;
}