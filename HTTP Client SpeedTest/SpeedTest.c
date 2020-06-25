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
#include <signal.h>
#define FULLTEST 2
#define UPLOAD 1
#define DOWNLOAD 0
#define buff_size 8192
#include <signal.h>
static int receive(int socket, double* speed);
static int sending(int ServerSocket, double *speed,int port,char* ip);
void intHandler(int dummy);
int speedTest(char* IPv4, char* port, double* DLSpeed, double *UPSpeed, int mode);
int keepRunning = 1;
void intHandler(int dummy) {
    keepRunning = 0;
}
int main(int argc, char ** argv)
{
  if (argc != 3)
  {
    fprintf(stderr,"Not enough arguments\n Enter IP address and Port Number to connect to. \n Example: 10.0.0.1 9009\n");
    exit(1);
  }
  double DLSpeed = 0;
  double UPSpeed = 0;
  int mode = 0;
  FILE *fp = fopen("log.csv","w");
  fprintf(fp,"Download (Mbps),Upload (Mbps),\n");
  fclose(fp);
  signal(SIGINT, intHandler);
  while(keepRunning)
  {
    fp = fopen("log.csv","a");
    mode = speedTest(argv[1],argv[2],&DLSpeed,&UPSpeed,FULLTEST);
    if (mode == FULLTEST)
    {
      fprintf(stderr,"Average Download Speed: %.3lf Mbit/s\n",DLSpeed);
      fprintf(stderr,"Average Upload Speed: %.3lf Mbit/s\n",UPSpeed);
      fprintf(fp,"%.3lf,%.3lf,\n",DLSpeed,UPSpeed);
    }
    else if (mode == DOWNLOAD)
    {
      fprintf(stderr,"Average Download Speed: %.3lf Mbit/s\n",DLSpeed);
      fprintf(fp,"%.3lf,\n",DLSpeed);
    }
    else if (mode == UPLOAD)
    {
      fprintf(stderr,"Average Upload Speed: %.3lf Mbit/s\n",UPSpeed);
      fprintf(fp,"%.3lf,\n",UPSpeed);
    }
    else if (mode == -1)
    {
      fclose(fp);
      return -1;
    }
    fclose(fp);
    sleep(300);
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
    int downloadCode = receive(ServerSocket,DLSpeed);
    int uploadCode = sending(ServerSocket,UPSpeed,ntohs(server.sin_port),inet_ntoa(server.sin_addr));
    if (uploadCode == 0 && downloadCode == 0)
    {
      close(ServerSocket);
      return FULLTEST;
    }
    else if (uploadCode == -1 && downloadCode == -1)
    {
      fprintf(stderr,"Error with both download and upload test\n");
      return -1;
    }
    else if (uploadCode == -1 )
    {
      fprintf(stderr,"Error with upload test\n");
      return -1;
    }
    else
    {
      fprintf(stderr,"Error with download test\n");
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
      fprintf(stderr,"Error with Download Test\n");
      return -1;
    }
  }
  else if (mode == UPLOAD)
  {
    if(sending(ServerSocket,UPSpeed,ntohs(server.sin_port),inet_ntoa(server.sin_addr)) == 0)
    {
      close(ServerSocket);
      return UPLOAD;
    }
    else
    {
      fprintf(stderr,"Error with Upload Test\n");
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
  char message[] = "GET /2M.b HTTP/1.1\r\nHost: 192.168.50.254\r\n\r\n";
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
  if (EndofHeaderPtr == NULL)
  {
    free(recv_buffer);
    return -1;
  }
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
    if (data_len && (strncmp(recv_buffer, response, strlen(response))==0)) {
      //fprintf(stderr,"HTTP OK received\n");
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
    else if (strncmp(recv_buffer, "HTTP/1.1 302", strlen(response))==0)
      { 
        printf("Download file not found\n");
        free(recv_buffer);
        return -1;
      }
  }
  else
  {
    free(recv_buffer);
    return -1;
  }
  return -1;
}
static int sending(int ServerSocket, double *speed,int port,char* ip)
{
  int packets2send = 1024*4;
  int size = buff_size*packets2send;
  int sent = 0;
  long bytes = 0;
  char cont[] = "HTTP/1.1 100 Continue";
  //printf("Port: %d\n",port);
  //printf("IP: %s\n",ip);
  struct timeval stop, start;
  dprintf(ServerSocket,"POST /process.php HTTP/1.1\r\nHost: %s:%d\r\n"
  "User-Agent: VIPTest\r\nAccept: */*\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n"
  "Content-Type: multipart/form-data;\r\nExpect: 100-continue\r\n\r\n",ip,port,size);\
  char *recv_buffer = malloc(sizeof(char)*buff_size);
  if (recv_buffer == NULL)
  {
    return -1;
  }
  recv(ServerSocket,recv_buffer,buff_size,0);
  if (strncmp(recv_buffer,cont,strlen(cont))!=0)
  {
      free(recv_buffer);
      fprintf(stderr,"%.12s received\n",recv_buffer);
      return -1;
  }
  else
  {
    //fprintf(stderr,"HTTP/1.1 100 Continue Received\n");
  }
  //printf("%s",recv_buffer);
  char *send_buffer = calloc(buff_size,sizeof(char));
  if (send_buffer == NULL)
  {
    free(recv_buffer);
    return -1;
  }
  gettimeofday(&start, NULL);
  for (int i = 0; i<packets2send; i++)
  {
    sent = send(ServerSocket,send_buffer,buff_size,0);
    bytes = bytes + sent;
    if (sent!=buff_size)
    {
      send(ServerSocket,send_buffer,buff_size-sent,0);
    }
  }
  recv(ServerSocket,recv_buffer,buff_size,0);
  gettimeofday(&stop, NULL);
  char response[] = "HTTP/1.1 200";
  //printf("%s",recv_buffer);
  if ((strncmp(recv_buffer, response, strlen(response))==0)) {
      //printf("HTTP OK received\n");
      double usecs = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;
      *speed = 8*bytes/usecs; //becomes Mb/s
  }
  else
    {
      printf("%s",recv_buffer);
      *speed = 0;
      free(recv_buffer);
      free(send_buffer);
      return -1;
    }
  
  free(recv_buffer);
  free(send_buffer);
  shutdown(ServerSocket,1);
  return 0;
}
