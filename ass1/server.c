/* Server File */

/* Header files are included here */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <netinet/in.h>
#define MAX_Player 10

/* Structure to store online client information */

typedef struct
{
    short in_game;
    short in_use;
    char login_name[50];
    char ip_addr[20];
    unsigned short portno;
    short request;
}client;

/* Additional Function Definitions here */

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

