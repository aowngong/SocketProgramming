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
#include <sys/sendfile.h>
#include <errno.h>
#define buff_size 8192
void* receive(void* arg);
void* sending(void* arg);
pthread_t tid[2];
pthread_mutex_t lock;
FILE* fpReceive;
FILE* fpSend;
int ServerSocket;
void main(int argc, char ** argv)
{
  if (argc != 3)
  {
    printf("Not enough arguments\n");
    return;
  }
  fpReceive =  fopen("ReceivedFile","w");
  fpSend = fopen("5M.b","r");
  if (fpSend == NULL)
  {
      printf("Invalid file opened\n");
      exit(1);
  }
  struct sockaddr_in server,client;
  ServerSocket = socket(AF_INET,SOCK_STREAM,0);
  //unsigned int length = sizeof(struct sockaddr);
  //SOCK_STREAM = TCP
  if (ServerSocket == -1){
    perror("socket: ");
    exit(-1);
  }
  printf("Socket created\n");

  server.sin_family = AF_INET;
  if(inet_pton(AF_INET, argv[1], &(server.sin_addr))<=0)
  {
      printf("Invalid IP Address Given\n");
      return;
  }
  server.sin_port = htons(strtol(argv[2],NULL,10));
  bzero(&server.sin_zero,8);
  if (connect(ServerSocket, (struct sockaddr *)&server, sizeof(server)) < 0)
      {
          printf("Connection Failed \n");
          return;
      }
  else
  {
      printf("Connected to server port no %d and IP %s\n",ntohs(server.sin_port),inet_ntoa(server.sin_addr));
  }
    if (pthread_mutex_init(&lock, NULL) != 0) {
     printf("mutex init has failed\n");
     return;
   }

    int rtv = pthread_create(&tid[0], NULL ,receive, ( void *)&ServerSocket);
    if ( rtv != 0)
    {
    printf (" ERROR;  pthread_create1()   returns   %d\n" , rtv);
    return;
    }
    int rtv1 = pthread_create(&tid[1], NULL ,sending, ( void *)&ServerSocket);
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
  fclose(fpSend);
  fclose(fpReceive);
  close(ServerSocket);
  return;
}

void* receive(void* arg){
  time_t startTime = time(NULL);
  int data_len = 0;
  //int ServerSocket = *((int *)arg);
  char recv_buffer[buff_size];
  long bytes = 0;
  do{
      data_len = recv(ServerSocket,recv_buffer,buff_size,0);
      bytes = bytes + data_len;
      /*
      if (errno == EAGAIN || errno == EWOULDBLOCK){
        return NULL;
      }
      */
      if (data_len)
      {
        fwrite(recv_buffer,sizeof(char),data_len,fpReceive);
      }
  } while(data_len);
  time_t endTime = time(NULL);
  double Mb = 8*bytes / (1000000);
  double secs = difftime(endTime,startTime);
  printf("Average Download Speed: %.2lf Mbit/s\n",Mb/secs);
  //pthread_mutex_unlock(&lock);
  return NULL;
}
void* sending(void* arg)
{
  //int ServerSocket = *((int *)arg);;
  int replySize = 0;
  int data_len = 0;
  char reply[buff_size];
  int fd = fileno(fpSend); //file descriptor for file about to be sent
  long bytes = 0;
  time_t startTime = time(NULL);
  int sent = 0;
  int cur = 0;
  do{
    sent = sendfile(ServerSocket,fd,NULL,buff_size);
    //printf("Sent: %d bytes\n",sent);
    bytes = bytes + sent;
  } while(sent>0);
  time_t endTime = time(NULL);
  double secs = difftime(endTime,startTime);
  double Mb = 8*bytes / (1000000);
  //printf("%ld bytes sent\n",bytes);
  //printf("Time Spent: %lf secs \n",secs);
  double speed = Mb/secs;
  shutdown(ServerSocket,1);
  printf("Average Upload Speed: %.2lf Mbit/s\n",speed);
  return NULL;
}
