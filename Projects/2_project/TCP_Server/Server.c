/* 
 * File:   Server.c
 * Author: eddie
 *
 * Created on June 3, 2014, 4:27 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <signal.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/mman.h>
#include "UtilsTCP.h"
#include "UtilsServer.h"

/*
 * 
 */
int main(int argc, char** argv) {
    //all my variables
    int theSocket; //the socket
    unsigned short serverPort; //the server port
    int numUsers = 0; //current number of users
    int userLimit = 10; //the user limit, will adjust when needed
    int userIndex; //used when addressing users in the data structure
    char cmd[100]; //whatever the user types in after the server starts

    //structures
    Message incoming; //incoming message
    Message outgoing; //outgoing message
    Client *users; //all the users
    pid_t processID; //the child process ID
    bool *handlingTalk; //bool if talking is being handled

    memset(&handlingTalk, 0, sizeof (bool));
    handlingTalk = mmap(NULL,
            sizeof (bool),
            PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_ANON,
            -1,
            0); //needs to be shared among all of the threads

    //address variables
    struct sockaddr_in theServerAddress; //the local address
    struct sockaddr_in theClientAddress; //the client address
    unsigned int clientAddressLength = sizeof (theClientAddress); //length of the client address

    //handle command line stuff

    //create socket
    if ((theSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        dieWithError("socket() failed");
    }

    serverPort = DEFAULT_PORT;

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

    puts("# Engaged.");
    fflush(stdout);

    processID = fork();
    //two threads, 
    if (processID > 0) { //parent: server commands
        for (;;) {
            char cmd[99]; //whatever the user types in after the server starts
            for (;;) {
                fgets(cmd, 100, stdin);
                cmd[strlen(cmd) - 1] = '\0';
                if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "logout") == 0) { //exit/logout: safely closes down the server
                    puts("# Connection Closed.");
                    kill(0, SIGTERM); //exits the program
                } else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
                    puts("# It's working. 'exit' and 'logout' are used to quit.");
                } else if (strcmp(cmd, "who") == 0 || strcmp(cmd, "list") == 0) { //who: displays on the server the current users logged in

                } else if (strcmp(cmd, "say") == 0 || strcmp(cmd, "talk") == 0) { //say: says a global message from the server

                } else if (strcmp(cmd, "\0") != 0) {
                    puts("# Invalid Command.");
                }
            }
        }
    } else if (processID == 0) { //child: sever operation, mainly just retrieving and responding to messages
        users = calloc(userLimit, sizeof (Client)); //memmory for data structure
        int i; //for loops (which are primarily for-loops)
        int j; //for counting (which is primarily done with numbers)
        memset(&incoming, 0, sizeof (incoming)); //memory
        memset(&outgoing, 0, sizeof (outgoing)); //memory
        outgoing.senderID = SERVER_ID;

        for (;;) {
            recvfrom(theSocket, &incoming, sizeof (incoming), MSG_PEEK,
                    (struct sockaddr *) &theClientAddress, &clientAddressLength);
            switch (incoming.type) {
                case TAG_LOGIN:
                    fflush(stdout);
                    recvfrom(theSocket, &incoming, sizeof (incoming), 0,
                            (struct sockaddr *) &theClientAddress, &clientAddressLength);

                    printf("\n# Received login request from %04i...\n", incoming.senderID);
                    //LOGIN, adds the user to the list of logged in users, sends a message to each other user that the user logged in
                    outgoing.type = TAG_LOGIN; //the outgoing message type is login
                    outgoing.senderID = incoming.senderID;

                    userIndex = getUserIndex(incoming.senderID, numUsers, users);

                    if (userIndex >= 0 && users[userIndex].isLoggedIn == true) { //the user has already logged in
                        outgoing.confirm = false;
                        if (sendto(theSocket,
                                &outgoing,
                                sizeof (outgoing),
                                0,
                                (struct sockaddr *) &theClientAddress,
                                sizeof (theClientAddress))
                                != sizeof (outgoing)) {
                            dieWithError("sendto() sent a different number of bytes than expected");
                        }
                        printf("# ...user %04i is already logged in.\n", incoming.senderID);
                        break;
                    }

                    outgoing.confirm = true; //the user has logged in
                    //send login confirmation
                    if (sendto(theSocket,
                            &outgoing,
                            sizeof (outgoing),
                            0,
                            (struct sockaddr *) &theClientAddress,
                            sizeof (theClientAddress))
                            != sizeof (outgoing)) {
                        dieWithError("sendto() sent a different number of bytes than expected");
                    }

                    printf("# ...login confirmation send to %04i...\n", incoming.senderID);
                    sprintf(outgoing.data,
                            "%d",
                            incoming.senderID); //inserts the user id into a string to send to all the other users
                    outgoing.senderID = SERVER_ID;
                    printf("# ...%04i logged in.\n", incoming.senderID);

                    printf("# Sending login notifications...\n");
                    //sends message to all users that the new user logged in
                    for (i = 0; i < numUsers; i++) {
                        if (users[i].isLoggedIn == true) {
                            if (sendto(theSocket,
                                    &outgoing,
                                    sizeof (outgoing),
                                    0,
                                    (struct sockaddr *) &users[i].theUDPAddress,
                                    sizeof (users[i].theUDPAddress))
                                    != sizeof (outgoing)) {
                                dieWithError("sendto() sent a different number of bytes than expected");
                            }
                        }
                        printf("# ...sent login notification to user %04i...\n", users[i].clientID);
                    }
                    printf("# ...finished sending login notifications.\n");

                    if (userIndex < 0) { //if the user doesn't exist, set user to logged in
                        userIndex = numUsers;
                        //if data structure is at capacity
                        if (numUsers == userLimit) {
                            //increment the user limit
                            userLimit += INCREMENT_USERS;
                            //alloc more space
                            users = realloc(users, sizeof (Client) * userLimit);
                        }
                    }

                    //sets the user to logged in
                    users[userIndex].isLoggedIn = true;

                    //gets user id/address
                    users[userIndex].theUDPAddress = theClientAddress;
                    users[userIndex].theTCPAddress = incoming.theAddress;
                    users[userIndex].clientID = incoming.senderID;
                    numUsers++;

                    outgoing.type = TAG_WHO;
                    printf("# Sending WHO result to user %04i...\n", incoming.senderID);
                    //sends who result to newly logged in user
                    for (i = 0; i < numUsers; i++) {
                        if (users[i].isLoggedIn && users[i].clientID != incoming.senderID) {
                            sprintf(outgoing.data,
                                    "%04d",
                                    users[i].clientID); //inserts the user id into a string to send to all the other users
                            if (sendto(theSocket,
                                    &outgoing,
                                    sizeof (outgoing),
                                    0,
                                    (struct sockaddr *) &theClientAddress,
                                    sizeof (theClientAddress))
                                    != sizeof (outgoing)) {
                                dieWithError("sendto() sent a different number of bytes than expected");
                            }
                            printf("# ...sent user %04i to %04i...\n", users[i].clientID, incoming.senderID);
                        }
                    }

                    //after all logged in user ids are delivered, send exit message
                    outgoing.confirm = false; //the user has logged out
                    if (sendto(theSocket,
                            &outgoing,
                            sizeof (outgoing),
                            0,
                            (struct sockaddr *) &theClientAddress,
                            sizeof (theClientAddress))
                            != sizeof (outgoing)) {
                        dieWithError("sendto() sent a different number of bytes than expected");
                    }
                    printf("# ...sent final message to user %04i.\n", incoming.senderID);
                    fflush(stdout);
                    break;
                case TAG_LOGOUT:
                    recvfrom(theSocket, &incoming, sizeof (incoming), 0,
                            (struct sockaddr *) &theClientAddress, &clientAddressLength);
                    fflush(stdout);
                    printf("\n# Received LOGOUT message from %04i.\n", incoming.senderID);
                    //LOGOUT, removes the user from the list of logged out users, sends a message to each other user that the user logged out
                    outgoing.type = TAG_LOGOUT; //the outgoing message type is logout
                    outgoing.confirm = true; //the user has logged out
                    outgoing.senderID = SERVER_ID;
                    sprintf(outgoing.data,
                            "%d",
                            incoming.senderID); //inserts the user id into a string to send to all the other users

                    printf("# Sending logout notifications...\n");
                    //sends message to all users that the user logged out
                    for (i = 0; i < numUsers; i++) {
                        if (users[i].isLoggedIn && users[i].clientID != incoming.senderID) {
                            if (sendto(theSocket,
                                    &outgoing,
                                    sizeof (outgoing),
                                    0,
                                    (struct sockaddr *) &users[i].theUDPAddress,
                                    sizeof (users[i].theUDPAddress))
                                    != sizeof (outgoing)) {
                                dieWithError("sendto() sent a different number of bytes than expected");
                            }
                            printf("# ...sent logout notification to user %04i...\n", users[i].clientID);
                        }
                    }
                    printf("# ...finished sending logout notifications.\n");

                    //actually logs out the user
                    userIndex = getUserIndex(incoming.senderID, numUsers, users);
                    users[userIndex].isLoggedIn = false;
                    printf("# User %04i logged out.\n", incoming.senderID);
                    fflush(stdout);
                    break;
                case TAG_WHO:
                    recvfrom(theSocket, &incoming, sizeof (incoming), 0,
                            (struct sockaddr *) &theClientAddress, &clientAddressLength);
                    fflush(stdout);
                    printf("\n# Received WHO request from user %04i...\n", incoming.senderID);
                    //WHO, returns to the user a list of all of the currently logged in users
                    outgoing.type = TAG_WHO; //the outgoing message type is logout
                    outgoing.confirm = true; //the user has logged out
                    outgoing.senderID = SERVER_ID;
                    //to user the list of all logged in users
                    for (i = 0; i < numUsers; i++) {
                        if (users[i].isLoggedIn && users[i].clientID != incoming.senderID) {
                            sprintf(outgoing.data,
                                    "%04d",
                                    users[i].clientID); //inserts the user id into a string to send to all the other users
                            if (sendto(theSocket,
                                    &outgoing,
                                    sizeof (outgoing),
                                    0,
                                    (struct sockaddr *) &theClientAddress,
                                    sizeof (theClientAddress))
                                    != sizeof (outgoing)) {
                                dieWithError("sendto() sent a different number of bytes than expected");
                            }
                            printf("# ...sent user %04i to %04i...\n", users[i].clientID, incoming.senderID);
                        }
                    }

                    //after all logged in user ids are delivered, send exit message
                    outgoing.confirm = false; //the user has all of the dudes
                    if (sendto(theSocket,
                            &outgoing,
                            sizeof (outgoing),
                            0,
                            (struct sockaddr *) &theClientAddress,
                            sizeof (theClientAddress))
                            != sizeof (outgoing)) {
                        dieWithError("sendto() sent a different number of bytes than expected");
                    }
                    printf("# ...sent final message to user %04i.\n", incoming.senderID);
                    fflush(stdout);
                    break;
                case TAG_TALK_REQ:
                    recvfrom(theSocket, &incoming, sizeof (incoming), 0,
                            (struct sockaddr *) &theClientAddress, &clientAddressLength);
                    fflush(stdout);
                    printf("\n# Received TALK request from user %04i to speak with %04i...\n", incoming.senderID, atoi(incoming.data));

                    userIndex = getUserIndex(atoi(incoming.data), numUsers, users);
                    if (userIndex >= 0) {
                        if (incoming.senderID == users[userIndex].clientID) {
                            outgoing.confirm = false;
                            printf("# ...user %04i attempted to communicate with their self...\n", atoi(incoming.data));
                        } else {
//                            outgoing.theAddress = incoming.theAddress;
//                            outgoing.senderID = incoming.senderID;
//                            if (sendto(theSocket,
//                                    &outgoing,
//                                    sizeof (outgoing),
//                                    0,
//                                    (struct sockaddr *) &theClientAddress,
//                                    sizeof (theClientAddress))
//                                    != sizeof (outgoing)) {
//                                dieWithError("sendto() sent a different number of bytes than expected");
//                            }
                            outgoing.senderID = SERVER_ID;
                            outgoing.theAddress = users[userIndex].theTCPAddress;
                            outgoing.theOtherAddress = users[userIndex].theUDPAddress;
                            outgoing.confirm = true;
                            printf("# ...assigning user %04i's address (%i) to out going message...\n",
                                    users[userIndex].clientID,
                                    users[userIndex].theTCPAddress.sin_port);
                        }
                    } else {
                        outgoing.confirm = false;
                        printf("# ...unable to find user %04i...\n", atoi(incoming.data));
                    }
                    outgoing.type = TAG_TALK_RES;
                    outgoing.senderID = SERVER_ID;

                    if (sendto(theSocket,
                            &outgoing,
                            sizeof (outgoing),
                            0,
                            (struct sockaddr *) &theClientAddress,
                            sizeof (theClientAddress))
                            != sizeof (outgoing)) {
                        dieWithError("sendto() sent a different number of bytes than expected");
                    }

                    printf("# ...sending address of %04i to %04i.\n", atoi(incoming.data), incoming.senderID);
                    fflush(stdout);
                    //                    if (*handlingTalk == false) {
                    //                        processID = fork();
                    //                        if (processID == 0) {
                    //                            *handlingTalk = true;
                    //                            recvfrom(theSocket, &incoming, sizeof (incoming), 0,
                    //                                    (struct sockaddr *) &theClientAddress, &clientAddressLength);
                    //                            fflush(stdin);
                    //                            printf("# Received TALK request from user %04i...\n", incoming.senderID);
                    //                            //TALK, sends a request to start talking with a user
                    //                            outgoing.type = TAG_WHO; //the outgoing message type is logout
                    //                            outgoing.confirm = true; //the user has logged out
                    //
                    //                            userIndex = getUserIndex(incoming.senderID, numUsers, users);
                    //                            //sends message to all users that the new user logged in
                    //                            for (i = 0; i < numUsers; i++) {
                    //                                if (users[i].isLoggedIn) {
                    //                                    sprintf(outgoing.data,
                    //                                            "%04d",
                    //                                            users[i].clientID); //inserts the user id into a string to send to all the other users
                    //                                    if (sendto(theSocket,
                    //                                            &outgoing,
                    //                                            sizeof (outgoing),
                    //                                            0,
                    //                                            (struct sockaddr *) &theClientAddress,
                    //                                            sizeof (theClientAddress))
                    //                                            != sizeof (outgoing)) {
                    //                                        dieWithError("sendto() sent a different number of bytes than expected");
                    //                                    }
                    //                                    printf("# ...sent user %04i to %04i...\n", users[i].clientID, incoming.senderID);
                    //                                }
                    //                            }
                    //
                    //                            //after all logged in user ids are delivered, send exit message
                    //                            outgoing.confirm = false; //the user has logged out
                    //                                                if (sendto(theSocket,
                    //                                                        &outgoing,
                    //                                                        sizeof (outgoing),
                    //                                                        0,
                    //                                                        (struct sockaddr *) &theClientAddress,
                    //                                                        sizeof (theClientAddress))
                    //                                                        != sizeof (outgoing)) {
                    //                                                    dieWithError("sendto() sent a different number of bytes than expected");
                    //                                                }
                    //                            printf("# ...sent final message to user %04i...\n", incoming.senderID);
                    //
                    //                            for (;;) {
                    //                                recvfrom(theSocket, &incoming, sizeof (incoming), MSG_PEEK,
                    //                                        (struct sockaddr *) &theClientAddress, &clientAddressLength);
                    //                                printf("# in id: %i | type: %i\n", incoming.type, TAG_TALK_REQ);
                    //                                printf("# in id: %i | u id: %i\n", incoming.senderID, users[userIndex].clientID);
                    //                                if (incoming.type == TAG_TALK_REQ && incoming.senderID == users[userIndex].clientID) {
                    //                                    //receive user request
                    //                                    recvfrom(theSocket, &incoming, sizeof (incoming), 0,
                    //                                            (struct sockaddr *) &theClientAddress, &clientAddressLength);
                    //                                    break;
                    //                                }
                    //                            }
                    //
                    //                            j = atoi(incoming.data);
                    //                            printf("# ...received request to communicate with the number %i logged in user...\n", j);
                    //                            if (j > numUsers) {
                    //                                outgoing.type = TAG_TALK_RES;
                    //                                outgoing.confirm = false;
                    //                                if (sendto(theSocket,
                    //                                        &outgoing,
                    //                                        sizeof (outgoing),
                    //                                        0,
                    //                                        (struct sockaddr *) &users[i].theUDPAddress,
                    //                                        sizeof (users[i].theUDPAddress))
                    //                                        != sizeof (outgoing)) {
                    //                                    dieWithError("sendto() sent a different number of bytes than expected");
                    //                                }
                    //                                printf("# ...user %04i attempted to talk to someone beyond limit.\n", incoming.senderID);
                    //                            } else {
                    //                                for (i = 0; i < numUsers; i++) {
                    //                                    if (users[i].isLoggedIn) {
                    //                                        j--;
                    //                                        if (j == 0) {
                    //                                            if (users[i].clientID == incoming.senderID) {
                    //                                                outgoing.type = TAG_TALK_RES;
                    //                                                outgoing.confirm = false;
                    //                                                if (sendto(theSocket,
                    //                                                        &outgoing,
                    //                                                        sizeof (outgoing),
                    //                                                        0,
                    //                                                        (struct sockaddr *) &users[i].theUDPAddress,
                    //                                                        sizeof (users[i].theUDPAddress))
                    //                                                        != sizeof (outgoing)) {
                    //                                                    dieWithError("sendto() sent a different number of bytes than expected");
                    //                                                }
                    //                                                printf("# ...user %04i attempted to talk to their self.\n", incoming.senderID);
                    //                                            } else {
                    //                                                outgoing.type = TAG_TALK_REQ;
                    //                                                outgoing.theAddress = theClientAddress;
                    //                                                outgoing.senderID = incoming.senderID;
                    //                                                if (sendto(theSocket,
                    //                                                        &outgoing,
                    //                                                        sizeof (outgoing),
                    //                                                        0,
                    //                                                        (struct sockaddr *) &users[i].theUDPAddress,
                    //                                                        sizeof (users[i].theUDPAddress))
                    //                                                        != sizeof (outgoing)) {
                    //                                                    dieWithError("sendto() sent a different number of bytes than expected");
                    //                                                }
                    //                                                printf("# ...sent request from user %04i to %04i...\n", incoming.senderID, users[i].clientID);
                    //                                            }
                    //                                            break;
                    //                                        }
                    //                                    }
                    //                                }
                    //                            }
                    //
                    //                            if (j >= 0) {
                    //                                for (;;) {
                    //                                    recvfrom(theSocket, &incoming, sizeof (incoming), MSG_PEEK,
                    //                                            (struct sockaddr *) &theClientAddress, &clientAddressLength);
                    //                                    printf("# in type: %i | type: %i\n", incoming.type, TAG_TALK_RES);
                    //                                    printf("# in id: %5i | u id: %i\n", incoming.senderID, users[i].clientID);
                    //                                    if (incoming.type == TAG_TALK_RES && incoming.senderID == users[i].clientID) {
                    //                                        //receive other user response
                    //                                        recvfrom(theSocket, &incoming, sizeof (incoming), 0,
                    //                                                (struct sockaddr *) &theClientAddress, &clientAddressLength);
                    //
                    //                                        printf("# ...received %s response from user %i...\n", incoming.confirm == true ? "true" : "false", incoming.senderID);
                    //                                        break;
                    //                                    }
                    //                                }
                    //
                    //                                outgoing.type = TAG_TALK_RES;
                    //                                outgoing.senderID = users[i].clientID;
                    //                                outgoing.confirm = incoming.confirm;
                    //                                outgoing.theAddress = users[i].theTCPAddress;
                    //
                    //                                if (sendto(theSocket,
                    //                                        &outgoing,
                    //                                        sizeof (outgoing),
                    //                                        0,
                    //                                        (struct sockaddr *) &users[userIndex].theUDPAddress,
                    //                                        sizeof (users[userIndex].theUDPAddress))
                    //                                        != sizeof (outgoing)) {
                    //                                    dieWithError("sendto() sent a different number of bytes than expected");
                    //                                }
                    //                            }
                    //
                    //                            //                            outgoing.senderID = SERVER_ID;
                    //                            fflush(stdin);
                    //                            *handlingTalk = false;
                    //                            //                            kill(0, SIGTERM); //exits the program
                    //                        } else {
                    //                            sleep(2);
                    //                        }
                    //                    }

                    break;
                default:
                    //not gonna happen
                    break;
            }
            puts("");
        }
    } else { //forking() no good
        dieWithError("fork() failed");
    }
    return (EXIT_SUCCESS);
}

