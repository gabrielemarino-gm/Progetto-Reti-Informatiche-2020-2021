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

void comandi_disponibili()
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
    char cmd[CMD_LEN]; // per comando da tastiera
    
    printf("Salve!");
    comandi_disponibili();

comando:
    scanf("%s", cmd);

    if (strcmp(cmd, "help") == 0)
    {
        printf("Comando help\n");
        goto comando;
    }
    else if (strcmp(cmd, "showpeers") == 0)
    {
        printf("Comando showpeers\n");
        goto comando;
    }
    else if (strcmp(cmd, "showneighbor") == 0)
    {
        printf("Comando showneighbor\n");
        goto comando;
    }
    else if (strcmp(cmd, "esc") == 0)
    {
        printf("Buona giornata!\n");
        exit(0);
    }
    else // errorre
    {
        printf("Comando non valido. riprovare\n");
        comandi_disponibili();
        goto comando;
    }    
}