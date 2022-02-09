#include <stdio.h>

#if defined(ARG_ENV)

int main (int argc, char *argv[], char *envp[])
{
	int i = 0;

	while (envp[i] != NULL)
		printf("Variabile di ambiente %d: %s\n", i+1, envp[i++]);

	return 0;
}

#elif defined(EXTR_ENV)

extern char **environ;

int main (void)
{
	int i = 0;

	if (environ != NULL)
		while (environ[i] != NULL)
			printf("Variabile di ambiente %d: %s\n", i+1, environ[i++]);

	return 0;
}

#else

extern char __libc_csu_init;

/* Come è fatto lo stack?
 *
 *  ___________
 * |           |
 * |     c     |
 * |___________| /____ SP
 * |     |     | \
 * |     |  i  |
 * |_____|_____| /____ BP  /____ c
 * |           | \         \
 * |     BP    |
 * |___________|
 * |           |
 * |     SP    |
 * |___________|
 * |           |
 * |  RET ADDR |
 * |___________|
 * |           |
 * |    ????   |
 * |    ????   |
 *
 */

int main (void)
{
	int i = 0;
	char *c = (char *) ((&i) + 1);

	printf("---------BASE-POINTER------------------------\n");
	printf("Indirizzo mantenuto in c:      %p\n", (void *) c);
	printf("Valore puntato da c:           %p\n", (void *) *((void **) c));
	printf("---------------------------------------------\n\n");

	c += sizeof(void *);

	printf("---------RETURN-ADDRESS----------------------\n");
	printf("Indirizzo mantenuto in c:      %p\n", (void *) c);
	printf("Valore puntato da c:           %p\n", (void *) *((void **) c));
	printf("Indirizzo di __libc_csu_init:  %p\n", (void *) &__libc_csu_init);
	printf("---------------------------------------------\n\n");

	c += sizeof(void *);

	printf("---------BELOW-STACK-FRAME-------------------\n");
	while (1)
		printf("%c", c[i++]);

	return 0;
}

#endif