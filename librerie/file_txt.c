#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "file_txt.h"
#include "date.h"
#include "costanti.h"

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

/* scrivo in append dentro il file di registro nome[] ciò che contiene stringa */
int scrivi_file_append(char nome[], char *stringa)
{
    FILE *fd;

    if ((fd = fopen(nome,"a+")) == NULL)
    {
        printf("ERR     scrivi_file_append(): non posso aprire il file %s\n", nome);
        return -1;
    }
    else 
    {
        fseek(fd, 0, SEEK_END);
        fprintf(fd, "%s\n", stringa);
        printf("LOG     Scrivo <%s> nel file <%s>\n", stringa, nome);
        fclose(fd);
    }

    return 0;
}

/* scrivo in append dentro il file di registro nome[] ciò che contiene stringa */
int sovrascrivi_file(char nome[], char *stringa)
{
    FILE *fd;

    if ((fd = fopen(nome,"w")) == NULL)
    {
        printf("ERR     sovrascrivi_file(): non posso aprire il file %s\n", nome);
        return -1;
    }
    else 
    {
        fseek(fd, 0, SEEK_END);
        fprintf(fd, "%s\n", stringa);
        //printf("LOG     Scrivo %s nel file %s\n", stringa, nome);
        fclose(fd);
    }

    return 0;
}

/* scrivo "aperto" dentro il file di registro nome[] */
int apri_registro(char nome[])
{
    char *stringa = "aperto";
    FILE *fd;
   
    printf("LOG     Apro il registro <%s>...\n", nome);
    if ((fd = fopen(nome,"r+")) == NULL)
    {
        printf("ERR     apri_registro(): Non posso aprire il file %s\n", nome);
        return -1;
    }
    else 
    {
        fseek(fd, 0, SEEK_SET);
        fprintf(fd, "%s\n", stringa);
        printf("LOG     Ho scritto <%s> nel file <%s>\n", stringa, nome);
        fclose(fd);
    }

    return 0;
}

/* scrivo "chiuso" dentro il file di registro nome[] */
int chiudi_registro(char nome[])
{
    char *stringa = "chiuso";
    FILE *fd;
    
    printf("LOG     Chiudo il registro <%s>...\n", nome);
    if ((fd = fopen(nome,"r+")) == NULL)
    {
        printf("ERR     chiudi_registro(): non posso aprire il file <%s>\n", nome);
        return -1;
    }
    else 
    {
        fseek(fd, 0, SEEK_SET);
        fprintf(fd, "%s\n", stringa);
        printf("LOG     Ho scritto <%s> nel file <%s>\n", stringa, nome);
        fclose(fd);
    }

    return 0;
}

/* Ritorna 1 se il registro è aperto */
int registro_aperto(char nome[])
{
    FILE *fd;
    char stringa[BUF_LEN];

    if ((fd = fopen(nome,"r")) == NULL)
    {
        printf("ERR     registro_aperto(): non posso aprire il file <%s>\n", nome);
        return -1;
    }
    else 
    {
        fseek(fd, 0, SEEK_SET);
        fgets(stringa, 7, fd);
        // printf("DBG     Ho letto dal file: <%s>\n", stringa);
        fclose(fd);
    }

    return !strcmp(stringa, "aperto");
}

/* crea un nuovo registro e ne scrive il nome in nome_file[]*/
int crea_nuovo_registro(int data[], int port, char nome_file[])
{
    FILE *fd;
    int ret;

    strcpy(nome_file, "");
    sprintf(nome_file, "%d-%d-%d_%d.txt", data[D], data[M], data[Y], port);
    
    
    fd = fopen(nome_file, "a");
    if (fd == NULL)
        ret = 0;
    else
        ret = -1;   
    fclose(fd);

    return ret;
}

/*  Funzione per aggregare i dati di un singolo registro. */
int aggrega_registro(char nome_file[])
{
    char buf[BUF_LEN];
    char tipo;
    int totale;
    int i, d1, m1, y1;
    int porta;
    int tt = 0; // Posizione Totale Tamponi
    int tp = 1; // Posizione Totale Positivi 
    FILE *fd;
    int aggregato[2]; // aggregato[] contiene i risultati sul totale di ogni tipo:
                      //    1) Totale tamponi
                      //    2) Totale positivi

    //printf("DBG     aggrega_registro(): START\n");

    // Inizializiamo 0x7FFFFFFF così da capire come calcolare le variazioni
    aggregato[tt] = aggregato[tp] = 0;
    //printf("DBG     pre_aggr_t e pre_aggr_p %d  %d\n", pre_aggr_t, pre_aggr_p);

    //printf("DBG     aggrega_registro(): START\n");

    // Se il registro è aperto non fare nulla
    if (registro_aperto(nome_file))
    {
        printf("ERR     aggrega_registro(): il registro è ancora aperto\n");
        return -1;
    }

    // Cerco la data del giorno richiesto
    sscanf(nome_file, "%d-%d-%d_%d", &d1, &m1, &y1, &porta);
    
    if ((fd = fopen(nome_file,"r")) == NULL)
    {
        printf("ERR     aggrega_registro(): non posso aprire il file <%s>\n", nome_file);
        return -1;
    }
    else 
    {
        i = strlen("aperto"); // per saltare l'intestazione del registro: aperto/chiuso
        //printf("\nDBG     scorro %s\n", nome_file);
        while (i<FILE_dim(nome_file)) //Registro
        {
            fseek(fd, i, SEEK_SET);
            fgets(buf, 50, fd);   

            //printf("DBG     Ho letto: %s", buf);
            i += strlen(buf);

            sscanf(buf, "%d:%d:%d %c %d", &d1, &m1, &y1, &tipo, &totale);
            
            if (tipo == 'T')
            {
                //printf("DBG     TAMPONI\n");
                //printf("DBG     aggregato[tt] = %d\n", aggregato[tt]);
                //printf("DBG     totale = %d\n", totale);
                aggregato[tt] += totale;
                //printf("DBG     aggregato[tt] = %d\n", aggregato[tt]);
            }
            
            if (tipo == 'N')
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
    
    // Elimino il registro e metto i dati totali nel file "miei_totali"
    remove(nome_file);
    sprintf(nome_file, "miei_totali_%d.txt", porta);
    sprintf(buf, "%d:%d:%d T %d\n%d:%d:%d N %d", d1, m1, y1, aggregato[tt], d1, m1, y1, aggregato[tp]);

    // Scrivo nel file
    scrivi_file_append(nome_file, buf);
    
    return 0;
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


/* Aggiunge nel file degli aggregati il dato che ha richiesto nella rete.*/
int aggiungi_aggr(char nome_file[], int this_data[], char this_tipo, int this_tot)
{
    FILE *fd;
    FILE *fd_app;
    char buf_app[BUF_LEN];
    char buf[BUF_LEN];    
    char tipo;
    int d, m, y, tot;
    int i;
    int data_succ[3];
    
    //printf("DBG     aggiungi_aggr(): START\n");

    printf("LOG     Aggiungo aggregato ricevuto nel file <%s>\n", nome_file);

    sprintf(buf, "%d:%d:%d %c %d", this_data[D], this_data[M], this_data[Y], this_tipo, this_tot);
    printf("LOG     Record da aggiungere: %d:%d:%d %c %d\n", this_data[D], this_data[M], this_data[Y], this_tipo, this_tot);

    if ((fd = fopen(nome_file, "r+")) == NULL)
    {
        printf("ERR     Non posso aprire il file <%s>\n", nome_file);
        perror(" ");
        return -1;
    }
    else
    {
        fd_app = fopen("temporaneo.txt", "a+");
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
                fprintf(fd_app, "%s", buf_app);
                //printf("DBG     1) Scrivo %s nel file 'temporaneo.txt'\n", buf_app);
            }
            else
            {
                i -= strlen(buf_app); // Non l'ho copiato e quindi devo riportare il cursore dietro per farlo dopo
                break;
            }
        }

        // Ora devo scrivere la nuova entry
        sprintf(buf, "%d:%d:%d %c %d", this_data[D], this_data[M], this_data[Y], this_tipo, this_tot);
        fprintf(fd_app, "%s\n", buf);
        //printf("DBG     -) Scrivo %s nel file 'temporaneo.txt'\n", buf);

        while (i < FILE_dim(nome_file)) // Copio da 0 alla posizione prima a quella in cui devo scrivere
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
            fprintf(fd_app, "%s", buf_app);
            //printf("DBG     2) Scrivo %s nel file 'temporaneo.txt'\n", buf_app);
        }

        fclose(fd);
        fclose(fd_app);

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

            sscanf(buf, "%d:%d:%d %c %d %d", &d, &m, &y, &tipo, &tot);
            
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
            printf("Dal %d:%d:%d al %d:%d:%d il totale di %s è %d \n", data_ini[D], data_ini[M], data_ini[Y], data_fin[D], data_fin[M], data_fin[Y], (tipo_attuale == 'T')? "tamponi fatti":"casi positivi", tot_aggr);
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