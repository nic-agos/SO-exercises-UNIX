#include <string.h>
#include <stdio.h>

int main()
{
	char *token;
	char buff[512];
	char *delim = " ";
	int token_number = 0;

	scanf("%[^\n]", buff);

	/*************************************************************************************
	*                                                                                    *
	* Defined in:   string.h                                                             *
	*                                                                                    *
	* Prototype:    char *strtok(char *str, const char *delim);                          *
	*                                                                                    *
	* Description:  It breaks a string into a sequence of zero or more nonempty tokens.  *
	*               On the first call to strtok(), the string to be parsed should be     *
	*               specified in "str". In each subsequent call that should parse the    *
	*               same string, "str" must be NULL. The "delim" argument specifies a    *
	*               set of bytes that delimit the tokens in the parsed string. Each call *
	*               to strtok() returns a pointer to a null-terminated string containing *
	*               the next token. If no more tokens are found, strtok() returns NULL.  *
	*                                                                                    *
	*************************************************************************************/
	token = strtok(buff, delim);

	while (token != NULL)
	{
		printf("Token %d: %s\n", ++token_number, token);
		token = strtok(NULL, delim);
	}
}