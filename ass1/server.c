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

/*Main function starts here */

int main(int argc, char **argv)
{
    int sockfd, newsockfd, portno;
    pid_t pid;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    
    int shm_id1,shm_id2;
    client *cli_list;
    int *numcli;                        /* Number of online client */
    const int size = MAX_Player*sizeof(client);
    shm_id1 = shmget(IPC_PRIVATE, size, S_IRUSR | S_IWUSR);
    shm_id2 = shmget(IPC_PRIVATE, sizeof(int), S_IRUSR | S_IWUSR);
    cli_list = (client *) shmat(shm_id1, NULL, 0);
    numcli = (int *) shmat(shm_id2, NULL, 0);
    memset(cli_list, 0, size);
    *numcli = 0;

    char buffer[256];
}
