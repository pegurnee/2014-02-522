/* 
 * File:   Server.c
 * Author: eddie
 *
 * Created on June 3, 2014, 4:27 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
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

    //structures
    Message incoming; //incoming message
    Message outgoing; //outgoing message
    Client *users; //all the users
    pid_t processID; //the child process ID

    //address variables
    struct sockaddr_in theServerAddress; //the local address
    struct sockaddr_in theClientAddress; //the client address
    unsigned int clientAddressLength = sizeof (theClientAddress); //length of the client address

    //handle command line stuff

    //create socket
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

    processID = fork();

    //two threads, 
    if (processID > 0) { //parent: server commands
        //exit/logout: safely closes down the server
        //who: displays on the server the current users logged in
        //say: says a global message from the server
    } else if (processID == 0) { //child: sever operation, mainly just retrieving and responding to messages
        users = calloc(userLimit, sizeof (Client)); //memmory for data structure
        int i; //for loops (which are primarily for-loops)
        memset(&incoming, 0, sizeof (incoming)); //memory
        memset(&outgoing, 0, sizeof (outgoing)); //memory
        outgoing.senderID = SERVER_ID;

        for (;;) {
            recvfrom(theSocket, &incoming, sizeof (incoming), 0,
                    (struct sockaddr *) &theClientAddress, &clientAddressLength);
            switch (incoming.type) {
                case TAG_LOGIN:
                    //LOGIN, adds the user to the list of logged in users, sends a message to each other user that the user logged in
                    outgoing.type = TAG_LOGIN; //the outgoing message type is login
                    outgoing.confirm = true; //the user has logged in
                    sprintf(outgoing.data,
                            "# User %d has logged in.\n" USER_PROMPT,
                            incoming.senderID); //inserts the user id into a string to send to all the other users

                    //sends message to all users that the new user logged in
                    for (i = 0; i < numUsers; i++) {
                        if (users[i].isLoggedIn) {
                            if (sendto(theSocket,
                                    &outgoing,
                                    sizeof (outgoing),
                                    0,
                                    (struct sockaddr *) &users[i].address,
                                    sizeof (users[i].address))
                                    != sizeof (outgoing)) {
                                dieWithError("sendto() sent a different number of bytes than expected");
                            }
                        }
                    }

                    userIndex = getUserIndex(incoming.senderID, numUsers, users);
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
                    //sets the user to logged in and sets the address
                    users[userIndex].isLoggedIn = true;
                    users[userIndex].address = theClientAddress;

                    //sends who result to newly logged in user

                    break;
                case TAG_LOGOUT:
                    //LOGOUT, removes the user from the list of logged out users, sends a message to each other user that the user logged out
                    outgoing.type = TAG_LOGOUT; //the outgoing message type is logout
                    outgoing.confirm = true; //the user has logged out
                    sprintf(outgoing.data,
                            "# User %d has logged out.\n" USER_PROMPT,
                            incoming.senderID); //inserts the user id into a string to send to all the other users

                    //sends message to all users that the new user logged out
                    for (i = 0; i < numUsers; i++) {
                        if (users[i].isLoggedIn) {
                            if (sendto(theSocket,
                                    &outgoing,
                                    sizeof (outgoing),
                                    0,
                                    (struct sockaddr *) &users[i].address,
                                    sizeof (users[i].address))
                                    != sizeof (outgoing)) {
                                dieWithError("sendto() sent a different number of bytes than expected");
                            }
                        }
                    }

                    userIndex = getUserIndex(incoming.senderID, numUsers, users);
                    users[userIndex].isLoggedIn = true;

                    break;
                case TAG_WHO:
                    //WHO, returns to the user a list of all of the currently logged in users

                    outgoing.type = TAG_WHO; //the outgoing message type is logout
                    outgoing.confirm = true; //the user has logged out
                    sprintf(outgoing.data,
                            "# User: %d\n",
                            incoming.senderID); //inserts the user id into a string to send to all the other users

                    //sends message to all users that the new user logged in
                    for (i = 0; i < numUsers; i++) {
                        if (users[i].isLoggedIn) {
                            if (sendto(theSocket,
                                    &outgoing,
                                    sizeof (outgoing),
                                    0,
                                    (struct sockaddr *) theClientAddress,
                                    sizeof (theClientAddress))
                                    != sizeof (outgoing)) {
                                dieWithError("sendto() sent a different number of bytes than expected");
                            }
                        }
                    }

                    //after all logged in user ids are delivered, send exit message
                    outgoing.confirm = false; //the user has logged out
                    if (sendto(theSocket,
                            &outgoing,
                            sizeof (outgoing),
                            0,
                            (struct sockaddr *) theClientAddress,
                            sizeof (theClientAddress))
                            != sizeof (outgoing)) {
                        dieWithError("sendto() sent a different number of bytes than expected");
                    }
                    break;
                case TAG_TALK:
                    //TALK, sends a request to start talking with a user
                    break;
                default:
                    //not gonna happen
                    break;
            }
        }
    } else { //forking() no good
        dieWithError("fork() failed");
    }
    return (EXIT_SUCCESS);
}

