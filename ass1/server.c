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

void child_pro (client *, struct sockaddr_in, int *, int);
int addclient (client *, struct sockaddr_in, char [256], int *, int);
void printlist (client *,int,int);
int checkRequest (client *,int,int);
int acceptRequest (client *,int,int,int);
void sendRequest (client *,int,int);
void exitGame(client *, int *, int, int);

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
    int n,cli_id;
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
    printf("Login name : %s\n",buffer);
    cli_id = addclient(cli_list,cli_addr,buffer,numcli,sock);
    if (cli_id == -1)
        return;
    while(1)
    {
        sleep(1);
        bzero(buffer,256);
        strcpy(buffer,"Following options are available :\nL\tPrint the list of Online player.\nR\tRequest a game with another client.\nC\tCheck if you have any game request.\nE\tExit the game.\nEnter your option : ");
        n = write(sock,buffer,strlen(buffer));
        if (n < 0)
            error("ERROR writing on socket... exiting");
        bzero(buffer,256);
        n = read(sock,buffer,255);
        if (n < 0)
            error("ERROR reading from socket... exiting");
        if ((buffer[0] == 'L') || (buffer[0] == 'l'))
        {
            printlist(cli_list,cli_id,sock);
            sleep(1);
            n = checkRequest(cli_list,cli_id,sock);
            if (n > -1)
            {
                n = acceptRequest(cli_list,cli_id,n,sock);
                if (n)
                {
                    exitGame(cli_list, numcli, cli_id, sock);
                    return;
                }
            }
        }
        else if ((buffer[0] == 'R') || (buffer[0] == 'r'))
        {
            sendRequest(cli_list,cli_id,sock);
            if (cli_list[cli_id].in_game)
            {
                exitGame(cli_list, numcli, cli_id, sock);
                return;
            }
        }
        else if ((buffer[0] == 'C') || (buffer[0] == 'c'))
        {
            n = checkRequest(cli_list,cli_id,sock);
            if (n > -1)
            {
                n = acceptRequest(cli_list,cli_id,n,sock);
                if (n)
                {
                    exitGame(cli_list, numcli, cli_id, sock);
                    return;
                }
            }
        }        
        else if ((buffer[0] == 'E') || (buffer[0] == 'e'))
        {
            exitGame(cli_list, numcli, cli_id, sock);
            return;
        }
        else
        {
            bzero(buffer,256);
            strcpy(buffer,"Invalid option.\n");
            n = write(sock, buffer, strlen(buffer));
            if (n < 0)
                error("ERROR writing on socket... exiting");
        }
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
        return -1;
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
            return -1;
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
            printf("Username : %s\tIP : %s\tPortno : %u\n", cli_list[i].login_name, cli_list[i].ip_addr, cli_list[i].portno);
            return i;
        }
    }
}

void printlist (client *cli_list, int cli_id, int sock)
{
    int n,i;
    char buffer[256];
    bzero(buffer,256);
    strcpy(buffer,"\nList of Online Player is as follows :\n");
    for (i = 0; i < MAX_Player; i++)
    {
        if (cli_list[i].in_use == 1)
        {
            strcat(buffer,"\t");
            strcat(buffer,cli_list[i].login_name);
            strcat(buffer,"\n");
        }

    }
    n = write(sock,buffer,strlen(buffer));
    if (n < 0)
        error("ERROR writing on socket... exiting");
    printf("List of online player printed successfully\n");
    return;
}

int checkRequest (client *cli_list, int cli_id, int sock)
{
    int n;
    char buffer[256];
    bzero(buffer,256);
    sprintf(buffer,"%d",cli_list[cli_id].request);
    n = write(sock, buffer, strlen(buffer));
    if (n < 0)
        error("ERROR writing on socket... exiting");
    return cli_list[cli_id].request;
}

int acceptRequest (client *cli_list, int cli_id, int op_id, int sock)
{
    int n;
    char buffer[256];
    bzero(buffer,256);
    strcpy(buffer,"You have a game request from ");
    strcat(buffer,cli_list[op_id].login_name);
    strcat(buffer,".\nDo you want to play(Y/N) : ");
    sleep(1);
    n = write(sock, buffer, strlen(buffer));
    if (n < 0)
        error("ERROR writing on socket... exiting");
    bzero(buffer,256);
    n = read(sock, buffer, 255);
    if (n < 0)
        error("ERROR reading from socket... exiting");
    cli_list[cli_id].request = -1;
    cli_list[op_id].request = -1;
    if ((buffer[0] == 'Y') || (buffer[0] == 'y'))
    {
        char port[10];
        cli_list[cli_id].in_game = 1;
        cli_list[op_id].in_game = 1;
        printf("%s and %s are involved in a game.\n",cli_list[cli_id].login_name, cli_list[op_id].login_name);
        bzero(buffer,256);
        strcpy(buffer,"You have accepted the game request.\nOpponent information : \nName : ");
        strcat(buffer,cli_list[op_id].login_name);
        strcat(buffer,"\tIP Address : ");
        strcat(buffer,cli_list[op_id].ip_addr);
        strcat(buffer,"\tPort No. : ");
        sprintf(port,"%u",cli_list[op_id].portno);
        strcat(buffer,port);
        strcat(buffer,"\n");
        n = write(sock, buffer, strlen(buffer));
        if (n < 0)
            error("ERROR writing on socket... exiting");
        return 1;
    }
    else if((buffer[0] == 'N') || (buffer[0] == 'n'))
    {
        bzero(buffer,256);
        strcpy(buffer,"You have denied the game request.\n");
        n = write(sock, buffer, strlen(buffer));
        if (n < 0)
            error("ERROR writing on socket... exiting");
        return 0;
    }
    else
    {
        bzero(buffer,256);
        strcpy(buffer,"Invalid option\n");
        n = write(sock, buffer, strlen(buffer));
        if (n < 0)
            error("ERROR writing on socket... exiting");
        return 0;
    }
    return 0;
}

void sendRequest (client *cli_list, int cli_id, int sock)
{
    int n,i,op_id=-1;
    char buffer[256];
    printlist(cli_list,cli_id,sock);
    sleep(1);
    n = checkRequest(cli_list,cli_id,sock);
    if (n > -1)
    {
        n = acceptRequest(cli_list,cli_id,n,sock);
        if (n)
            return;
    }
    
    bzero(buffer,256);
    n = read(sock, buffer, 255);
    if (n < 0)
        error("ERROR reading from socket... exiting");
    for (i = 0; i < MAX_Player; i++)
    {
        if ((cli_list[i].in_use) && (strcmp(buffer,cli_list[i].login_name) == 0))
        {
            op_id = i;
            break;
        }
    }
    if (op_id == -1)
    {
        bzero(buffer,256);
        strcpy(buffer,"No such client exist.\n");
        n = write(sock, buffer, strlen(buffer));
        if (n < 0)
            error("ERROR writing on socket... exiting");
        return;
    }
    if (cli_list[op_id].request != -1)
    {
        bzero(buffer,256);
        strcpy(buffer,"Specified client has already a request pending.\n");
        n = write(sock, buffer, strlen(buffer));
        if (n < 0)
            error("ERROR writing on socket... exiting");
        return;
    }
    if (cli_list[op_id].in_game)
    {
        bzero(buffer,256);
        strcpy(buffer,"Client is already involved in a game.\n");
        n = write(sock, buffer, strlen(buffer));
        if (n < 0)
            error("ERROR writing on socket... exiting");
        return;
    }
    if (op_id == cli_id)
    {
        bzero(buffer,256);
        strcpy(buffer,"You cannot play game with yourself.\n");
        n = write(sock, buffer, strlen(buffer));
        if (n < 0)
            error("ERROR writing on socket... exiting");
        return;
    }
    cli_list[op_id].request = cli_id;
    cli_list[cli_id].request = op_id;
    bzero(buffer,256);
    strcpy(buffer,"Your request has been sent.\nAwaiting opponent response... ");
    n = write(sock, buffer, strlen(buffer));
    if (n < 0)
        error("ERROR writing on socket... exiting");
    sleep(1);
    while(1)
    {
        if (cli_list[cli_id].request == -1)
            break;
    }
    if (cli_list[cli_id].in_game)
    {
        char port[10];
        bzero(buffer,256);
        strcpy(buffer,"Your request has been accepted.\nOpponent information :\nName : ");
        strcat(buffer,cli_list[op_id].login_name);
        strcat(buffer,"\tIP Address : ");
        strcat(buffer,cli_list[op_id].ip_addr);
        strcat(buffer,"\tPort No. : ");
        sprintf(port,"%u",cli_list[op_id].portno);
        strcat(buffer,port);
        strcat(buffer,"\n");
        n = write(sock, buffer, strlen(buffer));
        if (n < 0)
            error("ERROR writing on socket... exiting");
        return;
    }
    bzero(buffer,256);
    strcpy(buffer,"Your request has been denied.\n");
    n = write(sock, buffer, strlen(buffer));
    if (n < 0)
        error("ERROR writing on socket... exiting");
    return;
        
}

void exitGame(client *cli_list, int *numcli, int cli_id, int sock)
{
    int n;
    char buffer[256];
    *numcli = *numcli - 1;
    printf("%s has exited from the server.\n",cli_list[cli_id].login_name);
    cli_list[cli_id].in_use = 0;
    bzero(buffer,256);
    strcpy(buffer,"Exiting from game.\n");
    n = write(sock,buffer,strlen(buffer));
    if (n < 0)
        error("ERROR writing on socket... exiting");
    return;
}
