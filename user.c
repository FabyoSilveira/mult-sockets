#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024
#define MESSAGE_SIZE 2048
//Message types
#define REQ_ADD 1
#define REQ_REM 2
#define RES_LIST 4
#define MSG 6
#define ERROR 7
#define OK 8

typedef struct {
  int idMsg;
  int idSender;
  int idReceiver;
  char message[2048];
} Payload;

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

    printf("Conectado com sucesso ao server %s porta %d\n", ip, port);

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

    printf("Conectado com sucesso ao server %s porta %d\n", ip, port);
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

int main(int argc, char *argv[]){
  char keyBoardBuffer[BUFFER_SIZE];
  Payload *request = malloc(sizeof(Payload));
  char bufferRequest[sizeof(Payload)];
  Payload response;
  char bufferResponse[sizeof(Payload)];
  
  char *ip = argv[1];
  int port = atoi(argv[2]);

  printf("Client running!\n");
  printf("Endereço IP: %s\n", argv[1]);
  printf("Porta de conexão: %s\n", argv[2]);

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

  if(response.idMsg == ERROR){
    close(sock);
    return 0;
  }

  //Receive all users list
  /* recv(sock, &serverResponseBuffer, sizeof(serverResponseBuffer), 0);
  printf("%s\n", serverResponseBuffer.message); */

  while(1){
    fgets(keyBoardBuffer, sizeof(keyBoardBuffer), stdin);

    //Clear string \n
    keyBoardBuffer[strcspn(keyBoardBuffer, "\n")] = '\0'; 

    strcpy(request->message, keyBoardBuffer);

    //Prepare struct to send through socket
    memcpy(bufferRequest, request, sizeof(Payload));
    send(sock, bufferRequest, sizeof(Payload), 0);

    if(strcmp(keyBoardBuffer, "close connection") == 0){
      printf("close connection\n");
    }else if(strcmp(keyBoardBuffer, "list users") == 0){
      printf("list users\n");
    }else if(strstr(keyBoardBuffer, "send to") != NULL){
      printf("send private message\n");
    }else if(strstr(keyBoardBuffer, "send all") != NULL){
      printf("send broadcast message\n");
    }  
  }

  return 0;
}