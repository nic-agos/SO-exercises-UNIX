/*
Scrivere un programma per Windows/Unix che
permette al processo principale P di create un
nuovo thread T il cui percorso di esecuzione è
associato alla funzione “thread_function”.
Il processo principale P ed il nuovo thread T
dovranno stampare ad output una stringa che li
identifichi rispettando l’ordine T→P, senza
utilizzare
“WaitForSingleObject”/“pthread_join”,
ma sfruttando un concetto fondamentale che
accomuna tutti i threads di un determinato
processo.
*/

