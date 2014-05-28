/* 
 * File:   server.c
 * Author: eddie
 *
 * Created on May 21, 2014, 9:57 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#define MESSAGE_SIZE 100 // Longest size of any message
#define NUM_USERS 10 // number of users in the messaging system
#define DEFAULT_PORT 24564 // the default port number

//typedef struct sockaddr_in sockaddr_in;

typedef struct {

    enum {
        Login, Notify, Logout
    } message_Type; // same size as an unsigned int
    unsigned int ClientId; // unique client identifier
} NotifyMessage; // an unsigned int is 32 bits = 4 bytes

typedef struct {

    enum {
        Send, Retrieve
    } request_Type; // same size as an unsigned int 
    unsigned int SenderId; // unique client identifier 
    unsigned int RecipientId; // unique client identifier 
    char message[MESSAGE_SIZE]; // text message
} ClientMessage; // an unsigned int is 32 bits = 4 bytes 

typedef struct {

    enum {
        New, Old, No_Message
    } messageType; // same size as an unsigned int
    unsigned int SenderId; //unique client identifier 
    unsigned int RecipientId; // unique client identifier
    char message[MESSAGE_SIZE]; // text message
} ServerMessage; // an unsigned int is 32 bits = 4 bytes

/*
 *
 */
void dieWithError(char *errorMessage) {
    perror(errorMessage);
    exit(EXIT_FAILURE);
}

/*
 * 
 */
int main(int argc, char** argv) {
    char cmd[99];
    char *pNum;
    char confirm;
    unsigned short serverPort; /* Server port */
    int theSocket; /* Socket */
    struct sockaddr_in theServerAddress; /* Local address */

    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned int cliAddrLen; /* Length of incoming message */
    char echoBuffer[MESSAGE_SIZE]; /* Buffer for echo string */
    int recvMsgSize; /* Size of received message */

    //check parameters, just port number, defaults to 24564
    if (argc != 2) {
        printf("Use default port (24564)? [y/n]: ");
        confirm = getchar();
        if (confirm == 'Y' || confirm == 'y') {
            serverPort = DEFAULT_PORT;
        } else {
            puts("Enter 5-digit port number (between 20000-30000): ");
            scanf("%s", &pNum);
            serverPort = atoi(&pNum);
        }
    } else {
        serverPort = atoi(argv[1]);
    }

    printf("Using port %i.\n", serverPort);

    //create socket
    if ((theSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        dieWithError("socket() failed");
    }

    //create the address structure
    memset(&theServerAddress, 
            0, 
            sizeof (theServerAddress)); /* Zero out structure */
    theServerAddress.sin_family = AF_INET; /* Internet address family */
    theServerAddress.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    theServerAddress.sin_port = htons(serverPort); /* Local port */

    //bind() to the address
    if (bind(theSocket,
            (struct sockaddr *) &theServerAddress,
            sizeof (theServerAddress)) < 0) {
        dieWithError("bind() failed");
    }

    //main looping
    //needs THREE threads, one to maintain the login and notifications thereof

    //one to send and receive messages (MAIN PROGRAM)

    //and one to allow the server to gracefully exit
    *cmd = "first";
    for (;;) {
        fgets(cmd, 100, stdin);
        //scanf("%s", &cmd);
        cmd[strlen(cmd) - 1] = '\0';
        if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "logout") == 0) {
            puts("Connection Closed.");
            return (EXIT_SUCCESS);
        } else if (strcmp(cmd, "first") != 0) {
            puts("Invalid Command.");
        }
    }

    return (EXIT_SUCCESS);
}

