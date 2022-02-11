/*
Si supponga di avere un file system che supporta il
metodo di allocazione indicizzato, in cui il record di
sistema associato ad ogni file mantenga 120 indici diretti,
4 indici indiretti e 4 indici doppiamente indiretti. Si
supponga inoltre che il dispositivo di memoria di massa
ove il file system è ospitato abbia blocchi di taglia pari a
1024 record, e che un indice di blocco di dispositivo sia
esperesso con 8 record. Si indichi la massima taglia
possibile (in termini di numero di record) per un generico
file allocato su dispositivo secondo tale schema di
indicizzazione.
*/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>

static const char *filename = "test_file.txt";

int main(int argc, char *argv[]){
    
    int status;
    int fd; 
    pid_t child;
    char *input;
    size_t len;
    size_t count_write;
    size_t count_read;

    fd = open(filename, O_CREAT|O_RDWR|O_TRUNC, S_IRWXU|S_IRWXG|S_IRWXO);

    if (fd == -1){
        printf("Unable to create/open the specified file\n");
        return 0;
    }

    printf("Descriptor: %d\n", fd);

    child = fork();
    if(child ==-1){
        printf("Unable to fork\n");
        exit(-1);
    }
    
    if(child == 0){
        printf("I'm the child process\n");
        printf("Insert a string: ");
        if (scanf("%m[^\n]", &input)== -1){
            exit(-1);
        }

        len = strlen(input);

        count_write = write(fd, input, len);
        printf("Just write: %ld bytes\n", count_write);

        if(count_write == strlen(input)){
            printf("Good write execution\n");
            close(fd);
            free(input);
            exit(count_write);
            
        }
        close(fd);
        free(input);
        exit(-1);
        

    }
    else{

        int aux;
        int len2;

        if (wait(&status) == -1){
            printf("Wait has failed\n");
            exit(-1);
        }

        aux = WEXITSTATUS(status);

        printf("Child exit code is: %d\n", aux);
        if (aux == -1){
            printf("Child process terminated with error\n");
            exit(-1);
        }
             
        /*processo padre e figlio condividono stesso descrittore quindi stessa sessione,
            bisogna riposizionare il puntatore prima di poter leggere
        */
        
        len2 = (size_t) aux;

        char output[len2+1];

        lseek(fd, 0, SEEK_SET);

        count_read = read(fd, output, len2);
        printf("Just read: %ld bytes\n", count_read);
        
        output[len2] = '\0';

        close(fd);

        printf("Child process get the string: %s\n", output);
        
        exit(0);
            
    }

}