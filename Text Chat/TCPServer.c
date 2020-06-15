#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <error.h>
#include <pthread.h>
#define buff_size 8192
void* receive(void* arg);
void* sending(void* arg);
pthread_t tid[2];
pthread_mutex_t lock;
FILE* fp;
void main(int argc, char ** argv)
{
  if (argc != 2)
  {
    printf("Not enough arguments\n");
    return;
  }
  fp = fopen("ServerChatlog","w");
  int ServerSock,ClientSocket;
  struct sockaddr_in server,client;
  ServerSock = socket(AF_INET,SOCK_STREAM,0);
  //SOCK_STREAM = TCP
  if (ServerSock == -1){
    perror("socket: ");
    exit(-1);
  }
  puts("Socket created");
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(strtol(argv[1],NULL,10));
  bzero(&server.sin_zero,8);
  int b = bind(ServerSock,(struct sockaddr*)&server,sizeof(server));
  while(b == -1){
    server.sin_port = htons(ntohs(server.sin_port)+1);
    b = bind(ServerSock,(struct sockaddr*)&server,sizeof(server));
    }
  printf("Binded to port %d\n",htons(server.sin_port));

  if (listen(ServerSock,2) == -1){ //5 pending connections maximum
    perror("listen: ");
    exit(-1);
  }
  char recv_buffer[buff_size];
  int sockaddr_len = sizeof(struct sockaddr_in);
  ClientSocket = -1;
  ClientSocket = accept(ServerSock,(struct sockaddr*)&client,&sockaddr_len);
    if(ClientSocket == -1) //client is empty but it will get info from accept function
    {
      printf("ClientSocket: %d\n",ClientSocket);
      perror("accept: ");
      exit(-1);
    }
    else
    {
      printf("New client connected from port no %d and IP %s\n",ntohs(client.sin_port),inet_ntoa(client.sin_addr));
    }
    if (pthread_mutex_init(&lock, NULL) != 0) {
     printf("\n mutex init has failed\n");
     return;
   }
    int rtv = pthread_create(&tid[0], NULL ,receive, ( void *)&ClientSocket);
    if ( rtv != 0)
    {
    printf (" ERROR;  pthread_create1()   returns   %d\n" , rtv);
    return;
    }
    int rtv1 = pthread_create(&tid[1], NULL ,sending, ( void *)&ClientSocket);
    if ( rtv1 != 0)
    {
    printf (" ERROR;  pthread_create2()   returns   %d\n" , rtv1);
    return;
    }
    rtv = pthread_join( tid[0] , NULL );
    if ( rtv != 0)
    {
    printf (" ERROR ; pthread_join1()   returns   %d\n" , rtv);
    return;
    }
    rtv1 = pthread_join( tid[1] , NULL );
    if ( rtv1 != 0)
    {
    printf (" ERROR ; pthread_join2()   returns   %d\n" , rtv1);
    return;
    }
    pthread_mutex_destroy(&lock);


  printf("Client Disconnected\n");
  close(ClientSocket);
  close(ServerSock);
  fclose(fp);
  return;
}

void* receive(void* arg){
  int data_len = 0;
  int ClientSocket = *((int *)arg);
  char recv_buffer[buff_size];
  while (1){
      data_len = recv(ClientSocket,recv_buffer,buff_size,0);
      if (data_len)
      {
        pthread_mutex_lock(&lock);
        recv_buffer[data_len] = '\0';
        printf("Received: %s\n",recv_buffer);
        fprintf(fp,"Received: %s\n",recv_buffer);
        pthread_mutex_unlock(&lock);
      }
      else
      {
        pthread_cancel(tid[1]);
        //printf("receive thread ended\n");
        break;
      }
  }
  //pthread_mutex_unlock(&lock);
  return NULL;
}
void* sending(void* arg)
{
  //printf("Sending Thread\n");
  int ClientSocket = *((int *)arg);;
  //int ClientSocket = *((int *)arg);
  int replySize = 0;
  int data_len = 0;
  char reply[buff_size];
        do
        {
          if (fgets(reply, buff_size, stdin)!=NULL)
          {
            pthread_mutex_lock(&lock);
            replySize = strcspn(reply, "\n")+1;
            //printf("replySize: %d",replySize);
            //name[strcspn(name, "\n")] = 0;
            data_len = send(ClientSocket,reply,replySize,0);
            fprintf(fp,"Sent: %s\n",reply);
            //printf("data len: %d\n",data_len);
            pthread_mutex_unlock(&lock);
          }

        } while(data_len);

  return NULL;
}
