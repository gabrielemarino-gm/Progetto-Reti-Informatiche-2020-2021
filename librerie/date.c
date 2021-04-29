#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "date.h"

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
        return 1;
    }
    else if (d1 > d2 && m1 < m2 && y1 == y2)
    {
        return 1;
    }
    else  if (d1 < d2 && m1 < m2 && y1 == y2)
    {
        return 1;
    }
    else if (y1 < y2)
    {
        return 1;
    }

    return 0;
}


/* Ritorna 1 se la data d:m:y è valida, 0 altrimenti*/
int data_valida(int d, int m, int y)
{
    time_t rawtime;
    struct tm* timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    // timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900
    if(y > timeinfo->tm_year + 1900)
    {
        printf("ERR     Immessa una data futura, riprovare\n");
        return 0;
    }

    if (m <= 0 || m > 12)
    {
        printf("ERR     Immesso un mese non valido, riprovare\n");
        return 0;
    }

    if (d <= 0 || d > 31)
    {
        printf("ERR     Immesso un giorno non valido, riprovare\n");
        return 0;
    }

    if ((m == 4 || m == 6 || m == 9 || m == 11) && d >= 31)
    {
        printf("ERR     Immesso un giorno non valido, riprovare\n");
        return 0;
    }

    if (y%4 == 0) // Anno bisestile
    {
        if (m == 2 && (d == 31 || d == 30))
        {
            printf("ERR     Immesso un giorno non valido, riprovare\n");
            return 0;
        }
    }
    else // Anno non bisestile
    {
        if (m == 2 && (d == 31 || d == 30 || d == 29))
        {
            printf("ERR     Immesso un giorno non valido, riprovare\n");
            return 0;
        }
    }

    // controlliamo se non è una data futura
    if (m > timeinfo->tm_mon + 1 && y == timeinfo->tm_year + 1900)
    {
        printf("ERR     Immessa una data futura, riprovare\n");
        return 0;
    }
    

    if (m == timeinfo->tm_mon + 1 && d > timeinfo->tm_mday)
    {
        printf("ERR     Immessa una data futura, riprovare\n");
        return 0;
    }
    
    printf("LOG     La data analizzata è valida!\n");
    return 1;
}
