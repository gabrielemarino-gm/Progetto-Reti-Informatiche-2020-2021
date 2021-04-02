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
#define REQUEST_LEN     4    // Lunghezza di "REQ\0"
#define BUF_LEN         1024

char cmd[REQUEST_LEN];  // Per ricevere i comadi dai peera: REQ - STP
char buffer[BUF_LEN];   // Usato per memorizzare i comandi da tastiera
char comando[BUF_LEN];  // comando e parametro sono usati per differenziare i comandi dai parametri dei comandi da tastiera
int parametro;

// ( STRUTTURE DATI
    // TCP socket per comunicare con gli altri peer
    int tcp_socket;

    struct neighbor
    {
        struct sockaddr_in prec_addr;
        struct sockaddr_in succ_addr;
    } vicini;

    // UDP socket per comunicare col DS
    in_port_t DS_port;
    struct sockaddr_in ds_addr;
    int ds_socket;
// )

/* Passata una struttura sockaddr_in, inizializza i suoi campi */
void inizializza_sockaddr(struct sockaddr_in address, in_port_t port)
{
    // Creazione indirizzo del socket
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;
}

/* Crea il socket per la connessione TCP, in modo da assocciarlo all'indirizzo e alla porta passati.
   Ritorna -1 in caso di errore */
int creazione_socket_TCP_invio(struct sockaddr_in address, int port) 
{
	// Creazione del socket TCP
	if((tcp_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
    {
		perror("ERR     Errore nella creazione del socket TCP, ripovare: ");
		return -1;
	}

	// Connessione al peer
	if(connect(tcp_socket, (struct sockaddr*)&address, sizeof(address)) == -1) 
    {
		perror("ERR     Errore nella connect(), riprovare :");
		return -1;
	}
	  
	printf("LOG     Connessione al peer effettuata con successo\n"); 
    return 0;
}

/* Crea il socket per la connessione TCP, in modo da assocciarlo all'indirizzo e alla porta passati.
   Ritorna -1 in caso di errore */
int creazione_socket_TCP_ascolto(struct sockaddr_in address, int port) 
{
    printf("LOG     Creazione socket TCP in ascolto...\n"); 

    // Creazione del socket TCP
	if((tcp_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
    {
		perror("ERR     Errore nella creazione del socket TCP, ripovare: ");
		return -1;
	}

    if(bind(tcp_socket, (struct sockaddr*)&address, sizeof(address)) == -1) 
    {
		perror("ERR     Errore in fase di bind() TCP in ascolto, ripovare: ");
		return -1;
	}

	// Connessione al server
	if(listen(tcp_socket, 10) == -1) 
    {
		perror("ERR     Errore nella listen(), riprovare :");
		return -1;
	}
	  
	printf("LOG     Connessione al peer effettuata con successo\n"); 
    return 0;
}

/* Crea il socket per la connessione UDP, in modo da assocciarlo all'indirizzo e alla porta passati.
   Ritorna -1 in caso di errore */
int creazione_socket_UDP(struct sockaddr_in address, int port) 
{
    // Creazione del socket UDP
	if((tcp_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
    {
		perror("ERR     Errore nella creazione del socket UDP, ripovare: ");
		return -1;
	}
	  
	printf("LOG     Connessione al server %s (porta %d) effettuata con successo\n", address, port);
    return 0;
}

inline void comandi_disponibili()
{
    printf("I comandi disponibili sono:\n");
    printf("    help\n");
    printf("    start <DS_addr> <DS_port>\n");
    printf("    add <type> <quantity>\n");
    printf("    get <aggr> <type> <period>\n");
    printf("    stop\n");
    printf("Digitare un comando per iniziare.\n");
}

int main(int argc, char* argv[])
{
    // ( VARIABILI
        in_port_t portaDS = atoi(argv[1]);

        int ret, sd;
        struct sockaddr_in my_addr;

        fd_set master;
        fd_set read_fds;
        int fd_max;
    // )

    // Aggancio
    printf("LOG     Aggancio al socket: bind()\n");
    ret = bind(sd, (struct sockaddr*) &my_addr, sizeof(my_addr));
    if (ret < 0) 
    {
        perror("ERR     Bind non riuscita\n");
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

    comandi_disponibili();

    while (1)
    {
        // Uso la select() per differenziare e trattare diversamente le varie richieste fatte al DS 
        read_fds = master;
        select(fd_max+1, &read_fds, NULL, NULL, NULL);

        if (FD_ISSET(sd, &read_fds))
        {
            // gestione richieste esterne
            
        }
    }
    
}
