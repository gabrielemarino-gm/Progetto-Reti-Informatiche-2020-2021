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

#include "./librerie/date.h"
#include "./librerie/file_txt.h"
#include "./librerie/costanti.h"

// --------------------------------------------------------------------------------------------------------------------------------------//
// -------------------------------------------------------------- VARIABILI -------------------------------------------------------------//
// --------------------------------------------------------------------------------------------------------------------------------------//

// socket per comunicare 
int udp_socket, ascolto_sd, trasporto_sd;
in_port_t my_port;
struct sockaddr_in my_addr, peer_addr, mittente; // Indirizzi di questo peer

struct neighbor
{
    struct sockaddr_in addr_sinistro;
    struct sockaddr_in addr_destro;
} vicino;

// UDP socket per comunicare col DS
in_port_t DS_port = -1;
struct sockaddr_in ds_addr;
int ds_len_addr = sizeof(ds_addr);

int first = 1; // Se sono solo non posso contattare altri peer
int boot = 0;  // Se non ho fatto la richiesta di boot non posso contattare gli altri

// Variabili per i nomi dei file
char nome_reg[BUF_LEN];
char nome_miei_aggr[BUF_LEN];
char nome_aggr[BUF_LEN];

// --------------------------------------------------------------------------------------------------------------------------------------//
// ----------------------------------------------------------------- NET ----------------------------------------------------------------//
// --------------------------------------------------------------------------------------------------------------------------------------//


/* Instaura una connessione TCP con i due vicini che rispondono con l'eventuale dato mancante*/
int richiedi_ai_vicini(int data_i[], char tipo_attuale)
{
    int sd_destro, sd_sinistro;
    char buf[BUF_LEN];
    int buf_len;
    int totale;

    //printf("DBG     richiedi_ai_vicini(): START\n");

    // Creo il messaggio da inviare per fare la richiesta
    sprintf(buf, "NGB_REQ %d:%d:%d %c ", data_i[D], data_i[M], data_i[Y], tipo_attuale);
    buf_len = strlen(buf);

    // NEIGHBOR SINISTRO
    sd_sinistro = socket(AF_INET, SOCK_STREAM, 0);

    if(connect(sd_sinistro, (struct sockaddr *)&vicino.addr_sinistro, sizeof(vicino.addr_sinistro)) == -1)
    {
        perror("ERR     Errore nella connect() al vicino sinistro");
        return -1;
    }
    
    printf("LOG     Invio il messaggio <%s> al vicino sinistro %d\n", buf, ntohs(vicino.addr_sinistro.sin_port));

    // Invio lunghezza del messaggio
    if (send(sd_sinistro, (void*) &buf_len, sizeof(int), 0) == -1)
    {
        printf("ERR     Dimensione del messaggio non inviato correttamente\n");
        return -1;
    }

    // Invio messaggio di richiesta    
    if ((send(sd_sinistro, (void *)buf, strlen(buf), 0)) == -1)
    {
        printf("ERR     Messaggio non inviato correttamente\n");
    }

    if (recv(sd_sinistro, (void *)&totale, sizeof(int), 0) == -1)
    {
        perror("ERR     Errore nella recv() del totale");
        return -1;
    }

    printf("LOG     Ho ricevuto il totale: %d\n", totale);

    close(sd_sinistro);

    if (totale != 0xFFFFFFFF)
    {
        // Aggiornare il dato
        aggiungi_aggr(nome_aggr, data_i, tipo_attuale, totale);
        return 0;
    }

    // NEIGHBOR DESTRO
    sd_destro = socket(AF_INET, SOCK_STREAM, 0);

    if(connect(sd_destro, (struct sockaddr *)&vicino.addr_destro, sizeof(vicino.addr_destro)) == -1)
    {
        perror("ERR     Errore nella connect() al vicino sinistro");
        return -1;
    }
    
    printf("LOG     Invio il messaggio <%s> al vicino destro %d\n", buf, ntohs(vicino.addr_destro.sin_port));

    // Invio lunghezza del messaggio
    if (send(sd_destro, (void*) &buf_len, sizeof(int), 0) == -1)
    {
        printf("ERR     Dimensione del messaggio non inviato correttamente\n");
        return -1;
    }

    // Invio messaggio di richiesta    
    if ((send(sd_destro, (void *)buf, strlen(buf), 0)) == -1)
    {
        printf("ERR     Messaggio non inviato correttamente\n");
    }

    // Ricevo totale richiesto
    if (recv(sd_destro, (void *)&totale, sizeof(int), 0) == -1)
    {
        perror("ERR     Errore nella recv() del totale");
        return -1;
    }

    printf("LOG     Ho ricevuto il totale: %d\n", totale);

    close(sd_destro);

    if (totale != 0xFFFFFFFF)
    {
        // Aggiornare il dato
        aggiungi_aggr(nome_aggr, data_i, tipo_attuale, totale);        
        return 0;
    }

    return -1;
}


/* Richiesta di flooting.
   Il flag 'transito' serve a differenziare il comportamento della funzione quando è chiamata per iniziare una richiesta di flooting
   o quando è chiamata per far transitare una richiesta di flooting */
void flooting(char req[], int data_i[], char tipo_attuale, int totale_attuale, int porta, int transito)
{
    int sd_destro, sd_sinistro;
    char buf[BUF_LEN];
    int buf_len;
    int totale;

    //printf("DBG     dentro la chiamata a flooting(): porta_richiedente = %hd\n", porta);
    // Se la richiesta non è di transito
    if (transito == 0) // Mi connetto al peer sinistro da cui mi arriverà la risposta
    {
        //printf("DBG     La richiesta di flooting parte da me!\n");
        // Creo il messaggio da inviare per fare la richiesta
        strcpy(buf, "");
        sprintf(buf, "FLT_WAIT %d:%d:%d %c %d %d ", data_i[D], data_i[M], data_i[Y], tipo_attuale, totale_attuale, porta);
        buf_len = strlen(buf);

        sd_sinistro = socket(AF_INET, SOCK_STREAM, 0);

        if(connect(sd_sinistro, (struct sockaddr *)&vicino.addr_sinistro, sizeof(vicino.addr_sinistro)) == -1)
        {
            perror("ERR     Errore nella connect() al vicino sinistro");
            return;
        }
        
        printf("LOG     Invio il messaggio <%s> al vicino sinistro %d\n", buf, ntohs(vicino.addr_sinistro.sin_port));

        // Invio lunghezza del messaggio
        if (send(sd_sinistro, (void*) &buf_len, sizeof(int), 0) == -1)
        {
            printf("ERR     Dimensione del messaggio non inviato correttamente\n");
            return;
        }

        // Invio messaggio di richiesta    
        if ((send(sd_sinistro, (void *)buf, strlen(buf), 0)) == -1)
        {
            printf("ERR     Messaggio non inviato correttamente\n");
        }
    }

    // Creo il messaggio da inviare per fare la richiesta di flooting
    sprintf(buf, "%s %d:%d:%d %c %d %d ", req, data_i[D], data_i[M], data_i[Y], tipo_attuale, totale_attuale, porta);
    //printf("DBG     flooting(): buf = <%s> | len = <%ld>\n", buf, strlen(buf));
    buf_len = strlen(buf);    
 
    // La richiesta di floting percorre un solo verso. 
    sd_destro = socket(AF_INET, SOCK_STREAM, 0);

    if(connect(sd_destro, (struct sockaddr *)&vicino.addr_destro, sizeof(vicino.addr_destro)) == -1)
    {
        perror("ERR     Errore nella connect() al vicino sinistro");
        return;
    }
    
    printf("LOG     Invio il messaggio <%s> al vicino destro %d\n", buf, ntohs(vicino.addr_destro.sin_port));

    // Invio lunghezza del messaggio
    if (send(sd_destro, (void*) &buf_len, sizeof(int), 0) == -1)
    {
        printf("ERR     Dimensione del messaggio non inviato correttamente\n");
        return;
    }

    // Invio messaggio di richiesta    
    if ((send(sd_destro, (void *)buf, strlen(buf), 0)) == -1)
    {
        printf("ERR     Messaggio non inviato correttamente\n");
    }

    close(sd_destro);

    if (transito == 0)
    {
        // Dopo aver fatto il giro la richiesta di floot deve tornare qui
        if (recv(sd_sinistro, (void *)&totale, sizeof(int), 0) == -1)
        {
            perror("ERR     Errore nella recv() del totale");
            return;
        }

        close(sd_sinistro);

        printf("LOG     Ho ricevuto il totale: %d\n", totale);

        // Aggiornare il dato
        aggiungi_aggr(nome_aggr, data_i, tipo_attuale, totale);
    }

}


/* Data una data di inizio e fine, o eventualmente gli asterischi asterischi, verifico la presenza dei dati richiesti.
   Se manca qualche dato lo richiedo ai vicini, e poi faccio l'eventuale richiesta di flooting.
   Quando questa funzione finisce la sua esecuzione, il peer avrà a disposizione tutti i dati necessari*/
int verifica_presenza_dati(int data_ini[], int data_fin[], char asterisco1, char astrisco2, char tipo_attuale)
{
    int data_i[3];
    int n_giorni;
    int porta;
    int floot = 0;
    int mio_tot;

    if (asterisco1 != '*' && astrisco2 != '*') // Intervallo integro
    {
        data_i[D] = data_ini[D];
        data_i[M] = data_ini[M];
        data_i[Y] = data_ini[Y];
        n_giorni = numero_giorni_fra_due_date(data_ini[D], data_ini[M], data_ini[Y], data_fin[D], data_fin[M], data_fin[Y]);
        // Scorro giorno per giorno
        while (n_giorni > 0)
        {
            if (verifica_presenza_della_data(nome_aggr, data_i[D], data_i[M], data_i[Y], tipo_attuale) == 0) // Non ho trovato la data
            {
                if (first == 1)
                {
                    printf("WRN     Questo peer è l'unico della rete, non posso fare flooting\n");
                    return 0;
                }

                if (boot == 0)
                {
                    printf("ERR     Questo peer non ha fatto boot, si prega di eseguire prima il comando <start>\n");
                    return 0;
                }
                    
                if (richiedi_ai_vicini(data_i, tipo_attuale) == -1)
                {
                    printf("ERR     Non è stato possibile chiedere ai vicini, oppure i vicini non hanno il dato richiesto\n");
                    floot = 1;
                }

                if (floot == 1)
                {
                    // Cerco il mio totale di quel giorno per mandarlo insieme alla richiesta di flooting, così che possa fare il giro della rete
                    cerca_totale(nome_miei_aggr, data_i, tipo_attuale, &mio_tot, 0);
                    //printf("DBG     Il mio totale prima dell flooting: %d\n", mio_tot);
                    flooting("FLT_REQ", data_i, tipo_attuale, mio_tot, my_port, 0);
                    floot = 0;
                }
            }

            data_successiva(data_i[D], data_i[M], data_i[Y], data_i);
            n_giorni--;
        }
    }
    else if ( asterisco1 == '*' && astrisco2 != '*') // *-data_fine
    {
        // Non essendo stata indicata la data di lower bound, imposto come prima data una data comune per tutti.
        data_i[D] = data_ini[D] = 1;
        data_i[M] = data_ini[M] = 4;
        data_i[Y] = data_ini[Y] = 2021;

        n_giorni = numero_giorni_fra_due_date(data_ini[D], data_ini[M], data_ini[Y], data_fin[D], data_fin[M], data_fin[Y]);
        // Scorro giorno per giorno
        while (n_giorni > 0)
        {
            if (verifica_presenza_della_data(nome_aggr, data_i[D], data_i[M], data_i[Y], tipo_attuale) == 0) // Non ho trovato la data
            {
                if (richiedi_ai_vicini(data_i, tipo_attuale) == -1)
                {
                    printf("ERR     Non è stato possibile chiedere ai vicini, oppure i vicini non hanno il dato richiesto\n");
                    floot = 1;
                }

                if (floot == 1)
                {
                    // Cerco il mio totale di quel giorno per mandarlo insieme alla richiesta di flooting, così che possa fare il giro della rete
                    cerca_totale(nome_miei_aggr, data_i, tipo_attuale, &mio_tot, 0);
                    flooting("FLT_REQ", data_i, tipo_attuale, mio_tot, my_port, 0);
                    floot = 0;
                }
            }

            data_successiva(data_i[D], data_i[M], data_i[Y], data_i);
            n_giorni--;
        }
    }
    else if (asterisco1 != '*' && astrisco2 == '*') // data_inizio-*
    {
        // Devo trovare l'ultima data, controllo se il resistro di oggi è aperto o chiuso
        // Se è aperto allora la data è quella precedente
        sscanf(nome_reg, "reg_%d-%d-%d_%d", &data_fin[D], &data_fin[M], &data_fin[Y], &porta);
        //printf("DBG     Data fine trovata: %d:%d:%d, porta %d\n", data_fin[D], data_fin[M], data_fin[Y], porta);
        if(registro_aperto(nome_reg) == 1)
        {
            data_precedente(data_fin[D], data_fin[M], data_fin[Y], data_fin);
        }

        printf("DBG     data_inizio-* data presa dal registro: %d:%d:%d\n", data_fin[D], data_fin[M], data_fin[Y]);

        data_i[D] = data_ini[D];
        data_i[M] = data_ini[M];
        data_i[Y] = data_ini[Y];
        n_giorni = numero_giorni_fra_due_date(data_ini[D], data_ini[M], data_ini[Y], data_fin[D], data_fin[M], data_fin[Y]);
        // Scorro giorno per giorno
        while (n_giorni > 0)
        {
            if (verifica_presenza_della_data(nome_aggr, data_i[D], data_i[M], data_i[Y], tipo_attuale) == 0) // Non ho trovato la data
            {
                if (richiedi_ai_vicini(data_i, tipo_attuale) == -1)
                {
                    printf("ERR     Non è stato possibile chiedere ai vicini, oppure i vicini non hanno il dato richiesto\n");
                    floot = 1;
                }

                if (floot == 1)
                {
                    // Cerco il mio totale di quel giorno per mandarlo insieme alla richiesta di flooting, così che possa fare il giro della rete
                    cerca_totale(nome_miei_aggr, data_i, tipo_attuale, &mio_tot, 0);
                    flooting("FLT_REQ", data_i, tipo_attuale, mio_tot, my_port, 0);
                    floot = 0;;
                }
            }

            data_successiva(data_i[D], data_i[M], data_i[Y], data_i);
            n_giorni--;
        }
    }
    else // *-*
    {
        data_i[D] = data_ini[D] = 1;
        data_i[M] = data_ini[M] = 4;
        data_i[Y] = data_ini[Y] = 2021;

        sscanf(nome_reg, "reg_%d-%d-%d_", &data_fin[D], &data_fin[M], &data_fin[Y]);
        if(registro_aperto(nome_reg) == 1)
        {
            data_precedente(data_fin[D], data_fin[M], data_fin[Y], data_fin);
        }

        printf("DBG     *-* data presa dal registro: %d:%d:%d\n", data_fin[D], data_fin[M], data_fin[Y]);

        n_giorni = numero_giorni_fra_due_date(data_ini[D], data_ini[M], data_ini[Y], data_fin[D], data_fin[M], data_fin[Y]);
        // Scorro giorno per giorno
        while (n_giorni > 0)
        {
            if (verifica_presenza_della_data(nome_aggr, data_i[D], data_i[M], data_i[Y], tipo_attuale) == 0) // Non ho trovato la data
            {
                if (richiedi_ai_vicini(data_i, tipo_attuale) == -1)
                {
                    printf("ERR     Non è stato possibile chiedere ai vicini, oppure i vicini non hanno il dato richiesto\n");
                    floot = 1;
                }

                if (floot == 1)
                {
                    // Cerco il mio totale di quel giorno per mandarlo insieme alla richiesta di flooting, così che possa fare il giro della rete
                    cerca_totale(nome_miei_aggr, data_i, tipo_attuale, &mio_tot, 0);
                    flooting("FLT_REQ", data_i, tipo_attuale, mio_tot, my_port, 0);
                    floot = 0;
                }
            }

            data_successiva(data_i[D], data_i[M], data_i[Y], data_i);
            n_giorni--;
        }
    }
    return 0;
}


/* Crea il socket per la connessione UDP, in modo da assocciarlo all'indirizzo e alla porta passati.
   Ritorna -1 in caso di errore */
int creazione_socket_UDP() 
{
    // Creazione del socket UDP
    //printf("LOG     creazione del socket UDP...\n");
	if((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
    {
		perror("ERR     Errore nella creazione del socket UDP, ripovare: ");
		return -1;
	}
	
    // Aggancio
    if (bind(udp_socket, (struct sockaddr*) &my_addr, sizeof(my_addr)) == -1) 
    {
        perror("ERR     Bind non riuscita\n");
        exit(-1);
    }
    else
    {
        printf("LOG     bind() eseguita con successo\n");
    }

    printf("LOG     Creato socket UDP per la comunicazione col DS: %d\n", ascolto_sd);
    return 0;
}


/* Crea il socket per la connessione TCP, in modo da assocciarlo all'indirizzo e alla porta passati.
   Ritorna -1 in caso di errore */
int creazione_socket_TCP()
{
    //printf("LOG     Creazione socket TCP...\n");

    ascolto_sd = socket(AF_INET, SOCK_STREAM, 0);
    if (bind(ascolto_sd, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1)
    {
        perror("ERR     Errore nella bind()");
        return -1;
    }
    else
    {
        printf("LOG     bind() eseguita con successo\n");
    }
        
    listen(ascolto_sd, 10);
    printf("LOG     Creato socket TCP in ascolto: %d\n", ascolto_sd);
    return 0;
}


/* Invia una richiesta 'request' al DS*/
void invia_al_DS(char request[])
{
    
    printf("LOG     Invio di <%s>... ", request);

    if (sendto(udp_socket, request, REQUEST_LEN, 0, (struct sockaddr*) &ds_addr, ds_len_addr) == -1)
    {
        perror("\nERR     Errore durante l'invio dell' ACK");
    }
    
    printf("avvenuto con successo\n");
}



void ricevi_dal_DS(char this_buf[])
{
    char buf[REQUEST_LEN];
    int len_addr = sizeof(ds_addr);
    printf("LOG     Attendo %s... ", this_buf);

    if (recvfrom(udp_socket, buf, REQUEST_LEN, 0, (struct sockaddr*) &ds_addr, (socklen_t *__restrict)&len_addr) == -1)
    {
        perror("\nERR     Errore durante la ricezione dell' ACK");
        return;
    }
    
    if (strcmp(this_buf, buf) == 0)
        printf("ricevuto con successo\n");
}


/* Ricezione IP e porta dei neighbor*/
void ricezione_vicini_DS()
{
    //uint32_t ip;
    in_port_t porta;

    //printf("LOG     Ricezione IP e porta del primo neighbor...\n");
/*
    if (recvfrom(udp_socket, (void*)&ip, sizeof(uint32_t), 0, (struct sockaddr*) &ds_addr, (socklen_t *__restrict)&ds_len_addr) == -1)
    {
        perror("ERR     Errore durante la ricezione del primo IP");
    }
*/
    if (recvfrom(udp_socket, (void*)&porta, sizeof(in_port_t), 0, (struct sockaddr*) &ds_addr, (socklen_t *__restrict)&ds_len_addr) == -1)
    {
        perror("ERR     Errore durante la ricezione della prima porta");
    }

    printf("LOG     Ricezione porta del neighbor sinistro effettuata con successo: <%d>\n", porta);

    memset(&vicino.addr_sinistro, 0, sizeof(vicino.addr_sinistro));
    vicino.addr_sinistro.sin_family = AF_INET;
    vicino.addr_sinistro.sin_port = htons(porta);
    vicino.addr_sinistro.sin_addr.s_addr = INADDR_ANY;

    // Bisogna vedere se è il primo peer che si registra alla rete
    if (porta == 0)
        first = 1;
    else   
        first = 0;
    

    //printf("LOG     Ricezione IP e porta del secondo neighbor...\n");
/*
    if (recvfrom(udp_socket, (void*)&ip, sizeof(uint32_t), 0, (struct sockaddr*) &ds_addr, (socklen_t *__restrict)&ds_len_addr) == -1)
    {
        perror("ERR     Errore durante la ricezione del secondo IP");
    }
*/
    if (recvfrom(udp_socket, (void*)&porta, sizeof(in_port_t), 0, (struct sockaddr*) &ds_addr, (socklen_t *__restrict)&ds_len_addr) == -1)
    {
        perror("ERR     Errore durante la ricezione della seconda porta");
    }

    printf("LOG     Ricezione porta del neighbor destro effettuata con successo: <%d>\n", porta);

    memset(&vicino.addr_destro, 0, sizeof(vicino.addr_destro));
    vicino.addr_destro.sin_family = AF_INET;
    vicino.addr_destro.sin_port = htons(porta);
    vicino.addr_destro.sin_addr.s_addr = INADDR_ANY;
    
    // Bisogna vedere se è il primo peer che si registra alla rete
    if (porta == 0)
        first = 1;
    else   
        first = 0;
}


// --------------------------------------------------------------------------------------------------------------------------------------//
// ---------------------------------------------------------------- MAIN ----------------------------------------------------------------//
// --------------------------------------------------------------------------------------------------------------------------------------//
void comandi_disponibili()
{
    printf("I comandi disponibili sono:\n");
    printf("    1) help\n");
    printf("    2) start <DS_addr> <DS_port>\n");
    printf("    3) add <type> <quantity>\n");
    printf("    4) get <aggr> <type> <period>\n");
    printf("    5) stop\n");
    printf("Digitare un comando per iniziare.\n");
}

void comandi_disponibili_con_spiegazione()
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
    // ( VARIABILI MAIN
        fd_set master;
        fd_set read_fds;
        int fd_max;

        time_t rawtime;
        struct tm* timeinfo;

        char buf[BUF_LEN];   // Usato per memorizzare i comandi da tastiera
        char comando[BUF_LEN];  // comando e parametro sono usati per differenziare i comandi dai parametri dei comandi da tastiera
        int parametro;
        char tipo;
        int totale;
        char periodo[BUF_LEN];
        char tipo_aggr[BUF_LEN];
        int data[3];
        int data_ini[3];
        int data_fin[3];
        char asterisco1, asterisco2;
        int len_addr;
        int buf_len;
        int totale_vicino;
        int porta_richiedente;

        int sd_wait_for_flt;
    // )

    my_port = atoi(argv[1]);

    // Ricesco il nome del file degli aggregati di questo peer.
    strcpy(nome_miei_aggr, "");
    sprintf(nome_miei_aggr, "miei_aggr_%d.txt", my_port);
    crea_file(nome_miei_aggr);

    strcpy(nome_aggr, "");
    sprintf(nome_aggr, "aggr_%d.txt", my_port);
    crea_file(nome_aggr);

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    strcpy(nome_reg, "");
    sprintf(nome_reg, "reg_%d-%d-%d_%d.txt", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, ntohs(my_addr.sin_port));

    // Creazione indirizzo del socket
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(my_port);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    // Creo il due socket e li aggancio all'indirizzo
    creazione_socket_TCP(); 
    creazione_socket_UDP();

    // ( GESTIONE FD: Reset dei descrittori e inizializzazione
        FD_ZERO(&master); 
        FD_ZERO(&read_fds);

        FD_SET(0, &master);
        FD_SET(udp_socket, &master);
        FD_SET(ascolto_sd, &master);

        fd_max = (udp_socket > ascolto_sd) ? udp_socket : ascolto_sd;
    // )

    printf("\n");
    comandi_disponibili();
    printf("\n");

    while (1)
    {
        //printf("\n");
        // Uso la select() per differenziare e trattare diversamente le varie richieste fatte al DS 
        struct timeval intervallo = {0, 500};
        read_fds = master;
        select(fd_max + 1, &read_fds, NULL, NULL, &intervallo);

        // Gestione richieste TCP da parte degli altri peer
        if (FD_ISSET(ascolto_sd, &read_fds))
        {    
            printf("\n");
            printf("LOG     Nuova richiesta TCP (socket = %d)\n", ascolto_sd);
            strcpy(buf, "");

            len_addr = sizeof(mittente);
            trasporto_sd = accept(ascolto_sd, (struct sockaddr *)&mittente, (socklen_t *)&len_addr);
            //printf("DBG     accept(): %d\n", trasporto_sd);

            // Dimensione
            if (recv(trasporto_sd, (void *)&buf_len, sizeof(int), 0) == -1)
            {
                perror("ERR     Errore nella recv() di una richiesta");
                exit(-1);
            }
            
            // Messaggio
            if (recv(trasporto_sd, (void *)buf, buf_len, 0) == -1)
            {
                perror("ERR     Errore nella recv() di una richiesta");
                continue;
            }

            //printf("DBG     buf RICEVUTO: <%s> | <%d>\n", buf, buf_len);

            sscanf(buf, "%s", comando); 
            printf("LOG     Ho ricevuto la richiesta <%s> da parte del peer %d\n", comando, ntohs(mittente.sin_port));
            
            // ( Richesta da parte del vicino
            if (strcmp(comando, "NGB_REQ") == 0) 
            {
                sscanf(buf, "%s %d:%d:%d %c", comando, &data[D], &data[M], &data[Y], &tipo);
                cerca_totale(nome_aggr, data, tipo, &totale, 1);

                if (send(trasporto_sd, (void*) &totale, sizeof(int), 0) == -1)
                {
                    perror("ERR     Dato richiesto non inviato correttamente");
                    continue;
                }

                printf("LOG     Ho inviato il totale di %d %s al peer %d", totale, (tipo == 'T')? "tamponi fatti":"casi positivi", ntohs(peer_addr.sin_port));
                close(trasporto_sd);
            }
            // )

            // ( Richiesta di flooting
            if (strcmp(comando, "FLT_REQ") == 0)
            {
                sscanf(buf, "%s %d:%d:%d %c %d %d", comando, &data[D], &data[M], &data[Y], &tipo, &totale_vicino, &porta_richiedente);
                //printf("DBG     FLOOT_REQUEST, buf: <%s>\n", buf);
                //printf("DBG     Cerco la per la data passata il mio totale\n");

                // Cerco totale nel file degli aggregati della rete
                cerca_totale(nome_aggr, data, tipo, &totale, 1);
                
                //printf("DBG     Il mio totale è %d\n", totale);
                //printf("DBG     porta_richiedente: %d\n", porta_richiedente);

                if (totale == 0xFFFFFFFF) // Se non ho il dato richiesto, ropago la richiesta
                {
                    //printf("LOG     Devo propagare la richiesta di flooting\n");

                    // Vuol dire che il peer non ha alcun dato relativo al totale della rete, 
                    // quindi deve vedere se ha un dato relativo al suo registro chiuso, 
                    // propagarlo aggiungendolo al totale che gli ha inviato il vicino sinistro
                    cerca_totale(nome_miei_aggr, data, tipo, &totale, 0);
                    totale_vicino += totale;
                    //printf("DBG     Il totale che invio è %d\n", totale_vicino);

                    // Faccio la richiesta di flooting
                    if (porta_richiedente == ntohs(vicino.addr_destro.sin_port)) // Se sono sul peer precedente a quello che ha iniziato il flooting allora sarà una risposta
                    {
                        // Invio il totale al peer che ha fatto la richiesta di flooting
                        if (send(sd_wait_for_flt, (void*) &totale_vicino, sizeof(int), 0) == -1)
                        {
                            perror("ERR     Dato richiesto non inviato correttamente");
                            continue;
                        }

                        printf("LOG     Ho inviato il totale di %d %s al peer %d", totale_vicino, (tipo == 'T')? "tamponi fatti":"casi positivi", porta_richiedente);
                        close(sd_wait_for_flt); 
                    }
                    else    
                    {
                        //printf("DBG     Prima della chiamata a flooting(): porta_richiedente = %d\n", porta_richiedente);
                        flooting("FLT_REQ", data, tipo, totale_vicino, porta_richiedente, 1); // Altrimenti è una richiesta
                    }
                } 
                else // Propago la risposta
                {
                    // Possiede il dato aggregato del totale della rete richiesto, 
                    // quindi invia una richiesta di risposta al flooting verso il vicino destro che la riceverà
                    // e la propagherà fino al raggiungimento del destinatario che ne ha fatto richiesta

                    // Stavolta è una risposta al repli
                    flooting("FLT_RPL", data, tipo, totale, porta_richiedente, 1);
                }

                close(trasporto_sd);
                printf("\n");
            }
            // )

            // ( Risposta al flooting
            if (strcmp(comando, "FLT_RPL") == 0)
            {
                sscanf(buf, "%s %d:%d:%d %c %d %d", comando, &data[D], &data[M], &data[Y], &tipo, &totale_vicino, &porta_richiedente);
                //printf("DBG     FLOOT_RELPY, buf: <%s>\n", buf);
                //printf("DBG     ntohs(vicino.addr_destro.sin_port): <%hd>\n", ntohs(vicino.addr_destro.sin_port));
                //printf("DBG     porta_richiedente: %hd\n", porta_richiedente);

                if (porta_richiedente == ntohs(vicino.addr_destro.sin_port)) // Sono l'ultimo peer da cui deve passare la richiesta
                {
                    if (verifica_presenza_della_data(nome_aggr, data[D], data[M], data[Y], tipo) == 0) // Non ho trovato la data
                    {
                        aggiungi_aggr(nome_aggr, data, tipo, totale_vicino);
                    }

                    // Invio sul socket attivato con la richiesta FLT_WAIT
                    if (send(sd_wait_for_flt, (void*) &totale_vicino, sizeof(int), 0) == -1)
                    {
                        perror("ERR     Dato richiesto non inviato correttamente");
                        continue;
                    }

                    printf("LOG     Ho inviato il totale di %d %s al peer %d", totale_vicino, (tipo == 'T')? "tamponi fatti":"casi positivi", porta_richiedente);
                    close(sd_wait_for_flt);
                }
                else // La risposta è in transito, ma ne approfitto per salvare il dato aggregato se non lo ho
                {
                    if (verifica_presenza_della_data(nome_aggr, data[D], data[M], data[Y], tipo) == 0) // Non ho trovato la data
                    {
                        aggiungi_aggr(nome_aggr, data, tipo, totale_vicino);
                    }

                    // Stavolta è una risposta al repli
                    flooting("FLT_RPL", data, tipo, totale_vicino, porta_richiedente, 1);
                }

                close(trasporto_sd);
            }
            // )

            // ( Richiesta di attesa del flooting che il peer richiedente fa al suo vicino sinistro per avvertirlo che la richiesta dovrà poi spedirla su quel socket
            if (strcmp(comando, "FLT_WAIT") == 0)
            {
                printf("LOG     Salvo il socket collegato al peer %d che ha inviato FLT_WAIT\n", ntohs(vicino.addr_destro.sin_port));
                sd_wait_for_flt = trasporto_sd;
            }
            // )

            printf("\n");
        }

        if (FD_ISSET(udp_socket, &read_fds)) // DA FARE
        {
            printf("\n");
            // gestione richieste esterne
            printf("LOG     Nuova richiesta UDP (socket = %d)\n", ascolto_sd);
            strcpy(buf, "");
            
            // Messaggio
            if (recvfrom(udp_socket, (void *)buf, REQUEST_LEN, 0, (struct sockaddr *)&ds_addr, (socklen_t *__restrict)&len_addr) == -1)
            {
                perror("ERR     Errore nella recvfrom() di una richiesta dal server");
                exit(-1);
            }

            //printf("DBG     buf RICEVUTO: <%s>\n", buf);

            sscanf(buf, "%s", comando); 
            printf("LOG     Ho ricevuto la richiesta <%s> da parte del peer %d\n", comando, ntohs(mittente.sin_port));

            // ( Richiesta di UPDATE dei vicini
            if (strcmp(comando, "UPD_REQ") == 0)
            {
                // Invio ACK
                invia_al_DS("UPD_ACK");

                ricezione_vicini_DS();
            }
            // )

            // ( Richiesta ESC
            if (strcmp(comando, "ESC_REQ") == 0)
            {
                // Invio ACK
                invia_al_DS("ESC_ACK");

                chiudi_registro(nome_reg);
                aggrega_registro(nome_aggr);
                exit(-1);
            }   
            // )

            printf("\n");        
        }

        if (FD_ISSET(0, &read_fds))
        {
            printf("\n");
            // gestione richieste da stdin
            printf("LOG     Accetto richiesta da stdin...\n");
            strcpy(buf,"");

            // Attendo input da tastiera
            if(fgets(buf, 1024, stdin) == NULL)
            {
                printf("ERR     Immetti un comando valido!\n");
                continue;
            }

            sscanf(buf, "%s", comando);

            if( (strlen(comando) == 0) || (strcmp(comando," ") == 0) )
            {
                printf("ERR     Comando non valido! Prova ad inserire uno dei seguenti:\n");
                comandi_disponibili();
                continue;
            }
            
            // ( HELP
            if (strcmp(comando, "vicini") == 0) 
            {
                printf("\nVinino sinistro: <%hd>\n", ntohs(vicino.addr_sinistro.sin_port));
                printf("Vinino destro: <%hd>\n\n", ntohs(vicino.addr_destro.sin_port));
            }
            // )
            
            // ( HELP
             else if (strcmp(comando, "help") == 0) 
            {
                comandi_disponibili_con_spiegazione();
            }
            // )

            // ( START DA FINIRE
            else if (strcmp(comando, "start") == 0) // REQ_STR
            {
                int portaDS;
                char ipDS[BUF_LEN];

                sscanf(buf, "%s %d %s", comando, &portaDS, ipDS);

                if (ds_addr.sin_port == htons(portaDS))
                {
                    printf("WRN     Attenzione, il peer ha già eseguito il comando <start>\n");
                    continue;
                }

                // Creazione indirizzo del socket
                memset(&ds_addr, 0, sizeof(ds_addr));
                ds_addr.sin_family = AF_INET;
                ds_addr.sin_port = htons(portaDS);
                ds_addr.sin_addr.s_addr = INADDR_ANY;
                // inet_pton(AF_INET, ipDS, &ds_addr.sin_addr);

                //printf("DBG     porta e ip del DS: <%d> | <%s>\n", portaDS, ipDS);
                // Bisogna far richiesta al DS di entrare nella rete
                invia_al_DS("REQ_STR");

                // Attendo ACK_STR
                ricevi_dal_DS("ACK_REQ");

                // Ricezione dati IP - porta
                ricezione_vicini_DS();                

                boot = 1;
            }
            // )

            // ( ADD
            else if (strcmp(comando, "add") == 0)
            {
                sscanf(buf, "%s %c %d", comando, &tipo, &parametro);
                //printf("DBG     tipo = %c\n", tipo);

                // Controllo tipo carattere
                if (tipo != 'N' && tipo != 'T')
                {
                    printf("ERR     Errore nel tipo, deve essere 'N' o 'T'\n");
                    continue;
                }

                time(&rawtime);
                timeinfo = localtime(&rawtime);

                // Creo il nome
                strcpy(nome_reg, "");
                sprintf(nome_reg, "reg_%d-%d-%d_%d.txt", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, ntohs(my_addr.sin_port));
                //printf("LOG     Nome del registro creato: %s\n", nome_reg);

                // Bisogna controllare l'ora e se il registro è già stato chiuso per eventualmente crearne uno nuovo
                if ((timeinfo->tm_hour >= 17 && timeinfo->tm_min != 0 && timeinfo->tm_sec != 0)) // Se siamo oltre le 18:
                {
                    printf("LOG     Sono passate le 18!\n");
                    if (registro_aperto(nome_reg)) // Se il registro è aperto, lo chiudo, aggrego i dati e ne creo uno nuovo
                    {
                        //printf("DBG     Il registro <%s> è aperto\n", nome_reg);
                        // Chiudo registro
                        chiudi_registro(nome_reg);

                        // Aggrego i dati di quel registro
                        aggrega_registro(nome_reg);

                        // Creo registro con la data successiva a quella odierna
                        data_successiva(timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 19, data);
                        strcpy(nome_reg, "");
                        sprintf(nome_reg, "reg_%d-%d-%d_%d.txt", data[0], data[1], data[2], ntohs(my_addr.sin_port));
                        crea_file(nome_reg);
                        apri_registro(nome_reg);
                    }
                    else // Il registro è già chiuso, un add precedente avrà creato il nuovo file, creo il nome
                    {
                        //printf("DBG     Il registro <%s> è già chiuso\n", nome_reg);
                        data_successiva(timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 19, data);
                        strcpy(nome_reg, "");
                        sprintf(nome_reg, "reg_%d-%d-%d_%d.txt", data[0], data[1], data[2], ntohs(my_addr.sin_port));
                    }
                }

                // Creo stringa da scrivere
                sprintf(buf, "%d:%d:%d %c %d", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, tipo, parametro);

                // Scrivo in append nel file
                scrivi_file_append(nome_reg, buf);
            }
            // )

            // ( GET
            else if (strcmp(comando, "get") == 0) // get <aggr> <type> <period>
            {
                sscanf(buf, "%s %s %c %s", comando, tipo_aggr, &tipo, periodo);
                //printf("LOG     Servo il comando: %s %s %c %s\n", comando, tipo_aggr, tipo, periodo);

                // Controllo tipo carattere
                if (tipo != 'T' && tipo != 'N')
                {
                    printf("ERR     Errore nel tipo, deve essere 'N' per nuovi caso o 'T' per nuovi tamponi. Riprovare\n");
                    continue;
                }

                // Controllo tipo carattere
                if (strcmp(tipo_aggr, "totale") != 0 && strcmp(tipo_aggr, "variazione"))
                {
                    printf("ERR     Errore nel tipo di aggregato, deve essere 'totale' o 'variazione'. Riprovare\n");
                    continue;
                }

                
                // Cercare di aggregare da aggr.txt e salvare in cronologia_get.txt
                if (strcmp(periodo, "*-*") == 0)
                {
                    //printf("LOG     Periodo richiesto: *-*\n");
                    asterisco1 = asterisco2 = '*';
                    goto verifica;
                }

                sscanf(periodo, "%c-%d:%d:%d", &asterisco1, &data_fin[D], &data_fin[M], &data_fin[Y]);
                if (asterisco1 == '*')
                {
                    //printf("DBG     ASTERISCO 1\n");
                    printf("LOG     Controllo la data di fine periodo... ");
                    if (data_valida(data_fin[D], data_fin[M], data_fin[Y]) == 0)
                    {
                        continue;
                    }
                    asterisco2 = 'X';
                    goto verifica;
                }

                sscanf(periodo, "%d:%d:%d-%c", &data_ini[D], &data_ini[M], &data_ini[Y], &asterisco2);
                if (asterisco2 == '*')
                {
                    //printf("DBG     ASTERISCO 2\n");
                    printf("LOG     Controllo la data di inizio periodo... ");
                    if (data_valida(data_ini[D], data_ini[M], data_ini[Y]) == 0)
                    {
                        continue;
                    }
                    asterisco1 = 'X';
                    goto verifica;
                }

                sscanf(periodo, "%d:%d:%d-%d:%d:%d", &data_ini[D], &data_ini[M], &data_ini[Y], &data_fin[D], &data_fin[M], &data_fin[Y]);
                printf("LOG     Controllo la data di inizio periodo... ");
                if (data_valida(data_ini[D], data_ini[M], data_ini[Y]) == 0)
                {
                    continue;
                }

                printf("LOG     Controllo la data di fine periodo... ");
                if (data_valida(data_fin[D], data_fin[M], data_fin[Y]) == 0)
                {
                    continue;
                }

verifica:
                // Mi serve la data precedente a quella iniviale per calcolare la variazione dell'inizio del periodo
                if (asterisco1 != '*' && strcmp(tipo_aggr, "variazione") == 0)
                {
                    data_precedente(data_ini[D], data_ini[M], data_ini[Y], data_ini);
                }

                if (verifica_presenza_dati(data_ini, data_fin, asterisco1, asterisco2, tipo) == -1)
                {
                    //printf("ERR     Errore durante la ricerca negli aggregati giornalieri di questo peer, riprovare\n");
                    continue;
                }

                stampa_richiesta(nome_aggr, data_ini, data_fin, asterisco1, asterisco2, tipo, tipo_aggr);
                
            }
            //) 

            // ( STP
            else if (strcmp(comando, "stop") == 0) // DA FARE
            {
                chiudi_registro(nome_reg);
                aggrega_registro(nome_reg);
                invia_al_DS("STP_REQ");
                ricevi_dal_DS("STP_ACK");

                exit(-1);
            }
            // ) 

            else
            {
                printf("ERR     Comando non valido! Prova ad inserire uno dei seguenti:\n");
                comandi_disponibili();
            }

            printf("\n");       
        }

    }

    close(ascolto_sd);
}
