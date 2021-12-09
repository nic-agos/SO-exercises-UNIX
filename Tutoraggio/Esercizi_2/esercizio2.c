/*
Scrivere un programma che prende una stringa passata
come primo argomento (i.e. char *argv[ ]) al programma
stesso quando questo viene eseguito.
Copiare tale stringa all’interno di un buffer di dimensione
fissa facendo attenzione a non superare il limite imposto
dalla taglia, e stamparla quindi sullo schermo.
Rigirare la stringa (primo carattere in ultima posizione,
secondo carattere in penultima posizione, ecc.) senza
fare utilizzo di un ulteriore buffer per poi stampare anche
questa stringa sullo schermo.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUFF 100

int main(int argc, char *argv[]){

    char buff[MAX_BUFF];

    if (argc != 2){
        printf("please insert one string as argument\n");
        return 0;
    }

    if (strlen(argv[1]) > MAX_BUFF){
        return 0;
    }

    strcpy(buff, argv[1]);

    printf("ho letto: %s\n", buff);

    int len = strlen(argv[1]);
    printf("la stringa è lunga: %d\n", len);

    for (int i=0; i<len/2; i++) {
        char c = buff[len-1-i];
        buff[len-1-i] = buff[i];
        buff[i] = c;

    }

    printf("la stringa capovolta è: %s\n", buff);


    



    


}