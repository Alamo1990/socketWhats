#include "server.h"



// mutex and condition variables for message copy
pthread_mutex_t mutex_msg;
int msg_not_copied = TRUE;
pthread_cond_t cond_msg;

// users queue
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

/* Send the response to the client after receiving a request */
void clientResponse(char response, struct sockaddr_in clientAddr, int sc ){
  send(sc, &response, 1, 0);
}

/* Send all the pending messages to the given user */
/*
 * This function is used to send the pending messages to one user and
 * also to send the ACK message to a sender user.
 * Because the ACK interchange protocol is different respect the
 * rest of messages, empty messages are sent to the user to
 * display correctly the information, reusing the code as much as possible
*/
void clientSentMessages(struct userInformation* user){

  // Prepare connection elements
  int sd;
  struct sockaddr_in server_addr;
  struct hostent *hp;
  sd = socket(AF_INET, SOCK_STREAM, 0);
  bzero( (char *)&server_addr, sizeof(server_addr) );
  hp = gethostbyname( inet_ntoa( user->user_addr) );
  memcpy( &(server_addr.sin_addr), hp->h_addr, hp->h_length);
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(user->user_port);

  // Default command to send message
  char *command = "SEND_MESSAGE\0";

  // Stablish connection
  if( connect(sd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1){
   // Error making connection, set user as disconnected
   user->status = OFF;
   close(sd);
   return;
  }

  // Pending messages in the queue
  while( !queue_empty(user->pending_messages) ){

   // Get the first message from pending queue
   struct messages *nextMsg = (struct messages *) dequeue(user->pending_messages);

   // Check if it is an ACK message
   if( strcmp(nextMsg->message, "SEND_MESS_ACK\0") == 0 ){
    // Send ACK message
    if( send(sd, nextMsg->message, strlen(nextMsg->message)+1, 0) == -1){
     // Error sending, enqueue in pending list
     enqueue(user->pending_messages, (void *)nextMsg);
     return;
    }

   // Null character
   char *empty = "\0";
   // Send null character instead of user name
   if( send(sd, empty, 1, 0) == -1){
    // Error sending, enqueue in pending list
    enqueue(user->pending_messages, (void *)nextMsg);
    return;
   }

   // send message id
   unsigned int msg_id = nextMsg->message_id;
   // Obtain the length of the id in characters
   double x = (double)(msg_id);
   int n = log10(x) + 1;
   char response[n];
   // Store unsigned integer as string
   sprintf(response, "%u", msg_id);
   if( send(sd, &response, n+1, 0 ) == -1 ){
    // Error sending, enqueue in pending list
    enqueue(user->pending_messages, (void *)nextMsg);
    return;
   }

   // Send null character instead of message
   if( send(sd, empty, 1, 0) == -1){
    // Error sending, enqueue in pending list
    enqueue(user->pending_messages, (void *)nextMsg);
    return;
   }

   // Restart the while loop
   continue;
  }


 // Send a regular message

 // Send command SEND_MESSAGE
 if( send(sd, command, strlen(command)+1, 0) == -1){
  // Error sending, enqueue in pending list
  enqueue(user->pending_messages, (void *)nextMsg);
  return;
 }

  // Send user name of sender
  if( send(sd, nextMsg->sender, strlen(nextMsg->sender)+1, 0) == -1 ){
   // Error sending, enqueue in pending list
   enqueue(user->pending_messages, (void *)nextMsg);
   return;
  }

  // send message id
  unsigned int msg_id = nextMsg->message_id;
  // Obtain the length of the id in characters
  double x = (double)(msg_id);
  int n = log10(x) + 1;
  char response[n];
  sprintf(response, "%u", msg_id);
  if( send(sd, &response, n+1, 0 ) == -1 ){
   // Error sending, enqueue in pending list
   enqueue(user->pending_messages, (void *)nextMsg);
   return;
  }

   // Send message
   char msgg[256];
   // Include null character
   sprintf(msgg, "%s", nextMsg->message);
   if( send(sd, msgg, strlen(msgg)+1, 0) == -1 ){
    // Error sending, enqueue in pending list
    enqueue(user->pending_messages, (void *)nextMsg);
    return;
   }


   // Console output
   printf("s> SEND MESSAGE %u FROM %s TO %s\n",nextMsg->message_id, nextMsg->sender, nextMsg->receiver );

   // Create ACK message
   // Get target user
   struct userInformation *usr = queue_find(queueUsers, nextMsg->sender);
   if( usr != NULL){
     struct messages* ackMsg = (struct messages *) malloc(sizeof(struct messages));
     bzero(ackMsg, sizeof(struct messages));
     ackMsg->message_id = nextMsg->message_id;
     strcpy(ackMsg->message, "SEND_MESS_ACK\0");
     // Insert in the pending_messages queue
     enqueue(usr->pending_messages, (void *) ackMsg);
     // Call this function with the sender user to send the ACK
     clientSentMessages(usr);
   }

  }
  // Close socket
  close(sd);
}

/* Register new user */
void registerUser(struct argumentWrapper *args){
 // start critical section
 pthread_mutex_lock(&mutex_msg);
  // Copy arguments
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

  // Check if user is already register
  struct userInformation *usr = queue_find(queueUsers, username);
    // Not registered
    if( usr == NULL){

    struct userInformation *user = (struct userInformation*)malloc(sizeof(struct userInformation));
    bzero(user, sizeof(struct userInformation));

    strcpy(user->username, username);
    user->status = OFF; // default status OFF
    // Initialize pending messages
    user->pending_messages = queue_new();
    user->last_message_id = 0;

    // Store new user in the queue
    enqueue(queueUsers, (void *) user);

    clientResponse(0, clientAddr, sc); // success
    printf("s> REGISTER %s OK\n", username );
  }else{
   // User already registered
   clientResponse(1, clientAddr, sc);
   printf("s> REGISTER %s FAIL\n", username );
  }
  // Close socket
  close(sc);
}


/* Unregister user */
void unregisterUser(struct argumentWrapper *args){

  char username[sizeof(args->username)];
  int sc;
  struct sockaddr_in clientAddr;

  // start critical section
  pthread_mutex_lock(&mutex_msg);
  // Make local copies of arguments
  strcpy(username,args->username);
  sc = args->clientFD;
  clientAddr = args->clientAddr;

  // release and end critical section
  msg_not_copied = FALSE;
  pthread_cond_signal(&cond_msg);
  pthread_mutex_unlock(&mutex_msg);

  free(args->username);

  // Remove user from the queue
  if(queue_remove(queueUsers, username)){
    clientResponse(0, clientAddr, sc); // success
    printf("s> UNREGISTER %s OK\n", username );
  }else{
   clientResponse(1, clientAddr, sc); // user does not exist
   printf("s> UNREGISTER %s FAIL\n", username );
  }
  // Close socket
  close(sc);
}

/* User connection */
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

  // Check user is registered
  if((user = queue_find(queueUsers, username)) != NULL){

   if(user->status == CONNECTED){
    clientResponse(2, clientAddr, sc); // user is already connected
    printf("s> CONNECT %s FAIL\n", username );
   }else{
    // User registered and not connected
    // Fill ip address, port and set status to connected
     user->user_addr = clientAddr.sin_addr;
     user->user_port = port;
     user->status = CONNECTED;
     clientResponse(0, clientAddr, sc); // success
     printf("s> CONNECT %s OK\n", username );

    // Send pending messages
    if( !queue_empty(user->pending_messages)){
      clientSentMessages(user);
    }
   }
  }else{
    clientResponse(1, clientAddr, sc); // user does not exist
    printf("s> CONNECT %s FAIL\n", username );
   }
  // Close socket
  close(sc);
}

/* User disconnect */
void disconnectUser(struct argumentWrapper *args){

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
 // Check if user is registered
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
 // Close socket
 close(sc);
}

/* Send message handler */
/*
 * This function receives the messages sent between users and
 * store them in the correspondign pending message queue
 * The logic of the message send is made in clientSentMessages function
 */
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

// Check sender exists
if( queue_find(queueUsers, usernameS) == NULL ){
  printf("Error: sender doesn't exist\n");
  clientResponse(1, clientAddr, sc);
  close(sc);
  return;
}else if(strlen(msg)>255){
  // Check message length
  printf("Error: message too long\n");
  clientResponse(1, clientAddr, sc);
  close(sc);
  return;
}

 struct userInformation *user = ((struct userInformation *)queue_find(queueUsers, usernameD));

 // Check user receiver exists
 if( user != NULL ){
   // message container
   struct messages* message = (struct messages *) malloc(sizeof(struct messages));
   bzero(message, sizeof(struct messages));

   // fill fields and update message id
   strcpy(message->message, msg);
   strcpy(message->sender, usernameS);
   strcpy(message->receiver, usernameD);
   // Increment message id
   message->message_id = user->last_message_id + 1;
   // Update user last message id
   user->last_message_id++;


   // enqueue in pending list
   enqueue(user->pending_messages, (void *)message);

   clientResponse(0, clientAddr, sc);

   // send message id
   unsigned int msg_id = message->message_id;
   double x = (double)(msg_id);
   int n = log10(x) + 1;
   char response[n];
   sprintf(response, "%u", msg_id);
   send(sc, &response, n, 0);

   // Check if receiver user is connected
   if(user->status == CONNECTED){
    // Send messages
    clientSentMessages(user);
  }else{
   printf("s> MESSAGE: %u FROM %s TO %s STORED\n", msg_id, message->sender, message->receiver );
  }
 }else{
  clientResponse(1, clientAddr, sc);
 }
 // Close socket
 close(sc);
}


int main(int argc, char**argv){
  // Initialize user queue
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

  // Create socket
  sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  val = 1;
  // Set socket options
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&val, sizeof(int));

  bzero( (char *)&serverAddr, sizeof(serverAddr) );

  // Fill internet address structure
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  serverAddr.sin_port = htons(port);

  // Bind address to socket
  if( bind(sd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1){
   perror("Bind error: ");
  }

  // Listen for connections on socket
  listen(sd, 5);
  size = sizeof(clientAddr);

  // catch CTRL + C signal and execute handler
  signal(SIGINT, signal_handler);


  // Get IPv4 IP address from interface eth0
  struct ifreq ifr;
  ifr.ifr_addr.sa_family = AF_INET;
  strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
  ioctl(sd, SIOCGIFADDR, &ifr);


  // Console output
  printf("s> init server %s:%d\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), port );
  printf("s> \n");
  while(1){

    // Accept connection from socket
    sc = accept( sd, (struct sockaddr *) &clientAddr, &size);

    // Get command
    int count = readLine(sc, buffer,256);
    if(count == -1){
      perror("Error reading command: ");
      continue;
    }else if(count == 0){
      printf("Error: command empty\n");
      continue;
    }

    // REGISTER command
    if(!strcmp(buffer, "REGISTER")){
      char *username;
      username = (char *) calloc(256,sizeof(char));
      // Read username
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

      // Arguments to thread
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

      // UNREGISTER command
    }else if(!strcmp(buffer, "UNREGISTER")){
      char *username;
      username = (char *) calloc(256,sizeof(char));
      // Read user name
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

      // Arguments to thread
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
    // CONNECT command
  }else if(!strcmp(buffer, "CONNECT")){
      char *username;
      char *portString;
      int port;
      username = (char *) calloc(256,sizeof(char));
      portString = (char *) calloc(8,sizeof(char));
      // Read username
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
      // Read port
      readLine(sc, portString, 8);
      port = atoi(portString);

      free(portString);
      // Arguments to thread
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

    // DISCONNECT command
  }else if(!strcmp(buffer, "DISCONNECT")){
     char *username;
     username = (char *) calloc(256,sizeof(char));
     // Read user name
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

     // Arguments to thread
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

     // SEND command
    }else if(!strcmp(buffer, "SEND")){
     // Read sender user name
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


     // Read destination user name
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

     // Read message
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

   // Close Socket
   close(sd);

  // Destroy synchronization elements
  destroySync();

 return 0;
}
