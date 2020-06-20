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
  if (argc != 3)
  {
    printf("Not enough arguments\n");
    return;
  }
  fp = fopen("ClientChatlog","w");
  int ServerSock;
  struct sockaddr_in server,client;
  ServerSock = socket(AF_INET,SOCK_STREAM,0);
  //unsigned int length = sizeof(struct sockaddr);
  //SOCK_STREAM = TCP
  if (ServerSock == -1){
    perror("socket: ");
    exit(-1);
  }
  puts("Socket created\n");

  server.sin_family = AF_INET;
  if(inet_pton(AF_INET, argv[1], &(server.sin_addr))<=0)
  {
      printf("Invalid IP Address Given\n");
      return;
  }
  server.sin_port = htons(strtol(argv[2],NULL,10));
  bzero(&server.sin_zero,8);
  if (connect(ServerSock, (struct sockaddr *)&server, sizeof(server)) < 0)
      {
          printf("\nConnection Failed \n");
          return;
      }
  else
  {
      printf("Connected to server port no %d and IP %s\n",ntohs(server.sin_port),inet_ntoa(server.sin_addr));
  }
    if (pthread_mutex_init(&lock, NULL) != 0) {
     printf("\n mutex init has failed\n");
     return;
   }
    send(ServerSock,"Connect",8,0);
    int rtv = pthread_create(&tid[0], NULL ,receive, ( void *)&ServerSock);
    if ( rtv != 0)
    {
    printf (" ERROR;  pthread_create1()   returns   %d\n" , rtv);
    return;
    }
    int rtv1 = pthread_create(&tid[1], NULL ,sending, ( void *)&ServerSock);
    if ( rtv != 0)
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
  fclose(fp);
  close(ServerSock);
  return;
}

void* receive(void* arg){
  int data_len = 0;
  int ServerSocket = *((int *)arg);
  char recv_buffer[buff_size];
  while (1){
      data_len = recv(ServerSocket,recv_buffer,buff_size,0);
      if (data_len)
      {
        pthread_mutex_lock(&lock);
        recv_buffer[data_len] = '\0';
        //printf("Received: %s\n",recv_buffer);
        printf("%s\n",recv_buffer);
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
  int ServerSocket = *((int *)arg);;
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
            data_len = send(ServerSocket,reply,replySize,0);
            fprintf(fp,"Sent: %s\n",reply);
            pthread_mutex_unlock(&lock);
          }

        } while(data_len);

  return NULL;
}
