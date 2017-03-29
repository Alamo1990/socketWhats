#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <pthread.h>

#include "queue.h"

// FIXME
#include "queue.c"
#include "read_line.c"

#define TRUE 1
#define FALSE 0

// mutex and condition variables for message copy
pthread_mutex_t mutex_msg;
int msg_not_copied = TRUE;
pthread_cond_t cond_msg;

// FIXME put in a header file
struct argumentWrapper{
  char *username;
  int clientFD;
  struct sockaddr_in clientAddr;

};

struct messages{
    unsigned int message_id;
    char *message;
    char *sender;
    char *receiver;
};

struct userInformation{
  char *username;
  char status;
  struct in_addr user_addr;
  unsigned short user_port;
  struct messages *pending_messages;
  unsigned int last_message_id;

};
struct queue *queueUsers;


// Initialize synchronization elements
void initializeSync(){
  if( pthread_mutex_init(&mutex_msg, NULL) != 0){
    perror("Not possible to initialize mutex");
    exit(0);
  };

  if( pthread_cond_init(&cond_msg, NULL) != 0){
    perror("Not possible to initialize condition variable");
    exit(0);
  }
}

// destroy synchronization elements
void destroySync(){
  if( pthread_mutex_destroy(&mutex_msg) != 0){
    perror("Not possible to destroy mutex");
    exit(0);
  };

  if( pthread_cond_destroy(&cond_msg) != 0){
    perror("Not possible to destroy condition variable");
    exit(0);
  }
}


void registerUser(struct argumentWrapper *args){

  char username[sizeof(args->username)];
  int sc;
  struct sockaddr_in clientAddr;

  // start critical section
  pthread_mutex_lock(&mutex_msg);

  strcpy(username,args->username);
  sc = args->clientFD;
  clientAddr = args->clientAddr;

  // release and end critical section
  msg_not_copied = FALSE;
  pthread_cond_signal(&cond_msg);
  pthread_mutex_unlock(&mutex_msg);

  free(args->username);




  printf("username %s\n", username);
  char res = 1;
  sendto(sc, &res, 1, 0, (struct sockaddr *) &clientAddr,sizeof(clientAddr) );
  close(sc);

}



int main(int argc, char**argv){

  queueUsers = queue_new();

  // Initialize synchronization elements
  initializeSync();

  int port;
  // parse arguments
  if(argc != 3 || strcmp(argv[1], "-p")){
   printf("Invalid arguments, Usage: ./server -p <port number>\n");
   return -1;
  }else{
   port = atoi(argv[2]);
   if(port <= 1024 || port > 65535 ){
    printf("Invalid port number, use one from 1025 to 65535.\n");
    return -1;
   }
  }

  // create thread attributes and set detached
  pthread_attr_t thread_attributes;
  if( pthread_attr_init(&thread_attributes) !=0 ){
    perror("Not possible to initialize thread attributes");
    return 1;
  }
  if( pthread_attr_setdetachstate(&thread_attributes, PTHREAD_CREATE_DETACHED) !=0 ){
    perror("Not possible to set thread attributes to Detached");
    return 1;
  }


  // server and client file descriptor
  int sd, sc;
  // sockets addresses
  struct sockaddr_in serverAddr, clientAddr;

  int size, val;
  //int res;
  //char buffer[50];
  char *buffer;
  buffer = (char *) calloc(50,sizeof(char));


  sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  val = 1;
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&val, sizeof(int));

  bzero( (char *)&serverAddr, sizeof(serverAddr) );

  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  serverAddr.sin_port = htons(port);

  if( bind(sd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1){
   perror("Bind error: ");
  }

  listen(sd, 5);
  size = sizeof(clientAddr);

  while(1){
    printf("Waiting for connection \n");

    sc = accept( sd, (struct sockaddr *) &clientAddr, &size);
/*
     int count = recv_msg(sc, buffer, 8);
     printf("%s\n", buffer);
     int length = 0;
     count = recv_msg(sc, (char *)&length, 4);
     length = ntohl(length);
     printf("%d\n", length );
     char username[length+1];
     count = recv_msg(sc, (char *)&username, length);
     printf("%s\n", username );

    // send(sc, &res, sizeof(int), 0);
    char res = 1;
    //uint8_t res = '1';
    sendto(sc, &res, 1, 0, (struct sockaddr *) &clientAddr,size );
*/

    int count = readLine(sc, buffer,256);
    if(count == -1){
      perror("Error reading command: ");
    }else if(count == 0){
      printf("Error: command empty\n");
    }
    printf("Command: %s\n", buffer);


     if(strstr(buffer, "REGISTER")){
    //if(strcmp(buffer, "REGISTER")){
      char *username;
      username = (char *) calloc(256,sizeof(char));
      count = readLine(sc, username, 256);

      if(count == -1){
        perror("Error reading username: ");
      }else if(count == 0){
        printf("Error: username empty\n");
      }

      printf("count: %d, username: %s\n", count,username);
     // free(username);

      struct argumentWrapper args;
      args.username = username;
      args.clientFD = sc;
      args.clientAddr = clientAddr;

      pthread_t thid;
      // create new thread with message received
      if( pthread_create( &thid, &thread_attributes, (void *) registerUser, &args) != 0 ){
        perror("Error creating thread");
        continue;
      }

      // start critical section
      pthread_mutex_lock(&mutex_msg);
      while(msg_not_copied){
        pthread_cond_wait(&cond_msg, &mutex_msg);
      }

      // end critical section
      msg_not_copied = TRUE;
      pthread_mutex_unlock(&mutex_msg);



    //  char res = 1;
    //  sendto(sc, &res, 1, 0, (struct sockaddr *) &clientAddr,size );
    //close(sc);
    }


    //close(sc);

  }

   free(buffer);
   close(sd);

  // Destroy synchronization elements
  destroySync();

 return 0;
}
