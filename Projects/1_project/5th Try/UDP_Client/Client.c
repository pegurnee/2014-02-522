/* 
 * File:   client.c
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
#include <stdbool.h>
#include "UtilsUDP.h"
#include "Colors.h"


#define DEFAULT_IP   "127.0.0.1" //localhost
//#define DEFAULT_IP "192.168.1.6" //eddie's
//#define DEFAULT_IP "164.76.78.62" //eddie's at emich

int main(int argc, char *argv[]) {
    int cmd;
    char fname[20];
    void *ptr; //used to just check what is the first bit of data of messages

    int theSocket; /* Socket */
    struct sockaddr_in theClientAddress; //the incoming address
    struct sockaddr_in theServerAddress; //the address of the server

    FILE *ofp;
    bool loggedIn;
    pid_t processID;
    ClientMessage outbound; //message to send to other users
    ServerMessage incoming; //message brought in from the computer
    NotifyMessage notifier; //a notify message
    VerifyMessage verifier; //a verify message

    unsigned int clientAddressLength = sizeof (theClientAddress);

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

    processID = fork();

    if (processID == 0) { //child process, all the notifications and good stuff
        for (;;) {
            recvfrom(theSocket,
                    &notifier,
                    sizeof (notifier),
                    MSG_PEEK,
                    (struct sockaddr *) &theClientAddress,
                    &clientAddressLength);

            if (notifier.type == NOTIFYMSG_TAG
                    && notifier.messageType == NOTIFY) {
                recvfrom(theSocket,
                        &incoming,
                        sizeof (incoming),
                        0,
                        (struct sockaddr *) &theClientAddress,
                        &clientAddressLength);
                printRed("\n# You have received a new message.\n");
                printf(">");
                fflush(stdout);
            }
        }
    } else { //parent process
        unsigned int userID; //it needs to remain only in the parent process
        int i; //used for looping
        for (;;) {
            if (loggedIn == false) { //if not logged in: Login, Exit
                printGrn("# Not logged in.\n");
                puts("# 1 to log-in");
                puts("# 2 to exit");
                printf("> ");
                scanf("%d", &cmd);
                getchar();
                switch (cmd) {
                    case 1: //login: enter user ID
                        puts("# Enter ID:");
                        printf("> ");
                        if (scanf("%u", &userID) <= 0) {
                            printRed("# Invalid User ID.\n");
                            break;
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
            } else { //if logged in: Send Message, View Messages, Logout, Exit
                colorGrn();
                printf("# Logged in as user %i.\n", userID);
                colorRes();
                puts("# 1 to send message");
                puts("# 2 to view messages");
                puts("# 3 to save messages");
                puts("# 4 to logout");
                puts("# 5 to exit");
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
                        } else { //View Messages: sends request to server, waits for server response of messages
                            outbound.messageType = VIEW;
                            memset(&incoming, 0, sizeof (incoming));
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
                            i = 0;
                            for (;;) {
                                //just the tag
                                recvfrom(theSocket,
                                        &incoming,
                                        sizeof (incoming),
                                        MSG_PEEK,
                                        (struct sockaddr *) &theClientAddress,
                                        &clientAddressLength);
                                if (incoming.type == SERVERMSG_TAG) {
                                    recvfrom(theSocket,
                                            &incoming,
                                            sizeof (incoming),
                                            0,
                                            (struct sockaddr *) &theClientAddress,
                                            &clientAddressLength);
                                    //print out the message, sender and read status
                                    printf("# Message %i:\n", ++i);
                                    printf("# From: " KMAG "%-15d" KRES " Status: %s%-10s" KRES "\n",
                                            incoming.senderId,
                                            (incoming.messageType == NEW) ? KYEL : KCYN,
                                            (incoming.messageType == NEW) ? "Unread" : "Read");
                                    printf("# %s\n", incoming.message);
                                    fflush(stdout);
                                } else if (incoming.type == VERIFYMSG_TAG) { //end of the messages
                                    recvfrom(theSocket,
                                            &verifier,
                                            sizeof (verifier),
                                            0,
                                            (struct sockaddr *) &theClientAddress,
                                            &clientAddressLength);
                                    if (verifier.messageType == YES) {
                                        if (i == 0) { //if i was not incremented, no messages
                                            printRed("# You have no messages.\n");
                                        } else {
                                            printRed("# You have no more messages.\n");
                                        }
                                    }
                                    break;
                                }
                            }
                        } else {
                            puts("# Message sent.");
                        }
                        break;
                    case 3:
                        memset(&incoming, 0, sizeof (incoming));
                        puts("# Enter file name where to save messages:");
                        printf("> ");
                        fgets(fname, 20, stdin);
                        fname[strlen(fname) - 1] = '\0';
                        strcat(fname, ".txt\0");

                        ofp = fopen(fname, "w");

                        fprintf(ofp, "User %i's messages:\n\n", userID);
                        outbound.messageType = PRINT;
                        if (sendto(theSocket,
                                &outbound,
                                sizeof (outbound),
                                0,
                                (struct sockaddr *) &theServerAddress,
                                sizeof (theServerAddress))
                                != sizeof (outbound)) {
                            dieWithError("sendto() sent a different number of bytes than expected");
                        }
                        i = 0;
                        //prints out the messages to a file
                        for (;;) {
                            //just the tag
                            recvfrom(theSocket,
                                    &incoming,
                                    sizeof (incoming),
                                    MSG_PEEK,
                                    (struct sockaddr *) &theClientAddress,
                                    &clientAddressLength);
                            if (incoming.type == SERVERMSG_TAG) {
                                recvfrom(theSocket,
                                        &incoming,
                                        sizeof (incoming),
                                        0,
                                        (struct sockaddr *) &theClientAddress,
                                        &clientAddressLength);
                                //print out to the file the message and sender
                                fprintf(ofp, "# Message %i:\n", ++i);
                                fprintf(ofp, "# From user: %d\n", incoming.senderId);
                                fprintf(ofp, "# %s\n\n", incoming.message);
                            } else if (incoming.type == VERIFYMSG_TAG) { //end of the messages
                                recvfrom(theSocket,
                                        &verifier,
                                        sizeof (verifier),
                                        0,
                                        (struct sockaddr *) &theClientAddress,
                                        &clientAddressLength);
                                if (verifier.messageType == YES) {
                                    printYel("# File writing successful.\n");
                                    fclose(ofp);
                                }
                                break;
                            }
                        }
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
                        printf("# Successfully logged out.\n");
                        break;
                    case 5:
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