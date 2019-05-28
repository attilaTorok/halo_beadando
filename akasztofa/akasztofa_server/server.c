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
#define MAX_SIZE_OF_CHARS_IN_WORD_TO_FIND_OUT 100
#define PIECES_OF_WOODS_GALLOWS_TREE_HAS 10

char buffer[BUFSIZE+1];

int listenForClient(int socket) {
    puts("Waiting for players to connect...");
    int err = listen(socket, 2);//2- backlog - hany feldolozatlan connect kerest tarol //kapcsolatelfogadasi szandek es queue meret beallitas
    if (err < 0) {
        error("Cannot listen to the socket: %s\n", strerror(errno));
        exit(3);
    }
    return err;
}

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



void bindToSocket(int fd, struct sockaddr_in server, int server_size) {
    puts("Binding to socket...");
    int err = bind(fd,(struct sockaddr *) &server, server_size);
    if (err < 0) {
        error("Cannot bind to the socket: %s\n", strerror(errno));
        exit(2);
    }
}

void sendMessageToPlayer(int fdPlayer, char* string) {
    sleep(1);
    int bytes = strlen(string) + 1;
    int trnmsize = send(fdPlayer, string, bytes, 0); //flags ~prioritas
    if (trnmsize < 0) {
        error("Cannot send data to client: %s\n", strerror(errno));
        exit(3);
    }
    else{
        printf("Sended value: %s \n", string);
    }
}

void onPlayer1Connected(int fdPlayer1) {
    puts("Player1 connected");
    sendMessageToPlayer(fdPlayer1, "Server is waiting for Player2 to connect.");
}

int acceptConnection(int fd, struct sockaddr_in client, int client_size) {
    int fdPlayer = accept(fd, (struct sockaddr *) &client, &client_size);
    if (fdPlayer < 0) {
        error("Cannot accept on socket: %s\n", strerror(errno));
        exit(4);
    }
    return fdPlayer;
}

const char *getMessageFromPlayer(int fdPlayer, const char buffer[100]) {
    int rcvsize = recv(fdPlayer, (void *) buffer, BUFSIZE, 0 );
    if (rcvsize < 0) {
        error("Cannot receive data from the socket: %s\n", strerror(errno));
        exit(4);
    }
    return buffer;
}

void getWordToFindOutFromPlayer(int fdPlayer, const char string[100]) {
    sendMessageToPlayer(fdPlayer, "Give me a word!");
    getMessageFromPlayer(fdPlayer, string);
}

void switchPlayers(int* player1, int* player2) {
    int temp;
    temp = *player1;
    *player1 = *player2;
    *player2 = temp;
}

bool wordToFindOutContainsChar(const char *buffer, char character) {
    for (int i = 0; i < strlen(buffer); ++i) {
        if(buffer[i]==character)
            return true;
    }
    return false;
}

void initializeWordToFindOutHidden(const char wordToFindOut[MAX_SIZE_OF_CHARS_IN_WORD_TO_FIND_OUT], char wordToFindOutHidden[MAX_SIZE_OF_CHARS_IN_WORD_TO_FIND_OUT]) {
    size_t wordLength = strlen(wordToFindOut);

    for (int i = 0; i < wordLength; ++i) {
        strcat(wordToFindOutHidden,"_");
    }
}

void formatMessage(char messageToFormat[MAX_SIZE_OF_CHARS_IN_WORD_TO_FIND_OUT], char formattedMessage[MAX_SIZE_OF_CHARS_IN_WORD_TO_FIND_OUT*2]) {
    size_t wordLength = strlen(messageToFormat);
    for (int i = 0; i < wordLength; ++i) {
        if(i!=wordLength-1){
            char character[3] = {};
            sprintf( character, "%c ", messageToFormat[i]);
            strcat(formattedMessage,character);
        }
        else{
            char character[2] = {};
            sprintf( character, "%c", messageToFormat[i]);
            strcat(formattedMessage,character);
        }
    }
}

void placeCharToWordToFindOutHiddenIfWordToFindOutContainsIt(char *wordToFindOutHidden, const char *wordToFindOut, char c) {
    for (int i = 0; i < strlen(wordToFindOutHidden); ++i) {
        if(wordToFindOut[i]==c){
            wordToFindOutHidden[i]=c;
        }
    }
}

bool isItNewFailedCharacter(char c, char characters[PIECES_OF_WOODS_GALLOWS_TREE_HAS+1]) {
    int len = strlen(characters);
    if(len != 0){
        for (int i = 0; i < len; ++i) {
            if(characters[i]==c)
                return false;
        }
        return true;
    }
    else{
        return true;
    }
}

void putCharacterIntoFailedCharacters(char c, char characters[PIECES_OF_WOODS_GALLOWS_TREE_HAS]) {
    characters[strlen(characters)]=c;
}

void startTheGame(int fdPlayer1, int fdPlayer2) {
    const char wordToFindOut [MAX_SIZE_OF_CHARS_IN_WORD_TO_FIND_OUT]={};
    getWordToFindOutFromPlayer(fdPlayer1, wordToFindOut);

    char wordToFindOutHidden [MAX_SIZE_OF_CHARS_IN_WORD_TO_FIND_OUT]={};
    initializeWordToFindOutHidden(wordToFindOut, wordToFindOutHidden);

    char formattedMessage[MAX_SIZE_OF_CHARS_IN_WORD_TO_FIND_OUT*2]={};

    formatMessage(wordToFindOutHidden, formattedMessage);

    sendMessageToPlayer(fdPlayer1, formattedMessage);
    sendMessageToPlayer(fdPlayer2, formattedMessage);

    int piecesOfWoodsWeHave = 0;
    char failedCharacters[PIECES_OF_WOODS_GALLOWS_TREE_HAS+1]={};

    char gallowsTreeState[MAX_SIZE_OF_CHARS_IN_WORD_TO_FIND_OUT+1]={};

    while(1){
        formattedMessage[0] = '\0';
        getMessageFromPlayer(fdPlayer2, buffer);
        if(!strcmp(buffer, "feladom")){
            sendMessageToPlayer(fdPlayer2, "You lost.");
            sendMessageToPlayer(fdPlayer1, "You won.");
            break;
        }
        placeCharToWordToFindOutHiddenIfWordToFindOutContainsIt(wordToFindOutHidden, wordToFindOut, buffer[0]);
        if(strlen(buffer) > 1 || !strcmp(wordToFindOutHidden, wordToFindOut) || piecesOfWoodsWeHave == PIECES_OF_WOODS_GALLOWS_TREE_HAS-1){
            if(!strcmp(buffer, wordToFindOut) || !strcmp(wordToFindOutHidden, wordToFindOut)){
                sendMessageToPlayer(fdPlayer1, "You lost.");
                sendMessageToPlayer(fdPlayer2, "You won.");
                break;
            }
            else{
                sendMessageToPlayer(fdPlayer2, "You lost.");
                sendMessageToPlayer(fdPlayer1, "You won.");
                break;
            }
        }
        else{
            printf("%c \n", buffer[0]);
            if(wordToFindOutContainsChar(wordToFindOut, buffer[0])){
                formatMessage(wordToFindOutHidden, formattedMessage);
            }
            else{
                if(isItNewFailedCharacter(buffer[0], failedCharacters)){
                    putCharacterIntoFailedCharacters(buffer[0], failedCharacters);
                    piecesOfWoodsWeHave++;
                    strcat(gallowsTreeState,"|");
                }
                formatMessage(gallowsTreeState, formattedMessage);
            }
            sendMessageToPlayer(fdPlayer1, formattedMessage);
            sendMessageToPlayer(fdPlayer2, formattedMessage);
        }
    }
}

void onPlayer2Connected(int fdPlayer1, int fdPlayer2) {
    puts("Player2 connected.");

    while(1){
        startTheGame(fdPlayer1, fdPlayer2);
        sendMessageToPlayer(fdPlayer2,"Do u wanna try again?");
        bool restart = false;
        while(1){
            getMessageFromPlayer(fdPlayer2, buffer);
            puts(buffer);
            if(!strcmp(buffer, "vége")){
                break;
            }
            if(!strcmp(buffer, "újra")){
                restart = true;
                break;
            }
            sendMessageToPlayer(fdPlayer2,"Do u wanna try again?");
        }
        switchPlayers(&fdPlayer1, &fdPlayer2);
        if(!restart)
            break;
    }

    sendMessageToPlayer(fdPlayer1, "The End");
    sendMessageToPlayer(fdPlayer2, "The End");
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
    fd =createSocket(&on); //visszateresi ertek: fajlleiro

    /* Binding socket */
    bindToSocket(fd, server, server_size);//celja a socket hozzarendelese a nevterhez

    /* Listening */
    listenForClient(fd);

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