/* 
 * File:   server.c
 * Author: eddie
 *
 */

#include <stdio.h>      // for printf() and fprintf()
#include <stdlib.h>     // for atoi() and exit()
#include <sys/socket.h> // for PF_INET, SOCK_DGRAM, socket(), connect(), sendto(), and recvfrom()
#include <string.h>     // for memset() and strlen()
#include <arpa/inet.h>  // for IPPROTO_UDP, sockaddr_in and inet_addr()
#include "UtilsUDP.h"
#include "UtilsServer.h"

int main(int argc, char *argv[]) {
    int currentMaxUsers = 10; // Current maximum # of users the structure can hold
    int numUsers = 0; // Current number of registered users
    int userIndex; // Holder for the index location of a client
    char confirm; //used to confirm the default port
    char pNum[5]; //used to set a user defined port
    void *ptr; //used to just check what is the first bit of data

    int theSocket; // The socket file descriptor
    unsigned short serverPort; /* Server port */

    struct sockaddr_in theServerAddress; /* Local address */
    struct sockaddr_in theClientAddress; /* Client address */
    unsigned int clientAddressLength = sizeof (theClientAddress);

    NotifyMessage notifier; //the notification message
    ServerMessage outgoing; //the server message
    ClientMessage incoming; //the incoming client message
    pid_t processID;
    Client *users; // Data structure containing all users

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

    if ((theSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        dieWithError("socket() failed");
    }

    //create the address structure
    memset(&theServerAddress, 0, sizeof (theServerAddress)); // Zero out structure
    theServerAddress.sin_family = AF_INET; // Internet address family
    theServerAddress.sin_addr.s_addr = htonl(INADDR_ANY); // Any incoming interface
    theServerAddress.sin_port = htons(serverPort); // Local port

    //bind() to the address
    if (bind(theSocket,
            (struct sockaddr *) &theServerAddress,
            sizeof (theServerAddress))
            < 0) {
        dieWithError("bind() failed");
    }

    users = malloc(sizeof (Client) * currentMaxUsers); // Initialize the data structure for holding users

    int i; // Loop control
    processID = fork();
    //main looping
    //needs THREE threads, one to maintain the login and notifications thereof
    if (processID > 0) { //parent process
        for (;;) {
            recvfrom(theSocket, &ptr, sizeof (notifier), MSG_PEEK,
                    (struct sockaddr *) &theClientAddress, &clientAddressLength);
            switch ((int) ptr) {
                case NOTIFYMSG_TAG:
                    if (recvfrom(theSocket, &notifier, sizeof (notifier), 0,
                            (struct sockaddr *) &theClientAddress, &clientAddressLength) < 0) {
                        dieWithError("recvfrom() failed");
                    }
                    switch (notifier.messageType) {
                        case (NOTIFY):
                            break;
                        case (LOGIN):
                            // If the user already exists, log them in and send them a notification if
                            // they have new messages
                            if ((userIndex = getUserIndex(incoming.senderId, numUsers, users)) >= 0) {
                                users[userIndex].isLoggedIn = true;
                                users[userIndex].address = theClientAddress;

                                if (users[userIndex].hasNewMessages) {
                                    notifier.messageType = NOTIFY;
                                    notifier.clientID = users[userIndex].clientID;

                                    unsigned int notifySize = sizeof (ServerMessage);

                                    if (sendto(theSocket,
                                            &notifier,
                                            notifySize,
                                            0,
                                            (struct sockaddr *) &users[userIndex].address,
                                            sizeof (&users[userIndex].address))
                                            != notifySize) {
                                        dieWithError("sendto() sent a different number of bytes than expected");
                                    }
                                }
                                // Otherwise, create an account for them
                            } else {
                                if (numUsers == currentMaxUsers) {
                                    currentMaxUsers += USER_SIZE;
                                    users = realloc(users, sizeof (Client) * currentMaxUsers);
                                    // Make sure there wasn't an error reallocating memory
                                }
                                // Add them to the list of registered users
                                users[numUsers].clientID = incoming.senderId;
                                users[numUsers].address = theClientAddress;
                                users[numUsers].isLoggedIn = true;
                                users[numUsers].numMessages = 0;
                                users[numUsers].hasNewMessages = false;
                                users[numUsers].messageLimit = INCREMENT_MESSAGE;
                                users[numUsers].messages = malloc(
                                        sizeof (ServerMessage)
                                        * users[numUsers].messageLimit);
                                numUsers++;
                            }
                            break;
                        case (LOGOUT):
                            userIndex = getUserIndex(incoming.senderId, numUsers, users);
                            users[userIndex].isLoggedIn = false;
                            memset(&users[userIndex].address, 0, sizeof (users[userIndex].address));
                            break;
                    }
                    break;
                case CLIENTMSG_TAG:
                    if (recvfrom(theSocket, &incoming, sizeof (incoming), 0,
                            (struct sockaddr *) &theClientAddress, &clientAddressLength) < 0) {
                        dieWithError("recvfrom() failed");
                    }
                    switch (incoming.messageType) {
                        case (SEND):
                            // If the user has logged in or received a message than they are already in memory
                            // Get their index
                            if ((userIndex = getUserIndex(incoming.recipientId, numUsers, users)) >= 0) {

                                Client *messagedUser = &users[userIndex];

                                // If the receiving user has filled up their message buffer (default is 10 messages)
                                // Then allocate space for 10 more messages before saving their message
                                if (messagedUser->numMessages
                                        == messagedUser->messageLimit) {
                                    messagedUser->messageLimit += INCREMENT_MESSAGE;
                                    messagedUser->messages = realloc(messagedUser->messages,
                                            sizeof (ServerMessage)
                                            * messagedUser->messageLimit);
                                    // make sure there wasn't an error reallocating memory.
                                }

                                // Add the message to the user's data structure, increment their total
                                // number of messages and flag them as having new messages for notification
                                messagedUser->messages[messagedUser->numMessages] =
                                        convertMessage(&incoming);
                                messagedUser->numMessages++;
                                messagedUser->hasNewMessages = true;

                                // Send the user a notification of their message if they are logged in
                                if (messagedUser->isLoggedIn == true) {
                                    notifier.messageType = NOTIFY;
                                    notifier.clientID = messagedUser->clientID;

                                    unsigned int notifySize = sizeof (ServerMessage);

                                    if (sendto(theSocket,
                                            &notifier,
                                            notifySize,
                                            0,
                                            (struct sockaddr *) &messagedUser->address,
                                            sizeof (messagedUser->address))
                                            != notifySize) {
                                        dieWithError("sendto() sent a different number of bytes than expected");
                                    }
                                }

                                // The user has not yet logged in and this is their first received message.
                                // Create an entry for them in the data structure
                            } else {
                                // If the data structure is already full (default 10 users) then allocate
                                // space for 10 more users before adding the user to the data structure
                                if (numUsers == currentMaxUsers) {
                                    currentMaxUsers += USER_SIZE;
                                    users = realloc(users, sizeof (Client) * currentMaxUsers);
                                    // Make sure there wasn't an error reallocating memory
                                }
                                users[numUsers].clientID = incoming.recipientId;
                                memset(&users[numUsers].address, 0,
                                        sizeof (users[numUsers].address));
                                users[numUsers].isLoggedIn = false;
                                users[numUsers].messageLimit = INCREMENT_MESSAGE;
                                users[numUsers].messages = malloc(
                                        sizeof (ServerMessage)
                                        * users[numUsers].messageLimit);
                                users[numUsers].messages[0] = convertMessage(
                                        &incoming);
                                users[numUsers].numMessages = 1;
                                users[numUsers].hasNewMessages = true;
                                numUsers++;
                            }
                            break;
                        case (VIEW):
                            // Only look for messages if the user already exists
                            if ((userIndex = getUserIndex(incoming.senderId, numUsers, users)) >= 0) {
                                for (i = 0; i < users[userIndex].numMessages; i++) {
                                    // Send the user's messages
                                    if (sendto(theSocket,
                                            &users[userIndex].messages[i],
                                            sizeof users[userIndex].messages[i],
                                            0,
                                            (struct sockaddr *) &theClientAddress,
                                            clientAddressLength)
                                            != sizeof users[userIndex].messages[i]) {
                                        dieWithError(
                                                "sendto() sent a different number of bytes than expected");
                                    }
                                    users[userIndex].messages[i].messageType = OLD;
                                }
                                ServerMessage nullMessage;
                                nullMessage.messageType = NO_MESSAGE;
                                if (sendto(theSocket,
                                        &nullMessage,
                                        sizeof (nullMessage),
                                        0,
                                        (struct sockaddr *) &theClientAddress,
                                        clientAddressLength)
                                        != sizeof nullMessage) {
                                    dieWithError(
                                            "sendto() sent a different number of bytes than expected");
                                }
                                users[userIndex].hasNewMessages = false;
                            }
                            break;
                    }
            }
        }
    } else if (processID == 0) { //child process
        char cmd[99]; //whatever the user types in after the server starts
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
}