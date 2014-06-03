
#ifndef UTILSUDP_H_
#define UTILSUDP_H_

#define MESSAGE_SIZE 100
#define DEFAULT_PORT 24564
#define NOTIFYMSG_TAG 'N'
#define SERVERMSG_TAG 'S'
#define CLIENTMSG_TAG 'C'

typedef struct {
    int type;

    enum {
        LOGIN, NOTIFY, LOGOUT
    } messageType; //same size as an unsigned int
    unsigned int clientID; //unique client identifier
} NotifyMessage; //an unsigned int is 32 bits = 4 bytes

typedef struct {
    int type;

    enum {
        SEND, VIEW
    } messageType; // same size as an unsigned int
    unsigned int senderId; // unique client identifier
    unsigned int recipientId; // unique client identifier
    char message[MESSAGE_SIZE]; // text message
} ClientMessage; // an unsigned int is 32 bits = 4 bytes

typedef struct {
    int type;

    enum {
        NEW, OLD, NO_MESSAGE //same size as an unsigned int
    } messageType;
    unsigned int senderId; //unique client identifier
    unsigned int recipientId; // unique client identifier
    char message[MESSAGE_SIZE]; // text message
} ServerMessage; //an unsigned int is 32 bits = 4 bytes

// External error handling function
void dieWithError(char *errorMessage);

#endif
