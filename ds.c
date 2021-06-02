#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <math.h>

// -------------------------------------------------------------------------------------------------------------------------------------- //
// -------------------------------------------------------------- COSTANTI -------------------------------------------------------------- //
// -------------------------------------------------------------------------------------------------------------------------------------- //

// Lunghezza di un generico buffer
#define BUF_LEN         1024

// Lunghezza di "REQ_STR\0"
#define REQUEST_LEN     8      

// Lunghezza massima di una data DD:MM:YYYY
#define LEN_DATA        12      

// Indici per le date: Day, Month e Year
#define D               0       
#define M               1
#define Y               2

// Timer 
#define POLLING_TIME    5  

// lunghezza comando da tastiera
#define CMD_LEN         12   

// Lunghezza di "REQ_STR\0"
#define REQUEST_LEN     8 

char buf[BUF_LEN];   // Usato per memorizzare i comandi da tastiera
char comando[BUF_LEN];  // comando e parametro sono usati per differenziare i comandi dai parametri dei comandi da tastiera
char lista_peer[BUF_LEN];
int parametro;

// ( Salvo l'inidirizzo del primo peer che fa una richiesta di boot, per poi ricondattarlo quando ariiveranno dei neighbor
struct sockaddr_in first_peer; 
// )

/* struct usata da stampo per scrivere e leggere dal file 'lista_peer.bin'*/
struct des_peer
{
    in_port_t porta;
    uint32_t IP;
};

// -------------------------------------------------------------------------------------------------------------------------------------- //
// ---------------------------------------------------------------- DATE ---------------------------------------------------------------- //
// -------------------------------------------------------------------------------------------------------------------------------------- //

/* dato giorno, mese e anno, calcola la data del giorno successivo*/
void data_successiva(int d, int m, int y,  int ris[])
{    
    if (y%4 == 0) // Anno bisestile
    {
        if (m == 2 && d == 29) // Febbraio
        {
            d = 1;
            m++;
        }
        if (m == 2 && d < 29) // Febbraio
        {
            d++;
        }
        if ((m == 4 || m == 6 || m == 9 || m == 11) && d == 30) // Mesi da 30
        {
            d = 1;
            m++;
        }
        else if (m == 12 && d == 31) // L'ultimo dell'anno
        {
            d = 1;
            m = 1;
            y++;
        }
        else if (d == 31) // Mesi da 31
        {
            d = 1;
            m++;
        }
        else
        {
            d++;
        }
    }
    else // Anno non bisestile
    { 
        if (m == 2 && d == 28) // Febbraio
        {
            d = 1;
            m++;
        }
        if (m == 2 && d < 28) // Febbraio
        {
            d++;
        }
        else if ((m == 4 || m == 6 || m == 9 || m == 11) && d == 30) // Mesi da 30
        {
            d = 1;
            m++;
        }
        else if (m == 12 && d == 31) // L'ultimo dell'anno
        {
            d = 1;
            m = 1;
            y++;
        }
        else if (d == 31) // Mesi da 31
        {
            d = 1;
            m++;
        }
        else
        {
            d++;
        }
    }

    ris[0] = d;
    ris[1] = m;
    ris[2] = y;
}

/* dato giorno, mese e anno, calcola la data del giorno precedente*/
void data_precedente(int d, int m, int y,  int ris[3])
{
    if (y%4 == 0) // Anno bisestile
    {
        if (m == 3 && d == 1) // Febbraio-Marzo
        {
            d = 29;
            m--;
        }
        else if ((m == 5 || m == 7 || m == 10 || m == 12) && d == 1) // Mesi da 30
        {
            d = 30;
            m--;
        }
        else if (m == 1 && d == 1) // L'ultimo dell'anno
        {
            d = 31;
            m = 12;
            y--;
        }
        else if (d == 1 && m != 3 && m != 5 && m != 7 && m != 10 && m != 12) // Mesi da 31
        {
            d = 31;
            m--;
        }
        else
        {
            d--;
        }
    }
    else // Anno non bisestile
    { 
        if (m == 3 && d == 1) // Febbraio-Marzo
        {
            d = 28;
            m--;
        }
        else if ((m == 5 || m == 7 || m == 10 || m == 12) && d == 1) // Mesi da 30
        {
            d = 30;
            m--;
        }
        else if (m == 1 && d == 1) // L'ultimo dell'anno
        {
            d = 31;
            m = 12;
            y--;
        }
        else if (d == 1 && m != 3 && m != 5 && m != 7 && m != 10 && m != 12) // Mesi da 31
        {
            d = 31;
            m--;
        }
        else
        {
            d--;
        }
    }

    ris[0] = d;
    ris[1] = m;
    ris[2] = y;
}

int numero_giorni_fra_due_date(int d1, int m1, int y1, int d2, int m2, int y2)
{
    int data_i[3];
    int count = 1; // Estremi inclusi

    data_i[0] = d1;
    data_i[1] = m1;
    data_i[2] = y1;
    //printf("DBG     numero_giorni_fra_due_date() datat1: %d:%d:%d\n", d1, m1, y1);
    //printf("DBG     numero_giorni_fra_due_date() datat2: %d:%d:%d\n", d2, m2, y2);

    while (data_i[0] != d2 || data_i[1] != m2 || data_i[2] != y1)
    {
        data_successiva(data_i[0], data_i[1], data_i[2], data_i);
        //printf("DBG     numero_giorni_fra_due_date(): %d:%d:%d\n", data_i[0], data_i[1], data_i[2]);
        count++;
    }

    return count;
}

// Se data1 < data2 torna 1, 0 altrimenti
int data1_minore_data2(int d1, int m1, int y1, int d2, int m2, int y2)
{
    if (d1 < d2 && m1 == m2 && y1 == y2)
    {
        return 1;
    }
    else if (d1 > d2 && m1 < m2 && y1 == y2)
    {
        return 1;
    }
    else  if (d1 < d2 && m1 < m2 && y1 == y2)
    {
        return 1;
    }
    else if (y1 < y2)
    {
        return 1;
    }

    return 0;
}


/* Ritorna 1 se la data d:m:y è valida, 0 altrimenti*/
int data_valida(int d, int m, int y)
{
    time_t rawtime;
    struct tm* timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    // timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900
    if(y > timeinfo->tm_year + 1900)
    {
        printf("\nERR     Immessa una data futura, riprovare\n");
        return 0;
    }

    if (m <= 0 || m > 12)
    {
        printf("\nERR     Immesso un mese non valido, riprovare\n");
        return 0;
    }

    if (d <= 0 || d > 31)
    {
        printf("\nERR     Immesso un giorno non valido, riprovare\n");
        return 0;
    }

    if ((m == 4 || m == 6 || m == 9 || m == 11) && d >= 31)
    {
        printf("\nERR     Immesso un giorno non valido, riprovare\n");
        return 0;
    }

    if (y%4 == 0) // Anno bisestile
    {
        if (m == 2 && (d == 31 || d == 30))
        {
            printf("\nERR     Immesso un giorno non valido, riprovare\n");
            return 0;
        }
    }
    else // Anno non bisestile
    {
        if (m == 2 && (d == 31 || d == 30 || d == 29))
        {
            printf("\nERR     Immesso un giorno non valido, riprovare\n");
            return 0;
        }
    }

    // controlliamo se non è una data futura
    if (m > timeinfo->tm_mon + 1 && y == timeinfo->tm_year + 1900)
    {
        printf("\nERR     Immessa una data futura, riprovare\n");
        return 0;
    }
    

    if (m == timeinfo->tm_mon + 1 && d > timeinfo->tm_mday)
    {
        printf("\nERR     Immessa una data futura, riprovare\n");
        return 0;
    }
    
    printf("data analizzata è valida!\n");
    return 1;
}

// -------------------------------------------------------------------------------------------------------------------------------------- //
// -------------------------------------------------------------- FILE BIN -------------------------------------------------------------- //
// -------------------------------------------------------------------------------------------------------------------------------------- //

// Converte un IP binario nella notazione decimale puntata e mette 4 cifre in un array di int
int *da_binario_a_decPun(uint32_t ip, int ris[])
{
    uint8_t app;
    int i = 0;

    do
    {
        app = ip;
        ris[i] = app;
        ip = ip >> 8;
        i++;
    } while (i < 4);

    return ris;    
}


int FILE_dim(char *nome_FILE)
{
    int n;
    FILE *fd;
    fd = fopen(nome_FILE, "r");

    if (fd == NULL)
    {
        printf("ERR     FILE_dim(): non posso aprire il file %s\n", nome_FILE);
        return -1;  // Errore
    }
    else
    {
        // Posiviono il puntatore alla lista_peerne del FILE
        fseek(fd, 0, SEEK_END);

        // Leggo la posizione corrente de puntatore
        n = ftell(fd);
        fclose(fd);
        return n;
    }
}

void crea_file_da_zero(char *nome)
{
    FILE *fd;
    fd = fopen(nome, "w");
    printf("LOG     Creato il file: <%s>\n", nome);
    fclose(fd);
}

/* Stampo a video tutti i peer nella lista 'lista_peer.bin'*/
void stampa_lista_peer()
{
    struct des_peer dp;
    FILE * fd;
    long i;
    int str_ip[4];

    if ((fd = fopen("lista_peer.bin","r")) == NULL)
        perror("ERR     non posso aprire il file <lista_peer.bin>");
    else 
    {
        printf("\n---------------- PEER DISPONIBILI ----------------\n");
        for(i=1; i<=FILE_dim("lista_peer.bin")/sizeof(struct des_peer); i++)
        {
            fread(&dp, sizeof(struct des_peer), 1, fd);
            da_binario_a_decPun(dp.IP, str_ip);
            printf("Peer: <%d>, <%d.%d.%d.%d>\n", dp.porta, str_ip[0], str_ip[1], str_ip[2], str_ip[3]);
            fseek(fd, sizeof(struct des_peer)*i, SEEK_SET);
        }
        printf("--------------------------------------------------\n\n");
        fclose(fd);
    } 
}


/*  Cerco dentro il FILE la posizione dove inserire il nuovo peer.
    La scelta è stata fare un sistema circolare, ovvero sia che ogni peer ha due vicini
    uno precedente e l'altro successivo. Mettendo i peer i ordine di porta.
    Restituisce l'indice del file.
    Il falg my_pos serve a dichiarare se il chiamate è interessato a sapere la posizione 
    di un peer già presente nel file o meno.

    Il formato del FILE è binario, e contiene i descrittori dei peer della struttura des_peer */
int ricerca_posto_lista_peer(in_port_t this_port, int my_pos)
{
    int i = 0;
    FILE *fd;
    struct des_peer dp;

    printf("LOG     Ricerco nel file <lista_peer.bin> la posizione dove inserire il nuovo peer <%d>\n", this_port);
    //printf("DBG     FILE_dim(lista_peer.bin) = %d\n", FILE_dim("lista_peer.bin"));

    if ((fd = fopen("lista_peer.bin", "r")) == NULL)
    {
        perror("ERR     Errore nell'apertura del FILE <lista_peer.bin>");
        return -1;
    }
    else
    {
        //printf("DBG     FILE_dim(lista_peer.bin)/sizeof(struct des_peer) = %ld\n", FILE_dim("lista_peer.bin")/sizeof(struct des_peer));  
        for(i=1; i<=FILE_dim("lista_peer.bin")/sizeof(struct des_peer); i++)
        {
            fread(&dp, sizeof(struct des_peer), 1, fd);

            //printf("DBG     Ho letto la porta <%d>\n", dp.porta);

            if (this_port == dp.porta)
            {
                printf("WRN     Porta già assegnata a un peer\n");
                return (my_pos == 1)? i-1 : -1; 
            }

            if (this_port > dp.porta) // Se la porta da scrivere è più grande della porta del file  
            {
                //printf("DBG     this_port(%d) > dp.porta(%d)\n", this_port, dp.porta);
                fseek(fd, sizeof(struct des_peer)*i, SEEK_SET); // Porta avanti il cursore
            }      
            
            if (this_port < dp.porta) // Se la porta da scrivere è più piccola della porta del file
            {
                //printf("DBG     this_port(%d) < dp.porta(%d)\n", this_port, dp.porta);
                break;
            }
        }
        
        fclose(fd);
        i = (i == 1)? 0:i-1;
        //printf("DBG     Indice trovaro: <%d>\n", i);
        return i;
    }
}


/* Scrivo dentro il file 'lista_peer.bin' nella posizione (pos) del cursore. 
   Scrivo i parametri PORTA e IP passati*/
int inserimento_lista_peer(int pos, in_port_t porta_peer, uint32_t IP_peer)
{
    FILE *fd;
    FILE *fd_app;
    struct des_peer dp;
    long i;

    printf("LOG     Aggiungo il peer %hd nel file <lista_peer.bin>... ", porta_peer);  
    //printf("DBG     FILE_dim(lista_peer.bin)/sizeof(struct des_peer) = %ld\n", FILE_dim("lista_peer.bin")/sizeof(struct des_peer));  
    //printf("DBG     FILE_dim(lista_peer.bin) = %d\n", FILE_dim("lista_peer.bin")); 

    if ((fd = fopen("lista_peer.bin","r+")) == NULL)
    {
        perror("ERR     Non posso aprire il file <lista_peer.bin>");
        return -1;
    }
    else 
    {
        fd_app = fopen("temporaneo.bin","a+");
            
        for (i=0; i<pos; i++)
        {
            fseek(fd, sizeof(struct des_peer)*i, SEEK_SET); // Setto il cursore
            fread(&dp, sizeof(struct des_peer), 1, fd); // Leggo dal vecchio file
            fwrite(&dp, sizeof(struct des_peer), 1, fd_app); // Copio ne nuovo
        }

        dp.IP = IP_peer;
        dp.porta = porta_peer;
        fwrite(&dp, sizeof(struct des_peer), 1, fd_app); // Copio ne nuovo
    
        //printf("DBG     Ho scritto il mio dato <%d>\n", porta_peer);
        
        for (i=pos; i<FILE_dim("lista_peer.bin")/sizeof(struct des_peer); i++)
        {
            fseek(fd, sizeof(struct des_peer)*i, SEEK_SET); // Setto il cursore
            fread(&dp, sizeof(struct des_peer), 1, fd); // Leggo dal vecchio file
            fwrite(&dp, sizeof(struct des_peer), 1, fd_app); // Copio ne nuovo
        }

        fclose(fd);
        fclose(fd_app);
        
        // Rinomino il nuovo file e elimino il vecchio
        remove("lista_peer.bin");
        rename("temporaneo.bin", "lista_peer.bin");
        
    }

    //printf("DBG     FILE_dim(lista_peer.bin)/sizeof(struct des_peer) = %ld\n", FILE_dim("lista_peer.bin")/sizeof(struct des_peer));  
    //printf("DBG     FILE_dim(lista_peer.bin) = %d\n", FILE_dim("lista_peer.bin")); 
    
    printf("Effettuato correttamente\n");
    return 0;
}


/* Crea un nuovo file binario dove copia tutti record eccetto quello da eliminare.
   Rinomina il nuovo file e elimina il vecchio*/
int rimozione_lista_peer(int pos)
{
    FILE *fd;
    FILE *fd_app;
    struct des_peer dp;
    long i;

    printf("LOG     Rimuovo il peer dal file <lista_peer.bin>... ");  
    //printf("DBG     FILE_dim(lista_peer.bin)/sizeof(struct des_peer) = %ld\n", FILE_dim("lista_peer.bin")/sizeof(struct des_peer));  
    //printf("DBG     FILE_dim(lista_peer.bin) = %d\n", FILE_dim("lista_peer.bin")); 

    if ((fd = fopen("lista_peer.bin","r+")) == NULL)
    {
        perror("ERR     Non posso aprire il file <lista_peer.bin>");
        return -1;
    }
    else 
    {
        fd_app = fopen("temporaneo.bin","a+");
            
        for (i=0; i<pos; i++)
        {
            fseek(fd, sizeof(struct des_peer)*i, SEEK_SET); // Setto il cursore
            fread(&dp, sizeof(struct des_peer), 1, fd); // Leggo dal vecchio file
            fwrite(&dp, sizeof(struct des_peer), 1, fd_app); // Copio ne nuovo
        }
        
        for (i=pos+1; i<FILE_dim("lista_peer.bin")/sizeof(struct des_peer); i++)
        {
            fseek(fd, sizeof(struct des_peer)*i, SEEK_SET); // Setto il cursore
            fread(&dp, sizeof(struct des_peer), 1, fd); // Leggo dal vecchio file
            fwrite(&dp, sizeof(struct des_peer), 1, fd_app); // Copio ne nuovo
        }

        fclose(fd);
        fclose(fd_app);
        
        // Rinomino il nuovo file e elimino il vecchio
        remove("lista_peer.bin");
        rename("temporaneo.bin", "lista_peer.bin");
        
    }

    //printf("DBG     FILE_dim(lista_peer.bin)/sizeof(struct des_peer) = %ld\n", FILE_dim("lista_peer.bin")/sizeof(struct des_peer));  
    //printf("DBG     FILE_dim(lista_peer.bin) = %d\n", FILE_dim("lista_peer.bin")); 
    
    printf("Effettuato correttamente\n");
    stampa_lista_peer();
    return 0;
}


/* Ritrna il valore della porta relativo al peer che occupa la posizione (pos)
   Nel file 'lista_peer.bin' */
in_port_t get_port(int pos)
{
    FILE *fd;
    struct des_peer dp;

    if((fd = fopen("lista_peer.bin", "r")) == NULL)
    {
        perror("ERR     Non posso aprire il file <lista_peer.bin>");
        return -1;
    }
    else
    {
        fseek(fd, sizeof(struct des_peer)*pos, SEEK_SET);
        fread(&dp, sizeof(struct des_peer), 1, fd);
        fclose(fd);
    }

    return dp.porta;
}


/* Ritrna il valore dell'IP relativo al peer che occupa la posizione (pos)
   Nel file 'lista_peer.bin' */
uint32_t get_IP(int pos)
{
    FILE *fd;
    struct des_peer dp;

    if((fd = fopen("lista_peer.bin", "r")) == NULL)
    {
        perror("ERR     Non posso aprire il file <lista_peer.bin>");
        return -1;
    }
    else
    {
        fseek(fd, sizeof(struct des_peer)*pos, SEEK_SET);
        fread(&dp, sizeof(struct des_peer), 1, fd);
        fclose(fd);
    }

    return dp.IP;
}


// --------------------------------------------------------------------------------------------------------------------------------------//
// ----------------------------------------------------------------- NET ----------------------------------------------------------------//
// --------------------------------------------------------------------------------------------------------------------------------------//

void invia(int sd, struct sockaddr_in peer_addr, char buf[])
{
    int len_peer_addr = sizeof(peer_addr);
    printf("LOG     Invio di <%s> al peer %d... ", buf, ntohs(peer_addr.sin_port));
 
    if (sendto(sd, buf, REQUEST_LEN, 0, (struct sockaddr*) &peer_addr, len_peer_addr) == -1)
    {
        perror("ERR     Errore durante l'invio");
        return;
    }

    printf("effettuato con successo\n");
}

void ricevi(int sd, struct sockaddr_in peer_addr, char this_buf[])
{
    char buf[REQUEST_LEN];
    int len_peer_addr = sizeof(peer_addr);
    printf("LOG     Attendo <%s>... ", this_buf);

    if (recvfrom(sd, buf, REQUEST_LEN, 0, (struct sockaddr*) &peer_addr, (socklen_t *__restrict)&len_peer_addr) == -1)
    {
        perror("ERR     Errore durante la ricezione dell' ACK");
        return;
    }
    
    buf[REQUEST_LEN] = '\0';
    //printf("DBG     Ricevuto <%s>. strcmp(this_buf, buf) = %d\n", buf, strcmp(this_buf, buf));

    if (strcmp(this_buf, buf) == 0)
        printf("ricevuto con successo\n");
}


/* Invio al richiedente che fa boot, la porta di un suo vicino */
int invio_porta(in_port_t porta, int sd, struct sockaddr_in peer_addr)
{
    int ret;
    int len_peer_addr = sizeof(peer_addr);

    printf("LOG     Invio la porta <%d> al peer %d... ", porta, ntohs(peer_addr.sin_port));

    ret = sendto(sd, &porta, sizeof(in_port_t), 0, (struct sockaddr*) &peer_addr, len_peer_addr);
    if (ret == -1) // Errore recvfrom()
    {
        perror("ERR     Errore durante l'invio della porta dei vicini");
        return -1;
    }
    printf("avvenuto con successo\n");
    return 0;
}

/* Invio al richiedente che fa boot, l'IP di un suo vicino */
int invio_IP(uint32_t ip, int sd, struct sockaddr_in peer_addr)
{
    int ret;
    int len_peer_addr = sizeof(peer_addr);

    printf("LOG     Invio l'IP <%d> al peer %d... ", ip, ntohs(peer_addr.sin_port));

    ret = sendto(sd, &ip, sizeof(uint32_t), 0, (struct sockaddr*) &peer_addr, len_peer_addr);
    if (ret == -1) // Errore recvfrom()
    {
        perror("ERR     Errore durante l'invio degli indirizzi IP dei vicini");
        return -1;
    }
    printf("avvenuto con successo\n");
    return 0;
}


// --------------------------------------------------------------------------------------------------------------------------------------//
// ---------------------------------------------------------------- MAIN ----------------------------------------------------------------//
// --------------------------------------------------------------------------------------------------------------------------------------//

void comandi_disponibili()
{
    printf("I comandi disponibili sono:\n");
    printf("    1) help\n");
    printf("    2) showpeers\n");
    printf("    3) showneighbor\n");
    printf("    4) esc\n");
    printf("Digitare un comando per iniziare.\n");
}

void comandi_disponibili_con_spiegazione()
{
    printf("I comandi disponibili sono:\n");
    printf("    1) help:            Mostra il significato dei comandi e cio' che fanno\n");
    printf("    2) showpeers:       Mostra l'elenco dei peer connessi alla rete tramite il loro numero di porta\n");
    printf("    3) showneighbor:    Mostra i neighbor di un peer specifico come parametro opzionale.\n");
    printf("                        Se non viene specificato alcun parametro, il comando mostra i neighbor di ogni peer\n");
    printf("    4) esc:             Termina il DS. La terminazione causa la terminazione di tutti i peer. \n");
}

int main(int argc, char* argv[])
{
    // ( VARIABILI
        in_port_t portaDS = atoi(argv[1]);
        in_port_t porta_vuota = 0;
        //uint32_t IP_vuoto = 0;

        uint32_t IP_peer; // IP_succ, IP_prec;
        in_port_t porta_peer, porta_sinistra, porta_destra;
        int ret, sd, len_peer_addr, indx, i;
        int primo_peer = 1;
        int secondo_peer = 0;

        fd_set master;
        fd_set read_fds;
        int fd_max;

        struct sockaddr_in ds_addr, peer_addr, peer_destro, peer_sinistro;

        //FILE *fd;
        int file_len;

        // DBG
        //int str_ip[4];
    // )

    // Creiamo il file binario che utilizza il server
    // Ricesco il nome del file degli aggregati di questo peer.
    strcpy(lista_peer, "");
    sprintf(lista_peer, "lista_peer.bin");
    crea_file_da_zero(lista_peer);

    //printf("DBG     dopo crea_lista()\n");

    // Creazione del socket
    sd = socket(AF_INET, SOCK_DGRAM, 0); // UDP

    // Creazione indirizzo del socket
    memset(&ds_addr, 0, sizeof(ds_addr));
    ds_addr.sin_family = AF_INET;
    ds_addr.sin_port = htons(portaDS);
    ds_addr.sin_addr.s_addr = INADDR_ANY;

    // Aggancio
    printf("LOG     Aggancio al socket: bind()\n");
    ret = bind(sd, (struct sockaddr*) &ds_addr, sizeof(ds_addr));
    if (ret < 0) 
    {
        perror("ERR     Bind non riuscita");
        exit(-1);
    }
    else
    {
        printf("LOG     bind() eseguita con successo\n");
    }
    
    // ( GESTIONE FD: Reset dei descrittori e inizializzazione
        FD_ZERO(&master); 
        FD_ZERO(&read_fds);

        FD_SET(0, &master);
        FD_SET(sd, &master);

        fd_max = sd;
    // )

    len_peer_addr = sizeof(peer_addr);
    
    // costro a video i comandi disponibili
    printf("\n");
    comandi_disponibili();
    
    while (1)
    {     
        // printf("prompt$");
        printf("\n");
        // Uso la select() per differenziare e trattare diversamente le varie richieste fatte al DS 
        read_fds = master;
        select(fd_max+1, &read_fds, NULL, NULL, NULL);


        // ( RICHIESTE DUL SOCKET UDP
        if (FD_ISSET(sd, &read_fds)) 
        {
            // Ricevo la tipologia di comando dal peer:
            //      STR_REQ: Rischiesta di boot;
            //      STP_REQ: Il peer ha terminato la sua esecuzione 
        
            if (recvfrom(sd, (void *)buf, REQUEST_LEN, 0, (struct sockaddr*) &peer_addr, (socklen_t *__restrict)&len_peer_addr) == -1) // Errore recvfrom()
            {
                perror("ERR     Errore durante la ricezione della richiesta UDP");
                continue;
            }

            //printf("DBG     Buffer ricevuto: <%s>\n", buf);

            if (strcmp(buf, "REQ_STR") == 0) // Richiesta da un peer che ha appena fatto boot
            {    
                printf("LOG     Ricevuto messaggio <REQ_STR>\n");
                
                // Invio ACK
                invia(sd, peer_addr, "ACK_REQ");

                // Salvo porta e IP
                porta_peer = ntohs(peer_addr.sin_port);
                IP_peer = peer_addr.sin_addr.s_addr;
                
                //da_binario_a_decPun(IP_peer, str_ip);
                //printf("DBG     Peer ricevuto: <%d>, <%d> <%d.%d.%d.%d>\n", porta_peer, IP_peer, str_ip[0], str_ip[1], str_ip[2], str_ip[3]);
                
                // Ricercare nel file lista_peer.bin l'idice dove inserire
                indx = ricerca_posto_lista_peer(porta_peer, 1);
                //printf("DBG     Indice del file trovato: <%d>\n", indx);

                if (indx == -1)
                    continue;

                if (primo_peer) // Il file lista_peer.bin è vuoto. E' il primo peer a fare richiesta.
                {
                    // Invio due porte vuote per compatobilità con gli altri casi.
                    printf("LOG     E' il primo peer a fare richiesta di start. Invio indirizzi vuoti\n");
                    
                    invio_porta(porta_vuota, sd, peer_addr);

                    invio_porta(porta_vuota, sd, peer_addr);
                    
                    // Salvo il valore dell'indirizzo del primo_peer
                    first_peer = peer_addr; 

                    // Inserire nel FILE
                    inserimento_lista_peer(indx, porta_peer, IP_peer);

                    primo_peer = 0;
                    secondo_peer = 1;  
                    continue;
                }

                if (secondo_peer) // E' il secondo peer che fa richiesta. Bisogna integrare il primo peer nella network
                {
                    printf("LOG     E' il secondo peer a fare richiesta di start. Invio indirizzi al primo peer\n");
                    // Invio ai peer che hanno perso il vicino, l'aggiornamento del peer
                    invia(sd, first_peer, "UPD_REQ");

                    // Attendo l'ACK da parte del peer per poter inviare i nuovi vicini
                    ricevi(sd, first_peer, "UPD_ACK");

                    //invio_IP(IP_prec, sd, first_peer);
                    invio_porta(porta_peer, sd, first_peer);         
                    //invio_IP(IP_succ, sd, first_peer);
                    invio_porta(porta_peer, sd, first_peer);   

                    // Inserire nel FILE
                    inserimento_lista_peer(indx, porta_peer, IP_peer);
                    file_len = FILE_dim(lista_peer)/sizeof(struct des_peer);

                    indx += file_len;

                    // inviare i vicini al richiedente
                    printf("LOG     Cerco i peer sinistro (indx=%d) e destro (indx=%d)\n", (indx-1)%file_len, (indx+1)%file_len);
                    //printf("DBG     Vicino all'indx = %d\n", (indx-1)%file_len);
                    porta_sinistra = get_port((indx-1)%file_len); 
                    //printf("DBG     Porta = %d\n", porta_sinistra);

                    //printf("DBG     Vicino all'indx = %d\n", (indx+1)%file_len);
                    porta_destra = get_port((indx+1)%file_len); 
                    //printf("DBG     Porta = %d\n", porta_destra);

                    invio_porta(porta_sinistra, sd, peer_addr);         
                    invio_porta(porta_destra, sd, peer_addr);  

                    primo_peer = 0;
                    secondo_peer = 0;  
                    continue;    
                }

                // Inserire nel FILE
                inserimento_lista_peer(indx, porta_peer, IP_peer);
                file_len = FILE_dim(lista_peer)/sizeof(struct des_peer);

                indx += file_len;// In modo che se indx=0, (indx-1)%file_len != -1.

                // inviare i vicini al richiedente
                printf("LOG     Cerco i peer sinistro (indx=%d) e destro (indx=%d)\n", (indx-1)%file_len, (indx+1)%file_len);
                //printf("DBG     Vicino all'indx = %d\n", (indx-1)%file_len);
                porta_sinistra = get_port((indx-1)%file_len); 
                //printf("DBG     Porta = %d\n", porta_sinistra);

                //printf("DBG     Vicino all'indx = %d\n", (indx+1)%file_len);
                porta_destra = get_port((indx+1)%file_len); 
                //printf("DBG     Porta = %d\n", porta_destra);

                invio_porta(porta_sinistra, sd, peer_addr);         
                invio_porta(porta_destra, sd, peer_addr);         

                memset(&peer_sinistro, 0, sizeof(peer_sinistro));
                peer_sinistro.sin_family = AF_INET;
                peer_sinistro.sin_port = htons(porta_sinistra);
                peer_sinistro.sin_addr.s_addr = INADDR_ANY;

                memset(&peer_destro, 0, sizeof(peer_destro));
                peer_destro.sin_family = AF_INET;
                peer_destro.sin_port = htons(porta_destra);
                peer_destro.sin_addr.s_addr = INADDR_ANY;

                // ( Devo ora inviare una richiesta di UPDATE al peer SINISTRO
                    // Invio ai peer che hanno perso il vicino, l'aggiornamento del peer
                    invia(sd, peer_sinistro, "UPD_REQ");

                    // Attendo l'ACK da parte del peer per poter inviare i nuovi vicini
                    ricevi(sd, peer_sinistro, "UPD_ACK");

                    // inviare i vicini al richiedente
                    printf("LOG     Cerco i peer sinistro (indx=%d) e destro (indx=%d)\n", ((indx-1)-1)%file_len, ((indx-1)+1)%file_len);

                    //printf("DBG     Vicino all'indx = %d\n", ((indx-1)-1)%file_len);
                    porta_sinistra = get_port(((indx-1)-1)%file_len); 
                    //printf("DBG     Porta sinistra = %d\n", porta_sinistra);

                    //printf("DBG     Vicino all'indx = %d\n", ((indx-1)+1)%file_len);
                    porta_destra = get_port(((indx-1)+1)%file_len); 
                    //printf("DBG     Porta destra = %d\n", porta_destra);

                    invio_porta(porta_sinistra, sd, peer_sinistro);         
                    invio_porta(porta_destra, sd, peer_sinistro);         
                    
                // )

                // ( Devo ora inviare una richiesta di UPDATE al peer DESTRO    
                    // Invio ai peer che hanno perso il vicino, l'aggiornamento del peer
                    invia(sd, peer_destro, "UPD_REQ");

                    // Attendo l'ACK da parte del peer per poter inviare i nuovi vicini
                    ricevi(sd, peer_destro, "UPD_ACK");

                    // inviare i vicini al richiedente
                    printf("LOG     Cerco i peer sinistro (indx=%d) e destro (indx=%d)\n", ((indx-1)-1)%file_len, ((indx-1)+1)%file_len);

                    //printf("DBG     Vicino all'indx = %d\n", ((indx-1)-1)%file_len);
                    porta_sinistra = get_port(((indx+1)-1)%file_len); 
                    //printf("DBG     Porta sinistra = %d\n", porta_sinistra);

                    //printf("DBG     Vicino all'indx = %d\n", ((indx-1)+1)%file_len);
                    porta_destra = get_port(((indx+1)+1)%file_len); 
                    //printf("DBG     Porta destra = %d\n", porta_destra);

                    invio_porta(porta_sinistra, sd, peer_destro);         
                    invio_porta(porta_destra, sd, peer_destro);         
                    
                // )
            }
            else if (strcmp(buf, "STP_REQ") == 0) // Ho ricevuto STP_REQ 
            {
                // Devo cercare i vicini e inviare una richiesta di modifica dei loro neighbor
                printf("LOG     Ricevuto messaggio <STP_REQ>\n");

                invia(sd, peer_addr, "STP_ACK");
                // Prendo la porta dall'indirizzo
                porta_peer = ntohs(peer_addr.sin_port);
                
                file_len = FILE_dim(lista_peer)/sizeof(struct des_peer);
                printf("LOG     Ricerco peer nel file <lista_peer.bin>\n");
               
                indx = ricerca_posto_lista_peer(porta_peer, 1);
                //printf("DBG     indx = %d\n", indx);
                
                indx += file_len;   // In modo che se indx=0, (indx-1)%file_len != -1.

                porta_sinistra = get_port((indx-1)%file_len); 
                porta_destra = get_port((indx+1)%file_len); 

                printf("DBG     Invio aggiornamento al peer sinistro <%d> | indx = <%d>\n", porta_sinistra, (indx-1)%file_len);
                printf("DBG     Invio aggiornamento al peer destro <%d> | indx = <%d>\n", porta_destra, (indx+1)%file_len);

                memset(&peer_sinistro, 0, sizeof(peer_sinistro));
                peer_sinistro.sin_family = AF_INET;
                peer_sinistro.sin_port = htons(porta_sinistra);
                peer_sinistro.sin_addr.s_addr = INADDR_ANY;

                memset(&peer_destro, 0, sizeof(peer_destro));
                peer_destro.sin_family = AF_INET;
                peer_destro.sin_port = htons(porta_destra);
                peer_destro.sin_addr.s_addr = INADDR_ANY;

                // ( Devo ora inviare una richiesta di UPDATE al peer SINISTRO
                    // Invio ai peer che hanno perso il vicino, l'aggiornamento del peer
                    invia(sd, peer_sinistro, "UPD_REQ");

                    // Attendo l'ACK da parte del peer per poter inviare i nuovi vicini
                    ricevi(sd, peer_sinistro, "UPD_ACK");

                    // inviare i vicini al richiedente
                    printf("LOG     Cerco i peer sinistro (indx=%d) e destro (indx=%d)\n", ((indx-1)-1)%file_len, ((indx-1)+1)%file_len);

                    //printf("DBG     Vicino all'indx = %d\n", ((indx-1)-1)%file_len);
                    porta_sinistra = get_port(((indx-1)-1)%file_len); 
                    //printf("DBG     Porta sinistra = %d\n", porta_sinistra);

                    //printf("DBG     Vicino all'indx = %d\n", ((indx-1)+1)%file_len);
                    porta_destra = get_port(((indx-1)+2)%file_len); 
                    //printf("DBG     Porta destra = %d\n", porta_destra);

                    invio_porta(porta_sinistra, sd, peer_sinistro);         
                    invio_porta(porta_destra, sd, peer_sinistro);         
                // )

                // ( Devo ora inviare una richiesta di UPDATE al peer DESTRO    
                    // Invio ai peer che hanno perso il vicino, l'aggiornamento del peer
                    invia(sd, peer_destro, "UPD_REQ");

                    // Attendo l'ACK da parte del peer per poter inviare i nuovi vicini
                    ricevi(sd, peer_destro, "UPD_ACK");

                    // inviare i vicini al richiedente
                    printf("LOG     Cerco i peer sinistro (indx=%d) e destro (indx=%d)\n", ((indx-1)-1)%file_len, ((indx-1)+1)%file_len);

                    //printf("DBG     Vicino all'indx = %d\n", ((indx-1)-1)%file_len);
                    porta_sinistra = get_port(((indx+1)-2)%file_len); 
                    //printf("DBG     Porta sinistra = %d\n", porta_sinistra);

                    //printf("DBG     Vicino all'indx = %d\n", ((indx-1)+1)%file_len);
                    porta_destra = get_port(((indx+1)+1)%file_len); 
                    //printf("DBG     Porta destra = %d\n", porta_destra);

                    invio_porta(porta_sinistra, sd, peer_destro);         
                    invio_porta(porta_destra, sd, peer_destro);       
                // )

                // Elimino il peer che fa fatto STP_REQ dal file 'lista_peer.bin'
                printf("LOG     Rimozione peer che ha fatto richiesta STP_REQ\n");
                indx -= file_len;
                rimozione_lista_peer(indx); 
                
                // Controllare se ci sono più abbastanza peer connessi, per mantenere consistenti la variabile primo_peer e secondo_peer
                file_len = FILE_dim(lista_peer)/sizeof(struct des_peer);

                if (file_len == 0) // Non ci sono peer
                {
                    primo_peer = 1;
                    secondo_peer = 0;
                }
                
                if (file_len == 1) // C' solo un peer
                {
                    primo_peer = 0;
                    secondo_peer = 1;
                }
            }
        }
        // )


        //  ( RICHIESTA DA STDIN: 0
        if (FD_ISSET(0, &read_fds)) 
        {
            // comandi_disponibili();
            
            // pulisco buf
            strcpy(buf,"");
            strcpy(comando,"");
            parametro = 0;

            // Attendo input da tastiera
            if(fgets(buf, BUF_LEN, stdin) == NULL)
            {
                printf("ERR     Immetti un comando valido!\n");
                // printf("prompt$ ");
                continue;
            }

            sscanf(buf, "%s %d", comando, &parametro);

            if( (strlen(comando) == 0) || (strcmp(comando," ") == 0) )
            {
                printf("ERR     Comando non valido! Prova ad inserire uno dei seguenti:\n");
                // printf("prompt$ ");
                continue;
            }

            // Controllo il comando inserito e eseguo la richiesta

            // ( HELP
            if (strcmp(comando, "help") == 0)
            {
                comandi_disponibili_con_spiegazione();
            }
            //)

            // ( SHOWPEERS
            else if (strcmp(comando, "showpeers") == 0)
            {
                stampa_lista_peer();
            }
            // )

            // ( SHOWNEIGHBOR
            else if (strcmp(comando, "showneighbor") == 0)
            {
                // mostra i neighbor di un peer specifico come parametro opzionale. 
                
                // Se non viene specificato alcun parametro, il comando mostra i neighbor di ogni peer
                if (parametro != 0)
                {
                    printf("\n");
                    indx = ricerca_posto_lista_peer(parametro, 0);
                    if (ret == -1)
                    {
                        printf("ERR     Hai inserito un peer non esistente. Riprovare.\n");
                        // printf("prompt$ ");
                        continue;
                    }

                    file_len = FILE_dim(lista_peer)/sizeof(struct des_peer);
                    indx += file_len;
                    porta_sinistra = get_port((indx-1)%file_len); 
                    porta_destra = get_port((indx+1)%file_len); 
                    printf("\n--------------------- NEIGHBOR ---------------------\n");
                    printf("I vicini di %d sono: <%d>, <%d>\n", parametro, porta_sinistra, porta_destra);
                    printf("----------------------------------------------------\n\n");       
                }
                else
                {
                    printf("\n--------------------- NEIGHBOR ---------------------\n");
                    for(i=0; i<FILE_dim(lista_peer)/sizeof(struct des_peer); i++)
                    {
                        porta_peer = get_port(i);
                        file_len = FILE_dim(lista_peer)/sizeof(struct des_peer);
                        i += file_len;
                        porta_sinistra = get_port((i-1)%file_len); 
                        porta_destra = get_port((i+1)%file_len); 
                        i -= file_len;
                        printf("I vicini di %d sono: <%d>, <%d>\n", porta_peer, porta_sinistra, porta_destra);
                    }
                    printf("----------------------------------------------------\n\n"); 
                }

                // printf("prompt$ ");
            }
            // )

            // ( ESC FINIRE
            else if (strcmp(comando, "esc") == 0)
            {
                for(i=0; i<FILE_dim(lista_peer)/sizeof(struct des_peer); i++)
                {
                    porta_peer = get_port(i);
                    printf("DBG     porta_peer = %d\n", porta_peer);

                    memset(&peer_addr, 0, sizeof(peer_addr));
                    peer_addr.sin_family = AF_INET;
                    peer_addr.sin_port = htons(porta_peer);
                    peer_addr.sin_addr.s_addr = INADDR_ANY;

                    // devo avvidare tutti i peer che la connessione si sta per chiudere
                    invia(sd, peer_addr, "REQ_ESC"); // RISOLVERE ERRORE. VA inviaTO A TUTTI I PEER CONNESSI
                    ricevi(sd, peer_addr, "ESC_ACK");
                }
                
                printf("\n\n---------------- Buona Giornata! ----------------\n\n");
                
                close(sd);
                exit(-1);
            }
            // )

            else // errorre
            {
                printf("ERR     Comando non valido! Prova ad inserire uno dei seguenti:\n");
                comandi_disponibili();
                // printf("prompt$ ");
            }
        }
        // ) 
    }    

    close(sd);
    return 0;
}