/* 
 * File:   Client.c
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
#include <sys/time.h>
#include "UtilsTCP.h"

#define DEFAULT_IP   "127.0.0.1" //localhost
#define MAX_PENDING 5    /* Maximum outstanding connection requests */

/*
 * 
 */
int main(int argc, char** argv) {
    //all my variables
    int cmd; //the user input
    int theUDPSocket; //the socket connected to the server
    unsigned int *otherID; //the other user's id
    unsigned int *userID; //it needs to remain only in the parent process
    char chatMessage[100]; //whatever the user types in after the server starts

    //structures
    Message incoming; //incoming message
    Message outgoing; //outgoing message
    pid_t processID; //the child process ID
    bool *isTalking; //if the user is in talk mode
    bool *talkRequest; //if the user has received a talk request

    //server connection variables
    char *serverIP = DEFAULT_IP;
    unsigned short serverPort = DEFAULT_PORT;
    struct sockaddr_in *targetTCPAddress; //the other address
    struct sockaddr_in theServerAddress; //the client address
    struct sockaddr_in *targetUDPAddress; //the client address
    struct sockaddr_in *myAddress; //the client address

    unsigned int serverAddressLength = sizeof (theServerAddress); //length of the client address

    //establishes variables
    memset(&outgoing, 0, sizeof (outgoing));
    memset(&incoming, 0, sizeof (incoming));
    memset(&processID, 0, sizeof (pid_t));
    memset(&isTalking, 0, sizeof (bool));
    memset(&talkRequest, 0, sizeof (bool));
    memset(&targetTCPAddress, 0, sizeof (targetTCPAddress));
    isTalking = mmap(NULL,
            sizeof (bool),
            PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_ANON,
            -1,
            0); //needs to be shared among all of the threads
    *isTalking = false;
    talkRequest = mmap(NULL,
            sizeof (bool),
            PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_ANON,
            -1,
            0); //needs to be shared among all of the threads
    *talkRequest = false;
    otherID = mmap(NULL,
            sizeof (unsigned int),
            PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_ANON,
            -1,
            0); //needs to be shared among all of the threads
    userID = mmap(NULL,
            sizeof (unsigned int),
            PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_ANON,
            -1,
            0); //needs to be shared among all of the threads
    targetTCPAddress = mmap(NULL,
            sizeof (targetTCPAddress),
            PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_ANON,
            -1,
            0); //needs to be shared among all of the threads
    myAddress = mmap(NULL,
            sizeof (myAddress),
            PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_ANON,
            -1,
            0); //needs to be shared among all of the threads
    targetUDPAddress = mmap(NULL,
            sizeof (myAddress),
            PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_ANON,
            -1,
            0); //needs to be shared among all of the threads
    //handle command line stuff

    //create socket
    if ((theUDPSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        dieWithError("socket() failed");
    }

    //create the server address structure
    memset(&theServerAddress, 0, sizeof (theServerAddress)); // Zero out structure
    theServerAddress.sin_family = AF_INET; /* Internet addr family */
    theServerAddress.sin_addr.s_addr = inet_addr(serverIP); /* Server IP address */
    theServerAddress.sin_port = htons(serverPort); /* Server port */

    memset(myAddress, 0, sizeof (*myAddress)); /* Zero out structure */

    myAddress->sin_family = AF_INET; /* Internet address family */
    myAddress->sin_addr.s_addr = htonl(INADDR_ANY);

    memset(targetUDPAddress, 0, sizeof (*targetUDPAddress)); /* Zero out structure */

    processID = fork();

    if (processID > 0) { //three threads, parent thread: communicating with the server, when in talk mode, sending messages to the other person

        int i; //used for looping
        bool loggedIn = false; //if the user is loggedIn, different user menu
        memset(&loggedIn, 0, sizeof (bool)); //makes memory for it
        struct timeval timeoutTime; //the time allowed for user timeout
        timeoutTime.tv_sec = 10;

        for (;;) {
            if (loggedIn == false) { //options for users not logged in
                puts("# 1 to log-in");
                puts("# 2 to exit");
                USER_PROMPT;

                scanf("%d", &cmd);
                getchar();
                switch (cmd) {
                    case 1: //LOGIN, logs into the server
                        puts("# Enter ID:");
                        USER_PROMPT;
                        if (scanf("%u", userID) <= 0 || *userID > 9999) {
                            printf("# Invalid User ID.\n");
                            break;
                        }
                        getchar();

                        //sets the sender ID to the userID, doesn't need to change
                        outgoing.senderID = *userID;

                        //sends a login notification to the server
                        outgoing.type = TAG_LOGIN;
                        outgoing.confirm = true;

                        myAddress->sin_port = htons(20000 + *userID);
                        outgoing.theAddress = *myAddress;

                        if (sendto(theUDPSocket,
                                &outgoing,
                                sizeof (outgoing),
                                0,
                                (struct sockaddr *) &theServerAddress,
                                sizeof (theServerAddress))
                                != sizeof (outgoing)) {
                            dieWithError("sendto() sent a different number of bytes than expected");
                        }
                        puts("# Logging in...");
                        //trying to figure out set timeout
                        //                        if (setsockopt(theUDPSocket,
                        //                                SOL_SOCKET,
                        //                                SO_RCVTIMEO,
                        //                                &timeoutTime,
                        //                                sizeof (timeoutTime)) < 0) {
                        //                            puts("# Connection to server timed out.");
                        //                        }

                        for (;;) {
                            //waits for the server to confirm log in
                            recvfrom(theUDPSocket,
                                    &incoming,
                                    sizeof (incoming), MSG_PEEK,
                                    (struct sockaddr *) &theServerAddress,
                                    &serverAddressLength);
                            //                            printf("# in tag: %4i | tag: %4i"
                            //                                 "\n# in  id: %4i |  id: %4i\n",incoming.type, TAG_LOGIN, incoming.senderID, *userID);
                            if (incoming.type == TAG_LOGIN && incoming.senderID == *userID) {
                                recvfrom(theUDPSocket,
                                        &incoming,
                                        sizeof (incoming), 0,
                                        (struct sockaddr *) &theServerAddress,
                                        &serverAddressLength);
                                break;
                            }
                        }

                        //waits for the server to confirm login
                        if (incoming.confirm == true) {
                            printf("# ...success!\n");
                            loggedIn = true;

                            puts("\n# Logged in users:");
                            for (i = 1;; i++) {
                                //peeks at the message
                                recvfrom(theUDPSocket,
                                        &incoming,
                                        sizeof (incoming), MSG_PEEK,
                                        (struct sockaddr *) &theServerAddress,
                                        &serverAddressLength);

                                //waits for the server to confirm login
                                if (incoming.type == TAG_WHO) {
                                    recvfrom(theUDPSocket,
                                            &incoming,
                                            sizeof (incoming), 0,
                                            (struct sockaddr *) &theServerAddress,
                                            &serverAddressLength);
                                    if (incoming.confirm == true) { //there is still data to be brought in
                                        printf("# User %d: %s\n", i, incoming.data);
                                    } else { //there are no more users
                                        if (i == 1) {
                                            printf("# No users logged in besides you.\n");
                                        }
                                        break;
                                    }
                                }
                            }
                            processID = fork();
                            if (processID == 0) {
                                int theTCPReceiveSocket;
                                int someotherSocket;
                                int bytesReceived;

                                /* Create a reliable, stream socket using TCP */
                                if ((theTCPReceiveSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
                                    dieWithError("socket() failed");
                                }
                                if (bind(theTCPReceiveSocket, (struct sockaddr *) myAddress, sizeof (*myAddress)) < 0) {
                                    dieWithError("bind() failed");
                                }
                                if (listen(theTCPReceiveSocket, MAX_PENDING) < 0) {
                                    dieWithError("listen() failed");
                                }

                                for (;;) {
                                    socklen_t otherAddSize = sizeof (*targetTCPAddress);
                                    if ((someotherSocket = accept(theTCPReceiveSocket, (struct sockaddr *) targetTCPAddress,
                                            &otherAddSize)) < 0) {
                                        dieWithError("accept() failed");
                                    } else {
                                        puts("SUCCESS!");
                                    }
                                    for (; *isTalking == true;) {
                                        //receive messages and immediately displays it to the screen.
                                        if ((bytesReceived = recv(someotherSocket, chatMessage, sizeof (chatMessage) - 1, 0)) <= 0) {
                                            dieWithError("recv() failed or connection closed prematurely");
                                        }
                                        chatMessage[bytesReceived] = '\0'; /* Terminate the string! */
                                        printf("\nUser %05i# %s\n", *otherID, chatMessage); /* Print the echo buffer */
                                        fflush(stdout);
                                    }
                                }
                                kill(0, SIGKILL);
                            }
                        } else {
                            puts("# ...failed.");
                        }
                        break;
                    case 2: //EXIT, exits the program
                        puts("# Goodbye");
                        kill(0, SIGTERM); //exits the program
                        break;
                    default:
                        puts("# Invalid Command.");
                        break;
                }
                puts("");
                fflush(stdout);
            } else {
                if (*talkRequest == true) { //temp menu when a talk request is received
                    printf("# You have received request to talk from %i.\n", *otherID);
                    puts("# 1 to confirm request");
                    puts("# 2 to refuse request");
                    USER_PROMPT;

                    scanf("%d", &cmd);
                    getchar();

                    switch (cmd) {
                        case 1:
                            *isTalking = true;
                            outgoing.theAddress = *myAddress;
                        case 2:
                        default:
                            outgoing.type = TAG_TALK_RES;
                            outgoing.confirm = cmd == 1 ? true : false;
                            *talkRequest = false;
                            if (sendto(theUDPSocket,
                                    &outgoing,
                                    sizeof (outgoing),
                                    0,
                                    (struct sockaddr *) targetUDPAddress,
                                    sizeof (theServerAddress))
                                    != sizeof (outgoing)) {
                                dieWithError("sendto() sent a different number of bytes than expected");
                            }
                            break;
                    }
                    puts("");
                    fflush(stdout);
                } else if (*isTalking == false) { //menu for loggedIn users
                    printf("# Logged in as user %i.\n", *userID);
                    puts("# 1 to view users");
                    puts("# 2 to start talking");
                    puts("# 3 to logout");
                    puts("# 4 to respond to talk requests");
                    puts("# 5 to exit");
                    USER_PROMPT;

                    scanf("%d", &cmd);
                    getchar();

                    switch (cmd) {
                        case 1: //WHO, requests from the user to list all of the currently logged in users
                            //makes outgoing a who message
                            outgoing.type = TAG_WHO;
                            outgoing.confirm = true;

                            if (sendto(theUDPSocket,
                                    &outgoing,
                                    sizeof (outgoing),
                                    0,
                                    (struct sockaddr *) &theServerAddress,
                                    sizeof (theServerAddress))
                                    != sizeof (outgoing)) {
                                dieWithError("sendto() sent a different number of bytes than expected");
                            }

                            puts("# Logged in users:");
                            for (i = 1;; i++) {
                                //peeks at the message
                                recvfrom(theUDPSocket,
                                        &incoming,
                                        sizeof (incoming), MSG_PEEK,
                                        (struct sockaddr *) &theServerAddress,
                                        &serverAddressLength);

                                //waits for the server to confirm login
                                if (incoming.type == TAG_WHO) {
                                    recvfrom(theUDPSocket,
                                            &incoming,
                                            sizeof (incoming), 0,
                                            (struct sockaddr *) &theServerAddress,
                                            &serverAddressLength);
                                    if (incoming.confirm == true) { //there is still data to be brought in
                                        printf("# User %d: %s\n", i, incoming.data);
                                    } else { //there are no more users
                                        if (i == 1) {
                                            printf("# No users logged in besides you.\n");
                                        }
                                        break;
                                    }
                                }
                            }
                            break;
                        case 2: //TALK, starts a talk request with a user, waits for the user's response (via server)
                            outgoing.type = TAG_TALK_REQ;
                            outgoing.confirm = true;

                            puts("# Enter user ID:");
                            USER_PROMPT;

                            scanf("%d", &cmd);
                            getchar();

                            sprintf(outgoing.data,
                                    "%d",
                                    cmd);

                            printf("# Sending request for information...\n");
                            if (sendto(theUDPSocket,
                                    &outgoing,
                                    sizeof (outgoing),
                                    0,
                                    (struct sockaddr *) &theServerAddress,
                                    sizeof (theServerAddress))
                                    != sizeof (outgoing)) {
                                dieWithError("sendto() sent a different number of bytes than expected");
                            }
                            for (;;) {
                                recvfrom(theUDPSocket,
                                        &incoming,
                                        sizeof (incoming), MSG_PEEK,
                                        (struct sockaddr *) &theServerAddress,
                                        &serverAddressLength);
                                if (incoming.type == TAG_TALK_RES) {
                                    recvfrom(theUDPSocket,
                                            &incoming,
                                            sizeof (incoming), 0,
                                            (struct sockaddr *) &theServerAddress,
                                            &serverAddressLength);
                                }
                                break;
                            }
                            if (incoming.confirm == false) {
                                if (cmd == *userID) {
                                    puts("# You don't need a computer to talk to yourself.\n");
                                } else {
                                    printf("# User %d is unavailable.\n", cmd);
                                }
                            } else {
                                *targetUDPAddress = incoming.theOtherAddress;
                                *otherID = cmd;
                                printf("# Received address, contacting other user...\n");
                                //                                outgoing.theAddress = incoming.theAddress;
                                outgoing.theAddress = *myAddress;
                                outgoing.senderID = *userID;
                                outgoing.type = TAG_TALK_REQ;

                                if (sendto(theUDPSocket,
                                        &outgoing,
                                        sizeof (outgoing),
                                        0,
                                        (struct sockaddr *) &incoming.theOtherAddress,
                                        sizeof (incoming.theOtherAddress))
                                        != sizeof (outgoing)) {
                                    dieWithError("sendto() sent a different number of bytes than expected");
                                }
                                for (;;) {
                                    recvfrom(theUDPSocket,
                                            &incoming,
                                            sizeof (incoming), MSG_PEEK,
                                            (struct sockaddr *) targetUDPAddress,
                                            &serverAddressLength);
                                    if (incoming.type == TAG_TALK_RES) {
                                        recvfrom(theUDPSocket,
                                                &incoming,
                                                sizeof (incoming), 0,
                                                (struct sockaddr *) targetUDPAddress,
                                                &serverAddressLength);
                                    }
                                    break;
                                }
                                if (incoming.confirm == false) {
                                    printf("# Connection refused by user %i.\n", incoming.senderID);
                                } else {
                                    printf("# Talking initiated...\n");
                                    *targetTCPAddress = incoming.theAddress;
                                    *isTalking = true;
                                }

                            }

                            //                            for (i = 1;; i++) {
                            //                                //peeks at the message
                            //                                recvfrom(theUDPSocket,
                            //                                        &incoming,
                            //                                        sizeof (incoming), MSG_PEEK,
                            //                                        (struct sockaddr *) &theServerAddress,
                            //                                        &serverAddressLength);
                            //
                            //                                //waits for the server to send logins
                            //                                if (incoming.type == TAG_WHO) {
                            //                                    recvfrom(theUDPSocket,
                            //                                            &incoming,
                            //                                            sizeof (incoming), 0,
                            //                                            (struct sockaddr *) &theServerAddress,
                            //                                            &serverAddressLength);
                            //                                    if (incoming.confirm == true) { //there is still data to be brought in
                            //                                        printf("# %i for user %s\n", i, incoming.data);
                            //                                    } else { //there are no more users
                            //                                        break;
                            //                                    }
                            //                                }
                            //                            }
                            //                            USER_PROMPT;
                            //
                            //                            scanf("%d", &cmd);
                            //                            getchar();
                            //
                            //                            sprintf(outgoing.data,
                            //                                    "%d",
                            //                                    cmd);
                            //
                            //                            puts("# Attempting to connect...");
                            //                            sleep(1);
                            //                            //send requested user talking
                            //                            if (sendto(theUDPSocket,
                            //                                    &outgoing,
                            //                                    sizeof (outgoing),
                            //                                    0,
                            //                                    (struct sockaddr *) &theServerAddress,
                            //                                    sizeof (theServerAddress))
                            //                                    != sizeof (outgoing)) {
                            //                                dieWithError("sendto() sent a different number of bytes than expected");
                            //                            }
                            //
                            //
                            //                            for (;;) {
                            //                                //peeks at the message
                            //                                recvfrom(theUDPSocket,
                            //                                        &incoming,
                            //                                        sizeof (incoming), MSG_PEEK,
                            //                                        (struct sockaddr *) &theServerAddress,
                            //                                        &serverAddressLength);
                            //
                            //                                //waits for the server to respond from talk request
                            //                                if (incoming.type == TAG_TALK_RES) {
                            //                                    recvfrom(theUDPSocket,
                            //                                            &incoming,
                            //                                            sizeof (incoming), 0,
                            //                                            (struct sockaddr *) &theServerAddress,
                            //                                            &serverAddressLength);
                            //                                    if (incoming.confirm == true) { //begin talking
                            //                                        *otherID = incoming.senderID;
                            //                                        *theOtherAddress = incoming.theAddress;
                            //                                        printf("# ...connected to user %i!\n", *otherID);
                            //                                        *isTalking = true;
                            //                                        break;
                            //                                    } else { //other user couldn't talk
                            //                                        printf("# ...user %i is currently unavailable to chat.\n", incoming.senderID);
                            //                                        break;
                            //                                    }
                            //                                }
                            //                            }
                            break;
                        case 3: //LOGOUT, logs out from the server
                            //sends a logout notification to the server
                            outgoing.type = TAG_LOGOUT;
                            outgoing.confirm = true;

                            if (sendto(theUDPSocket,
                                    &outgoing,
                                    sizeof (outgoing),
                                    0,
                                    (struct sockaddr *) &theServerAddress,
                                    sizeof (theServerAddress))
                                    != sizeof (outgoing)) {
                                dieWithError("sendto() sent a different number of bytes than expected");
                            }
                            loggedIn = false;
                            puts("# Logged out.\n");
                            //                            puts("# Logging out...");

                            //                            for (;;) {
                            //                                //waits for the server to confirm log in
                            //                                recvfrom(theUDPSocket,
                            //                                        &incoming,
                            //                                        sizeof (incoming), MSG_PEEK,
                            //                                        (struct sockaddr *) &theServerAddress,
                            //                                        &serverAddressLength);
                            //                                if (incoming.type == TAG_LOGOUT && incoming.senderID == *userID) {
                            //                                    recvfrom(theUDPSocket,
                            //                                            &incoming,
                            //                                            sizeof (incoming), 0,
                            //                                            (struct sockaddr *) &theServerAddress,
                            //                                            &serverAddressLength);
                            //                                    break;
                            //                                }
                            //                            }
                            //
                            //                            if (incoming.confirm == true) {
                            //                                printf("# ...success!\n");
                            //                                loggedIn = false;
                            //                            } else {
                            //                                printf("# ...failed.\n");
                            //                            }
                            break;
                        case 4: //HANDLE, addresses pending talk requests
                            if (*talkRequest == false) {
                                puts("# You have no outstanding talk requests.");
                            }
                            break;
                        case 5: //EXIT, exits the program
                            kill(0, SIGTERM); //exits the program
                            break;
                        default:
                            printf("# Invalid Command.\n");
                            break;
                    }
                    puts("");
                } else { //once the user is in talk mode
                    int theTCPSendSocket;

                    if ((theTCPSendSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
                        dieWithError("socket() failed");
                    }
                    //                    if (bind(theTCPSendSocket, (struct sockaddr *) theOtherAddress, sizeof (*theOtherAddress)) < 0) {
                    //                        dieWithError("bind() failed (tcp)");
                    //                    }
                    //                    if (listen(theTCPSendSocket, MAX_PENDING) < 0) {
                    //                        dieWithError("listen() failed");
                    //                    }
                    if (connect(theTCPSendSocket, (struct sockaddr *) targetTCPAddress, sizeof (*targetTCPAddress)) < 0) {
                        dieWithError("connect() failed from talk mode");
                    }

                    for (; *isTalking == true;) {
                        USER_PROMPT;

                        fgets(chatMessage, sizeof (chatMessage), stdin);
                        chatMessage[strlen(chatMessage) - 1] = '\0';
                        fflush(stdout);
                        if (strcmp(chatMessage, ":q") == 0 || strcmp(chatMessage, ":quit") == 0) { //QUIT, by typing in ":quit" or ":q" the user can exit
                            puts("# Connection Closed.");
                            *isTalking = false;
                        } else { //SEND, everything that the user sends in talk mode is displayed on the other client's screen
                            if (send(theTCPSendSocket,
                                    chatMessage,
                                    strlen(chatMessage),
                                    0)
                                    != strlen(chatMessage)) {
                                dieWithError("send() sent a different number of bytes than expected");
                            }
                        }
                    }
                }
            }
        }
    } else if (processID == 0) {
        //        processID = fork();

        //        if (processID > 0) { //child thread: receiving communication from server
        for (;;) {
            //peeks at the message
            recvfrom(theUDPSocket,
                    &incoming,
                    sizeof (incoming), MSG_PEEK,
                    (struct sockaddr *) &theServerAddress,
                    &serverAddressLength);
            switch (incoming.type) {
                case TAG_TALK_REQ: //TALK, after receiving a talk request from a user,
                    recvfrom(theUDPSocket,
                            &incoming,
                            sizeof (incoming), 0,
                            (struct sockaddr *) targetUDPAddress,
                            &serverAddressLength);
                    if (*isTalking == true || *talkRequest == true) { //  if already talking, respond with a negative message
                        outgoing.type = TAG_TALK_RES;
                        outgoing.confirm = false;
                        *targetUDPAddress = incoming.theOtherAddress;
                        if (sendto(theUDPSocket,
                                &outgoing,
                                sizeof (outgoing),
                                0,
                                (struct sockaddr *) targetUDPAddress,
                                sizeof (theServerAddress))
                                != sizeof (outgoing)) {
                            dieWithError("sendto() sent a different number of bytes than expected");
                        }
                    } else { //  sets a shared boolean to true letting the parent thread know that the client has received a request to talk
                        *otherID = incoming.senderID;
                        *targetTCPAddress = incoming.theAddress;
                        printf("\n# You've received a new talk request from %i.\n", *otherID);
                        *talkRequest = true;
                    }

                    break;
                case TAG_LOGIN: //LOGIN, after receiving a login message from the server, displays that user id to the client
                    if (incoming.senderID == SERVER_ID) {
                        recvfrom(theUDPSocket,
                                &incoming,
                                sizeof (incoming), 0,
                                (struct sockaddr *) &theServerAddress,
                                &serverAddressLength);
                        printf("\n# User %s has logged in.\n> ", incoming.data);
                    }
                    break;
                case TAG_LOGOUT: //LOGOUT, after receiving a logout message from the server, displays that user id to the client
                    if (incoming.senderID == SERVER_ID) {
                        recvfrom(theUDPSocket,
                                &incoming,
                                sizeof (incoming), 0,
                                (struct sockaddr *) &theServerAddress,
                                &serverAddressLength);
                        printf("\n# User %s has logged out.\n> ", incoming.data);
                    }
                    break;
                case TAG_WHO:
                case TAG_TALK_RES:
                default:
                    break;
            }
        }
        //        } else if (processID == 0) { //child thread: the TCP client that receives communication from other user while in talk mode, displays messages on screen
        //            int theTCPReceiveSocket;
        //            int bytesReceived;
        //            for (;;) {
        //                if (myAddress->sin_port != 0) {
        //
        //                }
        //            }
        //
        //
        //
        //            printf("# client port: %i", myAddress->sin_port);
        //            /* Create a reliable, stream socket using TCP */
        //            if ((theTCPReceiveSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        //                dieWithError("socket() failed");
        //            }
        //            if (bind(theTCPReceiveSocket, (struct sockaddr *) myAddress, sizeof (*myAddress)) < 0) {
        //                dieWithError("bind() failed (tcp)");
        //            }
        //            if (listen(theTCPReceiveSocket, MAX_PENDING) < 0) {
        //                dieWithError("listen() failed");
        //            }
        //
        //            printf("# client port: %i", myAddress->sin_port);
        //
        //            socklen_t otherAddSize = sizeof (*theOtherAddress);
        //            if ((theTCPReceiveSocket = accept(theTCPReceiveSocket, (struct sockaddr *) theOtherAddress,
        //                    &otherAddSize)) < 0) {
        //                dieWithError("accept() failed");
        //            }
        //            /* Establish the connection to the echo server */
        //            //                    if (connect(theTCPReceiveSocket, (struct sockaddr *) theOtherAddress, sizeof (*theOtherAddress)) < 0) {
        //            //                        dieWithError("connect() failed");
        //            //                    }
        //
        //            //receive messages
        //            for (; *isTalking == true;) {
        //                if ((bytesReceived = recv(theTCPReceiveSocket, chatMessage, sizeof (chatMessage) - 1, 0)) <= 0) {
        //                    dieWithError("recv() failed or connection closed prematurely");
        //                }
        //                chatMessage[bytesReceived] = '\0'; /* Terminate the string! */
        //                printf("User %05i# %s", *otherID, chatMessage); /* Print the echo buffer */
        //            }


        //        } else { //forking() no good
        //            dieWithError("fork() failed");
        //        }
    } else { //forking() no good
        dieWithError("fork() failed");
    }

    return (EXIT_SUCCESS);
}

