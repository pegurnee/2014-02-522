/* 
 * File:   UtilsServer.h
 * Author: eddie
 *
 * Created on June 11, 2014, 5:43 PM
 */

#include <arpa/inet.h>
#include <stdbool.h>

#ifndef UTILSSERVER_H
#define	UTILSSERVER_H

typedef struct {
    unsigned int clientID; //the user's id
    struct sockaddr_in address; //the address for a client, used in all the real time work
    bool loggedIn; //if the user is logged in
} Client;

int getUserIndex(unsigned int id, int numUsers, Client *users); //returns the index of the user with a provided ID

#endif	/* UTILSSERVER_H */

