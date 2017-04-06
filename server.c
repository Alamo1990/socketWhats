#include "server.h"

#define TRUE 1
#define FALSE 0

// mutex and condition variables for message copy
pthread_mutex_t mutex_msg;
int msg_not_copied = TRUE;
pthread_cond_t cond_msg;

struct queue *queueUsers;

// server file descriptor
int sd;
// server buffer
char *buffer;

// Initialize synchronization elements
void initializeSync(){
        if( pthread_mutex_init(&mutex_msg, NULL) != 0) {
                perror("Not possible to initialize mutex");
                exit(0);
        };

        if( pthread_cond_init(&cond_msg, NULL) != 0) {
                perror("Not possible to initialize condition variable");
                exit(0);
        }
}

// destroy synchronization elements
void destroySync(){
        if( pthread_mutex_destroy(&mutex_msg) != 0) {
                perror("Not possible to destroy mutex");
                exit(0);
        };

        if( pthread_cond_destroy(&cond_msg) != 0) {
                perror("Not possible to destroy condition variable");
                exit(0);
        }
}

// retrieve CTRL+C signal
void signal_handler(int sig){
  // close server port
  close(sd);
  // free buffer
  free(buffer);
  // free queue
  free(queueUsers);
  // destroy synchronization elements
  destroySync();
  // end process
  exit(0);
}


void clientResponse(char response, struct sockaddr_in clientAddr, int sc ){
  // Response to client
  //sendto(sc, &response, 1, 0, (struct sockaddr *) &clientAddr,sizeof(clientAddr) );
  send(sc, &response, 1, 0);
}
// NINJA
//void clientSentMessages(struct userInformation* user,struct sockaddr_in clientAddr, int clientPort){
void clientSentMessages(struct userInformation* user, int clientPort){

 int sd;
  struct sockaddr_in server_addr;
  struct hostent *hp;
  sd = socket(AF_INET, SOCK_STREAM, 0);
  bzero( (char *)&server_addr, sizeof(server_addr) );

  //hp = gethostbyname( inet_ntoa( ((struct sockaddr_in *)&clientAddr)->sin_addr) );
  hp = gethostbyname( inet_ntoa( user->user_addr) );

  memcpy( &(server_addr.sin_addr), hp->h_addr, hp->h_length);
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(clientPort);

    char *command = "SEND_MESSAGE\0";
    char *usr = "Pepe\0";
    char *id = "312321\0";
    char *msg = "HOLA LOCO\0";
    char *endMsg = '\0';
    connect(sd, (struct sockaddr *) &server_addr, sizeof(server_addr));

    while( !queue_empty(user->pending_messages) ){
     struct messages *nextMsg = (struct messages *) dequeue(user->pending_messages);
     send(sd, command, strlen(command)+1, 0);
     send(sd, nextMsg->sender, strlen(command)+1, 0);
     send(sd, (char *)&nextMsg->message_id, sizeof(unsigned int)+1, 0);
     send(sd, nextMsg->message, strlen(nextMsg->message)+1, 0);

     printf("s> MESSAGE %u FROM %s TO %s\n",nextMsg->message_id, nextMsg->sender, nextMsg->receiver );
     //send(sd, endMsg, strlen(endMsg)+1, 0);
    }

    //send(sd, (char *)&num, sizeof(int), 0);
    //sendto(sd, (char *)&response, sizeof(int), 0, (struct sockaddr *) &clientAddr,sizeof(clientAddr) );
    //sendto(sd, (char *)&response, sizeof(int), 0, (struct sockaddr *) &user->user_addr,sizeof(user->user_addr) );
    //sendto(sd, command, strlen(command)+1, 0, (struct sockaddr *) &user->user_addr,sizeof(user->user_addr) );
    //sendto(sd, usr, strlen(usr)+1, 0, (struct sockaddr *) &user->user_addr,sizeof(user->user_addr) );
    //sendto(sd, id, strlen(id)+1, 0, (struct sockaddr *) &user->user_addr,sizeof(user->user_addr) );
    //sendto(sd, msg, strlen(msg)+1, 0, (struct sockaddr *) &user->user_addr,sizeof(user->user_addr) );

    //sendto(sd, command, strlen(command), 0, (struct sockaddr *) &user->user_addr,sizeof(user->user_addr) );
    //sendto(sd, usr, strlen(usr), 0, (struct sockaddr *) &user->user_addr,sizeof(user->user_addr) );
    //sendto(sd, id, strlen(id), 0, (struct sockaddr *) &user->user_addr,sizeof(user->user_addr) );
    //sendto(sd, msg, strlen(msg), 0, (struct sockaddr *) &user->user_addr,sizeof(user->user_addr) );

    // send(sd, command, strlen(command)+1, 0);
    // send(sd, usr, strlen(usr)+1, 0);
    // send(sd, id, strlen(id)+1, 0);
    // send(sd, msg, strlen(msg)+1, 0);
    //
    // send(sd, command, strlen(command)+1, 0);
    // send(sd, usr, strlen(usr)+1, 0);
    // send(sd, id, strlen(id)+1, 0);
    // send(sd, msg, strlen(msg)+1, 0);


    close(sd);
}


void registerUser(struct argumentWrapper *args){
 // start critical section
 pthread_mutex_lock(&mutex_msg);
  char username[sizeof(args->username)];
  int sc;
  struct sockaddr_in clientAddr;



  strcpy(username,args->username);
  sc = args->clientFD;
  clientAddr = args->clientAddr;

  // release and end critical section
  msg_not_copied = FALSE;
  pthread_cond_signal(&cond_msg);
  pthread_mutex_unlock(&mutex_msg);

  free(args->username);

  printf("___username send to find: %s \n", username);
  struct userInformation *usr = queue_find(queueUsers, username);
    //if( queue_find(queueUsers, username) == NULL){
    if( usr == NULL){

    struct userInformation *user = (struct userInformation*)malloc(sizeof(struct userInformation));
    bzero(user, sizeof(struct userInformation));

    strcpy(user->username, username);
    user->status = OFF; // default status OFF
    // Initialize pending messages
    user->pending_messages = queue_new();
    user->last_message_id = 0;

    enqueue(queueUsers, (void *) user);

    clientResponse(0, clientAddr, sc); // success
    printf("s> REGISTER %s OK\n", username );
  }else{
   printf("___username given: %s \n", username);
   printf("USER already registered %s //  status:%c\n", usr->username, usr->status);
   clientResponse(1, clientAddr, sc); // user exist
   printf("s> REGISTER %s FAIL\n", username );
  }
  close(sc);

}



void unregisterUser(struct argumentWrapper *args){

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

  if(queue_find_remove(queueUsers, username)){
    clientResponse(0, clientAddr, sc); // success
    printf("s> UNREGISTER %s OK\n", username );
  }else{
   clientResponse(1, clientAddr, sc); // user does not exist
   printf("s> UNREGISTER %s FAIL\n", username );
  }
  close(sc);
}

void  connectUser(struct argumentWrapper *args){
  char username[sizeof(args->username)];
  int sc;
  int port;
  struct userInformation* user;
  struct sockaddr_in clientAddr;

  // start critical section
  pthread_mutex_lock(&mutex_msg);

  strcpy(username,args->username);
  sc = args->clientFD;
  clientAddr = args->clientAddr;
  port = args->clientPort;
  // release and end critical section
  msg_not_copied = FALSE;
  pthread_cond_signal(&cond_msg);
  pthread_mutex_unlock(&mutex_msg);

  free(args->username);
  if((user = queue_find(queueUsers, username)) != NULL){
      if(user->status == CONNECTED){
       clientResponse(2, clientAddr, sc); // user is already connected
       printf("s> CONNECT %s FAIL\n", username );
      }else{
        user->user_addr = clientAddr.sin_addr;
        user->user_port = port;
        user->status = CONNECTED;
        clientResponse(0, clientAddr, sc); // success
        printf("s> CONNECT %s OK\n", username );


       //if( !queue_empty(user->pending_messages)){
         //clientSentMessages(user,clientAddr, port);
         clientSentMessages(user,port);
       //}

      }
  }else{
    clientResponse(1, clientAddr, sc); // user does not exist
    printf("s> CONNECT %s FAIL\n", username );
   }

  close(sc);
}


void disconnectUser(struct argumentWrapper *args){
 printf("in disconnectUser\n");
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

 struct userInformation *user = queue_find(queueUsers, username);

 if( user != NULL){
  if(user->user_addr.s_addr != clientAddr.sin_addr.s_addr){
   // registered ip and connection ip don't match
   clientResponse(3, clientAddr, sc);
   printf("s> DISCONNECT %s FAIL\n", username );
  }else if(user->status == CONNECTED){
   user->status = OFF; // disconnect status
   user->user_port = 0; // remove port
   user->user_addr.s_addr = 0; // remove ip
   clientResponse(0, clientAddr, sc); // success
   printf("s> DISCONNECT %s OK\n", username );
  }else{
   clientResponse(2, clientAddr, sc); // user not connected
   printf("s> DISCONNECT %s FAIL\n", username );
  }
 }else{
  clientResponse(1, clientAddr, sc); // user does not exist
  printf("s> DISCONNECT %s FAIL\n", username );
 }
 close(sc);
}


void sendMsg(struct argumentWrapper *args){
 int sc;
 struct sockaddr_in clientAddr;
 char usernameS[256];
 char usernameD[256];
 char msg[256];

 // start critical section
 pthread_mutex_lock(&mutex_msg);
 strcpy(usernameS,args->username);
 strcpy(usernameD,args->usernameD);
 strcpy(msg,args->msg);
 sc = args->clientFD;
 clientAddr = args->clientAddr;

 free(args->username);
 free(args->usernameD);
 free(args->msg);
 // release and end critical section
 msg_not_copied = FALSE;
 pthread_cond_signal(&cond_msg);
 pthread_mutex_unlock(&mutex_msg);


 printf("source: %s, destination: %s, message: %s\n",usernameS, usernameD, msg );

// Check sender exists
if( queue_find(queueUsers, usernameS) == NULL ){
  printf("Error: sender doesn't exist\n");
  clientResponse(1, clientAddr, sc);
  close(sc);
  return;
}else if(strlen(msg)>255){
  printf("Error: message too long\n");
  clientResponse(1, clientAddr, sc);
  close(sc);
  return;
}

 struct userInformation *user = ((struct userInformation *)queue_find(queueUsers, usernameD));

 // Check destination exists
 if( user != NULL ){
    // message container
    struct messages* message = (struct messages *) malloc(sizeof(struct messages));
    bzero(message, sizeof(struct messages));

    // fill fields and update message id
    strcpy(message->message, msg);
    message->sender = usernameS;
    message->receiver = usernameD;
    message->message_id = user->last_message_id + 1;
    user->last_message_id++;


    // enqueue in pending list
    enqueue(user->pending_messages, (void *)message);

    clientResponse(0, clientAddr, sc);

    // send message id
    unsigned int msg_id = message->message_id;
    printf("Message id: %d\n", msg_id);
    double x = (double)(msg_id);
    int n = log10(x) + 1;
    char response[n];
    sprintf(response, "%d\0", msg_id);
    printf("Response ID: %s\n", response);
    sendto(sc, &response, n, 0, (struct sockaddr *) &clientAddr,sizeof(clientAddr) );

  if(user->status == CONNECTED){//Send message straight away FIXME
    printf("User %s connected\n", user->username);
    int clientSocket;
    struct sockaddr_in client_addr;
    struct hostent *clientHP;

    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    bzero((char*)&client_addr, sizeof(client_addr));

    memcpy(&(client_addr.sin_addr), &(user->user_addr), sizeof(user->user_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(user->user_port);

    connect(clientSocket, (struct sockaddr *)&client_addr, sizeof(client_addr));

    send(clientSocket, "SEND_MESSAGE\0", 16, 0);
    sendto(clientSocket, &msg, 256, 0, (struct sockaddr *) &client_addr,sizeof(client_addr) );

  }
 }else{
  clientResponse(1, clientAddr, sc);
 }

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


  // client file descriptor
  int sc;
  // sockets addresses
  struct sockaddr_in serverAddr, clientAddr;

  int size, val;

  // Allocate memory for buffer
  buffer = (char *) calloc(BUFFER_SIZE,sizeof(char));


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

  // catch CTRL + C signal and execute handler
  signal(SIGINT, signal_handler);

//NINJA
  //char ipAddress[INET_ADDRSTRLEN];
  //inet_ntop(AF_INET, (void *)&(serverAddr.sin_addr), ipAddress, INET_ADDRSTRLEN);
  // Console output
  printf("s> init server %s:%d\n", inet_ntoa( ((struct sockaddr_in *)&serverAddr)->sin_addr  ), port );

  while(1){
    printf("Waiting for connection \n");

    sc = accept( sd, (struct sockaddr *) &clientAddr, &size);

    int count = readLine(sc, buffer,256);
    if(count == -1){
      perror("Error reading command: ");
    }else if(count == 0){
      printf("Error: command empty\n");
    }
    printf("Command: %s\n", buffer);


    if(!strcmp(buffer, "REGISTER")){
      char *username;
      username = (char *) calloc(256,sizeof(char));

      count = readLine(sc, username, 256);

      if(count == -1){
        perror("Error reading username: ");
        clientResponse(2, clientAddr, sc);
        close(sc);
        continue;
      }else if(count == 0){
        printf("Error: username empty\n");
        clientResponse(2, clientAddr, sc);
        close(sc);
        continue;
      }

     // free(username);

      struct argumentWrapper args;
      args.username = username;
      args.clientFD = sc;
      args.clientAddr = clientAddr;

      pthread_t thid;
      // create new thread with message received
      if( pthread_create( &thid, &thread_attributes, (void *) registerUser, &args) != 0 ){
        perror("Error creating thread");
        close(sc);
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

    }else if(!strcmp(buffer, "UNREGISTER")){
      char *username;
      username = (char *) calloc(256,sizeof(char));
      count = readLine(sc, username, 256);

      if(count == -1){
        perror("Error reading username: ");
        clientResponse(2, clientAddr, sc);
        close(sc);
        continue;
      }else if(count == 0){
        printf("Error: username empty\n");
        clientResponse(2, clientAddr, sc);
        close(sc);
        continue;
      }

      printf("count: %d, username: %s\n", count,username);
     // free(username);

      struct argumentWrapper args;
      args.username = username;
      args.clientFD = sc;
      args.clientAddr = clientAddr;

      pthread_t thid;
      // create new thread with message received
      if( pthread_create( &thid, &thread_attributes, (void *) unregisterUser, &args) != 0 ){
        perror("Error creating thread");
        close(sc);
        continue;
    }
  }else if(!strcmp(buffer, "CONNECT")){
      char *username;
      char *portString;
      int port;
      username = (char *) calloc(256,sizeof(char));
      portString = (char *) calloc(8,sizeof(char));
      count = readLine(sc, username, 256);

      if(count == -1){
        perror("Error reading username: ");
        clientResponse(2, clientAddr, sc);
        close(sc);
        continue;
      }else if(count == 0){
        printf("Error: username empty\n");
        clientResponse(2, clientAddr, sc);
        close(sc);
        continue;
      }
      readLine(sc, portString, 8);
      port = atoi(portString);

      printf("count: %d, username: %s, port: %d\n", count, username, port);
     // free(username);
     free(portString);

      struct argumentWrapper args;
      args.username = username;
      args.clientFD = sc;
      args.clientAddr = clientAddr;
      args.clientPort = port;

      pthread_t thid;
      // create new thread with message received
      if( pthread_create( &thid, &thread_attributes, (void *) connectUser, &args) != 0 ){
        perror("Error creating thread");
        close(sc);
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

  }else if(!strcmp(buffer, "DISCONNECT")){
     char *username;
     username = (char *) calloc(256,sizeof(char));
     count = readLine(sc, username, 256);

     if(count == -1){
       perror("Error reading username: ");
       clientResponse(3, clientAddr, sc);
       close(sc);
       continue;
     }else if(count == 0){
       printf("Error: username empty\n");
       clientResponse(3, clientAddr, sc);
       close(sc);
       continue;
     }

     struct argumentWrapper args;
     args.username = username;
     args.clientFD = sc;
     args.clientAddr = clientAddr;

     pthread_t thid;
     // create new thread with message received
     if( pthread_create( &thid, &thread_attributes, (void *) disconnectUser, &args) != 0 ){
       perror("Error creating thread");
       close(sc);
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

    }else if(!strcmp(buffer, "SEND")){
     // get sender user name
     char *usernameS;
     usernameS = (char *) calloc(256,sizeof(char));
     count = readLine(sc, usernameS, 256);
     if(count == -1){
       perror("Error reading source username: ");
       clientResponse(2, clientAddr, sc);
       close(sc);
       continue;
     }else if(count == 0){
       printf("Error: source username empty\n");
       clientResponse(2, clientAddr, sc);
       close(sc);
       continue;
     }


     // get destination user name
     char *usernameD;
     usernameD = (char *) calloc(256,sizeof(char));
     count = readLine(sc, usernameD, 256);
     if(count == -1){
       perror("Error reading destination username: ");
       clientResponse(2, clientAddr, sc);
       close(sc);
       continue;
     }else if(count == 0){
       printf("Error: destination username empty\n");
       clientResponse(2, clientAddr, sc);
       close(sc);
       continue;
     }

     char *msg;
     msg = (char *) calloc(256,sizeof(char));
     count = readLine(sc, msg, 256);
     if(count == -1){
       perror("Error reading message: ");
       clientResponse(2, clientAddr, sc);
       close(sc);
       continue;
     }else if(count == 0){
       printf("Error: message empty\n");
       clientResponse(2, clientAddr, sc);
       close(sc);
       continue;
     }

     struct argumentWrapper args;
     args.clientFD = sc;
     args.clientAddr = clientAddr;
     args.username = usernameS;
     args.usernameD = usernameD;
     args.msg = msg;


     pthread_t thid;
     // create new thread with message received
     if( pthread_create( &thid, &thread_attributes, (void *) sendMsg, &args) != 0 ){
       perror("Error creating thread");
       close(sc);
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

  }else printf("Function not found\n");


  }// while

  // free(buffer);
   close(sd);

  // Destroy synchronization elements
  destroySync();

 return 0;
}
