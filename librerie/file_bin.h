void crea_file_da_zero(char *nome);

// Converte un IP binario nella notazione decimale puntata e mette 4 cifre in un array di int
int *da_binario_a_decPun(uint32_t ip, int ris[]);

/*  Cerco dentro il FILE la posizione dove inserire il nuovo peer.
    La scelta è stata fare un sistema circolare, ovvero sia che ogni peer ha due vicini
    uno precedente e l'altro successivo. Mettendo i peer i ordine di porta.
    Restituisce l'indice del file.
    Il falg my_pos serve a dichiarare se il chiamate è interessato a sapere la posizione di un peer già presente 
    nel file o meno.

    Il formato del FILE è binario, e contiene i descrittori dei peer della struttura des_peer */
int ricerca_posto_lista_peer(in_port_t porta, int my_pos);


/* Scrivo dentro il file 'lista_peer.bin' nella posizione (pos) del cursore. 
   Scrivo i parametri PORTA e IP passati*/
int inserimento_lista_peer(int pos, in_port_t porta_peer, uint32_t IP_peer);

/* Crea un nuovo file binario dove copia tutti record eccetto quello da eliminare.
   Rinomina il nuovo file e elimina il vecchio*/
int rimozione_lista_peer(int pos);

/* Ritrna il valore della porta relativo al peer che occupa la posizione (pos)
   Nel file 'lista_peer.bin' */
in_port_t get_port(int pos);

/* Ritrna il valore dell'IP relativo al peer che occupa la posizione (pos)
   Nel file 'lista_peer.bin' */
uint32_t get_IP(int pos);

/* Stampo a video tutti i peer nella lista 'lista_peer.bin'*/
void stampa_lista_peer();

/* Stampo a video i peer neighbor del peer passato come parametro*/
void stampa_vicini(int pos, in_port_t peer);

