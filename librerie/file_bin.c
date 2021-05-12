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

#include "file_txt.h"
#include "date.h"
#include "costanti.h"
#include "file_bin.h"

/* struct usata da stampo per scrivere e leggere dal file 'lista_peer.bin'*/
struct des_peer
{
    in_port_t porta;
    uint32_t IP;
    // struct sockaddr_in addr; // Ridondante
};


void crea_file_da_zero(char *nome)
{
    FILE *fd;
    fd = fopen(nome, "w");
    printf("LOG     Creato il file: <%s>\n", nome);
    fclose(fd);
}

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
