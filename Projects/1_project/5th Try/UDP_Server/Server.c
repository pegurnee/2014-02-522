/* 
 * File:   server.c
 * Author: eddie
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "UtilsUDP.h"
#include "UtilsServer.h"
#include "Colors.h"

int main(int argc, char *argv[]) {
    int numUsers = 0; //current number of users
    int userLimit = 10; //the user limit, will adjust when needed
    int userIndex; //used when addressing users in the data structure
    char confirm; //used to confirm the default port
    char pNum[5]; //used to set a user defined port
    void *ptr; //used to just check what is the first bit of data

    int theSocket; //the socket
    unsigned short serverPort; /* Server port */

    struct sockaddr_in theServerAddress; /* Local address */
    struct sockaddr_in theClientAddress; /* Client address */
    unsigned int clientAddressLength = sizeof (theClientAddress);

    NotifyMessage notifier; //the notification message
    ServerMessage outgoing; //the server message
    ClientMessage incoming; //the incoming client message
    pid_t processID;
    Client *users; //the epic data structure with users

    notifier.type = NOTIFYMSG_TAG;
    outgoing.type = SERVERMSG_TAG;
    incoming.type = CLIENTMSG_TAG;

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
            printf("# Just kidding, you don't get to choose.");
        }
    } else {
        serverPort = atoi(argv[1]);
    }

    serverPort = DEFAULT_PORT;
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

    users = calloc(userLimit, sizeof (Client)); //memmory for data structure

    processID = fork();
    //main looping
    //needs two threads, one to maintain the login and user actions
    if (processID > 0) { //parent process
        int i;
        for (;;) {
            //just the tag
            recvfrom(theSocket, &ptr, sizeof (notifier), MSG_PEEK,
                    (struct sockaddr *) &theClientAddress, &clientAddressLength);

            switch ((int) ptr) {
                case NOTIFYMSG_TAG:
                    //get it all
                    if (recvfrom(theSocket, &notifier, sizeof (notifier), 0,
                            (struct sockaddr *) &theClientAddress, &clientAddressLength) < 0) {
                        dieWithError("recvfrom() failed");
                    }
                    switch (notifier.messageType) {
                        case NOTIFY:
                            break;
                        case LOGIN:
                            userIndex = getUserIndex(notifier.clientID, numUsers, users);

                            if (userIndex >= 0) { //the user exists in the data structure
                                users[userIndex].isLoggedIn = true;
                                users[userIndex].address = theClientAddress;

                                //send notification if they've got a waiting message
                                if (users[userIndex].hasNewMessages) {
                                    notifier.type = NOTIFYMSG_TAG;
                                    notifier.messageType = NOTIFY;
                                    notifier.clientID = users[userIndex].clientID;

                                    if (sendto(theSocket,
                                            &notifier,
                                            sizeof (notifier),
                                            0,
                                            (struct sockaddr *) &users[userIndex].address,
                                            sizeof (users[userIndex].address))
                                            != sizeof (notifier)) {
                                        dieWithError("sendto() sent a different number of bytes than expected");
                                    }
                                }
                            } else {
                                //sets the userIndex to the last place in the data structure
                                userIndex = numUsers;
                                //if data structure is at capacity
                                if (numUsers == userLimit) {
                                    //increment the user limit
                                    userLimit += INCREMENT_USERS;
                                    //alloc more space
                                    users = realloc(users, sizeof (Client) * userLimit);
                                }

                                //establishes the basic user stuff
                                users[userIndex].address = theClientAddress;
                                users[userIndex].clientID = notifier.clientID;
                                users[userIndex].isLoggedIn = true;

                                //maker the message stuff for the user
                                users[userIndex].numMessages = 0;
                                users[userIndex].hasNewMessages = false;
                                users[userIndex].messageLimit = INCREMENT_MESSAGE;

                                //clears the space of the users address
                                users[userIndex].messages = calloc(users[userIndex].messageLimit, sizeof (ServerMessage));

                                numUsers++;
                            }
                            //                            printf("\n# User %d has logged in.\n> ", users[userIndex].clientID);
                            //                            fflush(stdout);
                            break;
                        case LOGOUT:
                            userIndex = getUserIndex(incoming.senderId, numUsers, users);
                            users[userIndex].isLoggedIn = false;
                            memset(&users[userIndex].address, 0, sizeof (users[userIndex].address));

                            //                            printf("\n# User %d has logged out.\n> ", users[userIndex].clientID);
                            //                            fflush(stdout);
                            break;
                    }
                    break;
                case CLIENTMSG_TAG:
                    //get the rest of the message
                    if (recvfrom(theSocket, &incoming, sizeof (incoming), 0,
                            (struct sockaddr *) &theClientAddress, &clientAddressLength) < 0) {
                        dieWithError("recvfrom() failed");
                    }
                    switch (incoming.messageType) {
                        case SEND:
                            //find the user if they exist in the data structure
                            if ((userIndex = getUserIndex(incoming.recipientId, numUsers, users)) >= 0) {
                                Client *target = &users[userIndex];

                                //if target is at limit
                                if (target->numMessages
                                        == target->messageLimit) {
                                    //increment the message limit
                                    target->messageLimit += INCREMENT_MESSAGE;
                                    //alloc more space
                                    target->messages = realloc(target->messages,
                                            sizeof (ServerMessage)
                                            * target->messageLimit);
                                }

                                target->messages[target->numMessages] = storeMessage(&incoming); //adds the message
                                target->numMessages++;
                                target->hasNewMessages = true;

                                //notify if logged on
                                if (target->isLoggedIn == true) {
                                    notifier.type = NOTIFYMSG_TAG;
                                    notifier.messageType = NOTIFY;
                                    notifier.clientID = target->clientID;

                                    if (sendto(theSocket,
                                            &notifier,
                                            sizeof (notifier),
                                            0,
                                            (struct sockaddr *) &target->address,
                                            sizeof (target->address))
                                            != sizeof (notifier)) {
                                        dieWithError("sendto() sent a different number of bytes than expected");
                                    }
                                }
                            } else { //the user doesn't exit yet
                                //if data structure is at capacity
                                if (numUsers == userLimit) {
                                    //increment the user limit
                                    userLimit += INCREMENT_USERS;
                                    //alloc more space
                                    users = realloc(users, sizeof (Client) * userLimit);
                                }

                                //establish basic user information
                                users[numUsers].clientID = incoming.recipientId;
                                users[numUsers].isLoggedIn = false;
                                users[numUsers].messageLimit = INCREMENT_MESSAGE;

                                //allocate memory for the users messages
                                users[numUsers].messages = calloc(users[numUsers].messageLimit, sizeof (ServerMessage));

                                //stores the message to the user
                                users[numUsers].messages[0] = storeMessage(&incoming);
                                users[numUsers].numMessages = 1;
                                users[numUsers].hasNewMessages = true;

                                //clears the space of the users address
                                memset(&users[numUsers].address, 0, sizeof (users[numUsers].address));

                                numUsers++;
                            }

                            //                            printf("\n# User %d has sent a message to user %d.\n> ", incoming.senderId, incoming.recipientId);
                            //                            fflush(stdout);
                            break;
                        case VIEW:
                            //                            printf("\n# User %d has requested to see their messages.\n> ", incoming.senderId);
                            //sends all of the messages to the user
                            for (i = 0; i < users[userIndex].numMessages; i++) {
                                users[userIndex].messages[i].type = SERVERMSG_TAG;
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
                                //                                printf("\n# Sent message %i.\n> ", i);
                            }

                            //verify message is used to signal the end of the user's messages
                            VerifyMessage nullMessage;
                            nullMessage.messageType = YES;
                            nullMessage.type = VERIFYMSG_TAG;

                            if (sendto(theSocket,
                                    &nullMessage,
                                    sizeof (nullMessage),
                                    0,
                                    (struct sockaddr *) &theClientAddress,
                                    clientAddressLength)
                                    != sizeof (nullMessage)) {
                                dieWithError(
                                        "sendto() sent a different number of bytes than expected");
                            }
                            users[userIndex].hasNewMessages = false;
                            //                            printf("\n# Finished sending messages to user %d.\n> ", incoming.senderId);
                            break;
                        case PRINT:
                            for (i = 0; i < users[userIndex].numMessages; i++) {
                                users[userIndex].messages[i].type = SERVERMSG_TAG;
                                if (sendto(theSocket,
                                        &users[userIndex].messages[i],
                                        sizeof users[userIndex].messages[i],
                                        0,
                                        (struct sockaddr *) &theClientAddress,
                                        clientAddressLength)
                                        != sizeof (users[userIndex].messages[i])) {
                                    dieWithError(
                                            "sendto() sent a different number of bytes than expected");
                                }
                            }

                            //verify message is used to signal the end of the user's messages
                            VerifyMessage endMessage;
                            endMessage.messageType = YES;
                            endMessage.type = VERIFYMSG_TAG;

                            if (sendto(theSocket,
                                    &endMessage,
                                    sizeof (endMessage),
                                    0,
                                    (struct sockaddr *) &theClientAddress,
                                    clientAddressLength)
                                    != sizeof (endMessage)) {
                                dieWithError(
                                        "sendto() sent a different number of bytes than expected");
                            }
                            break;
                    }
            }
        }
    } else if (processID == 0) { //child process 
        //and one to allow the server to gracefully exit and execute server commands
        char cmd[99]; //whatever the user types in after the server starts
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