/* dato giorno, mese e anno in formato intero, mette in int ris[] la data successiva con lo stesso formato */
void data_successiva(int, int, int,  int[]);

/* dato giorno, mese e anno in formato intero, mette in int ris[] la data precedente con lo stesso formato */
void data_precedente(int, int, int,  int[]);

/* Ritorna il numero di giorni fra due date passate come interi giorno, mese e anno*/
int numero_giorni_fra_due_date(int, int, int, int, int, int);

/* Ritorna 1 se la data: d1:m1:y1 è minore della data: d2:m2:y2, altrimenti 0 */
int data1_minore_data2(int, int, int, int, int, int);

/* Ritorna 1 se la data d:m:y è valida, 0 altrimenti*/
int data_valida(int, int, int);