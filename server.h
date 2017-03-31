#ifndef _SERVER_H_
#define _SERVER_H_

#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <pthread.h>

#include "read_line.c"

// FIXME put in a header file
struct argumentWrapper {
        char *username;
        int clientFD;
        struct sockaddr_in clientAddr;

};

struct messages {
        unsigned int message_id;
        char *message;
        char *sender;
        char *receiver;
};
struct userInformation{
  char username[256];
  char status;
  struct in_addr user_addr;
  unsigned short user_port;
  //struct messages *pending_messages;
  struct queue *pending_messages;
  unsigned int last_message_id;
};
struct queue *queueUsers;


void initializeSync(void);
void destroySync(void);
void registerUser(struct argumentWrapper *args);
void connectUser(struct argumentWrapper *args);




#include "queue.h"

// FIXME
#include "queue.c"


#endif
