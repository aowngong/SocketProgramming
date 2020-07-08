#include "commands.h"
#include "SpeedTest.h"
extern pthread_mutex_t lock;
/*
int main()
{
    int mode = 0;
    receiveCommand(&mode);
}
*/
void *receiveCommand(void *arg)
{
    int *mode = ((int *)arg);
    int iSetOption = 1;
    char C1[] = "1";
    char C2[] = "2";
    char C3[] = "3";
    char C4[] = "4";                     //speedTest
    char correctIP[] = "192.168.50.254"; //IP allowed to be used
    char recv_buffer[command_size];
    struct sockaddr_in server, client;
    int ServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(ServerSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&iSetOption, sizeof(iSetOption));
    if (ServerSocket == -1)
    {
        perror("socket: ");
        return NULL;
    }
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(strtol(PORT, NULL, 10));
    bzero(&server.sin_zero, 8);
    int b = bind(ServerSocket, (struct sockaddr *)&server, sizeof(server));
    while (b == -1)
    {
        server.sin_port = htons(ntohs(server.sin_port) + 1);
        b = bind(ServerSocket, (struct sockaddr *)&server, sizeof(server));
    }
    printf("Binded to port %d\n", htons(server.sin_port));
    if (listen(ServerSocket, 1) == -1)
    { //1 pending connections maximum
        perror("listen: ");
        return NULL;
    }
    unsigned int sockaddr_len = sizeof(struct sockaddr_in);
    while (1)
    {
        int ClientSocket = accept(ServerSocket, (struct sockaddr *)&client, &sockaddr_len);
        if (ClientSocket == -1) //client is empty but it will get info from accept function
        {
            printf("ClientSocket: %d\n", ClientSocket);
            perror("accept: ");
            return NULL;
        }
        else
        {

            printf("New client connected from port no %d and IP %s\n", ntohs(client.sin_port), inet_ntoa(client.sin_addr));
            if (strncmp(inet_ntoa(client.sin_addr), correctIP, strlen(correctIP))) //strncmp == 0 means the comparison was correct
            {
                printf("Wrong IP\n");
                close(ClientSocket);
                return NULL;
            }
        }
        int write = recv(ClientSocket, recv_buffer, command_size, 0);
        if (write == -1)
        {
            perror("recv ");
            return NULL;
        }
        pthread_mutex_lock(&lock);
        if (!strncmp(recv_buffer, C1, strlen(C1)))
        {
            *mode = 1;
        }
        else if (!strncmp(recv_buffer, C2, strlen(C2)))
        {
            *mode = 2;
        }
        else if (!strncmp(recv_buffer, C3, strlen(C3)))
        {
            *mode = 3;
        }
        else if (!strncmp(recv_buffer, C4, strlen(C4)))
        {
            *mode = 4;
        }
        close(ClientSocket);
        pthread_mutex_unlock(&lock);
    }
    close(ServerSocket);
}
