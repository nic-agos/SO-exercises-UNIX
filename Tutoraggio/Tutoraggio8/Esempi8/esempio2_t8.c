#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <sys/wait.h>
#include <stdlib.h>


#define COMMAND_LENGTH 1024


int count_args(char *start_command, int *pipe_present, char **next_command)
{
  char copied_string[COMMAND_LENGTH + 1];
  char *temp, *temp_next;
  int args = 0, i;

  /*
   * Ci creiamo una copia locale della stringa contenente
   * il/i comando/i (eventualmente separati dal simbolo di
   * pipe "|"), in maniera tale da non modificare la stringa
   * originale.
   */
  strcpy(copied_string, start_command);

  /*
   * Utilizziamo al funzione "strtok" per ottenre il primo
   * argomento (se esiste), ed incrementare quindi il numero
   * di argomenti che compongono l'attuale stage di una
   * eventuale pipe.
   */
  if (((temp = strtok(copied_string, " \n")) == NULL) || (strcmp(temp, "\0") == 0))
    return(-1);

  args++;

  /*
   * Fintanto che troviamo nuovi tokens da includere tra gli
   * argomenti che compondo il comando in questa stage di
   * pipe, e quindi fintanto che non incontriamo il simbolo
   * di pipe "|", incrementiamo il numero di argomenti.
   */

  /*The strtok() and strtok_r() functions return a pointer to the next  token, or NULL if there are no more tokens.*/
  
  while (((temp = strtok(NULL, " \n")) != NULL) && (strcmp(temp, "|") != 0))
  {
    if (strcmp(temp, "\0") == 0)
      break;

    args++;
  }

  if (temp == NULL)
  {
    /*
     * Non abbiamo incrociato alcun simbolo di pipe, e
     * quindi non dobbiamo comunicare la presenza di un
     * ulteriore comando.
     */
    *pipe_present = 0;
    *next_command = NULL;
  }
  else
  {
    if ((temp = strtok(NULL, " \n")) != NULL)
    {
      *pipe_present = 1;

      for (i=0; ; i++)
        if (copied_string[i] == '|')
          break;

      /*
       * Indichiamo da dove parte la sub-stringa che
       * contine il prossimo comando in pipe.
       */
      *next_command = start_command + i + 2;
    }
    else
    {
      /*
       * Abbiamo incrociato il simbolo di pipe, ma non
       * abbiamo ulteriori argomenti da parsare nel resto
       * della stringa conentenete il/i comando/i.
       */
      *pipe_present = 0;
      *next_command = NULL;
    }
  }

  /*
   * Ritorniamo il numero di argomenti che compongono
   * il comando dell'attuale stage di pipe.
   */
  return(args);
}


char **build_arg_vector(char *command, int arg_num)
{
  char **arg_vector;
  char *temp;
  int i;

  /*
   * Ci allochiamo dinamicamente sufficiente spazio per
   * accogliere tanti puntatori a stringa quanti sono
   * gli argomenti di cui è composto il comando nel
   * corrente stage di pipe, più uno finale da inizializzare
   * a NULL.
   */
  arg_vector = malloc((arg_num + 1) * sizeof(char *));

  for(i=0; i < arg_num; i++) 
  {
    /*
     * A questo punto token-izziamo tutti gli argomenti
     * di cui si compone il comando alterando la stringa
     * originale in maniera safe (la valutazione della
     * corretta formattazione del comando è già stata
     * fatta dalla funzione "count_args").
     */
    if (i == 0)
      temp = strtok(command, " \n");
    else
      temp = strtok(NULL, " \n");

    arg_vector[i] = temp;
  }

  arg_vector[i] = NULL;

  /*
   * Ritorniamo quindi il vettore contente i puntatori
   * a tutte le stringhe rappresentati gli argomenti di
   * cui si compone il comando.
   */
  return(arg_vector);
}

int main ()
{
  char *command_pointer, *next_command;
  char line[COMMAND_LENGTH+1];
  char **arg;
  int *old_pipe_descriptors;
  int *new_pipe_descriptors;
  int arg_num;
  int pipe_present=0, pipe_pending=0;
  int pending_processes;
  int i, status;

  while (1)
  {
    printf("\nExample Shell");

    if (geteuid() == 0)  /*sono in modalità sudo*/
      printf("#");
    else
      printf(">");

    fflush(stdout);

    /*
     * Reperiamo l'intera stringa contenente il/i
     * comando/i da stdin e la inseriamo in un buffer
     * sullo stack.
     */
    fgets(line, COMMAND_LENGTH, stdin);

    line[COMMAND_LENGTH] = '\0';

    if (strlen(line) >= COMMAND_LENGTH)
    {
      printf("\nCommand too long!\n");
      continue;
    }

    pending_processes = 0;
    command_pointer = line;

    if (strcmp(command_pointer, "exit\n") == 0)
      break;

    do
    {
      /*
       * Invochiamo la funzione "count_args" per farci ritornare le
       * seguenti informazioni:
       *
       *  - il numero di argomenti che compongono il comando nell'attuale
       *    stage della command-pipe;
       *
       *  - l'eventuale presenza di un successivo comando nella command-pipe
       *
       *  - ed in caso vi sia un ulteriore comando in command-pipe, dove
       *    inizia la stringa contente quel comando.
       */
      arg_num = count_args(command_pointer, &pipe_present, &next_command);

      if (arg_num < 0)
        break;

      /*
       * Invochiamo la funzione "build_arg_vector" per ottenere un vettore
       * di puntatori a stringhe. Ogni stringa rappresenta un argomento
       * del comando nella corrente stage di pipe (se presente).
       */
      arg = build_arg_vector(command_pointer, arg_num);

      /*
       * Se la command-pipe non esiste, non dobbiamo fare altre se non
       * lanciare l'unico comando inserito ed attendere che restituisca
       * il rusultato a stdout.
       *
       * Diversamente se presenti più comandi in pipe dobbiamo redirigere
       * l'output del comando corrente su una pipe da cui il successivo
       * comando potrà andare a leggere il rispettivo input.
       */
      if (pipe_present)
      {
        /*
         * Quindi allochiamo dinamicamente uno spazio sufficiente ad
         * accogliere due descrittori che ci verranno restituiti dalla
         * funzione pipe. Uno per la scrittura ed uno per la lettura.
         */
        new_pipe_descriptors = malloc(sizeof(int) * 2);

        if (pipe (new_pipe_descriptors) < 0)
        {
          printf("Can't open a pipe for error %d!\n", errno);
          fflush(stdout);
          exit(EXIT_FAILURE);
        }
      }

      /*
       * Fork-iamo dunque un processo che si farà carico dell'esecuzione
       * del comando corrente.
       */
      if ((i = fork()) == 0)
      {
        /*
         * Tutti i comandi in pipe successivi al primo reperiranno l'input
         * dal canale di lettura della pipe generata durante il precedente
         * step di "fork" + "execv".
         * Lo facciamo chiudendo il canale di stdin e spostando il descrittore
         * di lettura della pipe al descrittore 0 appena liberato.
         */
        if (pipe_pending)
        {
          close(0);
          dup(old_pipe_descriptors[0]);
          close(old_pipe_descriptors[0]);
        }

        /*
         * Tutti i comandi in pipe, ad eccezione dell'ultimo, scriveranno il
         * loro risultato sul canale di scrittura della pipe generata durante
         * il corrente step di "fork" + "execv".
         * Lo facciamo chiudendo il canale di stdout e spostando il descrittore
         * di scrittura della pipe al descrittore 1 appena liberato.
         */
        if (pipe_present)
        {
          close(1);
          dup(new_pipe_descriptors[1]);
          close(new_pipe_descriptors[1]);
        }

        /*
         * Possiamo lanciare il comando che eventualemnte avrà i canali di
         * standard I/O redirezionati ai rispettivi canali della pipe.
         */
        execvp(arg[0], arg);
        printf("Can't execute file %s\n", arg[0]);
        fflush(stdout);
        exit(EXIT_FAILURE);
      }
      else if (i > 0)
      {
        pending_processes++;

        /*
         * Il processo padre non ha bisogno di leggere dal canale di lettura
         * della pipe dei comandi successivi al primo, e quindi può chiudere
         * il relativo canale.
         */
        if (pipe_pending)
        {
          close(old_pipe_descriptors[0]);
          free(old_pipe_descriptors);
          pipe_pending = 0;
        }

        /*
         * Il processo padre non ha bisogno di scrivere sul canele di scrittura
         * della pipe dei comandi che precedono l'ultimo, e quindi può chiudere
         * il relativo canale.
         */
        if (pipe_present)
        {
          close(new_pipe_descriptors[1]);
          old_pipe_descriptors = new_pipe_descriptors;
          pipe_pending = 1;
        }
      }
      else
      {
        printf("Can't spawn process for error %d\n", errno);
        fflush(stdout);
      }

      /*
       * Impostiamo il puntatore del comando da valutare nel successivo step a
       * puntare alla stringa che contiene il prossimo comando incontrato nella
       * command-pipe.
       */
      command_pointer = next_command;
    }
    while (next_command != NULL);

    for (i=0; i<pending_processes; i++)
      wait(&status);
  }

  printf("\nLeaving Shell\n\n");

  return(0);
}
