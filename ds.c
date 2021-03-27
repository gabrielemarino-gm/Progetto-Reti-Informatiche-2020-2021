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


void invia_ACK(int sd, struct sockaddr_in peer_addr)
{
    const char *ack = "ACK\0";
    const int ACK_len = 4;
    int ret;
    int len_peer_addr = sizeof(peer_addr);
    printf("LOG     Invio dell' ACK\n");

riprova:
    ret = sendto(sd, ack, ACK_len, 0, (struct sockaddr*) &peer_addr, (socklen_t *__restrict)&len_peer_addr);    
    if (ret < 0)
    {
        printf("ERR     Errore durante l'invio dell' ACK\n");
        goto riprova;
    }
}

void ricerca_file_peer()
{
    
}

/*
void inserimento_peer()
{

}

void invio_lista_vicini()
{

}
*/

inline void comandi_disponibili()
{
    printf("I comandi disponibili sono:\n");
    printf("    help\n");
    printf("    showpeers\n");
    printf("    showneighbor\n");
    printf("    esc\n");
    printf("Digitare un comando per iniziare.\n");
}

int main(int argc, char* argv[])
{
    // ( VARIABILI
    uint16_t portaDS = atoi(argv[1]);

    char cmd[CMD_LEN]; // per comando da tastiera
    char buf[1024];
    int ret, sd_boot, len_peer_addr;

    fd_set master;
    fd_set read_fds;
    int fd_max;

    struct sockaddr_in ds_addr, peer_addr;
    // )

    // Creazione del socket
    sd_boot = socket(AF_INET, SOCK_DGRAM, 0); // UDP

    // Creazione indirizzo del socket
    memset(&ds_addr, 0, sizeof(ds_addr));
    ds_addr.sin_family = AF_INET;
    ds_addr.sin_port = htons(portaDS);
    ds_addr.sin_addr.s_addr = INADDR_ANY;

    // Aggancio
    printf("LOG     Aggancio al socket: bind()\n");
    ret = bind(sd_boot, (struct sockaddr*) &ds_addr, sizeof(ds_addr));
    if (ret < 0) 
    {
        perror("Bind non riuscita\n");
        exit(-1);
    }
    
    // ( GESTIONE FD: Reset dei descrittori e inizializzazione
    FD_ZERO(&master); 
    FD_ZERO(&read_fds);

    FD_SET(sd_boot, &master);

    fd_max = sd_boot;
    // )

    len_peer_addr = sizeof(peer_addr);
    
    while (1)
    {     
        read_fds = master;
        select(fd_max+1, &read_fds, NULL, NULL, NULL);

        if (FD_ISSET(sd_boot, &read_fds)) // Richiesta da un peer che ha appena fatto boot (UDP)
        {
            ret = recvfrom(sd_boot, buf, REQUEST_LEN, 0, (struct sockaddr*) &peer_addr, (socklen_t *__restrict)&len_peer_addr);
            if (ret < 0) // Errore recvfrom()
            {
                printf("ERR     Errore durante la ricezione della richiesta UDP\n");
                exit(-1);
            }

            invio_ACK(sd_boot, peer_addr);
            printf("LOG     ACK inviato con successo\n");

            // Ricercare nel file.

            // Inserire nel file
            // Inviare i vicini al richiedente

        }
        if (FD_ISSET(0, &read_fds)) // Richiesta dallo stdin: 0
        {
            printf("Salve!\n");
            comandi_disponibili();

//comando:
            char *str = fgets(cmd, CMD_LEN, stdin);
            if (str == NULL)
            {
                printf("ERR     Errore durante la lettura del comando, riprovare\n");
            }
            
            printf("Stringa inserita: %s\n", cmd);
            printf("strcmp(cmd, help): %d\n", strcmp(cmd, "help"));
            printf("\n");

            if (strcmp(cmd, "help") == 0)
            {
                printf("Comando help\n");
//                goto comando;
            }
            else if (strcmp(cmd, "showpeers") == 0)
            {
                printf("Comando showpeers\n");
//                goto comando;
            }
            else if (strcmp(cmd, "showneighbor") == 0)
            {
                printf("Comando showneighbor\n");
//                goto comando;
            }
            else if (strcmp(cmd, "esc") == 0)
            {
                printf("Buona giornata!\n");
                exit(-1);
            }
            else // errorre
            {
                printf("Comando non valido. riprovare\n");
                comandi_disponibili();
//                goto comando;
            }
        }
    }    

    close(sd_boot);
    return 0;
}