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
#include <signal.h>

#define MESSAGE_SIZE 100 // Longest size of any message
#define NUM_USERS 10 // number of users in the messaging system
#define DEFAULT_PORT 24564 // the default port number
#define NOTIFYMSG_TAG 'N'
#define SERVERMSG_TAG 'S'
#define CLIENTMSG_TAG 'C'

//typedef struct sockaddr_in sockaddr_in;

typedef struct {
    int type;

    enum {
        LOGIN, NOTIFY, LOGOUT
    } messageType; //same size as an unsigned int
    unsigned int clientId; //unique client identifier
} NotifyMessage; //an unsigned int is 32 bits = 4 bytes

typedef struct {

    enum {
        SEND, VIEW
    } requestType; //same size as an unsigned int 
    unsigned int senderId; //unique client identifier 
    unsigned int recipientId; //unique client identifier 
    char message[MESSAGE_SIZE]; //text message
} ClientMessage; //an unsigned int is 32 bits = 4 bytes 

typedef struct {
    int type;

    enum {
        NEW, OLD, NO_MESSAGE
    } messageType; //same size as an unsigned int
    unsigned int senderId; //unique client identifier 
    unsigned int recipientId; //unique client identifier
    char message[MESSAGE_SIZE]; //text message
} ServerMessage; //an unsigned int is 32 bits = 4 bytes

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
    char cmd[99]; //whatever the user types in after the server starts
    char pNum[5]; //used to set a user defined port
    char confirm; //used to confirm the default port

    unsigned short serverPort; /* Server port */
    int theSocket; /* Socket */
    unsigned int clientAddressLength; /* Length of incoming message */
    struct sockaddr_in theServerAddress; /* Local address */
    struct sockaddr_in theClientAddress; /* Client address */

    ClientMessage currentMessage;
    NotifyMessage notifier;
    ServerMessage outgoing;
    pid_t processID;

    void *ptr; //used to just check what is the first bit of data
    char echoBuffer[MESSAGE_SIZE]; /* Buffer for echo string */
    int recvMsgSize; /* Size of received message */

    //check parameters, just port number, defaults to 24564
    if (argc != 2) {
        puts("# Use default port (24564)? [y/n]:");
        printf("> ");
        confirm = getchar();
        if (confirm == 'Y' || confirm == 'y') {
            serverPort = DEFAULT_PORT;
        } else {
            puts("# Enter 5-digit port number (between 20000-30000):");
            printf("> ");
            scanf("%s", pNum);
            serverPort = atoi(pNum);
        }
    } else {
        serverPort = atoi(argv[1]);
    }

    printf("# Using port %i.\n", serverPort);

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

    processID = fork();
    //main looping
    //needs THREE threads, one to maintain the login and notifications thereof
    if (processID > 0) { //parent process
        memset(&notifier, 0, sizeof (notifier));
        memset(&theClientAddress, 0, sizeof (theClientAddress));
        clientAddressLength = sizeof (theClientAddress);
        //one to send and receive messages (MAIN PROGRAM)
        for (;;) {
            printf("# ");
            recvfrom(theSocket, &ptr, sizeof (notifier), MSG_PEEK,
                    (struct sockaddr *) &theClientAddress, &clientAddressLength);
            switch ((int) ptr) {
                case NOTIFYMSG_TAG:
                    recvfrom(theSocket, &notifier, sizeof (notifier), 0,
                            (struct sockaddr *) &theClientAddress, &clientAddressLength);
                    printf("\n\n-NEW INCOMING MESSAGE-\n\n");
                    printf("UserID: %i\n", notifier.clientId);
                    break;
                case CLIENTMSG_TAG:
                    break;
            }
            if ((int) ptr == NOTIFYMSG_TAG) {

                printf("\n\n-NEW INCOMING MESSAGE-\n\n");
            }
            //            if ((recvMsgSize = recvfrom(theSocket, &notifier, sizeof (notifier), 0,
            //                    (struct sockaddr *) &theClientAddress, &clientAddressLength)) < 0) {
            //                dieWithError("recvfrom() failed");
            //            }
            /* Block until receive message from a client */
            //            if ((recvMsgSize = recvfrom(theSocket, &currentMessage, sizeof (currentMessage), 0,
            //                    (struct sockaddr *) &theClientAddress, &clientAddressLength)) < 0) {
            //                dieWithError("recvfrom() failed");
            //            }
        }
    } else if (processID == 0) {
        //and one to allow the server to gracefully exit
        for (;;) {
            fgets(cmd, 100, stdin);
            cmd[strlen(cmd) - 1] = '\0';
            if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "logout") == 0) {
                puts("# Connection Closed.");
                kill(processID, SIGTERM); //exits the program
            } else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
                puts("# It's working. 'exit' and 'logout' are used to quit.");
            } else if (strcmp(cmd, "\0") != 0) {
                puts("# Invalid Command.");
            }
            printf("> ");
        }
    } else { //bad fork
        dieWithError("fork() failed");
    }

    return (EXIT_SUCCESS);
}

