/* Da to un nome, crea un file con quel nome */
void crea_file(char *);

/* Dato un nome di un file, ne restituisce la dimensione */
int FILE_dim(char *);

/* scrivo in append dentro il file di registro nome[] ciò che contiene stringa */
int scrivi_file_append(char *, char *);

/* scrivo in append dentro il file di registro nome[] ciò che contiene stringa */
int sovrascrivi_file(char *, char *);

/* scrivo "aperto" dentro il file di registro nome[] */
int apri_registro(char *);

/* scrivo "chiuso" dentro il file di registro nome[] */
int chiudi_registro(char *);

/* Ritorna 1 se il registro è aperto */
int registro_aperto(char nome[]);

/* crea un nuovo registro e ne scrive il nome in nome_file[]*/
int crea_nuovo_registro(int [], int, char []);

/*  Funzione per aggregare i dati di un singolo registro. */
int aggrega_registro(char []);

/* Data una data singola, verifica se esiste un record con quella data nel file di cui necessita il nome.
   Se non esiste, allora scrive in data[] la data mancante
   Ritorna:
            -1  : In caso di errore
             1  : Nel caso trova la data
             0  : Nel caso non trova la data*/
int verifica_presenza_della_data(char [], int, int, int, char);

/* Aggiunge nel file di cui necessita il nome, il dato che ha richiesto nella rete.*/
int aggiungi_aggr(char nome_file[], int this_data[], char this_tipo, int this_tot);

/* Stampa la richiesta della get*/
void stampa_richiesta(char nome_file[], int data_ini[], int data_fin[], char asterisco1, char asterisco2, char tipo, char tipo_aggregato[]);

/* Dato tipo e data dell'aggregato, ricerca il dato nel file degli aggregati passato.
   Lascia in totale il valore cercato, altrimeti se non c'è nessun valore per quella data 
   scrive 0xFFFFFFFF
   Quando passo il file <aggr.txt> allora il flag = 1, 0 altrimenti*/
void cerca_totale(char nome_file[], int data[], char tipo_attuale, int *totale, int flag);