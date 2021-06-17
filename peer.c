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
#define BUF_LEN 1024

// Lunghezza di "REQ_STR\0"
#define REQUEST_LEN 8

// Lunghezza massima di una data DD:MM:YYYY
#define LEN_DATA 12

// Indici per le date: Day, Month e Year
#define D 0
#define M 1
#define Y 2

// Timer
#define POLLING_TIME 5

// lunghezza comando da tastiera
#define CMD_LEN 12

// Lunghezza di "REQ_STR\0"
#define REQUEST_LEN 8

// -------------------------------------------------------------------------------------------------------------------------------------- //
// -------------------------------------------------------------- VARIABILI ------------------------------------------------------------- //
// -------------------------------------------------------------------------------------------------------------------------------------- //

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

int alone = 1; // Se sono solo non posso contattare altri peer
int boot = 0;  // Se non ho fatto la richiesta di boot non posso contattare gli altri

// Variabili per i nomi dei file
char nome_reg[BUF_LEN];
char nome_miei_aggr[BUF_LEN];
char nome_aggr[BUF_LEN];
char nome_file_ultimo_reg[BUF_LEN];

// -------------------------------------------------------------------------------------------------------------------------------------- //
// ---------------------------------------------------------------- DATE ---------------------------------------------------------------- //
// -------------------------------------------------------------------------------------------------------------------------------------- //

/* dato giorno, mese e anno, calcola la data del giorno successivo*/
void data_successiva(int d, int m, int y, int ris[])
{
    if (y % 4 == 0) // Anno bisestile
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
void data_precedente(int d, int m, int y, int ris[3])
{
    if (y % 4 == 0) // Anno bisestile
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
        //printf("DBG     %d:%d:%d < %d:%d:%d\n", d1, m1, y1, d2, m2, y2);
        return 1;
    }
    else if (m1 < m2 && y1 == y2)
    {
        //printf("DBG     %d:%d:%d < %d:%d:%d\n", d1, m1, y1, d2, m2, y2);
        return 1;
    }
    else if (y1 < y2)
    {
        //printf("DBG     %d:%d:%d < %d:%d:%d\n", d1, m1, y1, d2, m2, y2);
        return 1;
    }

    return 0;
}

/* Ritorna 1 se la data d:m:y è valida, 0 altrimenti*/
int data_valida(int d, int m, int y)
{
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    // timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900
    if (y > timeinfo->tm_year + 1900)
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

    if (y % 4 == 0) // Anno bisestile
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
// -------------------------------------------------------------- FILE TXT -------------------------------------------------------------- //
// -------------------------------------------------------------------------------------------------------------------------------------- //

void crea_file(char *nome)
{
    FILE *fd;
    fd = fopen(nome, "a");
    printf("LOG     Creato il file: <%s>\n", nome);
    fclose(fd);
}

int FILE_dim(char *nome_FILE)
{
    int n;
    FILE *fd;
    fd = fopen(nome_FILE, "r");

    if (fd == NULL)
    {
        printf("ERR     FILE_dim(): non posso aprire il file %s\n", nome_FILE);
        return -1; // Errore
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

/* scrivo in append dentro il file di registro nome[] ciò che contiene stringa */
int scrivi_file_append(char nome[], char *stringa)
{
    FILE *fd;

    if ((fd = fopen(nome, "a+")) == NULL)
    {
        printf("ERR     scrivi_file_append(): non posso aprire il file %s\n", nome);
        return -1;
    }
    else
    {
        fseek(fd, 0, SEEK_END);
        fprintf(fd, "%s\n", stringa);
        fclose(fd);
    }

    return 0;
}

/* scrivo in append dentro il file di registro nome[] ciò che contiene stringa */
int sovrascrivi_file(char nome[], char *stringa)
{
    FILE *fd;

    if ((fd = fopen(nome, "w")) == NULL)
    {
        printf("ERR     sovrascrivi_file(): non posso aprire il file %s\n", nome);
        return -1;
    }
    else
    {
        fseek(fd, 0, SEEK_END);
        fprintf(fd, "%s", stringa);
        //printf("LOG     Scrivo %s nel file %s\n", stringa, nome);
        fclose(fd);
    }

    return 0;
}

/* Ritorna 1 se il registro è aperto
   Definisco un registro aperto se esiste il file con quella data */
int registro_aperto(char nome[])
{
    FILE *fd;

    // Apro in lettura così che se il file non esiste, fopen ritorna un errore
    if ((fd = fopen(nome, "r")) == NULL)
    {
        // Il registro non esiste, quindi è chiuso
        return 0;
    }

    fclose(fd);
    return 1;
}

/* Data una data singola, verifica se esiste un record con quella data nel file aggr.txt.
   Se non esiste, allora scrive i data[] la data mancante
   Ritorna:
            -1  : In caso di errore
             1  : Nel caso trova la data
             0  : Nel caso non trova la data*/
int verifica_presenza_della_data(char nome_file[], int this_d, int this_m, int this_y, char tipo_attuale)
{
    FILE *fd;
    int d, m, y;
    char tipo;
    int i = 0;
    char buf[BUF_LEN];

    //printf("DBG     verifica_presenza_della_data(): START\n");

    if ((fd = fopen(nome_file, "r")) == NULL)
    {
        printf("ERR     leggo_file(): non posso aprire il file '%s'\n", nome_file);
        return -1;
    }
    else
    {
        while (i < FILE_dim(nome_file))
        {
            fseek(fd, i, SEEK_SET);
            fgets(buf, BUF_LEN, fd);

            //printf("DBG     fgets(): %s", buf);
            i += strlen(buf);

            sscanf(buf, "%d:%d:%d %c", &d, &m, &y, &tipo);
            //printf("DBG     data trovata nel file aggr.txt: %d:%d:%d %c\n", d, m, y, tipo);
            //printf("DBG     &tipo = %c\n", tipo);

            if (d == this_d && m == this_m && y == this_y && tipo_attuale == tipo)
                return 1;
        }
    }

    return 0;
}

/* Data una data singola, verifica se esiste un record con quella data nel file aggr.txt.
   Se non esiste, allora scrive i data[] la data mancante
   Ritorna:
            -1  : In caso di errore
             1  : Nel caso trova la data
             0  : Nel caso non trova la data*/
int verifica_presenza_della_data_senza_tipo(char nome_file[], int this_d, int this_m, int this_y)
{
    FILE *fd;
    int d, m, y;
    char tipo;
    int i = 0;
    char buf[BUF_LEN];

    //printf("DBG     verifica_presenza_della_data(): START\n");

    if ((fd = fopen(nome_file, "r")) == NULL)
    {
        printf("ERR     leggo_file(): non posso aprire il file '%s'\n", nome_file);
        return -1;
    }
    else
    {
        while (i < FILE_dim(nome_file))
        {
            fseek(fd, i, SEEK_SET);
            fgets(buf, BUF_LEN, fd);

            //printf("DBG     fgets(): %s", buf);
            i += strlen(buf);

            sscanf(buf, "%d:%d:%d %c", &d, &m, &y, &tipo);
            //printf("DBG     data trovata nel file aggr.txt: %d:%d:%d %c\n", d, m, y, tipo);
            //printf("DBG     &tipo = %c\n", tipo);

            if (d == this_d && m == this_m && y == this_y)
                return 1;
        }
    }

    return 0;
}

/*  Funzione per aggregare i dati di un singolo registro.
    L'operazione elimina il file di registro passatogli */
int aggrega_registro(char nome_file[])
{
    char buf[BUF_LEN];
    char tipo;
    int totale;
    int d1, m1, y1;
    int porta;
    int tt = 0; // Posizione Totale Tamponi
    int tp = 1; // Posizione Totale Positivi
    FILE *fd;
    int aggregato[2]; // aggregato[] contiene i risultati sul totale di ogni tipo:
                      //    1) Totale tamponi
                      //    2) Totale positivi

    FILE *fd_temp;
    long i;
    int this_d, this_m, this_y;

    // Inizializiamo 0x7FFFFFFF così da capire come calcolare le variazioni
    aggregato[tt] = aggregato[tp] = 0;

    // Cerco la data del giorno richiesto
    sscanf(nome_file, "reg_%d-%d-%d_%d", &this_d, &this_m, &this_y, &porta);

    //printf("DBG     nome_file: %s\n", nome_file);

    // Aggrego tutte le entrate del registro
    if ((fd = fopen(nome_file, "r")) == NULL)
    {
        perror("ERR     aggrega_registro(): non posso aprire il file di registro");
        return -1;
    }
    else
    {
        i = 0;
        while (i < FILE_dim(nome_file)) //Registro
        {
            fseek(fd, i, SEEK_SET);
            fgets(buf, 50, fd);

            // printf("DBG     Ho letto: %s", buf);
            i += strlen(buf);

            sscanf(buf, "%d:%d:%d %c %d", &d1, &m1, &y1, &tipo, &totale);

            if (tipo == 'T')
            {
                aggregato[tt] += totale;
            }

            if (tipo == 'N')
            {
                aggregato[tp] += totale;
            }
        }
        fclose(fd);
    }

    printf("LOG     Aggr: <%d:%d:%d T %d | %d:%d:%d N %d>\n", d1, m1, y1, aggregato[tt], d1, m1, y1, aggregato[tp]);

    // Elimino il registro e metto i dati totali nel file "miei_aggr"
    remove(nome_file); // Chiudo il registro

    // Devo controllare se nel file <miei_aggr.txt> esiste già una data inerente ai totali che devo inserire.
    // Se si, allora aggiungo a quella l'aggregato trovato.
    sprintf(nome_file, "miei_aggr_%d.txt", porta);

    //printf("DBG     la data target è %d:%d:%d\n", this_d, this_m, this_y);

    if (verifica_presenza_della_data_senza_tipo(nome_file, d1, m1, y1) == 1)
    {
        // Devo aggiungere a quella data, il mio totale

        printf("DBG     verifica_presenza_della_data_senza_tipo() == 1\n");

        if ((fd = fopen(nome_file, "r")) == NULL)
        {
            perror("ERR     Non posso aprire il file <miei_aggr.txt>");
            return -1;
        }
        else
        {
            fd_temp = fopen("temporaneo.bin", "a+");

            i = 0;
            while (i < FILE_dim(nome_file))
            {
                fseek(fd, i, SEEK_SET);
                fgets(buf, 50, fd); // Leggo dal vecchio file
                i += strlen(buf);
                printf("DBG     Ho letto: %s", buf);
                sscanf(buf, "%d:%d:%d %c %d", &d1, &m1, &y1, &tipo, &totale);

                if (this_d == d1 && this_m == m1 && this_y == y1)
                {
                    if (tipo == 'T')
                    {
                        aggregato[tt] += totale;
                        sprintf(buf, "%d:%d:%d T %d\n", d1, m1, y1, aggregato[tt]);
                    }

                    if (tipo == 'N')
                    {
                        aggregato[tp] += totale;
                        sprintf(buf, "%d:%d:%d N %d\n", d1, m1, y1, aggregato[tp]);
                    }

                    fprintf(fd_temp, "%s", buf);
                    continue;
                }

                sscanf(buf, "%d:%d:%d %c %d", &d1, &m1, &y1, &tipo, &totale);
                fprintf(fd_temp, "%s", buf);
            }

            fclose(fd);
            fclose(fd_temp);

            // Rinomino il nuovo file e elimino il vecchio
            remove(nome_file);
            rename("temporaneo.bin", nome_file);
        }
    }
    else
    {
        sprintf(buf, "%d:%d:%d T %d\n%d:%d:%d N %d", d1, m1, y1, aggregato[tt], d1, m1, y1, aggregato[tp]);

        // Scrivo nel file
        printf("LOG     Scrivo <%d:%d:%d T %d | %d:%d:%d N %d> nel file <%s>\n", d1, m1, y1, aggregato[tt], d1, m1, y1, aggregato[tp], nome_file);
        scrivi_file_append(nome_file, buf);
    }

    return 0;
}

/* Aggiunge nel file degli aggregati il dato che ha richiesto nella rete.*/
int aggiungi_aggr(char nome_file[], int this_data[], char this_tipo, int this_tot)
{
    FILE *fd;
    FILE *fd_temp;
    char buf_app[BUF_LEN];
    char buf[BUF_LEN];
    char tipo;
    int d, m, y, tot;
    int i;
    //int data_succ[3];

    //printf("DBG     aggiungi_aggr(): START\n");
    // Creo la entry da aggiungere
    sprintf(buf, "%d:%d:%d %c %d", this_data[D], this_data[M], this_data[Y], this_tipo, this_tot);
    printf("LOG     Aggiungo il record <%d:%d:%d %c %d> nel file <%s>\n", this_data[D], this_data[M], this_data[Y], this_tipo, this_tot, nome_file);

    if ((fd = fopen(nome_file, "r+")) == NULL)
    {
        printf("ERR     Non posso aprire il file <%s>\n", nome_file);
        perror(" ");
        return -1;
    }
    else
    {
        fd_temp = fopen("temporaneo.txt", "a+");
        i = 0;

        while (i < FILE_dim(nome_file)) // Copio da 0 alla posizione prima a quella in cui devo scrivere
        {
            // Leggo dal vecchio file
            fseek(fd, i, SEEK_SET);
            fgets(buf_app, BUF_LEN, fd);
            //printf("DBG     fgets(): %s\n", buf_app);
            i += strlen(buf_app);

            sscanf(buf_app, "%d:%d:%d %c %d", &d, &m, &y, &tipo, &tot);
            //printf("DBG     Data presa %d:%d:%d / %ld\n", d, m, y, strlen(buf_app));

            // Scrivo nel nuovo file
            if (data1_minore_data2(d, m, y, this_data[D], this_data[M], this_data[Y]))
            {
                fprintf(fd_temp, "%s", buf_app);
                //printf("DBG     1) Scrivo %s nel file 'temporaneo.txt'\n", buf_app);
            }
            else
            {
                i -= strlen(buf_app); // Non l'ho ancora scritto e quindi devo riportare il cursore dietro per farlo dopo
                break;
            }
        }

        // Ora devo scrivere la nuova entry
        sprintf(buf, "%d:%d:%d %c %d", this_data[D], this_data[M], this_data[Y], this_tipo, this_tot);
        fprintf(fd_temp, "%s\n", buf);
        //printf("DBG     -) Scrivo <%s> nel file 'temporaneo.txt'", buf);

        while (i < FILE_dim(nome_file)) // Copio dalla posizione dopo quella in cui ho scritto fine alla fine
        {
            // Leggo dal vecchio file
            fseek(fd, i, SEEK_SET);
            fgets(buf_app, BUF_LEN, fd);
            //printf("DBG     fgets(): %s\n", buf_app);
            i += strlen(buf_app);
            sscanf(buf_app, "%d:%d:%d %c %d", &d, &m, &y, &tipo, &tot);
            /*data_successiva(this_d, this_m, this_y, data_succ);

            if (d == data_succ[D] && m == data_succ[M] && y == data_succ[Y] && tipo == this_tipo)
            {
                sprintf(buf_app, "%d:%d:%d %c %d\n", d, m, y, tipo, tot);
            }*/

            // Scrivo nel nuovo file
            fprintf(fd_temp, "%s", buf_app);
            //printf("DBG     2) Scrivo %s nel file 'temporaneo.txt'\n", buf_app);
        }

        fclose(fd);
        fclose(fd_temp);

        // Rinomino il nuovo file e elimino il vecchio
        remove(nome_file);
        rename("temporaneo.txt", nome_file);
        printf("LOG     File <%s> aggiornato correttamente\n", nome_file);
    }

    return 0;
}

void stampa_richiesta(char nome_file[], int data_ini[], int data_fin[], char asterisco1, char asterisco2, char tipo_attuale, char tipo_aggregato[])
{
    FILE *fd;
    int d, m, y;
    int tot = 0, tot_pre = 0, tot_aggr = 0;
    int variazione = 0;
    char tipo;
    int primo = 1;
    int stampa = 0;
    int i = 0;
    char buf[BUF_LEN];

    //printf("DBG     stampa_richiesta(%s): START\n", tipo_aggregato);
    //printf("DBG     strcmp(tipo_aggregato, 'variazione') = %d\n", strcmp(tipo_aggregato, "variazione"));

    if ((fd = fopen(nome_file, "r")) == NULL)
    {
        printf("ERR     leggo_file(): non posso aprire il file '%s'\n", nome_file);
        return;
    }
    else
    {
        if (strcmp(tipo_aggregato, "totale") == 0)
            printf("\n--------------------- Totale richiesto ---------------------\n");

        if (strcmp(tipo_aggregato, "variazione") == 0)
            printf("\n------------------- Variazioni richieste -------------------\n");

        while (i < FILE_dim(nome_file))
        {
            fseek(fd, i, SEEK_SET);
            fgets(buf, BUF_LEN, fd);

            //printf("DBG     fgets(): %s", buf);
            i += strlen(buf);

            sscanf(buf, "%d:%d:%d %c %d", &d, &m, &y, &tipo, &tot);

            if (data_ini[D] == d && data_ini[M] == m && data_ini[Y] == y)
                stampa = 1;

            // TOTALE
            if (strcmp(tipo_aggregato, "totale") == 0 && stampa == 1)
            {
                if (tipo == 'T' && tipo == tipo_attuale) // Devo semplicemete calcolare il totale
                {
                    tot_aggr += tot;
                }

                if (tipo == 'N' && tipo == tipo_attuale) // Devo calcolare volta per volta la variazione
                {
                    tot_aggr += tot;
                }
            }

            // VARIAZIONE
            if (strcmp(tipo_aggregato, "variazione") == 0 && stampa == 1)
            {
                if (tipo == 'T' && tipo == tipo_attuale) // Devo semplicemete calcolare il totale
                {
                    if (primo)
                    {
                        tot_pre = tot;
                        primo = 0;
                        continue;
                    }

                    variazione = tot - tot_pre;
                    tot_pre = tot;
                    printf("%d:%d:%d %d\n", d, m, y, variazione);
                }

                if (tipo == 'N' && tipo == tipo_attuale) // Devo calcolare volta per volta la variazione
                {
                    if (primo)
                    {
                        tot_pre = tot;
                        primo = 0;
                        continue;
                    }

                    variazione = tot - tot_pre;
                    tot_pre = tot;
                    printf("%d:%d:%d %d\n", d, m, y, variazione);
                }
            }

            if (data_fin[D] == d && data_fin[M] == m && data_fin[Y] == y)
                stampa = 0;
        }

        // Stampo il totale
        if (strcmp(tipo_aggregato, "totale") == 0)
        {
            printf("Dal %d:%d:%d al %d:%d:%d il totale di %s è %d \n", data_ini[D], data_ini[M], data_ini[Y], data_fin[D], data_fin[M], data_fin[Y], (tipo_attuale == 'T') ? "tamponi fatti" : "casi positivi", tot_aggr);
        }

        printf("------------------------------------------------------------\n");
        printf("\n");
    }
}

/*  Dato tipo e data dell'aggregato, ricerca il dato nel file degli aggregati passato.
    Lascia in totale il valore cercato, altrimeti se non c'è nessun valore per quella data 
    scrive 0xFFFFFFFF.
    Quando passo il file <aggr.txt> allora il flag = 1, 0 altrimenti*/
void cerca_totale(char nome_file[], int data[], char tipo_attuale, int *totale, int flag)
{
    FILE *fd;
    int i = 0;
    char tipo;
    int d, m, y;
    int tot;
    char buf[BUF_LEN];

    //printf("DBG     cerca_in_aggr(): START\n");

    if ((fd = fopen(nome_file, "r")) == NULL)
    {
        printf("ERR     cerca_in_aggr(): non posso aprire il file '%s'\n", nome_file);
        return;
    }
    else
    {
        while (i < FILE_dim(nome_file))
        {
            fseek(fd, i, SEEK_SET);
            fgets(buf, BUF_LEN, fd);

            //printf("DBG     fgets(): %s", buf);
            i += strlen(buf);

            sscanf(buf, "%d:%d:%d %c %d", &d, &m, &y, &tipo, &tot);
            //printf("DBG     data trovata nel file aggr.txt: %d:%d:%d %c\n", d, m, y, tipo);
            //printf("DBG     &tipo = %c\n", tipo);

            if (d == data[D] && m == data[M] && y == data[Y] && tipo_attuale == tipo)
            {
                *totale = tot;
                return;
            }
        }

        // Se sono qui la data è mancante
        if (flag == 1)
            *totale = 0xFFFFFFFF;
        else
            *totale = 0;
    }
}

// -------------------------------------------------------------------------------------------------------------------------------------- //
// ---------------------------------------------------------------- NET TCP ------------------------------------------------------------- //
// -------------------------------------------------------------------------------------------------------------------------------------- //

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

    if (connect(sd_sinistro, (struct sockaddr *)&vicino.addr_sinistro, sizeof(vicino.addr_sinistro)) == -1)
    {
        perror("ERR     Errore nella connect() al vicino sinistro");
        return -1;
    }

    printf("LOG     Invio il messaggio <%s> al vicino sinistro %d\n", buf, ntohs(vicino.addr_sinistro.sin_port));

    // Invio lunghezza del messaggio
    if (send(sd_sinistro, (void *)&buf_len, sizeof(int), 0) == -1)
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

    if (connect(sd_destro, (struct sockaddr *)&vicino.addr_destro, sizeof(vicino.addr_destro)) == -1)
    {
        perror("ERR     Errore nella connect() al vicino sinistro");
        return -1;
    }

    printf("LOG     Invio il messaggio <%s> al vicino destro %d\n", buf, ntohs(vicino.addr_destro.sin_port));

    // Invio lunghezza del messaggio
    if (send(sd_destro, (void *)&buf_len, sizeof(int), 0) == -1)
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

/* Richiesta di flooding.
   Il flag 'transito' serve a differenziare il comportamento della funzione quando è chiamata per iniziare una richiesta di flooding
   o quando è chiamata per far transitare una richiesta di flooding */
void flooding(char req[], int data_i[], char tipo_attuale, int totale_attuale, int porta, int transito)
{
    int sd_destro, sd_sinistro;
    char buf[BUF_LEN];
    int buf_len;
    int totale;

    //printf("DBG     dentro la chiamata a flooding(): porta_richiedente = %hd\n", porta);
    // Se la richiesta non è di transito
    if (transito == 0) // Mi connetto al peer sinistro da cui mi arriverà la risposta
    {
        //printf("DBG     La richiesta di flooding parte da me!\n");
        // Creo il messaggio da inviare per fare la richiesta
        strcpy(buf, "");
        sprintf(buf, "FLD_WAIT %d:%d:%d %c %d %d ", data_i[D], data_i[M], data_i[Y], tipo_attuale, totale_attuale, porta);
        buf_len = strlen(buf);

        sd_sinistro = socket(AF_INET, SOCK_STREAM, 0);

        if (connect(sd_sinistro, (struct sockaddr *)&vicino.addr_sinistro, sizeof(vicino.addr_sinistro)) == -1)
        {
            perror("ERR     Errore nella connect() al vicino sinistro");
            return;
        }

        printf("LOG     Invio il messaggio <%s> al vicino sinistro %d\n", buf, ntohs(vicino.addr_sinistro.sin_port));

        // Invio lunghezza del messaggio
        if (send(sd_sinistro, (void *)&buf_len, sizeof(int), 0) == -1)
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

    // Creo il messaggio da inviare per fare la richiesta di flooding
    sprintf(buf, "%s %d:%d:%d %c %d %d ", req, data_i[D], data_i[M], data_i[Y], tipo_attuale, totale_attuale, porta);
    //printf("DBG     flooding(): buf = <%s> | len = <%ld>\n", buf, strlen(buf));
    buf_len = strlen(buf);

    // La richiesta di floting percorre un solo verso.
    sd_destro = socket(AF_INET, SOCK_STREAM, 0);

    if (connect(sd_destro, (struct sockaddr *)&vicino.addr_destro, sizeof(vicino.addr_destro)) == -1)
    {
        perror("ERR     Errore nella connect() al vicino sinistro");
        return;
    }

    printf("LOG     Invio il messaggio <%s> al vicino destro %d\n", buf, ntohs(vicino.addr_destro.sin_port));

    // Invio lunghezza del messaggio
    if (send(sd_destro, (void *)&buf_len, sizeof(int), 0) == -1)
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
        // Dopo aver fatto il giro la richiesta di flood deve tornare qui
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
   Se manca qualche dato lo richiedo ai vicini, e poi faccio l'eventuale richiesta di flooding.
   Quando questa funzione finisce la sua esecuzione, il peer avrà a disposizione tutti i dati necessari*/
int verifica_presenza_dati(int data_ini[], int data_fin[], char asterisco1, char astrisco2, char tipo_attuale)
{
    int data_i[3];
    int n_giorni;
    int porta;
    int flood = 0;
    int mio_tot;

    if (asterisco1 != '*' && astrisco2 != '*') // Intervallo integro
    {
        data_i[D] = data_ini[D];
        data_i[M] = data_ini[M];
        data_i[Y] = data_ini[Y];

        printf("LOG     periodo preso in cosiderazione: dal %d:%d:%d al %d:%d:%d\n", data_ini[D], data_ini[M], data_ini[Y], data_fin[D], data_fin[M], data_fin[Y]);

        n_giorni = numero_giorni_fra_due_date(data_ini[D], data_ini[M], data_ini[Y], data_fin[D], data_fin[M], data_fin[Y]);
        // Scorro giorno per giorno
        while (n_giorni > 0)
        {
            if (verifica_presenza_della_data(nome_aggr, data_i[D], data_i[M], data_i[Y], tipo_attuale) == 0) // Non ho trovato la data
            {
                if (alone == 1)
                {
                    printf("WRN     Questo peer è l'unico della rete, non posso fare flooding\n");
                    return -1;
                }

                if (boot == 0)
                {
                    printf("ERR     Questo peer non ha fatto boot, si prega di eseguire prima il comando <start>\n");
                    return -1;
                }

                if (richiedi_ai_vicini(data_i, tipo_attuale) == -1)
                {
                    printf("ERR     Non è stato possibile chiedere ai vicini, oppure i vicini non hanno il dato richiesto\n");
                    flood = 1;
                }

                if (flood == 1)
                {
                    // Cerco il mio totale di quel giorno per mandarlo insieme alla richiesta di flooding, così che possa fare il giro della rete
                    cerca_totale(nome_miei_aggr, data_i, tipo_attuale, &mio_tot, 0);
                    //printf("DBG     Il mio totale prima dell flooding: %d\n", mio_tot);
                    flooding("FLD_REQ", data_i, tipo_attuale, mio_tot, my_port, 0);
                    flood = 0;
                }
            }

            data_successiva(data_i[D], data_i[M], data_i[Y], data_i);
            n_giorni--;
        }
    }
    else if (asterisco1 == '*' && astrisco2 != '*') // *-data_fine
    {
        // Non essendo stata indicata la data di lower bound, imposto come prima data una data comune per tutti.
        data_i[D] = data_ini[D] = 1;
        data_i[M] = data_ini[M] = 5;
        data_i[Y] = data_ini[Y] = 2021;

        printf("LOG     periodo preso in cosiderazione: dal %d:%d:%d al %d:%d:%d\n", data_ini[D], data_ini[M], data_ini[Y], data_fin[D], data_fin[M], data_fin[Y]);

        n_giorni = numero_giorni_fra_due_date(data_ini[D], data_ini[M], data_ini[Y], data_fin[D], data_fin[M], data_fin[Y]);
        // Scorro giorno per giorno
        while (n_giorni > 0)
        {
            if (verifica_presenza_della_data(nome_aggr, data_i[D], data_i[M], data_i[Y], tipo_attuale) == 0) // Non ho trovato la data
            {
                if (alone == 1)
                {
                    printf("WRN     Questo peer è l'unico della rete, non posso fare flooding\n");
                    return -1;
                }

                if (boot == 0)
                {
                    printf("ERR     Questo peer non ha fatto boot, si prega di eseguire prima il comando <start>\n");
                    return -1;
                }

                if (richiedi_ai_vicini(data_i, tipo_attuale) == -1)
                {
                    printf("ERR     Non è stato possibile chiedere ai vicini, oppure i vicini non hanno il dato richiesto\n");
                    flood = 1;
                }

                if (flood == 1)
                {
                    // Cerco il mio totale di quel giorno per mandarlo insieme alla richiesta di flooding, così che possa fare il giro della rete
                    cerca_totale(nome_miei_aggr, data_i, tipo_attuale, &mio_tot, 0);
                    flooding("FLD_REQ", data_i, tipo_attuale, mio_tot, my_port, 0);
                    flood = 0;
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
        if (registro_aperto(nome_reg) == 1)
        {
            data_precedente(data_fin[D], data_fin[M], data_fin[Y], data_fin);
        }

        //printf("DBG     data_inizio-* data presa dal registro: %d:%d:%d\n", data_fin[D], data_fin[M], data_fin[Y]);

        data_i[D] = data_ini[D];
        data_i[M] = data_ini[M];
        data_i[Y] = data_ini[Y];

        printf("LOG     periodo preso in cosiderazione: dal %d:%d:%d al %d:%d:%d\n", data_ini[D], data_ini[M], data_ini[Y], data_fin[D], data_fin[M], data_fin[Y]);

        n_giorni = numero_giorni_fra_due_date(data_ini[D], data_ini[M], data_ini[Y], data_fin[D], data_fin[M], data_fin[Y]);
        // Scorro giorno per giorno
        while (n_giorni > 0)
        {
            if (verifica_presenza_della_data(nome_aggr, data_i[D], data_i[M], data_i[Y], tipo_attuale) == 0) // Non ho trovato la data
            {
                if (alone == 1)
                {
                    printf("WRN     Questo peer è l'unico della rete, non posso fare flooding\n");
                    return -1;
                }

                if (boot == 0)
                {
                    printf("ERR     Questo peer non ha fatto boot, si prega di eseguire prima il comando <start>\n");
                    return -1;
                }

                if (richiedi_ai_vicini(data_i, tipo_attuale) == -1)
                {
                    printf("ERR     Non è stato possibile chiedere ai vicini, oppure i vicini non hanno il dato richiesto\n");
                    flood = 1;
                }

                if (flood == 1)
                {
                    // Cerco il mio totale di quel giorno per mandarlo insieme alla richiesta di flooding, così che possa fare il giro della rete
                    cerca_totale(nome_miei_aggr, data_i, tipo_attuale, &mio_tot, 0);
                    flooding("FLD_REQ", data_i, tipo_attuale, mio_tot, my_port, 0);
                    flood = 0;
                    ;
                }
            }

            data_successiva(data_i[D], data_i[M], data_i[Y], data_i);
            n_giorni--;
        }
    }
    else // *-*
    {
        data_i[D] = data_ini[D] = 1;
        data_i[M] = data_ini[M] = 5;
        data_i[Y] = data_ini[Y] = 2021;

        sscanf(nome_reg, "reg_%d-%d-%d_", &data_fin[D], &data_fin[M], &data_fin[Y]);

        if (registro_aperto(nome_reg) == 1)
        {
            data_precedente(data_fin[D], data_fin[M], data_fin[Y], data_fin);
        }

        printf("LOG     periodo preso in cosiderazione: dal %d:%d:%d al %d:%d:%d\n", data_ini[D], data_ini[M], data_ini[Y], data_fin[D], data_fin[M], data_fin[Y]);

        n_giorni = numero_giorni_fra_due_date(data_ini[D], data_ini[M], data_ini[Y], data_fin[D], data_fin[M], data_fin[Y]);
        // Scorro giorno per giorno
        while (n_giorni > 0)
        {
            if (verifica_presenza_della_data(nome_aggr, data_i[D], data_i[M], data_i[Y], tipo_attuale) == 0) // Non ho trovato la data
            {
                if (alone == 1)
                {
                    printf("WRN     Questo peer è l'unico della rete, non posso fare flooding\n");
                    return -1;
                }

                if (boot == 0)
                {
                    printf("ERR     Questo peer non ha fatto boot, si prega di eseguire prima il comando <start>\n");
                    return -1;
                }

                if (richiedi_ai_vicini(data_i, tipo_attuale) == -1)
                {
                    printf("ERR     Non è stato possibile chiedere ai vicini, oppure i vicini non hanno il dato richiesto\n");
                    flood = 1;
                }

                if (flood == 1)
                {
                    // Cerco il mio totale di quel giorno per mandarlo insieme alla richiesta di flooding, così che possa fare il giro della rete
                    cerca_totale(nome_miei_aggr, data_i, tipo_attuale, &mio_tot, 0);
                    flooding("FLD_REQ", data_i, tipo_attuale, mio_tot, my_port, 0);
                    flood = 0;
                }
            }

            data_successiva(data_i[D], data_i[M], data_i[Y], data_i);
            n_giorni--;
        }
    }

    return 0;
}

// -------------------------------------------------------------------------------------------------------------------------------------- //
// ---------------------------------------------------------------- NET UDP ------------------------------------------------------------- //
// -------------------------------------------------------------------------------------------------------------------------------------- //

/* Crea il socket per la connessione UDP, in modo da assocciarlo all'indirizzo e alla porta passati.
   Ritorna -1 in caso di errore */
int creazione_socket_UDP()
{
    // Creazione del socket UDP
    //printf("LOG     creazione del socket UDP...\n");
    if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("ERR     Errore nella creazione del socket UDP, ripovare: ");
        return -1;
    }

    // Aggancio
    if (bind(udp_socket, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1)
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

/* Invia una richiesta 'request' al DS*/
int invia_al_DS(char request[])
{

    printf("LOG     Invio di <%s>... ", request);

    if (sendto(udp_socket, request, REQUEST_LEN, 0, (struct sockaddr *)&ds_addr, ds_len_addr) == -1)
    {
        perror("\nERR     Errore durante l'invio dell' ACK");
        return -1;
    }

    printf("avvenuto con successo\n");
    return 0;
}

/*  Ricevi messaggio dal DS*/
int ricevi_dal_DS(char this_buf[])
{
    char buf[REQUEST_LEN];
    int len_addr = sizeof(ds_addr);
    printf("LOG     Attendo <%s>... ", this_buf);

    if (recvfrom(udp_socket, buf, REQUEST_LEN, 0, (struct sockaddr *)&ds_addr, (socklen_t *__restrict)&len_addr) == -1)
    {
        perror("\nERR     Errore durante la ricezione dell' ACK");
        return -1;
    }

    if (strcmp(this_buf, buf) == 0)
        printf("ricevuto con successo\n");
    else    
        printf("ERRORE, ho ricevuto <%s>\n", buf);

    return 0;
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
    printf("LOG     Ricezione porta del neighbor sinistro...");
    if (recvfrom(udp_socket, (void *)&porta, sizeof(in_port_t), 0, (struct sockaddr *)&ds_addr, (socklen_t *__restrict)&ds_len_addr) == -1)
    {
        perror("ERR     Errore durante la ricezione della prima porta");
    }
    printf(" effettuata con successo: <%d>\n", porta);

    invia_al_DS("PRT_ACK");

    memset(&vicino.addr_sinistro, 0, sizeof(vicino.addr_sinistro));
    vicino.addr_sinistro.sin_family = AF_INET;
    vicino.addr_sinistro.sin_port = htons(porta);
    vicino.addr_sinistro.sin_addr.s_addr = INADDR_ANY;
    
    // Bisogna vedere se è il primo peer che si registra alla rete
    if (porta == 0)
        alone = 1;
    else
        alone = 0;

    //printf("LOG     Ricezione IP e porta del secondo neighbor...\n");
    /*
    if (recvfrom(udp_socket, (void*)&ip, sizeof(uint32_t), 0, (struct sockaddr*) &ds_addr, (socklen_t *__restrict)&ds_len_addr) == -1)
    {
        perror("ERR     Errore durante la ricezione del secondo IP");
    }
*/
    printf("LOG     Ricezione porta del neighbor destro...");
    if (recvfrom(udp_socket, (void *)&porta, sizeof(in_port_t), 0, (struct sockaddr *)&ds_addr, (socklen_t *__restrict)&ds_len_addr) == -1)
    {
        perror("ERR     Errore durante la ricezione della seconda porta");
    }
    printf(" effettuata con successo: <%d>\n", porta);

    invia_al_DS("PRT_ACK");

    memset(&vicino.addr_destro, 0, sizeof(vicino.addr_destro));
    vicino.addr_destro.sin_family = AF_INET;
    vicino.addr_destro.sin_port = htons(porta);
    vicino.addr_destro.sin_addr.s_addr = INADDR_ANY;

    // Bisogna vedere se è il primo peer che si registra alla rete
    if (porta == 0)
        alone = 1;
    else
        alone = 0;
}

// -------------------------------------------------------------------------------------------------------------------------------------- //
// ---------------------------------------------------------------- MAIN ---------------------------------------------------------------- //
// -------------------------------------------------------------------------------------------------------------------------------------- //
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
    printf("                                    'type' puo' essere: 'T' per tampone o 'N' per caso positivo.\n");
    printf("                                    'quantity' e' un numero intero positivo.\n");
    printf("    4) get <aggr> <type> <period>   Effettua una richiesta di elaborazione per ottenere l'aggregato aggr sui dati relativi\n");
    printf("                                    'aggr' puo' essere: 'totale' o 'variazione'.\n");
    printf("                                    'type' puo' essere: 'T' per tampone o 'N' per caso positivo.\n");
    printf("                                    'period' deve essere nel formato: DD:MM:YY-DD:MM:YY\n");
    printf("                                    a un lasso temporale d'interesse period sulle entry di tipo type\n");
    printf("    5) stop                         Il peer richiede di disconnettersi dal network\n");
}

int main(int argc, char *argv[])
{
    // ( VARIABILI MAIN
    fd_set master;
    fd_set read_fds;
    int fd_max;

    FILE *fd;
    int len;

    time_t rawtime;
    struct tm *timeinfo;

    char buf[BUF_LEN];     // Usato per memorizzare i comandi da tastiera
    char comando[BUF_LEN]; // comando e parametro sono usati per differenziare i comandi dai parametri dei comandi da tastiera
    int parametro;         // Dei un comando generico
    char tipo;
    int totale;
    char periodo[BUF_LEN];
    char tipo_aggr[BUF_LEN];
    int data[3];
    int data_ini[3];
    int data_fin[3];
    char asterisco1, asterisco2; // Controllo sulle date passate con asterisco
    int len_addr;
    int buf_len;
    int totale_vicino;
    int porta_richiedente;
    //int start_fatta = 0;

    int sd_wait_for_flt;
    int diciotto = 0; //Flag per chiudere una sola volta il registro dopo le 18
    // )

    my_port = atoi(argv[1]);

    // Ricesco il nome del file degli aggregati di questo peer.
    strcpy(nome_miei_aggr, "");
    sprintf(nome_miei_aggr, "miei_aggr_%d.txt", my_port);
    crea_file(nome_miei_aggr);

    strcpy(nome_aggr, "");
    sprintf(nome_aggr, "aggr_%d.txt", my_port);
    crea_file(nome_aggr);

    strcpy(nome_file_ultimo_reg, "");
    sprintf(nome_file_ultimo_reg, "nome_ultimo_reg_%d.txt", my_port);
    crea_file(nome_file_ultimo_reg);

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    // Vado a prendere il nome dell'ultimo registro che è stato aperto per fare tutti i congtrolli del caso
    // Se per esempio l'applicazione si riavvia dopo qualche giorno devo controllare se quel registro è stato chiuso o meno
    len = FILE_dim(nome_file_ultimo_reg);
    fd = fopen(nome_file_ultimo_reg, "r");
    fseek(fd, 0, SEEK_SET);
    fgets(buf, len, fd);
    fclose(fd);

    // Creo il nome col giorno attuale
    strcpy(nome_reg, "");
    sprintf(nome_reg, "reg_%d-%d-%d_%d.txt", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, my_port);

    // reg_2-6-2021 10:00
    // CASO IN CUI APRO UN NUOVO REGISTRO DOPO LE 18 NELLO STESSO GIORNO DELL'ATTUALE REGISTRO
    if ((timeinfo->tm_hour >= 17 && timeinfo->tm_min != 0 && timeinfo->tm_sec != 0)) // Se siamo oltre le 18:
    {
        // Creo registro con la data successiva a quella odierna
        data_successiva(timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, data);
        strcpy(nome_reg, "");
        sprintf(nome_reg, "reg_%d-%d-%d_%d.txt", data[D], data[M], data[Y], my_port);
        diciotto = 1;
    }

    // CASO IN CUI APRO UN REGISTRO UN'ALTRO GIORNO DA QUELLO DEL REGISTRO ATTUALE
    // Per mantenere la cosistenza del registro dopo un arresto imprevisto
    if (strcmp(buf, nome_reg) != 0)
    {
        //printf("DBG     buf = %s", buf);
        // Aggrego i dati di quel registro, questa operazione sancisce anche la chiusura di quel registro
        if (aggrega_registro(buf) == -1)
            printf("WRN     Il registro è stato già chiuso\n");

        // Salvo nel file "nome_uoltimo_reg", il nome del registro appena aperto
        sovrascrivi_file(nome_file_ultimo_reg, nome_reg);
    }

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

                if (send(trasporto_sd, (void *)&totale, sizeof(int), 0) == -1)
                {
                    perror("ERR     Dato richiesto non inviato correttamente");
                    continue;
                }

                printf("LOG     Ho inviato il totale di %d %s al peer %d", totale, (tipo == 'T') ? "tamponi fatti" : "casi positivi", ntohs(peer_addr.sin_port));
                close(trasporto_sd);
            }
            // )

            // ( Richiesta di flooding
            if (strcmp(comando, "FLD_REQ") == 0)
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
                    //printf("LOG     Devo propagare la richiesta di flooding\n");

                    // Vuol dire che il peer non ha alcun dato relativo al totale della rete,
                    // quindi deve vedere se ha un dato relativo al suo registro chiuso,
                    // propagarlo aggiungendolo al totale che gli ha inviato il vicino sinistro
                    cerca_totale(nome_miei_aggr, data, tipo, &totale, 0);
                    totale_vicino += totale;
                    //printf("DBG     Il totale che invio è %d\n", totale_vicino);

                    // Faccio la richiesta di flooding
                    if (porta_richiedente == ntohs(vicino.addr_destro.sin_port)) // Se sono sul peer precedente a quello che ha iniziato il flooding allora sarà una risposta
                    {
                        // Invio il totale al peer che ha fatto la richiesta di flooding
                        if (send(sd_wait_for_flt, (void *)&totale_vicino, sizeof(int), 0) == -1)
                        {
                            perror("ERR     Dato richiesto non inviato correttamente");
                            continue;
                        }

                        printf("LOG     Ho inviato il totale di %d %s al peer %d", totale_vicino, (tipo == 'T') ? "tamponi fatti" : "casi positivi", porta_richiedente);
                        close(sd_wait_for_flt);
                    }
                    else
                    {
                        //printf("DBG     Prima della chiamata a flooding(): porta_richiedente = %d\n", porta_richiedente);
                        flooding("FLD_REQ", data, tipo, totale_vicino, porta_richiedente, 1); // Altrimenti è una richiesta
                    }
                }
                else // Propago la risposta
                {
                    // Possiede il dato aggregato del totale della rete richiesto,
                    // quindi invia una richiesta di risposta al flooding verso il vicino destro che la riceverà
                    // e la propagherà fino al raggiungimento del destinatario che ne ha fatto richiesta

                    // Stavolta è una risposta al repli
                    flooding("FLD_RPL", data, tipo, totale, porta_richiedente, 1);
                }

                close(trasporto_sd);
                printf("\n");
            }
            // )

            // ( Risposta al flooding
            if (strcmp(comando, "FLD_RPL") == 0)
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

                    // Invio sul socket attivato con la richiesta FLD_WAIT
                    if (send(sd_wait_for_flt, (void *)&totale_vicino, sizeof(int), 0) == -1)
                    {
                        perror("ERR     Dato richiesto non inviato correttamente");
                        continue;
                    }

                    printf("LOG     Ho inviato il totale di %d %s al peer %d", totale_vicino, (tipo == 'T') ? "tamponi fatti" : "casi positivi", porta_richiedente);
                    close(sd_wait_for_flt);
                }
                else // La risposta è in transito, ma ne approfitto per salvare il dato aggregato se non lo ho
                {
                    if (verifica_presenza_della_data(nome_aggr, data[D], data[M], data[Y], tipo) == 0) // Non ho trovato la data
                    {
                        aggiungi_aggr(nome_aggr, data, tipo, totale_vicino);
                    }

                    // Stavolta è una risposta al repli
                    flooding("FLD_RPL", data, tipo, totale_vicino, porta_richiedente, 1);
                }

                close(trasporto_sd);
            }
            // )

            // ( Richiesta di attesa del flooding che il peer richiedente fa al suo vicino sinistro per avvertirlo che la richiesta dovrà poi spedirla su quel socket
            if (strcmp(comando, "FLD_WAIT") == 0)
            {
                printf("LOG     Salvo il socket collegato al peer %d che ha inviato FLD_WAIT\n", ntohs(vicino.addr_destro.sin_port));
                sd_wait_for_flt = trasporto_sd;
            }
            // )

            printf("\n");
        }

        // Gestione richieste UDP da parte del DS
        if (FD_ISSET(udp_socket, &read_fds))
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
            printf("LOG     Ho ricevuto la richiesta <%s> da parte del DS\n", comando);

            // ( Richiesta di UPDATE dei vicini
            if (strcmp(comando, "UPD_REQ") == 0)
            {
                // Invio ACK
                invia_al_DS("UPD_ACK");

                ricezione_vicini_DS();
            }
            // )

            // ( Richiesta ESC
            if (strcmp(comando, "REQ_ESC") == 0)
            {
                // Invio ACK
                invia_al_DS("ESC_ACK");

                aggrega_registro(nome_reg);
                printf("\n\n---------------- Buona Giornata! ----------------\n\n");
                exit(-1);
            }
            // )

            printf("\n");
        }

        // Gestione delle richieste da STDIN
        if (FD_ISSET(0, &read_fds))
        {
            printf("\n");
            // gestione richieste da stdin
            printf("LOG     Accetto richiesta da stdin...\n");
            strcpy(buf, "");

            // Attendo input da tastiera
            if (fgets(buf, 1024, stdin) == NULL)
            {
                printf("ERR     Immetti un comando valido!\n");
                continue;
            }

            sscanf(buf, "%s", comando);

            if ((strlen(comando) == 0) || (strcmp(comando, " ") == 0))
            {
                printf("ERR     Comando non valido! Prova ad inserire uno dei seguenti:\n");
                comandi_disponibili();
                printf("\n");
                continue;
            }

            // ( HELP
            if (strcmp(comando, "help") == 0)
            {
                comandi_disponibili_con_spiegazione();
                printf("\n");
            }
            // )

            // ( START
            else if (strcmp(comando, "start") == 0) // REQ_STR
            {
                int portaDS;
                char ipDS[BUF_LEN];

                sscanf(buf, "%s %s %d", comando, ipDS, &portaDS);

                if (ds_addr.sin_port == htons(portaDS))
                {
                    printf("WRN     Attenzione, il peer ha già eseguito il comando <start>\n");
                    printf("\n");
                    continue;
                }

                // Creazione indirizzo del socket
                memset(&ds_addr, 0, sizeof(ds_addr));
                ds_addr.sin_family = AF_INET;
                ds_addr.sin_port = htons(portaDS);
                ds_addr.sin_addr.s_addr = INADDR_ANY;
                // inet_pton(AF_INET, ipDS, &ds_addr.sin_addr);

                //printf("DBG     porta e ip del DS: <%d> | <%s>\n", portaDS, ipDS);
            riprova: // Bisogna far richiesta al DS di entrare nella rete
                if (invia_al_DS("REQ_STR") == -1)
                {
                    sleep(POLLING_TIME);
                    goto riprova;
                }

                // Attendo ACK_STR
                if (ricevi_dal_DS("ACK_REQ") == -1)
                {
                    sleep(POLLING_TIME);
                    goto riprova;
                }

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
                    printf("\n");
                    continue;
                }

                // Creo stringa da scrivere
                sprintf(buf, "%d:%d:%d %c %d", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, tipo, parametro);

                // Scrivo in append nel file
                printf("LOG     Scrivo <%s> nel file <%s>\n", buf, nome_reg);
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
                    printf("\n");
                    continue;
                }

                // Controllo tipo carattere
                if (strcmp(tipo_aggr, "totale") != 0 && strcmp(tipo_aggr, "variazione"))
                {
                    printf("ERR     Errore nel tipo di aggregato, deve essere 'totale' o 'variazione'. Riprovare\n");
                    printf("\n");
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
                        printf("\n");
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
                        printf("\n");
                        continue;
                    }
                    asterisco1 = 'X';
                    goto verifica;
                }

                sscanf(periodo, "%d:%d:%d-%d:%d:%d", &data_ini[D], &data_ini[M], &data_ini[Y], &data_fin[D], &data_fin[M], &data_fin[Y]);
                printf("LOG     Controllo la data di inizio periodo... ");
                if (data_valida(data_ini[D], data_ini[M], data_ini[Y]) == 0)
                {
                    printf("\n");
                    continue;
                }

                printf("LOG     Controllo la data di fine periodo... ");
                if (data_valida(data_fin[D], data_fin[M], data_fin[Y]) == 0)
                {
                    printf("\n");
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
                    printf("\n");
                    continue;
                }

                stampa_richiesta(nome_aggr, data_ini, data_fin, asterisco1, asterisco2, tipo, tipo_aggr);
            }
            //)

            // ( STOP
            else if (strcmp(comando, "stop") == 0) // DA FARE
            {
                aggrega_registro(nome_reg);
                invia_al_DS("STP_REQ");
                ricevi_dal_DS("STP_ACK");

                printf("\n\n---------------- Buona Giornata! ----------------\n\n");
                exit(-1);
            }
            // )

            else
            {
                printf("ERR     Comando non valido! Prova ad inserire uno dei seguenti:\n");
                comandi_disponibili();
            }

            printf("\n");

            // ( CONTROLLO SE SONO PASSATE LE 18 PER CHIUDERE IL REGISTRO
            time(&rawtime);
            timeinfo = localtime(&rawtime);

            // CASO IN CUI APRO UN NUOVO REGISTRO DOPO LE 18 NELLO STESSO GIORNO DELL'ATTUALE REGISTRO
            // Bisogna controllare l'ora e se il registro è già stato chiuso per eventualmente crearne uno nuovo
            if ((timeinfo->tm_hour >= 17 && timeinfo->tm_min != 0 && timeinfo->tm_sec != 0) && !diciotto) // Se siamo oltre le 18:
            {
                printf("LOG     Sono passate le 18!\n");

                // Aggrego i dati di quel registro, questa operazione sancisce anche la chiusura di quel registro
                if (registro_aperto(nome_reg) == 1)
                    aggrega_registro(nome_reg);

                // Creo registro con la data successiva a quella odierna
                data_successiva(timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, data);
                strcpy(nome_reg, "");
                sprintf(nome_reg, "reg_%d-%d-%d_%d.txt", data[D], data[M], data[Y], ntohs(my_addr.sin_port));
                diciotto = 1;
            }

            // Se l'applicazione rimane accesa oltre 00:00 resetto il flag.
            if (timeinfo->tm_hour < 17)
            {
                diciotto = 0;
            }
            // )
        }
    }

    close(ascolto_sd);
}