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

struct des_peer
{
    uint16_t porta;
    uint32_t IP;
};

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

        fclose(fd);
        return n;
    }
}

void invia_ACK(int sd, struct sockaddr_in peer_addr)
{
    const char *ack = "ACK\0";
    const int ACK_len = 4;
    int ret;
    int len_peer_addr = sizeof(peer_addr);
    printf("LOG     Invio dell' ACK\n");

riprovaACK:
    ret = sendto(sd, ack, ACK_len, 0, (struct sockaddr*) &peer_addr, len_peer_addr);    
    if (ret < 0)
    {
        printf("ERR     Errore durante l'invio dell' ACK\n");
        goto riprovaACK;
    }
}

/*  Cerco dentro il FILE la posizione dove inserire il nuovo peer.
    La scelta è stata fare un sistema circolare, ovvero sia che ogni peer ha due vicini
    uno precedente e l'altro successivo. Mettendo i peer i ordine di porta.

    Il formato del FILE è binario, e contieni gli des_peerenti della struttura des_peer */
int ricerca_posto_lista_peer(int porta)
{
    int i;
    FILE *fd;
    struct des_peer dp;

    printf("LOG     Ricerco nel FILE 'lista_peer.bin' la posizione dove inserire il nuovo peer\n");

    if ((fd = fopen("lista_peer.bin", "r+")) == NULL)
    {
        printf("ERR     Errore nell'apertura del FILE 'lista_peer.bin'\n");
        return -1;
    }
    else
    {
        for(i=0; i<FILE_dim("prova.bin")/sizeof(struct des_peer); i++)
        {
            fread(&dp, sizeof(struct des_peer), 1, fd);

            if (porta == dp.porta)
            {
                printf("ERR     Porta già assegnata a un peer");
                return -1; // Errore
            }

            if (porta < dp.porta)            
                fseek(fd, sizeof(struct des_peer)*i, SEEK_SET);
            
            if (porta > dp.porta)
            {
                i = i - 1;
                break;
            }
        }
        
        fclose(fd);
        return i;
    }
}

int inserimento_lista_peer(int pos, uint16_t porta_peer, uint32_t IP_peer)
{
    FILE *fd;
    struct des_peer dp;

    if ((fd = fopen("lista_peer.bin","r+")) == NULL)
    {
        printf("ERR     Non posso aprire il file 'lisra_peer.bin");
        return -1;
    }
    else 
    {
        fseek(fd, sizeof(struct des_peer)*pos, SEEK_SET);
        fwrite(&dp, sizeof(struct des_peer), 1, fd);
        fclose(fd);
    }

    return 0;
}

uint32_t IP_precedente(int pos)
{
    FILE *fd;
    struct des_peer dp;

    if((fd = fopen("lista_peer.bin", "r")) == NULL)
    {
        printf("ERR     Non posso aprire il file 'lista_peer.bin'\n");
        return -1;
    }
    else
    {
        pos--;
        fseek(fd, sizeof(struct des_peer)*pos, SEEK_SET);
        fread(&dp, sizeof(struct des_peer), 1, fd);
        fclose(fd);
    }

    return dp.IP;
}

uint32_t IP_successivo(int pos)
{
    FILE *fd;
    struct des_peer dp;

    if((fd = fopen("lista_peer.bin", "r")) == NULL)
    {
        printf("ERR     Non posso aprire il file 'lista_peer.bin'\n");
        return -1;
    }
    else
    {
        pos++;
        fseek(fd, sizeof(struct des_peer)*pos, SEEK_SET);
        fread(&dp, sizeof(struct des_peer), 1, fd);
        fclose(fd);
    }

    return dp.IP;
}


int invio_IP(uint32_t ip_addr, int soc)
{
    int ret;
    struct sockaddr_in peer_addr;
    int len_peer_addr = sizeof(peer_addr);

    ret = sendto(soc, (void *)(uint64_t)ip_addr, sizeof(uint32_t), 0, (struct sockaddr*) &peer_addr, len_peer_addr);
    if (ret < sizeof(uint32_t)) // Errore recvfrom()
    {
        printf("ERR     Errore durante l'invio degli indirizzi IP dei vicini\n");
        exit(-1);
    }
    return 0;
}


inline void comandi_disponibili()
{
    printf("I comandi disponibili sono:\n");
    printf("    1) help\n");
    printf("    2) showpeers\n");
    printf("    3) showneighbor\n");
    printf("    4) esc\n");
    printf("Digitare un comando per iniziare.\n");
}

int main(int argc, char* argv[])
{
    // ( VARIABILI
    uint16_t portaDS = atoi(argv[1]);

    char cmd[CMD_LEN]; // per comando da tastiera
    // char buf[1024];
    uint32_t IP_peer, IP_prec, IP_succ;
    uint16_t porta_peer;
    int ret, sd_boot, len_peer_addr, indx;

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
            // La richiesta usa il protocollo binario. Ci sono due campi da ricevere IP e porta.
            ret = recvfrom(sd_boot, (void *)(uint64_t)IP_peer, sizeof(uint32_t), 0, (struct sockaddr*) &peer_addr, (socklen_t *__restrict)&len_peer_addr);
            if (ret < sizeof(uint32_t)) // Errore recvfrom()
            {
                printf("ERR     Errore durante la ricezione della richiesta UDP\n");
                continue;
            }
            
            IP_peer = ntohl(IP_peer);

            ret = recvfrom(sd_boot, (void *)(uint64_t)porta_peer, sizeof(uint16_t), 0, (struct sockaddr*) &peer_addr, (socklen_t *__restrict)&len_peer_addr);
            if (ret < sizeof(uint16_t)) // Errore recvfrom()
            {
                printf("ERR     Errore durante la ricezione della richiesta UDP\n");
                continue;
            }

            porta_peer = ntohl(porta_peer);

            invia_ACK(sd_boot, peer_addr);
            printf("LOG     ACK inviato con successo\n");

            // Ricercare nel file lista_peer.bin l'idice dove inserire
            indx = ricerca_posto_lista_peer(porta_peer);

            // Inserire nel FILE
            inserimento_lista_peer(indx, porta_peer, IP_peer);

            // Inviare i vicini al richiedente
            IP_prec = IP_precedente(indx);
            IP_succ = IP_successivo(indx);

            invio_IP(IP_prec, sd_boot);
            invio_IP(IP_succ, sd_boot);

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
