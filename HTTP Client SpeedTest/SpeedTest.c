#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <error.h>
#define FULLTEST 2
#define UPLOAD 1
#define DOWNLOAD 0
#define buff_size 8192
static int receive(int socket, double* speed);
static int sending(int socket, double* speed);
int speedTest(char* IPv4, char* port, double* DLSpeed, double *UPSpeed, int mode);
int main(int argc, char ** argv)
{
  if (argc != 3)
  {
    fprintf(stderr,"Not enough arguments\n Enter IP address and Port Number to connect to. \n Example: 10.0.0.1 9009\n");
    exit(1);
  }
  double DLSpeed = 0;
  double UPSpeed = 0;
  int mode = speedTest(argv[1],argv[2],&DLSpeed,&UPSpeed,FULLTEST);
  if (mode == FULLTEST)
  {
    fprintf(stderr,"Average Download Speed: %.3lf Mbit/s\n",DLSpeed);
    fprintf(stderr,"Average Upload Speed: %.3lf Mbit/s\n",UPSpeed);
  }
  else if (mode == DOWNLOAD)
  {
    fprintf(stderr,"Average Download Speed: %.3lf Mbit/s\n",DLSpeed);
  }
    else if (mode == UPLOAD)
  {
    fprintf(stderr,"Average Upload Speed: %.3lf Mbit/s\n",UPSpeed);
  }
  return 0;
}

int speedTest(char* IPv4, char* port, double* DLSpeed, double *UPSpeed, int mode)
{
  struct sockaddr_in server;
  const int ServerSocket = socket(AF_INET,SOCK_STREAM,0);
  if (ServerSocket == -1){
    perror("socket: ");
    return -1;
  }
  server.sin_family = AF_INET;
  if(inet_pton(AF_INET, IPv4, &(server.sin_addr))<=0)
  {
      printf("Invalid IP Address Given\n");
      return -1;
  }
  server.sin_port = htons(strtol(port,NULL,10));
  bzero(&server.sin_zero,8);
  if (connect(ServerSocket, (struct sockaddr *)&server, sizeof(server)) < 0)
      {
          fprintf(stderr,"Connection Failed \n");
          return -1;
      }
  else
  {
      fprintf(stderr,"Connected to server port no %d and IP %s\n",ntohs(server.sin_port),inet_ntoa(server.sin_addr));
  }
  if (mode == FULLTEST)
  {
    if (receive(ServerSocket,DLSpeed) == 0 && sending(ServerSocket,UPSpeed) == 0)
    {
      close(ServerSocket);
      return FULLTEST;
    }
    else
    {
      fprintf(stderr,"Error\n");
      return -1;
    }
    
  }
  else if (mode == DOWNLOAD)
  {
    if (receive(ServerSocket,DLSpeed) == 0)
    {
      close(ServerSocket);
      return DOWNLOAD;
    }
    else
    {
      fprintf(stderr,"Error\n");
      return -1;
    }
  }
  else if (mode == UPLOAD)
  {
    if(sending(ServerSocket,UPSpeed) == 0)
    {
      close(ServerSocket);
      return UPLOAD;
    }
    else
    {
      fprintf(stderr,"Error\n");
      return -1;
    }
  }
  return -1;
}

static int receive(int ServerSocket, double *speed){
  int data_len = 0;
  //int ServerSocket = *((int *)arg);
  char *recv_buffer = malloc(sizeof(char)*buff_size);
  long bytes = 0;
  char message[] = "GET /1Ms.b HTTP/1.1\r\nHost: 192.168.50.254\r\n\r\n";
  char response[] = "HTTP/1.1 200";
  char sizeMessage[] = "Content-Length: ";
  struct timeval stop, start;
  data_len = write(ServerSocket,message,strlen(message));
  if (data_len != strlen(message))
  {
    free(recv_buffer);
    return -1;
  }
  data_len = recv(ServerSocket,recv_buffer,buff_size,0);
  char *EndofHeaderPtr = strstr(recv_buffer, "\r\n\r\n");
  EndofHeaderPtr = &EndofHeaderPtr[4];
  if (EndofHeaderPtr != NULL)
  {
    int Blength = EndofHeaderPtr - recv_buffer;
    bytes = bytes + (data_len - Blength);
    //printf("Found end of header: %d\n",Blength);
  }
  else
  {
    free(recv_buffer);
    return -1;
  }
  char *ptr = strstr(recv_buffer, sizeMessage);
  if (ptr != NULL)
  {

    ptr = &ptr[strlen(sizeMessage)];
    int ctr = 0;
    while (ptr[ctr] != '\n')
	  {
		  ctr++;
	  }
    char* filesizeStr = malloc(sizeof(char)*(ctr));
    if (filesizeStr == NULL)
    {
      return -1;
    }
    memcpy(filesizeStr,ptr,sizeof(char)*ctr);
    long filesize =  strtol(filesizeStr, NULL, 10);
    free(filesizeStr);
    if (data_len && (strncmp(recv_buffer, response, strlen(response))==0))
    {
      fprintf(stderr,"HTTP OK received\n");
      gettimeofday(&start, NULL);
    do{
        data_len = recv(ServerSocket,recv_buffer,buff_size,0);
        bytes = bytes + data_len;
        if (bytes >= filesize)
        {
          free(recv_buffer);
          break;
        }

    } while(data_len);
    gettimeofday(&stop, NULL);
    ///printf("took %lu us\n", (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec);
    double usecs = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;
    *speed = 8*bytes/usecs; //becomes Mb/s
    return 0;
  }
  
  }
  else
  {
    free(recv_buffer);
    return -1;
  }
  return -1;
}
static int sending(int ServerSocket, double *speed)
{
  //int ServerSocket = *((int *)arg);;
  struct timeval stop, start;
  long bytes = 0;
  int sent = 0;
  char *send_buffer = calloc(buff_size,sizeof(char));
  if (send_buffer == NULL)
  {
    return -1;
  }
  gettimeofday(&start, NULL);
  do{
    sent = write(ServerSocket,send_buffer,buff_size);
    //printf("Sent: %d bytes\n",sent);
    bytes = bytes + sent;
  } while(bytes<buff_size*1024);
  gettimeofday(&stop, NULL);
  shutdown(ServerSocket,1);
  free(send_buffer);
  double usecs = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;
  *speed = 8*bytes/usecs; //becomes Mb/s
  return 0;
}
