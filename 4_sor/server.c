/***************************************/
/*              TCP server             */
/***************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>

#define BUFSIZE 1024                      // buffer size
#define PORT_NO 2001                      // port number
#define error(a,b) fprintf(stderr, a, b)  // error 'function'

char buffer[BUFSIZE+1];
int scores[2];

typedef struct Table {
    int rows;
    int columns;
    char grid[6][7];
    int top[7];
} Table;

//////////////////////////////////////////////////////////////////////////

void settingSocketOptions(int fd, char** on) {
    puts("Setting socket options...");
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof on);//ha bontjuk a kapcsolatot es ujracsatlakozunk, akkor ujrahasznaljuk a portot -> elkerulhetjuk vele a "port already in use" hibauzenetet
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof on);//The SO_KEEPALIVE socket option is designed to allow an application to enable keep-alive packets for a socket connection.
}

int createSocket(char* on){

    puts("Creating socket...");
    int fd = socket(AF_INET, SOCK_STREAM, 0 );//sock_stream -> tcp, sock_dgram -> udp
    if (fd < 0) {
        error("Socket creation error: %s\n", strerror(errno));
        exit(1);
    }

    settingSocketOptions(fd, &on);
    return fd;
}

int listenForClient(int socket) {
    puts("Waiting for players to connect...");
    int err = listen(socket, 2);//2- backlog - hany feldolozatlan connect kerest tarol //kapcsolatelfogadasi szandek es queue meret beallitas
    if (err < 0) {
        error("Cannot listen to the socket: %s\n", strerror(errno));
        exit(3);
    }
    return err;
}

void bindToSocket(int fd, struct sockaddr_in server, int server_size) {
    puts("Binding to socket...");
    int err = bind(fd,(struct sockaddr *) &server, server_size);
    if (err < 0) {
        error("Cannot bind to the socket: %s\n", strerror(errno));
        exit(2);
    }
}

int acceptConnection(int fd, struct sockaddr_in client, int client_size) {
    int fdPlayer = accept(fd, (struct sockaddr *) &client, &client_size);
    if (fdPlayer < 0) {
        error("Cannot accept on socket: %s\n", strerror(errno));
        exit(4);
    }
    return fdPlayer;
}


///////////////////////////////////////////////////////////////////////


void sendMessageToPlayer(int fdPlayer, char* string) {
    sleep(1);
    int bytes = strlen(string) + 1;
    int trnmsize = send(fdPlayer, string, bytes, 0); //flags ~prioritas
    if (trnmsize < 0) {
        error("Cannot send data to client: %s\n", strerror(errno));
        exit(3);
    }
    else {
        printf("Sended value: %s \n", string);
    }
}

const char *getMessageFromPlayer(int fdPlayer, const char buffer[100]) {
    int rcvsize = recv(fdPlayer, (void *) buffer, BUFSIZE, 0 );
    if (rcvsize < 0) {
        error("Cannot receive data from the socket: %s\n", strerror(errno));
        exit(4);
    }
    return buffer;
}

void switchPlayers(int* player1, int* player2) {
    int temp;
    temp = *player1;
    *player1 = *player2;
    *player2 = temp;
}

void readScores() {
	FILE * pFile;
	pFile = fopen ("result", "r");
	if (pFile != NULL) {
		fscanf(pFile, "%d %d", &scores[0], &scores[1]);
		printf("scores: Player1: %d\tPlayer2: %d\n", scores[0], scores[1]);
		fclose (pFile);
	}
}

void writeScores() {
	FILE * pFile;
	pFile = fopen ("result", "w");
	if (pFile != NULL) {
		fprintf(pFile, "%d %d", scores[0], scores[1]);
		fclose (pFile);
	}
}

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

int playerTurn(int fdPlayer1, int fdPlayer2, char c, Table* TABLE) {
	while (1) {
		sendMessageToPlayer(fdPlayer1, "Choose a column.");
		getMessageFromPlayer(fdPlayer1, buffer);

		if (!strcmp(buffer, "feladom")) {
			return -1;
		}

		int answer = atoi(buffer);
		if (answer && answer > 0 && answer < 8) {
			if (putAnswerToGrid(TABLE, answer-1, c)) {
				sendMessageToPlayer(fdPlayer1, "Answer was valid.");
				sendMessageToPlayer(fdPlayer2, "Opponent answer incoming.");
				sendMessageToPlayer(fdPlayer2, buffer);
				return 1;
			} else {
				sendMessageToPlayer(fdPlayer1, "Answer was invalid.");
			}			
		} else {
			sendMessageToPlayer(fdPlayer1, "Answer was invalid.");
		}
	}
}

int checkHorizontally(Table* TABLE, int row, int col, char c) {
	int count = 1;
	for (int j = col + 1; j < 7; j++) {
		if (TABLE->grid[row][j] == c) {
			count++;
		} else {
			break;
		}
	}

	if (count >= 4) {
		return 1;
	} else {
		return 0;
	}
}

int checkVertically(Table* TABLE, int row, int col, char c) {
	int count = 1;
	for (int j = row + 1; j < 6; j++) {
		if (TABLE->grid[j][col] == c) {
			count++;
		} else {
			break;
		}
	}

	if (count >= 4) {
		return 1;
	} else {
		return 0;
	}
}

int checkDiagonallyRight(Table* TABLE, int row, int col, char c) {
	int count = 1;
	for (int j = row + 1, i = col + 1; j < 6, i < 7; j++, i++) {
		if (TABLE->grid[j][i] == c) {
			count++;
		} else {
			break;
		}
	}

	if (count >= 4) {
		return 1;
	} else {
		return 0;
	}
}

int checkDiagonallyLeft(Table* TABLE, int row, int col, char c) {
	int count = 1;
	for (int j = row + 1, i = col - 1; j < 6, i > -1; j++, i--) {
		if (TABLE->grid[j][i] == c) {
			count++;
		} else {
			break;
		}
	}

	if (count >= 4) {
		return 1;
	} else {
		return 0;
	}
}

int checkIfWon(Table* TABLE, char c) {
	for (int row = 0; row < 6; row++) {
		for (int col = 0; col < 7; col++) {
			if (TABLE->grid[row][col] == c) {
				if (checkHorizontally(TABLE, row, col, c)) {
					printf("%c won horizontally\n", c);
					return 1;
				}

				if (checkVertically(TABLE, row, col, c)) {
					printf("%c won vertically\n", c);
					return 1;
				}

				if (checkDiagonallyRight(TABLE, row, col, c)) {
					printf("%c won diagonallyRight\n", c);
					return 1;
				}

				if (checkDiagonallyLeft(TABLE, row, col, c)) {
					printf("%c won DiagonallyLeft\n", c);
					return 1;
				}
			}
		}
	}

	return 0;
}

int tableIsFull(Table TABLE) {
	int count = 0;

	for (int i = 0; i < 7; i++) {
		if (TABLE.top[i] == 0) {
			count++;
		}
	}

	if (count == 7) {
		return 1;
	}

	return 0;
}

void startTheGame(int fdPlayer1, int fdPlayer2) {
	Table TABLE;
	initTable(&TABLE);

	sendMessageToPlayer(fdPlayer1, "O is your symbol.");
	sendMessageToPlayer(fdPlayer2, "X is your symbol.");

    while (1) {
    	if (playerTurn(fdPlayer1, fdPlayer2, 'O', &TABLE) == -1) {
    		sendMessageToPlayer(fdPlayer1, "You Lost.");
    		sendMessageToPlayer(fdPlayer2, "You Won.");
    		scores[1] += 1;
    		break;
    	}

    	if (checkIfWon(&TABLE, 'O')) {
    		showTable(TABLE);
    		sendMessageToPlayer(fdPlayer1, "You Won.");
    		sendMessageToPlayer(fdPlayer2, "You Lost.");
    		scores[0] += 1; 
    		break;
    	}

    	if (tableIsFull(TABLE)) {
    		sendMessageToPlayer(fdPlayer1, "Draw.");
    		sendMessageToPlayer(fdPlayer2, "Draw.");
    		break;
    	}

    	///////////////////////////////////////////////////////////

    	if (playerTurn(fdPlayer2, fdPlayer1, 'X', &TABLE) == -1) {
    		sendMessageToPlayer(fdPlayer2, "You Lost.");
    		sendMessageToPlayer(fdPlayer1, "You Won.");
    		scores[0] += 1;
    		break;
    	}

    	if (checkIfWon(&TABLE, 'X')) {
    		showTable(TABLE);
    		sendMessageToPlayer(fdPlayer2, "You Won.");
    		sendMessageToPlayer(fdPlayer1, "You Lost.");
    		scores[1] += 1; 
    		break;
    	}

    	if (tableIsFull(TABLE)) {
    		sendMessageToPlayer(fdPlayer1, "Draw.");
    		sendMessageToPlayer(fdPlayer2, "Draw.");
    		break;
    	}
    }
}

void onPlayer1Connected(int fdPlayer1) {
    puts("Player1 connected");
    sendMessageToPlayer(fdPlayer1, "Server is waiting for Player2 to connect.");
}

void onPlayer2Connected(int fdPlayer1, int fdPlayer2) {
    puts("Player2 connected.");

    startTheGame(fdPlayer1, fdPlayer2);

    sendMessageToPlayer(fdPlayer1, "The End");
    sendMessageToPlayer(fdPlayer2, "The End");

    puts("All time scores:");
    printf("Player1: %d\tPlayer2: %d\n", scores[0], scores[1]);

    writeScores();
}

int main(int argc, char *argv[] ){ 	// arg count, arg vector

    /* Declarations */
    int fd;	        	           		// socket endpt
    int fdPlayer1;                        	// socket endpt
    int fdPlayer2;                        	// socket endpt
    int flags;                      	// rcv flags
    struct sockaddr_in server;      	// socket name (addr) of server
    struct sockaddr_in client;	     	// socket name of client
    int server_size;                	// length of the socket addr. server
    int client_size;                	// length of the socket addr. client
    int bytes;		           					// length of buffer
    int rcvsize;                    	// received bytes
    int trnmsize;                   	// transmitted bytes
    int err;                        	// error code
    char on;                        	//

    /* Initialization */
    on                     = 1;
    flags                  = 0;
    bytes                  = BUFSIZE;
    server_size            = sizeof server;
    client_size            = sizeof client;
    server.sin_family      = AF_INET;//cim csalad
    server.sin_addr.s_addr = INADDR_ANY; //internet ip cim hálózati byte sorrendben
    server.sin_port        = htons(PORT_NO);//konvertalo fuggveny, host to network short - network byte sorrend


    sprintf(buffer,"%s","");

    /* Creating socket */
    fd = createSocket(&on); //visszateresi ertek: fajlleiro

    /* Binding socket */
    bindToSocket(fd, server, server_size);//celja a socket hozzarendelese a nevterhez

    /* Listening */
    listenForClient(fd);

    readScores();

    /* Accepting connection request */
    fdPlayer1 = acceptConnection(fd, client, client_size); //visszateresi ertek: fajlleiro
    onPlayer1Connected(fdPlayer1);
    fdPlayer2 = acceptConnection(fd, client, client_size);//visszateresi ertek: fajlleiro
    onPlayer2Connected(fdPlayer1, fdPlayer2);

    close(fdPlayer1);
    close(fdPlayer2);
    close(fd);
    exit(0);
} 