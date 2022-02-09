#include <string.h>
#include <stdio.h>

int main()
{
	int index;
	int num_tokens;
	char buff[512];
	char *token[512] = { NULL };
	char *delim = "_";

	scanf("%[^\n]", buff);

	index = 0;

	token[index] = strtok(buff, delim);

	while (token[index] != NULL)
		token[++index] = strtok(NULL, delim);

	num_tokens = index;

	for (index=0; index<num_tokens; index++)
	{
		printf("%s", token[index]);
		
		if (index < num_tokens-1)
			putchar((int) ' ');
		else
			putchar((int) '\n');
	}

	return 0;
}