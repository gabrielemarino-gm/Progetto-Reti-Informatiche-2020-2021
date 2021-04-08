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

#define CMD_LEN         12   // lunghezza comando da tastiera
#define REQUEST_LEN     8    // Lunghezza di "REQ_STR\0"
#define BUF_LEN         1024
#define POLLING_TIME    5    // Timer 

// --------------------------------------------------------------------------------------------------------------------------------------//
// -------------------------------------------------------------- VARIABILI -------------------------------------------------------------//
// --------------------------------------------------------------------------------------------------------------------------------------//
// socket per comunicare 
int udp_socket;
struct sockaddr_in my_addr; // Indirizzi di questo peer

struct neighbor
{
    struct sockaddr_in prec_addr;
    struct sockaddr_in succ_addr;
} vicino;

// UDP socket per comunicare col DS
in_port_t DS_port = -1;
struct sockaddr_in ds_addr;
int ds_len_addr = sizeof(ds_addr);

int first = 1; // Se sono solo non posso contattare altri peer


// --------------------------------------------------------------------------------------------------------------------------------------//
// ---------------------------------------------------------------- DATE ----------------------------------------------------------------//
// --------------------------------------------------------------------------------------------------------------------------------------//
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

    ris[1] = d;
    ris[2] = m;
    ris[3] = y;
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

    ris[1] = d;
    ris[2] = m;
    ris[3] = y;
}

// --------------------------------------------------------------------------------------------------------------------------------------//
// ---------------------------------------------------------------- FILE ----------------------------------------------------------------//
// --------------------------------------------------------------------------------------------------------------------------------------//
/* Restituisce la dimensione del file nome_FILE */
int FILE_dim(char *nome_FILE)
{
    int n;
    FILE *fd;
    fd = fopen(nome_FILE, "r");

    if (fd == NULL)
        return -1;  // Errore
    else
    {
        // Posiviono il puntatore alla lista_peerne del FILE
        fseek(fd, 0, SEEK_END);

        // Leggo la posizione corrente de puntatore
        n = ftell(fd);
        printf("DBG     file_dim(): posizione puntatore = %d\n", n);

        fclose(fd);
        return n;
    }
}

/* scrivo in append dentro il file di registro nome[] ciò che contiene stringa */
int scrivi_file_append(char nome[], char *stringa)
{
    FILE *fd;

    if ((fd = fopen(nome,"ab+")) == NULL)
    {
        printf("ERR     Non posso aprire il file %s\n", nome);
        return -1;
    }
    else 
    {
        fseek(fd, 0, SEEK_END);
        fprintf(fd, "%s\n", stringa);
        printf("LOG     Scrivo %s nel file %s\n", stringa, nome);
        fclose(fd);
    }

    return 0;
}

/* Leggo la riga (riga) del file nome 
int leggo_file(char nome[], int riga, char *stringa)
{
    FILE *fd;
    int i;
    int len = 8;

    if ((fd = fopen(nome,"r")) == NULL)
    {
        printf("ERR     leggo_file(): non posso aprire il file %s\n", nome);
        return -1;
    }
    else 
    {
        for (i=0; i<riga; i++)
        {
            fseek(fd, len, SEEK_SET);
            fgets(stringa, 50, fd);
            len += strlen(stringa);
        }

        printf("LOG     Ho letto: %s\n", stringa);
        fclose(fd);
    }

    return 0;
}*/

/* Leggo la riga (riga) del file nome */
int leggo_file_len(char nome[], int len, char *stringa)
{
    FILE *fd;
    int file_len = 8;

    if ((fd = fopen(nome,"r")) == NULL)
    {
        printf("ERR     leggo_file(): non posso aprire il file %s\n", nome);
        return -1;
    }
    else 
    {
        fseek(fd, file_len, SEEK_SET);
        fgets(stringa, 50, fd);
        printf("LOG     Ho letto: %s\n", stringa);
        fclose(fd);
    }

    return 0;
}

/* scrivo dentro il file di registro nome[] ciò che contiene stringa */
int apri_o_chiudi_registro(char nome[], char *stringa)
{
    FILE *fd;
   
    if ((fd = fopen(nome,"r+")) == NULL)
    {
        printf("ERR     Non posso aprire il file %s\n", nome);
        return -1;
    }
    else 
    {
        fseek(fd, 0, SEEK_SET);
        fprintf(fd, "%s\n", stringa);
        printf("LOG     Ho scritto %s nel file %s\n", stringa, nome);
        fclose(fd);
    }

    return 0;
}

/* scrivo in append dentro il file di registro nome[] ciò che contiene stringa */
int registro_aperto(char nome[])
{
    FILE *fd;
    char stringa[BUF_LEN];

    if ((fd = fopen(nome,"r")) == NULL)
    {
        printf("ERR     Non posso aprire il file %s\n", nome);
        return -1;
    }
    else 
    {
        fseek(fd, 0, SEEK_SET);
        fgets(stringa, 7, fd);
        printf("DBG     Ho letto dal file: %s\n", stringa);
        fclose(fd);
    }

    return !strcmp(stringa, "aperto");
}

/* crea un nuovo registro e ne restituisce il nome*/
int crea_nuovo_registro(int day, int month, int year, char nome_file[])
{
    FILE *fd;
    int ret;
    int data[3];
    
    data_successiva(day, month, year, data);

    strcpy(buffer, "");
    sprintf(buffer, "%d-%d-%d_%d", data[0], data[1], data[2], my_addr.sin_port);
    strcpy(nome_file, buffer);
    strncat(nome_file, ".txt", 5);
    
    fd = fopen(nome_file, "a");
    if (fd == NULL)
        ret = 0;
    else
        ret = -1;   
    fclose(fd);

    return ret;
}

// --------------------------------------------------------------------------------------------------------------------------------------//
// ----------------------------------------------------------------- NET ----------------------------------------------------------------//
// --------------------------------------------------------------------------------------------------------------------------------------//

/* Crea il socket per la connessione UDP, in modo da assocciarlo all'indirizzo e alla porta passati.
   Ritorna -1 in caso di errore */
int creazione_socket_UDP() 
{
    // Creazione del socket UDP
    printf("LOG     creazione del socket...\n");
	if((udp_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
    {
		perror("ERR     Errore nella creazione del socket UDP, ripovare: ");
		return -1;
	}
    printf("LOG     creazione del socket effettuata con successo\n");
	
    // Aggancio
    printf("LOG     Aggancio al socket: bind()...\n");
    if (bind(udp_socket, (struct sockaddr*) &my_addr, sizeof(my_addr)) == -1) 
    {
        perror("ERR     Bind non riuscita\n");
        exit(-1);
    }
    else
    {
        printf("LOG     bind() eseguita con successo\n");
    }

	printf("LOG     Connessione al server %d (porta %d) effettuata con successo\n", my_addr.sin_addr.s_addr, my_addr.sin_port);
    return 0;
}

void invio_richiesta_start()
{
    char req_str[REQUEST_LEN];
    strcpy(req_str, "REQ_STR");
    printf("LOG     Invio di %s...\n", req_str);

    if (sendto(udp_socket, req_str, REQUEST_LEN, 0, (struct sockaddr*) &ds_addr, ds_len_addr) == -1)
    {
        perror("ERR     Errore durante l'invio dell' ACK");
    }
    
    printf("LOG     %s inviato con successo\n", req_str);
}

/* Ricezione dell'ACK da parte di addr*/
void attendo_ACK(struct sockaddr_in addr, char ack[])
{
    char buf[REQUEST_LEN];
    int ret;
    int len_addr = sizeof(addr);
    printf("LOG     Attendo %s...\n", ack);

    do
    {
        ret = recvfrom(udp_socket, (void*)buf, REQUEST_LEN, 0, (struct sockaddr*) &addr, (socklen_t *__restrict)&len_addr);
    } while (!strcmp(buf, ack) || ret > 0);

    printf("LOG     %s ricevuto con successo\n", ack);
}

/* Ricezione IP e porta dei neighbor*/
void ricezione_vicini()
{
    uint32_t ip;
    in_port_t porta;

    printf("LOG     Ricezione IP e porta del primo neighbor...\n");

    if (recvfrom(udp_socket, (void*)&ip, sizeof(uint32_t), 0, (struct sockaddr*) &ds_addr, (socklen_t *__restrict)&ds_len_addr) == -1)
    {
        perror("ERR     Errore durante la ricezione del primo IP");
    }

    if (recvfrom(udp_socket, (void*)&porta, sizeof(in_port_t), 0, (struct sockaddr*) &ds_addr, (socklen_t *__restrict)&ds_len_addr) == -1)
    {
        perror("ERR     Errore durante la ricezione della prima porta");
    }

    printf("LOG     Ricezione IP e porta del primo neighbor effettuata con successo\n");

    memset(&vicino.prec_addr, 0, sizeof(vicino.prec_addr));
    vicino.prec_addr.sin_family = AF_INET;
    vicino.prec_addr.sin_addr.s_addr = ip;
    vicino.prec_addr.sin_port = ntohl(porta);

    printf("LOG     Ricezione IP e porta del secondo neighbor...\n");

    if (recvfrom(udp_socket, (void*)&ip, sizeof(uint32_t), 0, (struct sockaddr*) &ds_addr, (socklen_t *__restrict)&ds_len_addr) == -1)
    {
        perror("ERR     Errore durante la ricezione del secondo IP");
    }

    if (recvfrom(udp_socket, (void*)&porta, sizeof(in_port_t), 0, (struct sockaddr*) &ds_addr, (socklen_t *__restrict)&ds_len_addr) == -1)
    {
        perror("ERR     Errore durante la ricezione della seconda porta");
    }

    printf("LOG     Ricezione IP e porta del secondo neighbor effettuata con successo\n");

    memset(&vicino.succ_addr, 0, sizeof(vicino.succ_addr));
    vicino.succ_addr.sin_family = AF_INET;
    vicino.succ_addr.sin_addr.s_addr = ip;
    vicino.succ_addr.sin_port = ntohl(porta);
}

// --------------------------------------------------------------------------------------------------------------------------------------//
// ---------------------------------------------------------------- AGGR ----------------------------------------------------------------//
// --------------------------------------------------------------------------------------------------------------------------------------//

/*  Funzione per aggregare i dati di un singolo registro. */
int aggrega_regitro(char nome_file[])
{
    char buf[BUF_LEN];
    char tipo[2];
    int totale, variazione;
    int i, d, m, y;
    int data_pre[3];
    in_port_t porta;
    int pre_aggr_t=0, pre_aggr_p=0;
    int tt = 0; // Posizione Totale Tamponi
    int vt = 1; // Posizione Variazione Tamponi 
    int tp = 2; // Posizione Totale Positivi
    int vp = 3; // Posizione Variazione Positivi
    FILE *fd;
    int aggregato[4]; // aggregato[] contiene i risultati sul totale e sulla variazione di ogni tipo:
                      //    1) Totale tamponi
                      //    2) Variazione del numero di tamponi rispetto al giorno precedente
                      //    3) Totale positivi
                      //    4) Variazione del numero di positivi rispetto al giorno precedente

    // Inizializiamo 0x7FFFFFFF così da capire come calcolare le variazioni
    pre_aggr_t = pre_aggr_p = 0x7FFFFFFF;
    aggregato[tt] = aggregato[vt] = aggregato[tp] = aggregato[vp] = 0;
    //printf("DBG     pre_aggr_t e pre_aggr_p %d  %d\n", pre_aggr_t, pre_aggr_p);

    // Se il registro è aperto non fare nulla
    if (registro_aperto(nome_file))
    {
        printf("ERR     errore in aggrega_registro(): il registro è ancora aperto\n");
        return -1;
    }
    
    // Cerco aggregati del giorno precedente per calcolare la variazione

    // Calcolo dei totali del giorno
    sscanf(nome_file, "%d-%d-%d_%hd", &d, &m, &y, &porta);

    // Cerco data precedente
    data_precedente(d, m, y, data_pre);

    //printf("DBG     data precedente: %d-%d-%d\n", d, m, y);

    // vado a prendere gli aggregati precedenti
    if ((fd = fopen("aggr.txt","r")) == NULL)
    {
        printf("ERR     leggo_file(): non posso aprire il file 'aggr.txt'\n");
        return -1;
    }
    else
    {
        i = 0;
        //printf("\nDBG     scorro aggr.txt\n");
        while (i<FILE_dim("aggr.txt"))
        {
            fseek(fd, i, SEEK_SET);
            fgets(buf, 50, fd);   

            //printf("LOG     Ho letto: %s", buf);
            i += strlen(buf);

            sscanf(buf, "%d:%d:%d %s %d %d", &d, &m, &y, tipo, &totale, &variazione);
            //printf("DBG     data trovata nel file aggr.txt: %d-%d-%d\n", d, m, y);
            //printf("DBG     tipo = %s\n", tipo);
            
            if (strcmp(tipo, "t") == 0 && data_pre[0] == d && data_pre[1] == m && data_pre[2] == y)
            {
                pre_aggr_t = totale;
                printf("DBG     pre_aggr_t = %d\n", pre_aggr_t);
            }
            else if (strcmp(tipo, "n") == 0 && data_pre[0] == d && data_pre[1] == m && data_pre[2] == y)
            {
                pre_aggr_p = totale;
                printf("DBG     pre_aggr_p = %d\n", pre_aggr_p);
            }          
        }
        fclose(fd);
    }

    // Resetto le variabili usate prima
    d = 0;
    m = 0; 
    y = 0; 
    strcpy(tipo, "");
    totale = 0;
    
    if ((fd = fopen(nome_file,"r")) == NULL)
    {
        printf("ERR     leggo_file(): non posso aprire il file %s\n", nome_file);
        return -1;
    }
    else 
    {
        i = strlen("aperto"); // per saltare l'intestazione del registro: aperto/chiuso
        //printf("\nDBG     scorro %s\n", nome_file);
        while (i<FILE_dim(nome_file))
        {
            fseek(fd, i, SEEK_SET);
            fgets(buf, 50, fd);   

            printf("LOG     Ho letto: %s", buf);
            i += strlen(buf);

            sscanf(buf, "%d:%d:%d %s %d", &d, &m, &y, tipo, &totale);
            
            if (strcmp(tipo, "t") == 0)
            {
                //printf("DBG     TAMPONI\n");
                //printf("DBG     aggregato[tt] = %d\n", aggregato[tt]);
                //printf("DBG     totale = %d\n", totale);
                aggregato[tt] += totale;
                //printf("DBG     aggregato[tt] = %d\n", aggregato[tt]);
            }
            else
            {
                //printf("DBG     POSITIVI\n");
                //printf("DBG     aggregato[tp] = %d\n", aggregato[tp]);
                //printf("DBG     totale = %d\n", totale);
                aggregato[tp] += totale;
                //printf("DBG     aggregato[tp] = %d\n", aggregato[tp]);
            }
        }
        fclose(fd);
    }

    // Ora ho il totale del giorno d'oggi.
    // Calcolo la variazione se posso farlo
    if (pre_aggr_p == 0x7FFFFFFF && pre_aggr_t == 0x7FFFFFFF) // L'aggregazione precedente è mancante
    {
        printf("LOG     L'aggregazione precedente è mancante\n");
        aggregato[vt] = 0x7FFFFFFF;
        aggregato[vp] = 0x7FFFFFFF; 
    }
    else if (pre_aggr_p == 0x7FFFFFFF && pre_aggr_t != 0x7FFFFFFF) // Tamponi tutti negativi
    {
        
        printf("LOG     Tamponi negativi: pre_aggr_p = %d\n", pre_aggr_p);
        aggregato[vt] = aggregato[tt] - pre_aggr_t;
        aggregato[vp] = aggregato[tp];
    }
    else if (pre_aggr_p != 0x7FFFFFFF && pre_aggr_t != 0x7FFFFFFF)
    {
        aggregato[vt] = aggregato[tt] - pre_aggr_t;
        aggregato[vp] = aggregato[tp] - pre_aggr_p;
    }

    // Scrivo sul file aggr.txt
    // ( TAMPONI
        // Creo stringa da scrivere 
        strcpy(tipo, "t");
        sprintf(buf, "%d:%d:%d %s %d %d", d, m, y, tipo, aggregato[tt], aggregato[vt]);
        printf("LOG     Sto scrivendo: %s\n", buf);
        
        // Scrivo in append nel file
        scrivi_file_append("aggr.txt", buf);
    // )

    // ( POSITIVI
        // Creo stringa da scrivere per i 
        strcpy(tipo, "n");
        sprintf(buf, "%d:%d:%d %s %d %d", d, m, y, tipo, aggregato[tp], aggregato[vp]);
        printf("LOG     Sto scrivendo: %s\n", buf);
        
        // Scrivo in append nel file
        scrivi_file_append("aggr.txt", buf);
    // )
    return 0;
}

/*  Funzione per aggregare i dati. L'array aggregato[] contiene
    i risultati sul totale e sulla variazione di ogni tipo:
        1) Totale tamponi
        2) Variazione del numero di tamponi rispetto al giorno precedente
        3) Totale positivi
        4) Variazione del numero di positivi rispetto al giorno precedente

    In caso di mancaza di file, ritorna il nome del file mancate in nome_file_eff[]
int aggrega(char data1[], char data2[], int aggregato[], char nome_file_err[])
{
    char nome_file[BUF_LEN];
    char buf[BUF_LEN];
    int data_inizio[3];
    int data_fine[3];
    int data_corrente[3];
    char tipo[2];
    int totale;
    int i;
    int d, m, y;
    int ret;
    
    // Cerco la data odierna
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    data_corrente[0] = timeinfo->tm_mday;
    data_corrente[1] = timeinfo->tm_mon + 1;
    data_corrente[2] = timeinfo->tm_year + 1900;

    sscanf(data1, "%s:%s:%s", data_inizio[0], data_inizio[1], data_inizio[2]);
    sscanf(data2, "%s:%s:%s", data_fine[0], data_fine[1], data_fine[2]);

    // Controllo sulle date: data_inizio < data_fine
    if ((data_inizio[0] > data_fine[0] && data_inizio[1] == data_fine[1] && data_inizio[2] == data_fine[2]) ||
        (data_inizio[1] > data_fine[1] && data_inizio[2] == data_fine[2]) ||
        (data_inizio[2] > data_fine[2]))
    {
        printf("ERR     Errore nella aggrega(): le date sono inconsistenti\n");
        return -1;
    }

    // creo il nome del registro per controllare se è aperto o meno
    strcpy(nome_file, "");
    sprintf(nome_file, "%d-%d-%d_%d", timeinfo->tm_mday, timeinfo->tm_mon+1, timeinfo->tm_year+1900, my_addr.sin_port);            
    strncat(nome_file, ".txt", 5);

    // Controllo se la data di inivio o la data di fine è di un registro aperto
    if ((data_inizio[0] == data_corrente[0] && data_inizio[1] == data_corrente[1] && data_inizio[2] == data_corrente[2]) ||
        (data_fine[0] == data_corrente[0] && data_fine[1] == data_corrente[1] && data_fine[2] == data_corrente[2]) &&
        registro_aperto(nome_file))
    {
        printf("ERR     Errore nella aggrega(): Una delle date immesse ha un registro ancora aperto\n");
        return -1;
    }

    d = data_inizio[0];
    m = data_inizio[1];
    y = data_inizio[2];

    aggregato[0] = 0;
    aggregato[1] = 0;
    aggregato[2] = 0;
    aggregato[3] = 0;

    // scorro tutti i file, e aggrego i dati
    while (d <= data_fine[0] || m <= data_fine[1] || y <= data_fine[2])
    {
        // Salto i giorni che non esistono 
        if ((y%4 != 0) && (// Anno non bisestile
            (d == 29 && m == 2) || (d == 30 && m == 2) || (d == 31 && m == 2) || // Febbraio
            (d == 31 && m == 4) || // Aprile
            (d == 31 && m == 6) || // Giugno
            (d == 31 && m == 9) || // Settembre
            (d == 31 && m == 11))) // Novembre
        {
            continue;
        }

        if ((y%4 == 0) && ( // Anno bisestile
            (d == 30 && m == 2) || (d == 31 && m == 2) || // Febbraio
            (d == 31 && m == 4) || // Aprile
            (d == 31 && m == 6) || // Giugno
            (d == 31 && m == 9) || // Settembre
            (d == 31 && m == 11))) // Novembre
        {
            continue;
        }

        // Creo il nome del file
        strcpy(nome_file, "");
        sprintf(nome_file, "%d-%d-%d_%d", d, m, y, my_addr.sin_port);            
        strncat(nome_file, ".txt", 5);

        // Calcolo dei totali del giorno
        i = strlen("aperto"); // per saltare l'intestazione del registro: aperto/chiuso
        while(i < FILE_dim(nome_file))
        {
            if (leggo_file_len(nome_file, i, buf) == -1)
            {
                printf("ERR     Il file %s non esiste\n", nome_file);
                strcpy(nome_file_err, nome_file);
                return -1;
            }

            sscanf(buf, "%d:%d:%d %s %d", d, m, y, tipo, totale);
            
            if (strcmp(tipo, "t") == 0)
            {
                aggregato[0] += totale;
            }
            else
            {
                aggregato[2] += totale;
            }

            i += strlen(buf);
        }
                   
    }
}*/

// --------------------------------------------------------------------------------------------------------------------------------------//

inline void comandi_disponibili()
{
    printf("I comandi disponibili sono:\n");
    printf("    1) help\n");
    printf("    2) start <DS_addr> <DS_port>\n");
    printf("    3) add <type> <quantity>\n");
    printf("    4) get <aggr> <type> <period>\n");
    printf("    5) stop\n");
    printf("Digitare un comando per iniziare.\n");
}

inline void comandi_disponibili_con_spiegazione()
{
    printf("I comandi disponibili sono:\n");
    printf("    1) help:                        Mostra il significato dei comandi e cio' che fanno\n");
    printf("    2) start <DS_addr> <DS_port>:   Richiede al DS in ascolto all'indirizzo DS_addr:DS_port, la connessione alla network\n");
    printf("    3) add <type> <quantity>:       Aggiunge al register della data corrente l'evento type con quantità quantity\n");
    printf("    4) get <aggr> <type> <period>   Effettua una richiesta di elaborazione per ottenere l'aggregato aggr sui dati relativi\n");
    printf("                                    a un lasso temporale d'interesse period sulle entry di tipo type\n");
    printf("    5) stop                         Il peer richiede di disconnettersi dal network\n");
}

int main(int argc, char* argv[])
{
    // ( VARIABILI
        in_port_t my_port = atoi(argv[1]);

        fd_set master;
        fd_set read_fds;
        int fd_max;

        char nome_file[BUF_LEN];
        char cmd[REQUEST_LEN];  // Per ricevere i comadi dai peera: REQ - STP
        char buffer[BUF_LEN];   // Usato per memorizzare i comandi da tastiera
        char comando[BUF_LEN];  // comando e parametro sono usati per differenziare i comandi dai parametri dei comandi da tastiera
        int parametro1, parametro2;
        char carattere1[2];

        time_t rawtime;
        struct tm* timeinfo;
    // )

    // Creazione indirizzo del socket
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(my_port);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    // Creo il socket e lo aggancio all'indirizzo
    creazione_socket_UDP();

    // ( GESTIONE FD: Reset dei descrittori e inizializzazione
        FD_ZERO(&master); 
        FD_ZERO(&read_fds);

        FD_SET(0, &master);
        FD_SET(udp_socket, &master);

        fd_max = udp_socket;
    // )

    comandi_disponibili();

    while (1)
    {
        // Uso la select() per differenziare e trattare diversamente le varie richieste fatte al DS 
        read_fds = master;
        select(fd_max+1, &read_fds, NULL, NULL, NULL);

        if (FD_ISSET(udp_socket, &read_fds)) // DA FARE
        {
            // gestione richieste esterne
            
        }

        if (FD_ISSET(0, &read_fds))
        {
            // gestione richieste da stdin
            printf("LOG     Accetto richiesta da stdin...");
            strcpy(buffer,"");

            // Attendo input da tastiera
            if(fgets(buffer, 1024, stdin) == NULL)
            {
                printf("ERR     Immetti un comando valido!\n");
                continue;
            }

            sscanf(buffer, "%s", comando);

            if( (strlen(comando) == 0) || (strcmp(comando," ") == 0) )
            {
                printf("ERR     Comando non valido! Prova ad inserire uno dei seguenti:\n");
                comandi_disponibili();
                continue;
            }
            
            // ( HELP
            if (strcmp(comando, "help") == 0) 
            {
                comandi_disponibili_con_spiegazione();
            }
            // )

            // ( START
            else if (strcmp(comando, "start") == 0) // REQ_STR
            {
                // Bisogna far richiesta al DS di entrare nella rete
                invio_richiesta_start();

                // Attendo ACK_STR
                attendo_ACK(ds_addr, "ACK_REQ");

                // Ricezione dati IP - porta
                ricezione_vicini();

                // Bisogna vedere se siamo i primi
                if (vicino.prec_addr.sin_addr.s_addr == 0 && vicino.succ_addr.sin_addr.s_addr == 0)
                {
                    first = 1; // Non posso comunicare con gli altri peer, perchè non ce ne sono
                }
            }
            // )

            // ( ADD
            else if (strcmp(comando, "add") == 0)
            {
                sscanf(buffer, "%s %s %d", comando, carattere1, &parametro1);
                
                // Controllo parametro carattere
                if(strcmp(carattere1, "n") != 0 || strcmp(carattere1, "t") != 0)
                {
                    printf("ERR     Errore nel tipo, deve essere 'n' o 't'\n");
                    continue;
                }
                
                time(&rawtime);
                timeinfo = localtime(&rawtime);
                
                // Creo il nome
                strcpy(buffer, "");
                sprintf(buffer, "%d-%d-%d_%d", timeinfo->tm_mday, timeinfo->tm_mon+1, timeinfo->tm_year+1900, my_addr.sin_port);
                printf("LOG     Data: %s\n", buffer);                
                strcpy(nome_file, buffer);
                strncat(nome_file, ".txt", 5);

                printf("LOG     Nome del regostro creato: %s\n", nome_file);

                // Bisogna controllare l'ora e se il registro è già stato chiuso per eventualmente crearne uno nuovo
                if ((timeinfo->tm_hour >= 18 && timeinfo->tm_min != 0 && timeinfo->tm_sec != 0)) // Se siamo oltre le 18:
                {
                    if (registro_aperto(nome_file)) // Se il registro è aperto, lo chiudo, aggrego i dati e ne creo uno nuovo
                    {
                        // Chiudo registro
                        apri_o_chiudi_registro(nome_file, "chiuso");

                        // Aggrego i dati di quel registro
                        aggrega_regitro(nome_file);

                        // Creo registro con la data successiva a quella odierna
                        if (crea_nuovo_registro(timeinfo->tm_mday, timeinfo->tm_mon+1, timeinfo->tm_year+1900, nome_file) != 0)
                        {
                            printf("ERR     non ho potuto creare il nuovo registro\n");
                            continue;
                        }

                        // Apro il registro
                        apri_o_chiudi_registro(nome_file, "aperto");
                    }
                    else // Il registro è già chiuso, un add precedente avrà creato il nuovo file, creo il nome
                    {                        
                        strcpy(buffer, "");
                        sprintf(buffer, "%d-%d-%d_%d", timeinfo->tm_mday, timeinfo->tm_mon+1, timeinfo->tm_year+1900, my_addr.sin_port);
                        strcpy(nome_file, buffer);
                        strncat(nome_file, ".txt", 5);
                    }
                }

                if (FILE_dim(nome_file) == -1) // File vuoto, appena creato
                {                  

                    apri_o_chiudi_registro(nome_file, "aperto");
                }

                // Creo stringa da scrivere
                sprintf(buffer, "%d:%d:%d %s %d", timeinfo->tm_mday, timeinfo->tm_mon+1, timeinfo->tm_year+1900, carattere1, parametro2);
                printf("LOG     Sto scrivendo: %s\n", buffer);
                
                // Scrivo in append nel file
                scrivi_file_append(nome_file, buffer);
            }
            // )

            // ( GET
            else if (strcmp(comando, "get") == 0) // DA FARE
            {
                /* code */
            }
            //) 

            // ( STP
            else if (strcmp(comando, "stop") == 0) // DA FARE
            {
                /* code */
            }
            // )

            else
            {
                printf("ERR     Comando non valido! Prova ad inserire uno dei seguenti:\n");
                comandi_disponibili();
            }       
        }

    }
}
