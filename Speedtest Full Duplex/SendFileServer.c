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
#define buff_size 8096
void* receive(void* arg);
void* sending(void* arg);
pthread_t tid[2];
pthread_mutex_t lock;
FILE* fpSend;
FILE* fpReceive;
int ServerSocket;
int ClientSocket;
int main(int argc, char ** argv)
{
  if (argc != 2)
  {
    printf("Not enough arguments\n");
    exit(1);
  }
  fpSend = fopen("5M.b","r");
  if (fpSend == NULL)
  {
    printf("Invalid file opened\n");
    exit(1);
  }
  fpReceive = fopen("ReceivedFile","w");
  if (fpReceive == NULL)
  {
    printf("Could not create file to receive data with\n");
    exit(1);
  }
  struct sockaddr_in server,client;
  ServerSocket = socket(AF_INET,SOCK_STREAM,0);
  //SOCK_STREAM = TCP
  if (ServerSocket == -1){
    perror("socket: ");
    exit(-1);
  }
  puts("Socket created");
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(strtol(argv[1],NULL,10));
  bzero(&server.sin_zero,8);
  int b = bind(ServerSocket,(struct sockaddr*)&server,sizeof(server));
  while(b == -1){
    server.sin_port = htons(ntohs(server.sin_port)+1);
    b = bind(ServerSocket,(struct sockaddr*)&server,sizeof(server));
    }
  printf("Binded to port %d\n",htons(server.sin_port));

  if (listen(ServerSocket,2) == -1){ //2 pending connections maximum
    perror("listen: ");
    exit(-1);
  }
  unsigned int sockaddr_len = sizeof(struct sockaddr_in);
  ClientSocket = -1;
  ClientSocket = accept(ServerSocket,(struct sockaddr*)&client,&sockaddr_len);
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
     exit(1);
   }

    int rtv = pthread_create(&tid[0], NULL ,receive, ( void *)&ClientSocket);
    if ( rtv != 0)
    {
    printf (" ERROR;  pthread_create1()   returns   %d\n" , rtv);
    exit(1);
    }
    int rtv1 = pthread_create(&tid[1], NULL ,sending, ( void *)&ClientSocket);

    if ( rtv1 != 0)
    {
    printf (" ERROR;  pthread_create2()   returns   %d\n" , rtv1);
    exit(1);
    }

    rtv = pthread_join( tid[0] , NULL );
    if ( rtv != 0)
    {
    printf (" ERROR ; pthread_join1()   returns   %d\n" , rtv);
    exit(1);
    }
    rtv1 = pthread_join( tid[1] , NULL );
    if ( rtv1 != 0)
    {
    printf (" ERROR ; pthread_join2()   returns   %d\n" , rtv1);
    exit(1);
    }
    pthread_mutex_destroy(&lock);
  printf("Speed Test Finished\n");
  fclose(fpReceive);
  fclose(fpSend);
  close(ServerSocket);
  close(ClientSocket);
  return 0;
}

void* receive(void* arg){
  int data_len = 0;
  char recv_buffer[buff_size];
  time_t startTime = time(NULL);
  long bytes = 0;
  do{
      data_len = recv(ClientSocket,recv_buffer,buff_size,0);
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
  //pthread_mutex_unlock(&lock);
  time_t endTime = time(NULL);
  double Mb = 8*bytes / (1000000);
  double secs = difftime(endTime,startTime);
  printf("Average Download Speed: %.2lf Mbit/s\n",Mb/secs);
  return NULL;
}
void* sending(void* arg)
{
  //int ClientSocket = *((int *)arg);;
  int fd = fileno(fpSend); //file descriptor for file about to be sent

  long bytes = 0;
  time_t startTime = time(NULL);
  int sent = 0;
  do{
    sent = sendfile(ClientSocket,fd,NULL,buff_size);
    //printf("Sent: %d bytes\n",sent);
    bytes = bytes + sent;
  } while(sent>0);
  time_t endTime = time(NULL);
  double secs = difftime(endTime,startTime);
  double Mb = 8*bytes / (1000000);
  //printf("%ld bytes sent\n",bytes);
  //printf("Time Spent: %lf secs \n",secs);
  double speed = Mb/secs;
  printf("Average Download Speed: %.2lf Mbit/s\n",speed);
  shutdown(ClientSocket,1);
  return NULL;
}
