/***************************************/
/*             TCP client              */
/***************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>

#define BUFSIZE 1024
#define PORT_NO 2001
#define error(a,b) fprintf(stderr, a, b)

char buffer[BUFSIZE+1];

int main(int argc, char *argv[] ) {// arg count, arg vector
    bool amIPlayer1 = false;

    /* Declarations */
    int fd;	                       // socket endpt
    int flags;                      // rcv flags
    struct sockaddr_in server;	     // socket name (addr) of server
    struct sockaddr_in client;	     // socket name of client
    int server_size;                // length of the socket addr. server
    int client_size;                // length of the socket addr. client
    int bytes;    	                 // length of buffer
    int rcvsize;                    // received bytes
    int trnmsize;                   // transmitted bytes
    int err;                        // error code
    int ip;							// ip address
    char on;                        //
    char server_addr[16];           // server address

    /* Initialization */
    on    = 1;
    flags = 0;
    server_size = sizeof server;
    client_size = sizeof client;

    ip = inet_addr(argv[1]);
    server.sin_family      = AF_INET;
    server.sin_addr.s_addr = ip;
    server.sin_port        = htons(PORT_NO);


    sprintf(buffer,"%s","");

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        error("%s: Socket creation error.\n",argv[0]);
        exit(1);
    }

    /* Setting socket options */
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof on);
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof on);

    /* Connecting to the server */
    err = connect(fd, (struct sockaddr *) &server, server_size);
    if (err < 0) {
        printf("Oh dear, something went wrong with read()! %s\n", strerror(errno));
        error("%s: Cannot connect to the server.\n", argv[0]);
        exit(2);
    }

    while(1){
        rcvsize = recv( fd, buffer, BUFSIZE, flags );
        if (rcvsize < 0) {
            error("%s: Cannot receive data from the socket.\n", argv[0]);
            exit(4);
        }
        if(!strcmp(buffer, "The End")){
            break;
        }

        printf("%s \n", buffer);

        if(!strcmp(buffer, "Give me a word!")){
            amIPlayer1 = true;
            scanf("%s",buffer);
            bytes = strlen(buffer)+1;
            trnmsize = send(fd, buffer, bytes, flags);
            if (trnmsize < 0) {
                error("%s: Cannot send data to server.\n", argv[0]);
                exit(3);
            }
        }

        if( strcmp(buffer, "Server is waiting for Player2 to connect.") && strcmp(buffer, "You lost.") && strcmp(buffer, "You won.") && !amIPlayer1){
            scanf("%s",buffer);
            bytes = strlen(buffer)+1;
            trnmsize = send(fd, buffer, bytes, flags);
            if (trnmsize < 0) {
                error("%s: Cannot send data to server.\n", argv[0]);
                exit(3);
            }
        }
        if(!strcmp(buffer, "You lost.") || !strcmp(buffer, "You won.")){
            if(amIPlayer1){
                amIPlayer1 = !amIPlayer1;
            }
        }
    }

    /* Closing sockets and quit */
    close(fd);
    exit(0);
}