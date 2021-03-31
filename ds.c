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

char buffer[BUF_LEN];   // Usato per memorizzare i comandi da tastiera
char comando[BUF_LEN];  // comando e parametro sono usati per differenziare i comandi dai parametri dei comandi da tastiera
int parametro;

/* struct usata da stampo per scrivere e leggere dal file 'lista_peer.bin'*/
struct des_peer
{
    uint16_t porta;
    uint32_t IP;
};

/* Passato il nome del file, ritorna la sua dimensione*/
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

void attendo_ACK(int sd, struct sockaddr_in peer_addr)
{
    const char *ack = "ACK\0";
    const int ACK_len = 4;
    char buf[4];
    int ret;
    int len_peer_addr = sizeof(peer_addr);
    printf("LOG     Attendo ACK\n");

    do
    {
        ret = recvfrom(sd, buf, ACK_len, 0, (struct sockaddr*) &peer_addr, (socklen_t *__restrict)&len_peer_addr);
    } while (!strcmp(buf, ack) || ret > 0);

}

void invia_richiesta_UPD(int sd, struct sockaddr_in peer_addr)
{
    const char *upd = "UPD\0";
    const int UPD_len = 4;
    int ret;
    int len_peer_addr = sizeof(peer_addr);
    printf("LOG     Invio dell' UPD\n");

riprovaACK:
    ret = sendto(sd, upd, UPD_len, 0, (struct sockaddr*) &peer_addr, len_peer_addr);    
    if (ret < 0)
    {
        printf("ERR     Errore durante l'invio dell' UPD\n");
        goto riprovaACK;
    }
}

void invia_ESC(int sd, struct sockaddr_in peer_addr)
{
    const char *esc = "ESC\0";
    const int ESC_len = 4;
    int ret;
    int len_peer_addr = sizeof(peer_addr);
    printf("LOG     Invio di ESC\n");

riprovaESC:
    ret = sendto(sd, esc, ESC_len, 0, (struct sockaddr*) &peer_addr, len_peer_addr);    
    if (ret < 0)
    {
        printf("ERR     Errore durante l'invio di ESC\n");
        goto riprovaESC;
    }
}

/*  Cerco nel FILE 'lista_peer.bin' la posizione del cursore (l'indice) dove si trova il peer passato come parametro
    formale alla funzione. Complessità o(logn): 'Divide et impera'.
    Prima di usare la funzione, dato che è ricorsiva, bisogna passargli il file già aperto, la posizione iniziale e
    finale del cursore, più la porta target*/
int ricerca_binaria_file(FILE *fd, long inizio, long fine, uint32_t target)
{
    long c_indx;        // Posizione centrale del puntatore nel file  
    int len = sizeof(struct des_peer);
    struct des_peer dp;

    if (inizio > fine)
        return -1; // Errore
    else
    {
        c_indx = ((inizio + fine) / 2) / len; // Diviso len, per tener conto della grandezza della struct des_peer
        fseek(fd, c_indx-1, SEEK_SET);
        fread(&dp, sizeof(struct des_peer), 1, fd);

        if (dp.porta == target)
            return c_indx;
        else
        {
            if (dp.porta > target)
                return ricerca_binaria_file(fd, inizio, c_indx-len, target);
            else
                return ricerca_binaria_file(fd, c_indx+len, fine, target);
        }
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

/* Scrivo dentro il file 'lista_peer.bin' nella posizione (pos) del cursore. 
   Scrivo i parametri PORTA e IP passati*/
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

/* Crea un nuovo file binario dove copia tutti record eccetto quello da eliminare.
   Rinomina il nuovo file e elimina il vecchio*/
int rimozione_lista_peer(int pos)
{

    FILE *fd;
    FILE *fd_app;
    struct des_peer dp;
    int i;

    if ((fd = fopen("lista_peer.bin","r+")) == NULL)
    {
        printf("ERR     Non posso aprire il file 'lisra_peer.bin");
        return -1;
    }
    else 
    {
        fd_app = fopen("temporaneo.bin","r+");
        
        for (i=0; i<pos; i++) // Copio da 0 a pos-1
        {
            fseek(fd, sizeof(struct des_peer)*i, SEEK_SET); // Setto il cursore
            fseek(fd_app, sizeof(struct des_peer)*i, SEEK_SET); // Setto il cursore
            fread(&dp, sizeof(struct des_peer), 1, fd); // Leggo dal vecchio file
            fwrite(&dp, sizeof(struct des_peer), 1, fd_app); // Copio ne nuovo
        }

        for (i=pos+1; i<FILE_dim("prova.bin")/sizeof(struct des_peer); i++) // Copio da pos+1 fino alla fine
        {
            fseek(fd, sizeof(struct des_peer)*i, SEEK_SET); // Setto il cursore
            fseek(fd_app, sizeof(struct des_peer)*i, SEEK_SET); // Setto il cursore
            fread(&dp, sizeof(struct des_peer), 1, fd); // Leggo dal vecchio file
            fwrite(&dp, sizeof(struct des_peer), 1, fd_app); // Copio ne nuovo
        }

        fclose(fd);
        fclose(fd_app);
        
        // Rinomino il nuovo file e elimino il vecchio
        remove("lista_peer.bin");
        rename("temporaneo.bin", "lista_peer.bin");
        printf("LOG     Peer rimosso correttamente\n");
    }

    return 0;
}

uint32_t porta_precedente(int pos)
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

    return dp.porta;
}

uint32_t porta_successivo(int pos)
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

    return dp.porta;
}

// Invio al richiedente che fa boot, l'IP di un suo vicino
int invio_porta(uint16_t porta, int soc)
{
    int ret;
    struct sockaddr_in peer_addr;
    int len_peer_addr = sizeof(peer_addr);

    ret = sendto(soc, porta, sizeof(uint32_t), 0, (struct sockaddr*) &peer_addr, len_peer_addr);
    if (ret < sizeof(uint32_t)) // Errore recvfrom()
    {
        printf("ERR     Errore durante l'invio degli indirizzi IP dei vicini\n");
        return -1;
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

inline void comandi_disponibili_con_spiegazione()
{
    printf("I comandi disponibili sono:\n");
    printf("    1) help:            Mostra il significato dei comandi e cio' che fanno\n");
    printf("    2) showpeers:       Mostra l'elenco dei peer connessi alla rete tramite il loro numero di porta\n");
    printf("    3) showneighbor:    Mostra i neighbor di un peer specifico come parametro opzionale. Se non viene specificato alcun parametro, il comando mostra i neighbor di ogni peer\n");
    printf("    4) esc:             Termina il DS. La terminazione causa la terminazione di tutti i peer. \n");
}

// Converte un IP binario nella notazione decimale puntata e mette 4 cifre in un array di int
int *da_binario_a_decPun(uint32_t ip, int ris[4])
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

/* Stampo a video tutti i peer nella lista 'lista_peer.bin'*/
void stampa_lista_peer()
{
    struct des_peer dp;
    FILE * fd;
    long i;
    int str_ip[4];

    if ((fd = fopen("lista_peer.bin","r")) == NULL)
        printf("ERR     non posso aprire il file 'lista_peer.bin'");
    else 
    {
        printf("------ PEER DISPONIBILI ------\n");
        for(i=0; i<=file_dim("prova.bin")/sizeof(struct des_peer); i++)
        {
            fread(&dp, sizeof(struct des_peer), 1, fd);
            da_binario_a_decPun(dp.IP, str_ip);
            printf("peer : ");
            for (i=3; i>=0; i--)
            {
                if (i > 0)
                    printf("%d.", str_ip[i]);
                else
                    printf("%d", str_ip[i]);
            }
            printf(", posta = %d\n", dp.porta);
            fseek(fd, sizeof(struct des_peer)*i, SEEK_SET);
        }
        printf("------------------------------\n");
        fclose(fd);
    } 
}

/* Stampo a video i peer neighbor del peer passato come parametro*/
void stampa_vicini(int pos, uint16_t peer)
{
    struct des_peer dp;
    FILE * fd;

    if ((fd = fopen("lista_peer.bin","r")) == NULL)
        printf("ERR     non posso aprire il file 'lista_peer.bin'");
    else 
    {
        if (peer == 0) // In caso non si è specificato il parametro nel comando, lo vado a recuperare
        {
            fseek(fd, sizeof(struct des_peer)*pos, SEEK_SET);
            fread(&dp, sizeof(struct des_peer), 1, fd);
            peer = dp.porta;
        }

        // stampa precedente
        fseek(fd, sizeof(struct des_peer)*(pos-1), SEEK_SET);
        fread(&dp, sizeof(struct des_peer), 1, fd);
        printf("I vicini di %d sono: %d, ", peer, dp.porta);

        // stampa successivo
        fseek(fd, sizeof(struct des_peer)*(pos+1), SEEK_SET);
        fread(&dp, sizeof(struct des_peer), 1, fd);
        printf(" %d\n", dp.porta);

        fclose(fd);
    }
}

int main(int argc, char* argv[])
{
    // ( VARIABILI
    uint16_t portaDS = atoi(argv[1]);

    uint32_t IP_peer;
    uint16_t porta_peer, porta_prec, porta_succ;
    int ret, sd, len_peer_addr, indx;

    fd_set master;
    fd_set read_fds;
    int fd_max;

    struct sockaddr_in ds_addr, peer_addr;
    // )

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

    len_peer_addr = sizeof(peer_addr);
    
    // costro a video i comandi disponibili
    comandi_disponibili();
    
    while (1)
    {     
        // printf("prompt$");

        // Uso la select() per differenziare e trattare diversamente le varie richieste fatte al DS 
        read_fds = master;
        select(fd_max+1, &read_fds, NULL, NULL, NULL);

        if (FD_ISSET(sd, &read_fds)) 
        {
            // Ricevo la tipologia di comando dal peer:
            //      REQ: Rischiesta di boot;
            //      STP: Il peer ha terminato la sua esecuzione 
            
            ret = recvfrom(sd, (void *)(uint64_t)IP_peer, sizeof(uint32_t), 0, (struct sockaddr*) &peer_addr, (socklen_t *__restrict)&len_peer_addr);
            if (ret < sizeof(uint32_t)) // Errore recvfrom()
            {
                printf("ERR     Errore durante la ricezione della richiesta UDP\n");
                continue;
            }

            invia_ACK(sd, peer_addr);
            printf("LOG     ACK per la richiesta inviato con successo\n");

            if (!strcmp(buffer, "REQ")) // Richiesta da un peer che ha appena fatto boot
            {    
                // RISOLVERE CASO PARTICOLARE DEL PRIMO PEER CHE SI REGISTRA
                printf("LOG     Ricevuto messaggio di REQ\n");
                // La richiesta usa il protocollo binario. Ci sono due campi da ricevere IP e porta.
                // ricevo IP
                ret = recvfrom(sd, (void *)(uint64_t)IP_peer, sizeof(uint32_t), 0, (struct sockaddr*) &peer_addr, (socklen_t *__restrict)&len_peer_addr);
                if (ret < sizeof(uint32_t)) // Errore recvfrom()
                {
                    printf("ERR     Errore durante la ricezione della richiesta UDP\n");
                    continue;
                }
                
                IP_peer = ntohl(IP_peer);

                // Ricevo porta
                ret = recvfrom(sd, (void *)(uint64_t)porta_peer, sizeof(uint16_t), 0, (struct sockaddr*) &peer_addr, (socklen_t *__restrict)&len_peer_addr);
                if (ret < sizeof(uint16_t)) // Errore recvfrom()
                {
                    printf("ERR     Errore durante la ricezione della richiesta UDP\n");
                    continue;
                }

                porta_peer = ntohl(porta_peer);

                // Invio l'ACK
                invia_ACK(sd, peer_addr);
                printf("LOG     ACK per IP e porta inviato con successo\n");

                // Ricercare nel file lista_peer.bin l'idice dove inserire
                indx = ricerca_posto_lista_peer(porta_peer);

                // Inserire nel FILE
                inserimento_lista_peer(indx, porta_peer, IP_peer);

                // Inviare i vicini al richiedente
                printf("LOG     Cerco i peer precedente e successivo\n");
                porta_prec = porta_precedente(indx); // RISOLVERE L'ERRORE
                porta_succ = porta_successivo(indx); // RISOLVERE L'ERRORE

                printf("LOG     Invio lista neighbor\n");
                invio_porta(porta_prec, sd);         // RISOLVERE L'ERRORE
                invio_porta(porta_succ, sd);         // RISOLVERE L'ERRORE
                printf("LOG     Lista neighbor inviata con successo\n");
            }
            else if (!strcmp(buffer, "STP")) // Ho ricevuto STP 
            {
                FILE *fd;
                // Devo cercare i vicini e inviare una richiesta di modifica dei loro neighbor
                printf("LOG     Ricevuto messaggio di STP\n");

                // Ricevo porta
                ret = recvfrom(sd, (void *)(uint64_t)porta_peer, sizeof(uint16_t), 0, (struct sockaddr*) &peer_addr, (socklen_t *__restrict)&len_peer_addr);
                if (ret < sizeof(uint16_t)) // Errore recvfrom()
                {
                    printf("ERR     Errore durante la ricezione della richiesta UDP\n");
                    continue;
                }

                porta_peer = ntohl(porta_peer);

                // Invio l'ACK
                invia_ACK(sd, peer_addr);
                printf("LOG     ACK per la porta inviato con successo\n");

                fd = fopen("lista_peer.bin", "r");
                indx = ricerca_binaria_file(fd, SEEK_SET, SEEK_END, porta_peer);
                fclose(fd);

                // Invio ai peer che hanno perso il vicino, l'aggiornamento del peer
                invia_richiesta_UPD(sd, peer_addr);

                // Attendo l'ACK da parte del peer per poter inviare i nuovi vicini
                attendo_ACK(sd, peer_addr);

                // Ricevuto l'ACK rinvio i vicini del peer, entrambi.
                // Inviare i vicini al richiedente
                printf("LOG     Cerco i peer precedente e successivo\n");
                porta_prec = porta_precedente(indx); // RISOLVERE L'ERRORE
                porta_succ = porta_successivo(indx); // RISOLVERE L'ERRORE

                printf("LOG     Invio lista neighbor\n");
                invio_porta(porta_prec, sd);         // RISOLVERE L'ERRORE
                invio_porta(porta_succ, sd);         // RISOLVERE L'ERRORE
                printf("LOG     Lista neighbor inviata con successo\n");
                
                // Elimino il peer che fa fatto STP dal file 'lista_peer.bin'
                printf("LOG     Rimozione peer che ha fato richiesta STP\n");
                ret = rimozione_lista_peer(indx); // RISOLVERE L'ERRORE
            }
        }
        if (FD_ISSET(0, &read_fds)) // Richiesta dallo stdin: 0
        {
            // comandi_disponibili();
            
            // pulisco buffer
            strcpy(buffer,"");
            strcpy(comando,"");
            parametro = 0;

            // Attendo input da tastiera
            char* str = fgets(buffer, BUF_LEN, stdin);
            if(str == NULL)
            {
                printf("ERR     Immetti un comando valido!\n");
                // printf("prompt$ ");
                continue;
            }

            sscanf(buffer, "%s %d", comando, parametro);

            if( (strlen(comando) == 0) || (strcmp(comando," ") == 0) )
            {
                printf("ERR     Comando non valido! Prova ad inserire uno dei seguenti:\n");
                // printf("prompt$ ");
                continue;
            }

            // Controllo il comando inserito e eseguo la richiesta
            if (strcmp(comando, "help") == 0)
            {
                comandi_disponibili_con_spiegazione();
            }
            else if (strcmp(comando, "showpeers") == 0)
            {
                stampa_lista_peer();
            }
            else if (strcmp(comando, "showneighbor") == 0)
            {
                // mostra i neighbor di un peer specifico come parametro opzionale. 
                // Se non viene specificato alcun parametro, il comando mostra i neighbor di ogni peer
                if (parametro != NULL)
                {
                    FILE *fd = fopen("lista_peer.bin", "r");
                    ret = ricerca_binaria_file(fd, SEEK_SET, SEEK_END, parametro);
                    if (ret == -1)
                    {
                        printf("ERR     Hai inserito un peer non esistente. Riprovare.\n");
                        // printf("prompt$ ");
                        continue;
                    }

                    stampa_vicini(ret, parametro);
                    fclose(fd);              
                }
                else
                {
                    int i;
                    for(i=0; i<=file_dim("prova.bin")/sizeof(struct des_peer); i++)
                    {
                        stampa_vicini(i, 0);
                    }
                }

                // printf("prompt$ ");
            }
            else if (strcmp(comando, "esc") == 0)
            {
                // devo avvidare tutti i peer che la connessione si sta per chiudere
                invia_ESC(sd, peer_addr);
                printf("Buona giornata!\n");
                exit(-1);
            }
            else // errorre
            {
                printf("ERR     Comando non valido! Prova ad inserire uno dei seguenti:\n");
                comandi_disponibili();
                // printf("prompt$ ");
            }
        }
    }    

    close(sd);
    return 0;
}
