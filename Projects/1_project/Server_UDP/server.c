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
#include <stdbool.h>

#define MESSAGE_SIZE 100 //Longest size of any message
#define MESSAGE_LIMIT 20 //max number of messages that a user can have.
#define NUM_USERS 10 //number of users in the messaging system
#define DEFAULT_PORT 24564 //the default port number
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
    int type;

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

typedef struct {
    struct sockaddr_in address; //the address for a client, used in all the real time work
    unsigned int clientID; //the unique user ID 
    int numMessages; //the number of messages the user has
    char *name; //the name of the user

    bool isLoggedIn;
    bool hasNewMessages;
    ServerMessage *messages; //the pointer to all of the client's messages

} Client;

/*
 *
 */
void dieWithError(char *errorMessage) {
    perror(errorMessage);
    exit(EXIT_FAILURE);
}

/*
 * Used to find the index of the specific user in the given array
 */
int getUserIndex(Client *users, unsigned int targetID) {
    int i;
    for (i = 0; i < NUM_USERS; i++) {
        if (users[i].clientID == targetID) {
            return i;
        }
    }
    return -1;
}

/*
 * 
 */
int main(int argc, char** argv) {
    char cmd[99]; //whatever the user types in after the server starts
    char pNum[5]; //used to set a user defined port
    char confirm; //used to confirm the default port
    int numUsers; //the current number of users
    int userIndex; //the currently active user

    unsigned short serverPort; /* Server port */
    int theSocket; /* Socket */
    unsigned int clientAddressLength; /* Length of incoming message */
    struct sockaddr_in theServerAddress; /* Local address */
    struct sockaddr_in theClientAddress; /* Client address */

    ClientMessage currentMessage; //the client message
    NotifyMessage notifier; //the notification message
    ServerMessage outgoing; //the server message
    pid_t processID;
    Client *users; //the epic data structure for storing messages

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

    clientAddressLength = sizeof (theClientAddress);

    processID = fork();
    //main looping
    //needs THREE threads, one to maintain the login and notifications thereof
    if (processID > 0) { //parent process
        users = calloc(NUM_USERS * sizeof (Client));
        //        memset(&notifier, 0, sizeof (notifier));
        //        memset(&theClientAddress, 0, sizeof (theClientAddress));
        //one to send and receive messages (MAIN PROGRAM)
        for (;;) {
            //used to check what type of data it is
            recvfrom(theSocket, &ptr, sizeof (notifier), MSG_PEEK,
                    (struct sockaddr *) &theClientAddress, &clientAddressLength);
            switch ((int) ptr) {
                case NOTIFYMSG_TAG:
                    recvfrom(theSocket, &notifier, sizeof (notifier), 0,
                            (struct sockaddr *) &theClientAddress, &clientAddressLength);
                    userIndex = getUserIndex(users, notifier.clientId);
                    switch (notifier.messageType) {
                        case LOGIN:
                            if (userIndex < 0) { //the user doesn't exist, make a new one
                                if (numUsers < NUM_USERS) {
                                    users[numUsers].clientID = notifier.clientId;
                                    users[numUsers].numMessages = 0;
                                    users[numUsers].hasNewMessages = false;
                                    users[numUsers].messages = calloc(
                                            sizeof (ServerMessage)
                                            * MESSAGE_LIMIT);
                                    userIndex = numUsers++;
                                }
                            } else { //
                                
                            }
                            users[userIndex].isLoggedIn = true;
                            users[userIndex].address = theClientAddress;

                            notifier.type = NOTIFYMSG_TAG;
                            notifier.messageType = NOTIFY;

                            if (sendto(theSocket,
                                    &notifier,
                                    sizeof (notifier),
                                    0,
                                    (struct sockaddr *) &users[userIndex].address,
                                    sizeof (users[userIndex].address))
                                    != sizeof (notifier)) { //returns data to confirm user log in 
                                dieWithError("sendto() sent a different number of bytes than expected");
                            }
                            break;
                        case LOGOUT:
                            break;
                    }
                    printf("UserID: %i logged in\n", notifier.clientId);
                    break;
                case CLIENTMSG_TAG:
                    recvfrom(theSocket, &currentMessage, sizeof (currentMessage), 0,
                            (struct sockaddr *) &theClientAddress, &clientAddressLength);
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

