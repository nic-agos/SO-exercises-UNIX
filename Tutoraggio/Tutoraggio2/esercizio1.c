/*
Scrivere un programma che prende una stringa da
tastiera e la inserisce all’interno di un buffer allocato
dinamicamente nella heap da parte della funzione
scanf().
Copiare poi tale stringa all’interno di un secondo buffer
allocato sullo stack della taglia necessaria a contenerla.
Liberare quindi il buffer allocato nella heap utilizzando la
funzione free().
Stampare sullo schermo la stringa copiata nel buffer
allocato sullo stack.*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(){
    char *buff;
    int len;
    
    printf("inserire una stringa: ");
    scanf("%m[^\n]", &buff);
    
    len = strlen(buff);
    printf("%d\n", len);
    
    char buff2[len];
    strcpy(buff2, buff);
    free(buff);
    printf("%s\n", buff2);
}
