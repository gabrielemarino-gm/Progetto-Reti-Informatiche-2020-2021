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
    struct sockaddr_in addr_sinistro;
    struct sockaddr_in addr_destro;
} vicino;

// UDP socket per comunicare col DS
in_port_t DS_port = -1;
struct sockaddr_in ds_addr;
int ds_len_addr = sizeof(ds_addr);

int first = 1; // Se sono solo non posso contattare altri peer

// Variabili per i nomi dei file
char nome_reg[BUF_LEN];
char nome_miei_aggr[BUF_LEN];
char nome_aggr[BUF_LEN];
char nome_crono[BUF_LEN];

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

    memset(&vicino.addr_sinistro, 0, sizeof(vicino.addr_sinistro));
    vicino.addr_sinistro.sin_family = AF_INET;
    vicino.addr_sinistro.sin_addr.s_addr = ip;
    vicino.addr_sinistro.sin_port = ntohl(porta);

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

    memset(&vicino.addr_destro, 0, sizeof(vicino.addr_destro));
    vicino.addr_destro.sin_family = AF_INET;
    vicino.addr_destro.sin_addr.s_addr = ip;
    vicino.addr_destro.sin_port = ntohl(porta);
}

// --------------------------------------------------------------------------------------------------------------------------------------//
// ---------------------------------------------------------------- MAIN ----------------------------------------------------------------//
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
    // ( VARIABILI MAIN
        in_port_t my_port = atoi(argv[1]);

        fd_set master;
        fd_set read_fds;
        int fd_max;

        char cmd[REQUEST_LEN];  // Per ricevere i comadi dai peera: REQ - STP
        char buffer[BUF_LEN];   // Usato per memorizzare i comandi da tastiera
        char comando[BUF_LEN];  // comando e parametro sono usati per differenziare i comandi dai parametri dei comandi da tastiera
        int parametro;
        char tipo[2];
        char periodo[21];
        char tipo_aggr[10];

        time_t rawtime;
        struct tm* timeinfo;
    // )

    // Ricesco il nome del file degli aggregati di questo peer.
    strcpy(nome_miei_aggr, "");
    sprintf(nome_miei_aggr, "miei_aggr_%d.txt", my_port);

    strcpy(nome_aggr, "");
    sprintf(nome_aggr, "aggr_%d.txt", my_port);

    strcpy(nome_crono, "");
    sprintf(nome_crono, "cronologia_get_%d.txt", my_port);

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

            // ( START DA FINIRE
            else if (strcmp(comando, "start") == 0) // REQ_STR
            {
                // Bisogna far richiesta al DS di entrare nella rete
                invio_richiesta_start();

                // Attendo ACK_STR
                attendo_ACK(ds_addr, "ACK_REQ");

                // Ricezione dati IP - porta
                ricezione_vicini();

                // Bisogna vedere se siamo i primi
                if (vicino.addr_sinistro.sin_addr.s_addr == 0 && vicino.addr_destro.sin_addr.s_addr == 0)
                {
                    first = 1; // Non posso comunicare con gli altri peer, perchè non ce ne sono
                }
            }
            // )

            // ( ADD
            if (strcmp(comando, "add") == 0)
            {
                sscanf(buffer, "%s %c %d", comando, &tipo, &parametro);
                printf("DBG     tipo = %c\n", tipo);

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
                printf("LOG     Nome del registro creato: %s\n", nome_reg);

                // Bisogna controllare l'ora e se il registro è già stato chiuso per eventualmente crearne uno nuovo
                if ((timeinfo->tm_hour >= 9 && timeinfo->tm_min != 0 && timeinfo->tm_sec != 0)) // Se siamo oltre le 18:
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
                sprintf(buffer, "%d:%d:%d %c %d", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, tipo, parametro);

                // Scrivo in append nel file
                scrivi_file_append(nome_reg, buffer);
            }
            // )

            // ( GET
            else if (strcmp(comando, "get") == 0) // get <aggr> <type> <period>
            {
                sscanf(buffer, "%s %s %s %s", comando, tipo_aggr, tipo, periodo);
                
                //Se peer non connesso non faccio nulla
                if(DS_port == -1)
                {
                    printf("ERR     Peer non connesso, ptima fare start\n");
                    continue;
                }

                // Controllo tipo carattere
                if(strcmp(tipo, "N") != 0 || strcmp(tipo, "T") != 0)
                {
                    printf("ERR     Errore nel tipo, deve essere 'N' per nuovi casi o 'T' per nuovi tamponi\n");
                    continue;
                }
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
