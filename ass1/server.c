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

/* Additional Function Declarations here */

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void child_pro(client *, struct sockaddr_in, int *, int);
int addclient(client *, struct sockaddr_in, char [256], int *, int);
void printlist(client *,int);

/*Main function starts here */

int main(int argc, char **argv)
{
    int sockfd, newsockfd, portno;
    pid_t pid;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    
    if (argc < 2)
        error("ERROR No port provided... exiting");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket... exiting");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding socket... exiting");

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

    listen(sockfd, 10);
    clilen = sizeof(cli_addr);
    while(1)
    {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
            error("ERROR on accept... exiting");
        pid = fork();
        if (pid < 0)
            error("ERROR on fork... exiting");
        if (pid == 0)
        {
            close(sockfd);
            child_pro(cli_list,cli_addr,numcli,newsockfd);
            close(newsockfd);
            exit(0);
        }
        else
            close(newsockfd);
        
    }
    shmdt((char *) cli_list);
    shmdt((char *) numcli);
    shmctl(shm_id1, IPC_RMID, NULL);
    shmctl(shm_id2, IPC_RMID, NULL);
    return 0;
   
}

/* Additional functions definitions below */

void child_pro (client *cli_list, struct sockaddr_in cli_addr, int *numcli, int sock)
{
    int n;
    char buffer[256];
    
    bzero(buffer,256);
    strcpy(buffer,"Enter your desired login name : ");
    n = write(sock,buffer,strlen(buffer));
    if (n < 0)
        error("ERROR writing to socket... exiting");
    bzero(buffer,256);
    n = read(sock,buffer,255);
    if (n < 0)
        error("ERROR reading from socket... exiting");
    printf("\nLogin name : %s",buffer);
    n = addclient(cli_list,cli_addr,buffer,numcli,sock);
    if (n == 0)
        return;
    while(1)
    {
        bzero(buffer,256);
        strcpy(buffer,"Following options are available :\nL\tPrint the list of Online player.\nE\tExit the game.\nEnter your option : ");
        n = write(sock,buffer,strlen(buffer));
        if (n < 0)
            error("ERROR writing on socket... exiting");
        bzero(buffer,256);
        n = read(sock,buffer,255);
        if (n < 0)
            error("ERROR reading from socket... exiting");
        if ((buffer[0] == 'L') || (buffer[0] == 'l'))
            printlist(cli_list,sock);
        else if ((buffer[0] == 'E') || (buffer[0] == 'e'))
        {
            *numcli = *numcli -1;
            return;
        }
        sleep(1);
    }
    return;
}

int addclient (client *cli_list, struct sockaddr_in cli_addr, char buffer[256], int *numcli, int sock)
{
    int i,n;
    char *ip;
    if (*numcli == MAX_Player)
    {
        bzero(buffer,256);
        strcpy(buffer,"Rejected\nServer cannot accomodate more clients.\nTry again after some time.\n");
        n = write(sock,buffer,strlen(buffer));
        if (n < 0)
            error("ERROR writng on socket... exiting");
        return 0;
    }
    //if (*numcli == 0)
    //    return 1;
    for(i = 0; i < MAX_Player; i++)
    {
        if ((cli_list[i].in_use == 1) && (strcmp(buffer,cli_list[i].login_name) == 0))
        {
            bzero(buffer,256);
            strcpy(buffer,"Rejected\nA client with same login name already exist.\nTry again using different login name.\n");
            n = write(sock,buffer,strlen(buffer));
            if (n < 0)
                error("ERROR writing on socket... exiting");
            return 0;
        }
    }
    *numcli = *numcli+1;
    for ( i = 0; i < MAX_Player; i++)
    {
        if (cli_list[i].in_use == 0)
        {
            cli_list[i].in_use = 1;
            strcpy(cli_list[i].login_name,buffer);
            if ((ip = (char *) inet_ntoa(cli_addr.sin_addr)) < 0)
                error("ERROR cannot obtain ip address of client... exiting");
            strcpy(cli_list[i].ip_addr,ip);
            cli_list[i].portno = ntohs(cli_addr.sin_port);
            cli_list[i].in_game = 0;
            cli_list[i].request = -1;
            bzero(buffer,256);
            strcpy(buffer,"Logged in successfully.\nNow you can play the game.\n");
            n = write(sock,buffer,strlen(buffer));
            if (n < 0)
                error("ERROR writing on socket... exiting");
            printf("Username : %sIP : %s\tPortno : %u", cli_list[i].login_name, cli_list[i].ip_addr, cli_list[i].portno);
            return 1;
        }
    }
}

void printlist (client *cli_list, int sock)
{
    int n,i;
    char buffer[256];
    bzero(buffer,256);
    strcpy(buffer,"List of Online Player is as follows :\n");
    for (i = 0; i < MAX_Player; i++)
    {
        if (cli_list[i].in_use == 1)
            strcat(buffer,cli_list[i].login_name);

    }
    n = write(sock,buffer,strlen(buffer));
    if (n < 0)
        error("ERROR writing on socket... exiting");
    return;
}
