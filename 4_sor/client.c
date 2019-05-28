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
int lastChosenCol;
char mySymbol;
char opSymbol;

typedef struct Table {
    int rows;
    int columns;
    char grid[6][7];
    int top[7];
} Table;

void initTable(Table* TABLE) {
    TABLE->rows = 6;
    TABLE->columns = 7;
    for (int i = 0; i < 6; i++) {
        TABLE->top[i] = 6;
    }

    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 7; j++) {
            TABLE->grid[i][j] = '-';
        }
    }
    
}

int putAnswerToGrid(Table* TABLE, int col, char c) {
    if (TABLE->top[col] > 0) {
        TABLE->grid[TABLE->top[col] - 1][col] = c;
        TABLE->top[col]--;

        return 1;
    }

    return 0;
}

void showTable(Table TABLE) {
    for (int i = -1; i < TABLE.rows; i++) {
        if (i != -1) {
            printf("\n|");
        }

        for (int j = 0; j < TABLE.columns; j++) {
            if (i == -1) {
                printf(" _");
            } else if (i > -1 && i < TABLE.columns) {
                printf("%c ", TABLE.grid[i][j]);
            }
        }

        if (i != -1) {
            printf("|");
        }

        if (i + 1 == TABLE.rows) {
            printf("\n");
            for (int j = 1; j <= TABLE.columns; j++) {
                printf(" %d", j);
            }
            printf("\n");
        }
    }
}

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

    Table TABLE;
    initTable(&TABLE);

    while (1) {
        rcvsize = recv( fd, buffer, BUFSIZE, flags );
        if (rcvsize < 0) {
            error("%s: Cannot receive data from the socket.\n", argv[0]);
            exit(4);
        }

        printf("%s \n", buffer);

        if (!strcmp(buffer, "Choose a column.")) {
        	printf(">");
            scanf("%s", buffer);
            lastChosenCol = atoi(buffer) - 1;
            bytes = strlen(buffer) + 1;
            trnmsize = send(fd, buffer, bytes, flags);
            if (trnmsize < 0) {
                error("%s: Cannot send data to server.\n", argv[0]);
                exit(3);
            }
        }

        if (!strcmp(buffer, "O is your symbol.")) {
            mySymbol = 'O';
            opSymbol = 'X';
            showTable(TABLE);
        }

        if (!strcmp(buffer, "X is your symbol.")) {
            mySymbol = 'X';
            opSymbol = 'O';
        }

        if (!strcmp(buffer, "Answer was valid.")) {
            putAnswerToGrid(&TABLE, lastChosenCol, mySymbol);
        }

        if (!strcmp(buffer, "Opponent answer incoming.")) {
            rcvsize = recv( fd, buffer, BUFSIZE, flags );
            if (rcvsize < 0) {
                error("%s: Cannot receive data from the socket.\n", argv[0]);
                exit(4);
            }

            int col = atoi(buffer);
            putAnswerToGrid(&TABLE, col - 1, opSymbol);
            showTable(TABLE);
        }

        if (!strcmp(buffer, "You Won.") || !strcmp(buffer, "You Lost.")) {
            showTable(TABLE);
        }

        if (!strcmp(buffer, "The End")) {
            break;
        }

    }

    /* Closing sockets and quit */
    close(fd);
    exit(0);
}
