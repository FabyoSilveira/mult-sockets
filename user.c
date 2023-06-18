#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>

#define BUFFER_SIZE 1024
#define MESSAGE_SIZE 2048
//Message types
#define REQ_ADD 1
#define REQ_REM 2
#define RES_LIST 4
#define MSG 6
#define ERROR 7
#define OK 8

#define MAX_CLIENTS 15

typedef struct {
  int idMsg;
  int idSender;
  int idReceiver;
  char message[2048];
} Payload;

int clientsIdDb[MAX_CLIENTS] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
int myID; 

bool connectedToServer = 1;

int createAndConnectSockToServerIPV4orIPV6(char* ip, int port){
  int sock;
  struct sockaddr_in servAddrV4;
  struct sockaddr_in6 servAddrV6;

  //IPV4
  if(inet_pton(AF_INET, ip, &servAddrV4.sin_addr) == 1){

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      printf("Criação do socket falhou!\n");
      return -1;
    }
    
    memset(&servAddrV4, '0', sizeof(servAddrV4));
    servAddrV4.sin_family = AF_INET;
    servAddrV4.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &servAddrV4.sin_addr) <= 0) {
      printf("Endereço v4 não suportado\n");
      return -1;
    }

    if (connect(sock, (struct sockaddr *)&servAddrV4, sizeof(servAddrV4)) < 0) {
      printf("Connection Failed\n");
      return -1;
    }

    //printf("Conectado com sucesso ao server %s porta %d\n", ip, port);

  //IPV6
  }else if(inet_pton(AF_INET6, ip, &servAddrV6.sin6_addr) == 1){

    if ((sock = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
      printf("Criação do socket falhou!\n");
      return -1;
    }
    
    memset(&servAddrV6, '0', sizeof(servAddrV6));
    servAddrV6.sin6_family = AF_INET6;
    servAddrV6.sin6_port = htons(port);

    if (inet_pton(AF_INET6, ip, &servAddrV6.sin6_addr) <= 0) {
      printf("Endereço v6 não suportado\n");
      return -1;
    }

    if (connect(sock, (struct sockaddr *)&servAddrV6, sizeof(servAddrV6)) < 0) {
      printf("Connection Failed\n");
      return -1;
    }

    //printf("Conectado com sucesso ao server %s porta %d\n", ip, port);
  }

  return sock;
}

int requestServerToEnterChat(int sock){
  Payload *request = malloc(sizeof(Payload));
  request->idMsg = REQ_ADD;

  if (send(sock, request, sizeof(request), 0) == -1) {
    printf("Failed to send request to the server\n");
    free(request);
    return 0;
  }

  free(request);
  return 1;
}

void closeSocket(int sock){
  if(close(sock) == -1){
    printf("Erro ao fechar o socket\n");
  }
}

void *handleServer(void *arg) {
  int sock = *(int *)arg;
  Payload serverMessage;
  char bufferServerMessage[sizeof(Payload)];

  while (connectedToServer) {  
    //Receive from server and resolve struct
    int recvReturn = recv(sock, bufferServerMessage, sizeof(Payload), 0);
    memcpy(&serverMessage, bufferServerMessage, sizeof(Payload));

    if (recvReturn <= 0) {
      break;
    }

    if(serverMessage.idMsg == MSG){     
      //Added user message or broadcast msg
      if(serverMessage.idReceiver == (intptr_t)NULL){   
        bool userFound = false;  
        for(int i=0; i < MAX_CLIENTS; i++){
          if(clientsIdDb[i] == serverMessage.idSender){
            userFound = true;
            break;
          }
        }

        //Broadcast message
        if(userFound){
          printf("%s\n", serverMessage.message);
        //Add user message
        }else{
          //Search for an empty space in DB and add the new userID
          for(int i=0; i < MAX_CLIENTS; i++){
            if(clientsIdDb[i] == -1){
              clientsIdDb[i] = serverMessage.idSender;
              break;
            }
          }
          printf("%s\n", serverMessage.message);
        }

      //Private message
      }else{
        //Retrieve time from system
        time_t systemNow = time(NULL);
        struct tm* localTime = localtime(&systemNow);

        // Format string with retrieved time
        char timeStr[6];
        strftime(timeStr, sizeof(timeStr), "%H:%M", localTime);

        printf("P [%s] %02d: %s\n", timeStr, serverMessage.idSender, serverMessage.message);
      }
      
    }else if(serverMessage.idMsg == OK){
      //Confirmação de solicitação de desconexão da rede
      if(serverMessage.idSender == (intptr_t)NULL){
        printf("%s\n", serverMessage.message);
        connectedToServer = 0;
        break;

      //Confirmação de mensagem privada
      }else if(serverMessage.idSender != (intptr_t)NULL && serverMessage.idReceiver != (intptr_t)NULL){
        //Retrieve time from system
        time_t systemNow = time(NULL);
        struct tm* localTime = localtime(&systemNow);

        // Format string with retrieved time
        char timeStr[6];
        strftime(timeStr, sizeof(timeStr), "%H:%M", localTime);

        printf("P [%s] -> %02d: %s\n", timeStr, serverMessage.idReceiver, serverMessage.message);
      }
      
    }else if(serverMessage.idMsg == ERROR){
      printf("%s\n", serverMessage.message);

    //Server warning a client left the group
    }else if(serverMessage.idMsg == REQ_REM){
      printf("User %02d left the group!\n", serverMessage.idSender);
      //Search and remove client from database
      for(int i=0; i < MAX_CLIENTS; i++){
        if(clientsIdDb[i] == serverMessage.idSender){
          clientsIdDb[i] = -1;
        }
      }
    }
  }

  close(sock);
  pthread_exit(NULL);
}

void clearPayload(Payload *payload){
  payload->idReceiver = (intptr_t)NULL;
  payload->idSender = (intptr_t)NULL;
  payload->idMsg = (intptr_t)NULL;
}

int main(int argc, char *argv[]){
  char keyBoardBuffer[BUFFER_SIZE];
  Payload *request = malloc(sizeof(Payload));
  char bufferRequest[sizeof(Payload)];
  Payload response;
  char bufferResponse[sizeof(Payload)];
  
  char *ip = argv[1];
  int port = atoi(argv[2]);

  //printf("Client running!\n");
  //printf("Endereço IP: %s\n", argv[1]);
  //printf("Porta de conexão: %s\n", argv[2]);

  int sock;

  //Create and connect sock to server
  if((sock = createAndConnectSockToServerIPV4orIPV6(ip, port)) < 0){
    return 0;
  }

  if(requestServerToEnterChat(sock) == 0){
    return 0;
  }

  //Receive response from server and resolve struct
  recv(sock, bufferResponse, sizeof(Payload), 0);
  memcpy(&response, bufferResponse, sizeof(Payload));
  printf("%s\n", response.message);

  //Get my id from server confirmation and store
  myID = response.idSender;

  if(response.idMsg == ERROR){
    close(sock);
    return 0;
  }

  //Receive all users list
  recv(sock, bufferResponse, sizeof(Payload), 0);
  memcpy(&response, bufferResponse, sizeof(Payload));

  //Get the list of users and store in the database
  char *token = strtok(response.message, ", ");
  int count = 0;
  while (token != NULL) {
    clientsIdDb[count] = atoi(token);
    count++;

    token = strtok(NULL, ", ");
  }

  //Create thread to receive data from client while receiving data from keyboard below
  pthread_t serverThread;
  if (pthread_create(&serverThread, NULL, handleServer, (void *)&sock) != 0) {
    perror("Erro ao criar a thread");
    exit(EXIT_FAILURE);
  }

  while(connectedToServer){
    fgets(keyBoardBuffer, sizeof(keyBoardBuffer), stdin);

    //Clear string \n
    keyBoardBuffer[strcspn(keyBoardBuffer, "\n")] = '\0';

    if(strcmp(keyBoardBuffer, "close connection") == 0){
      //Build payload
      request->idMsg = REQ_REM;
      request->idSender = myID;

      //Prepare struct to send through socket
      memcpy(bufferRequest, request, sizeof(Payload));
      send(sock, bufferRequest, sizeof(Payload), 0);
      clearPayload(request);

      //Wait for serverThread to set connection is over
      pthread_join(serverThread, NULL);
    }else if(strcmp(keyBoardBuffer, "list users") == 0){
      //Print the list of users retrieve from usersDB
      for(int i=0; i < MAX_CLIENTS; i++){
        if(clientsIdDb[i] != -1 && clientsIdDb[i] != myID){
          printf("%02d ", clientsIdDb[i]);
        }
      }
      printf("\n");
    }else if(strstr(keyBoardBuffer, "send to") != NULL){
      int idReceiver;
      char message[2048];
      sscanf(keyBoardBuffer, "send to %d \"%[^\"]\"", &idReceiver, message);

      //Build payload
      request->idMsg = MSG;
      request->idSender = myID;
      request->idReceiver = idReceiver;
      strcpy(request->message, message);

      //Prepare struct to send through socket
      memcpy(bufferRequest, request, sizeof(Payload));
      send(sock, bufferRequest, sizeof(Payload), 0);
      clearPayload(request);
    }else if(strstr(keyBoardBuffer, "send all") != NULL){
      char message[2048];
      sscanf(keyBoardBuffer, "send all \"%[^\"]\"", message);

      //Build payload
      request->idMsg = MSG;
      request->idSender = myID;
      strcpy(request->message, message);
      //Prepare struct to send through socket
      memcpy(bufferRequest, request, sizeof(Payload));
      send(sock, bufferRequest, sizeof(Payload), 0);
      clearPayload(request);
    }  
  }

  return 0;
}