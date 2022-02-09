#include <unistd.h>
#include <stdio.h>

#define STEP_PERIOD 2000000000ULL

int main()
{
	unsigned int c = 0;
	unsigned long long int i = 0ULL;

	while (1)
		if ((++i % STEP_PERIOD) == 0)
			printf("PROCESS %d\t@\tSTEP %u\n", getpid(), ++c);

	return 0;
}