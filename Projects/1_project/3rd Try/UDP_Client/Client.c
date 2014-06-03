/* 
 * File:   client.c
 * Author: eddie
 *
 */


#include <stdio.h>      // for printf() and fprintf()
#include <stdlib.h>     // for atoi() and exit()
#include <sys/socket.h> // for PF_INET, SOCK_DGRAM, socket(), connect(), sendto(), and recvfrom()
#include <string.h>     // for memset() and strlen()
#include <arpa/inet.h>  // for IPPROTO_UDP, sockaddr_in and inet_addr()
#include <unistd.h>     // for close()
#include <stdbool.h>
#include "UtilsUDP.h"
#include "Colors.h"


//#define DEFAULT_IP   "127.0.0.1" //emunix
#define DEFAULT_IP "192.168.1.6" // eddie's

int main(int argc, char *argv[]) {
    int cmd;
    unsigned int userID;

    int theSocket; /* Socket */
    struct sockaddr_in theClientAddress; //the incoming address
    struct sockaddr_in theServerAddress; //the address of the server

    bool loggedIn;
    pid_t processID;
    ClientMessage outbound; //message to send to other users
    ServerMessage incoming; //message brought in from the computer
    NotifyMessage notifier; //a notify message

    unsigned int fromSize = sizeof (theClientAddress);

    char *serverIP = DEFAULT_IP;
    unsigned short serverPort = DEFAULT_PORT;

    if ((theSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        dieWithError("socket() failed");
    }

    //create the server address structure
    memset(&theServerAddress, 0, sizeof (theServerAddress)); // Zero out structure
    theServerAddress.sin_family = AF_INET; /* Internet addr family */
    theServerAddress.sin_addr.s_addr = inet_addr(serverIP); /* Server IP address */
    theServerAddress.sin_port = htons(serverPort); /* Server port */

    //processCommandLineArgs(argc, argv, serverIP, &serverPort);
    //getUserId(&userID);
    //theSocket = createSocketAndSetAddress(&theServerAddress, serverIP, serverPort);
    //logIn(theSocket, userID, &outbound, &theServerAddress);

    processID = fork();

    if (processID == 0) { //child process
        for (;;) {
            recvfrom(theSocket, &incoming, sizeof (incoming), MSG_PEEK,
                    (struct sockaddr *) &theClientAddress, &fromSize);

            if (incoming.messageType == NOTIFY) {
                recvfrom(theSocket, &incoming, sizeof (incoming), 0,
                        (struct sockaddr *) &theClientAddress, &fromSize);
                printRed("\n-NEW INCOMING MESSAGE-\n");
            }
        }
    } else { //parent process
        for (;;) {
            if (loggedIn == false) { //if not logged in: Login, Exit
                printGrn("# Not logged in.\n");
                puts("1 to log-in");
                puts("2 to exit");
                printf("> ");
                scanf("%d", &cmd);
                getchar();
                switch (cmd) {
                    case 1: //login: enter user ID
                        puts("# Enter ID:");
                        printf("> ");
                        if (scanf("%u", &userID) <= 0) {
                            exit(EXIT_FAILURE);
                        }
                        getchar();

                        //sends a login notification to the server
                        memset(&notifier, 0, sizeof (notifier));
                        notifier.type = NOTIFYMSG_TAG;
                        notifier.clientID = userID;
                        notifier.messageType = LOGIN;

                        if (sendto(theSocket,
                                &notifier,
                                sizeof (notifier),
                                0,
                                (struct sockaddr *) &theServerAddress,
                                sizeof (theServerAddress))
                                != sizeof (notifier)) {
                            dieWithError("sendto() sent a different number of bytes than expected");
                        }

                        loggedIn = true;
                        break;
                    case 2: //exit: exits the client program
                        puts("# Goodbye");
                        kill(processID, SIGKILL); //kills the child notification process on logout
                        exit(EXIT_SUCCESS);
                        break;
                    default:
                        printRed("# Invalid Command.\n");
                        break;
                }
                puts("");
            } else {//if logged in: Send Message, View Messages, Logout, Exit
                printf("# Logged in as user %i.\n", userID);
                puts("1 to send message");
                puts("2 to view messages");
                puts("3 to logout");
                puts("4 to exit");
                printf("> ");
                scanf("%d", &cmd);
                getchar();
                switch (cmd) { //send/view message: creates an out message, applies message type and userID
                    case 1:
                    case 2:
                        memset(&outbound, 0, sizeof (outbound));
                        outbound.type = CLIENTMSG_TAG;
                        outbound.senderId = userID;

                        if (cmd == 1) { //Send Message: applies user entered text (up to 100 chars) to enter a message, sends it to user
                            outbound.messageType = SEND;
                            for (;;) {
                                puts("# Enter ID of the person you wish to message:");
                                printf("> ");
                                if (scanf("%u", &outbound.recipientId) > 0) {
                                    getchar();
                                    break;
                                }
                                printRed("# Invalid user ID, try again.\n");
                                scanf("%*[^\n]%*c"); //clears out all the input data
                            }

                            puts("# Enter your message:");
                            printf("> ");
                            fgets(outbound.message, MESSAGE_SIZE, stdin);
                            scanf("%*[^\n]%*c"); //clears out all the input data
                        } else { //View Messages: sends request to server, waits for server response of messages
                            outbound.messageType = VIEW;
                        }
                        if (sendto(theSocket,
                                &outbound,
                                sizeof (outbound),
                                0,
                                (struct sockaddr *) &theServerAddress,
                                sizeof (theServerAddress))
                                != sizeof (outbound)) {
                            dieWithError("sendto() sent a different number of bytes than expected");
                        }
                        if (cmd == 2) {
                            char *messageType;
                            bool firstMessage = false;
                            while (true) {
                                recvfrom(theSocket,
                                        &incoming,
                                        sizeof (incoming),
                                        MSG_PEEK,
                                        (struct sockaddr *) &theClientAddress,
                                        &fromSize);
                                if (incoming.type != NOTIFYMSG_TAG) {
                                    recvfrom(theSocket,
                                            &incoming,
                                            sizeof (incoming),
                                            0,
                                            (struct sockaddr *) &theClientAddress,
                                            &fromSize);
                                    if (incoming.messageType == NEW) {
                                        printf("# From: %-12d Status: %-8s\n",
                                                incoming.senderId,
                                                "Unread");
                                        printf("# %s", incoming.message);
                                    } else if (incoming.messageType == OLD) {
                                        printf("# From: %-12d Status: %-8s\n",
                                                incoming.senderId,
                                                "Read");
                                        printf("# %s", incoming.message);
                                    } else {
                                        printBlu("# You have no more messages :(");
                                        break;
                                    }
                                }
                            }
                        }
                        break;
                    case 3:
                        //sends a logout notification to the server
                        memset(&notifier, 0, sizeof (notifier));
                        notifier.type = NOTIFYMSG_TAG;
                        notifier.clientID = userID;
                        notifier.messageType = LOGOUT;

                        if (sendto(theSocket,
                                &notifier,
                                sizeof (notifier),
                                0,
                                (struct sockaddr *) &theServerAddress,
                                sizeof (theServerAddress))
                                != sizeof (notifier)) {
                            dieWithError("sendto() sent a different number of bytes than expected");
                        }

                        loggedIn = false;
                        printBlu("# Successfully logged out.\n");
                        break;
                    case 4:
                        //sends a logout notification to the server
                        memset(&notifier, 0, sizeof (notifier));
                        notifier.type = NOTIFYMSG_TAG;
                        notifier.clientID = userID;
                        notifier.messageType = LOGOUT;

                        if (sendto(theSocket,
                                &notifier,
                                sizeof (notifier),
                                0,
                                (struct sockaddr *) &theServerAddress,
                                sizeof (theServerAddress))
                                != sizeof (notifier)) {
                            dieWithError("sendto() sent a different number of bytes than expected");
                        }
                        loggedIn = false;
                        puts("# Goodbye.");
                        kill(processID, SIGKILL); //kills the child notification process on logout
                        exit(EXIT_SUCCESS);
                        break;
                    default:
                        printRed("# Invalid Command.\n");
                        break;
                }
                puts("");
            }
        }
    }
    return 0;
}