This is the first assignment of CS425 in the 1st semester of academic year 2011-12.
A game server is created to handle some basic client request.
The assignment consist of two files namily : client.c and server.c
The file server.c contains code for game server while client.c contains code for client side.
Various functions present in server.c are :
childpro() : It handles the client after a new process has been created.
addclient() : It logins the client in the server according to some criteria.
printlist() : Prints the list of online player.
checkRequest() : Checks if you have any game request from another client.
acceptRequest() : Function to handle the game request.
sendRequest() : Function to send a game request to another client if you do not have any pending request.
exitGame() : to exit the game server.

Server starts another process within the master process using fork() whenever a new client connects. Here the concept of shared memory is utilised to store the information of the clients and avoid duplicacy and redundancy of the information in various process.

Client is just a basic client which request various operation based on the data it receives.

To run the server,you have to follow below commands :
$gcc -o server server.c
$./server <portno>
portno is the port number you want to assign the server.

To run the client,you have to follow below commands :
$gcc -o client client.c
$./client <server ip> <portno>
server ip is the ip address of the server and portno is the port number of server.
