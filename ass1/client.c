#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

void acceptRequest (int);
void sendRequest (int);

int main(int argc, char *argv[])
{
    int sockfd, portno, n,i;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char check[100];
    char buffer[256];

    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");


    bzero(buffer,256);
    n = read(sockfd,buffer,255);
    if (n<0)
         error("Error reading from socket");
    printf("%s",buffer);
    bzero(buffer,256);
    fgets(buffer,255,stdin);
    n = write(sockfd,buffer,strlen(buffer)-1);
    if (n < 0) 
         error("ERROR writing to socket");
    bzero(buffer,256);
    n = read(sockfd,buffer,255);
    if (n < 0) 
         error("ERROR reading from socket");
    printf("%s",buffer);
    for ( i =0 ;i < 99; i++)
    {
         if (buffer[i]=='\n')
              break;
         check[i] = buffer[i];
    }
    check[i]='\0';
    if ( strcmp(check,"Rejected") == 0)
    {
         close(sockfd);
         return 0;
    }
    while(1)
    {
         bzero(buffer,256);
         n = read(sockfd,buffer,255);
         if (n < 0)
              error("ERROR reading from socket");
         printf("%s",buffer);
         bzero(buffer,256);
         fgets(buffer,255,stdin);
         n = write(sockfd,buffer,strlen(buffer)-1);
         if (n < 0)
              error("ERROR writing to socket");
         if ((buffer[0] == 'L') || (buffer[0] == 'l'))
         {
              bzero(buffer,256);
              n = read(sockfd,buffer,255);
              if (n < 0)
                   error("ERROR reading from socket... exiting");
              printf("%s",buffer);
              bzero(buffer,256);
              n = read(sockfd,buffer,255);
              if (n < 0)
                   error("ERROR reading from socket... exiting");
              if (strcmp(buffer,"-1") == 0);
              else
                   acceptRequest(sockfd);
         }
         else if ((buffer[0] == 'R') || (buffer[0] == 'r'))
              sendRequest(sockfd);
         else if ((buffer[0] == 'C') || (buffer[0] == 'c'))
         {
              bzero(buffer,256);
              n = read(sockfd,buffer,255);
              if (n < 0)
                   error("ERROR reading from socket... exiting");
              if (strcmp(buffer,"-1") == 0);
              else
                   acceptRequest(sockfd);
         }
         else if ((buffer[0] == 'E') || (buffer[0] == 'e'))
         {
              bzero(buffer,256);
              n = read(sockfd,buffer,255);
              if (n < 0)
                   error("ERROR reading from socket... exiting");
              printf("%s",buffer);
              break;
         }
    }
    close(sockfd);
    return 0;
}

void acceptRequest(int sock)
{
    int n;
    char buffer[256];
    bzero(buffer,256);
    n = read(sock, buffer, 255);
    if (n < 0)
        error("ERROR reading from socket");
    printf("%s",buffer);
    bzero(buffer,256);
    fgets(buffer,255,stdin);
    n = write(sock, buffer, strlen(buffer) - 1);
    if (n < 0)
        error("ERROR writing on socket");
    if ((buffer[0] == 'y') || (buffer[0] == 'Y'))
    {
        bzero(buffer,256);
        n = read(sock, buffer, 255);
        if (n < 0)
            error("ERROR reading from socket");
        printf("%s",buffer);
        exit(0);
    }
    else if ((buffer[0] == 'n') || (buffer[0] == 'N'))
    {
        bzero(buffer,256);
        n = read(sock, buffer, 255);
        if (n < 0)
            error("ERROR reading from socket");
        printf("%s",buffer);
    }
    return;

}

void sendRequest(int sock)
{
    int n;
    char buffer[256];
    bzero(buffer,256);
    n = read(sock, buffer, 255);
    if (n < 0)
        error("ERROR reading from socket");
    printf("%s",buffer);
    bzero(buffer,256);
    n = read(sock, buffer, 255);
    if (n < 0)
        error("ERROR reading from socket");
    printf("%s",buffer);
    if (strcmp(buffer,"-1") == 0);
    else
        acceptRequest(sock);
    printf("Enter the name of client with which you want to play : ");
    bzero(buffer,256);
    fgets(buffer,255,stdin);
    n = write(sock, buffer, strlen(buffer) - 1);
    if (n < 0)
        error("ERROR writing on socket");
    bzero(buffer,256);
    n = read(sock, buffer, 255);
    if (n < 0)
        error("ERROR reading from socket");
    if (buffer[strlen(buffer)-1] == '\n')
    {
        printf("%s",buffer);
        return;
    }
    bzero(buffer,256);
    n = read(sock, buffer, 255);
    if (n < 0)
        error("ERROR reading from socket");
    printf("%s",buffer);
    if (strlen(buffer) > 35)
        exit(0);
    return;
}
