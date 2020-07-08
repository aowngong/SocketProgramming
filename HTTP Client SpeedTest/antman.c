#include "SpeedTest.h"
#include "commands.h"
#define SECS 10800 //seconds in 3 hours
#define MAXLength 10000
//argv[1] = IPv4 Address
//argv[2] = Port number to be used
static int sendSpeed(char *IPv4, char *Port, int modeChosen, int *ServerSocket);
int getRandomInt(int low, int high, unsigned int *randval);
static void catString(char *start, int *currentPos, char *str2Copy);
static int createJSON(double *DLSpeedArray, double *UPSpeedArray, char *JSON, int *JSONlength, int mode);
static void getMaxMinAvg(double *DLSpeedArray, double *max, double *min, double *average);
static int sendJSON(int ServerSocket, int port, char *ip, int size, char *JSON);
pthread_t tid[1];
pthread_mutex_t lock;
int main(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "Not enough arguments\n Enter IP address and Port Number to connect to. \n Example: 10.0.0.1 9009\n");
        exit(1);
    }
    int modeChosen = FULLTEST;
    int ServerSocket = 0;
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = gmtime(&rawtime);
    printf("Current GMT time and date: %s", asctime(timeinfo));
    unsigned int randval = 0;
    int command = 0;
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("mutex init has failed\n");
        return -1;
    }
    int rtv = pthread_create(&tid[0], NULL, receiveCommand, (void *)&command);
    if (rtv != 0)
    {
        printf(" ERROR;  pthread_create1()   returns   %d\n", rtv);
        return -1;
    }
    while (1)
    {
        if (!getRandomInt(0, SECS, &randval))
        {
            randval = rand() % SECS;
        }
        if (timeinfo->tm_hour >= 19 && timeinfo->tm_hour <= 22)
        {
            sleep(randval);
            sendSpeed(argv[1], argv[2], modeChosen, &ServerSocket);
        }
        switch (command)
        {
        case 1:
            printf("Mode = 1\n");
            break;
        case 2:
            printf("Mode = 2\n");
            break;
        case 3:
            printf("Mode = 3\n");
            break;
        case 4:
            printf("Mode = speedTest\n");
            sendSpeed(argv[1], argv[2], modeChosen, &ServerSocket);
            break;
        default:
            printf("Mode: %d\n",command);
        }
        command = 0;
        sleep(2);
    }
    pthread_mutex_destroy(&lock);
    close(ServerSocket);
    //sleep(300);
    return 0;
}
static int sendSpeed(char *IPv4, char *Port, int modeChosen, int *ServerSocket)
{
    double *DLSpeedArray = calloc(DATAPOINTS, sizeof(double));
    if (DLSpeedArray == NULL)
    {
        return -1;
    }
    double *UPSpeedArray = calloc(DATAPOINTS, sizeof(double));
    if (UPSpeedArray == NULL)
    {
        free(DLSpeedArray);
        return -1;
    }
    int mode = speedTest(ServerSocket, IPv4, Port, DLSpeedArray, UPSpeedArray, modeChosen);
    /*
    if (mode == FULLTEST)
    {
        for (int i = 0; i < DATAPOINTS; i++)
        {
            fprintf(stderr, "Download[%d]: %lf\nUpload[%d]: %lf\n\n", i, DLSpeedArray[i], i, UPSpeedArray[i]);
        }
    }
    else if (mode == DOWNLOAD)
    {
        for (int i = 0; i < DATAPOINTS; i++)
        {
            fprintf(stderr, "Download[%d]: %lf\n", i, DLSpeedArray[i]);
        }
    }
    else if (mode == UPLOAD)
    {
        for (int i = 0; i < DATAPOINTS; i++)
        {
            fprintf(stderr, "Upload[%d]: %lf\n", i, UPSpeedArray[i]);
        }
    }
    */
    if (mode == -1)
    {
        return -1;
    }
    char *JSON = malloc(sizeof(char) * MAXLength);
    if (JSON == NULL)
    {
        return -1;
    }
    int length = 0;
    createJSON(DLSpeedArray, UPSpeedArray, JSON, &length, mode);
    sendJSON(*ServerSocket, strtol(Port, NULL, 10), IPv4, length, JSON);
    free(JSON);
    free(DLSpeedArray);
    free(UPSpeedArray);
    return 0;
}
int getRandomInt(int low, int high, unsigned int *randval)
{
    FILE *f;
    f = fopen("/dev/urandom", "r");
    if (f == NULL)
    {
        return -1;
    }
    if (fread(randval, sizeof(unsigned int), 1, f))
    {
        *randval = *randval % (high - low);
        fclose(f);
        return 1;
    }
    fclose(f);
    return -1;
}

static int createJSON(double *DLSpeedArray, double *UPSpeedArray, char *JSON, int *JSONlength, int mode)
{

    FILE *fp = fopen("test.json", "w");
    int position = 0;
    catString(JSON, &position, "{\n");
    catString(JSON, &position, "\"SpeedTest\": {\n");
    char *speedBuffer = malloc(sizeof(char) * 100);
    if (mode == FULLTEST || mode == DOWNLOAD)
    {
        catString(JSON, &position, "    \"Download\": {\n");
        catString(JSON, &position, "        \"Values\": [\n");
        if (DLSpeedArray == NULL || UPSpeedArray == NULL)
        {
            return -1;
        }
        for (int i = 0; i < DATAPOINTS; i++)
        {

            if (i != DATAPOINTS - 1)
            {
                snprintf(speedBuffer, 50, "            %0.3lf,\n", DLSpeedArray[i]);
                catString(JSON, &position, speedBuffer);
            }
            else
            {
                snprintf(speedBuffer, 50, "            %0.3lf\n        ],\n", DLSpeedArray[i]);
                catString(JSON, &position, speedBuffer);
            }
        }
        double DLMax = -99999999999999;
        double DLMin = 99999999999999;
        double DLAvg = 0;
        getMaxMinAvg(DLSpeedArray, &DLMax, &DLMin, &DLAvg);
        snprintf(speedBuffer, 50, "        \"Max\": %.3lf,\n", DLMax);
        catString(JSON, &position, speedBuffer);
        snprintf(speedBuffer, 50, "        \"Min\": %.3lf,\n", DLMin);
        catString(JSON, &position, speedBuffer);
        snprintf(speedBuffer, 50, "        \"Average\": %.3lf\n", DLAvg);
        catString(JSON, &position, speedBuffer);
        catString(JSON, &position, "      }");
        if (mode == FULLTEST)
        {
            catString(JSON, &position, ",\n");
        }
        else
        {
            catString(JSON, &position, "\n");
        }
    }
    if (mode == FULLTEST || mode == UPLOAD)
    {
        catString(JSON, &position, "    \"Upload\": {\n");
        catString(JSON, &position, "        \"Values\": [\n");
        for (int i = 0; i < DATAPOINTS; i++)
        {
            if (i != DATAPOINTS - 1)
            {
                snprintf(speedBuffer, 50, "            %0.3lf,\n", UPSpeedArray[i]);
                catString(JSON, &position, speedBuffer);
            }
            else
            {
                snprintf(speedBuffer, 50, "            %0.3lf\n        ],\n", UPSpeedArray[i]);
                catString(JSON, &position, speedBuffer);
            }
        }
        double UPMax = -99999999999999;
        double UPMin = 99999999999999;
        double UPAvg = 0;
        getMaxMinAvg(UPSpeedArray, &UPMax, &UPMin, &UPAvg);
        snprintf(speedBuffer, 50, "        \"Max\": %.3lf,\n", UPMax);
        catString(JSON, &position, speedBuffer);
        snprintf(speedBuffer, 50, "        \"Min\": %.3lf,\n", UPMin);
        catString(JSON, &position, speedBuffer);
        snprintf(speedBuffer, 50, "        \"Average\": %.3lf\n", UPAvg);
        catString(JSON, &position, speedBuffer);
        catString(JSON, &position, "      }\n");
    }
    catString(JSON, &position, "   }\n}");
    fprintf(fp, "%s", JSON);
    *JSONlength = position;
    fclose(fp);
    free(speedBuffer);
    return 0;
}
static void catString(char *start, int *currentPos, char *str2Copy)
{
    char *ptr = &start[*currentPos];
    int i = 0;
    while (str2Copy[i] != '\0')
    {
        ptr[i] = str2Copy[i];
        i++;
    }
    ptr[i] = '\0';
    *currentPos = *currentPos + i;
    return;
}
static void getMaxMinAvg(double *DLSpeedArray, double *max, double *min, double *average)
{
    double curr = 0;
    double total = 0;
    for (int i = 0; i < DATAPOINTS; i++)
    {
        curr = DLSpeedArray[i];
        if (curr > *max)
        {
            *max = curr;
        }
        if (curr < *min)
        {
            *min = curr;
        }
        total = total + curr;
    }
    *average = total / DATAPOINTS;
    return;
}
static int sendJSON(int ServerSocket, int port, char *ip, int size, char *JSON)
{
    int sent = 0;
    long bytes = 0;
    char cont[] = "HTTP/1.1 100 Continue";
    //printf("Port: %d\n", port);
    //printf("IP: %s\n", ip);
    //printf("Size: %d",size);
    dprintf(ServerSocket, "POST /JSONPHP.php HTTP/1.1\r\nHost: %s:%d\r\n"
                          "User-Agent: VIPTest\r\nAccept: */*\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n"
                          "Content-Type: multipart/form-data;\r\nExpect: 100-continue\r\n\r\n",
            ip, port, size);
    char *recv_buffer = malloc(sizeof(char) * buff_size);
    if (recv_buffer == NULL)
    {
        return -1;
    }
    recv(ServerSocket, recv_buffer, buff_size, 0);
    if (strncmp(recv_buffer, cont, strlen(cont)) != 0)
    {
        free(recv_buffer);
        fprintf(stderr, "%s received\n", recv_buffer);
        return -1;
    }
    else
    {
        //fprintf(stderr,"HTTP/1.1 100 Continue Received\n");
    }
    //printf("%s",recv_buffer);
    char *send_buffer = calloc((size), sizeof(char));
    if (send_buffer == NULL)
    {
        free(recv_buffer);
        return -1;
    }
    memcpy(send_buffer, JSON, size);
    while (1)
    {
        sent = send(ServerSocket, send_buffer, (size), 0);
        bytes = bytes + sent;
        if (bytes >= size)
        {
            break;
        }
        send_buffer = &send_buffer[sent];
        size = size - sent;
    }
    memset(recv_buffer, 0, buff_size);
    recv(ServerSocket, recv_buffer, buff_size, 0);
    char response[] = "HTTP/1.1 200 OK\r\n";
    //printf("\n%.13s",recv_buffer);
    //printf("\nbytes: %ld,  size: %d\n",bytes,size);

    if ((strncmp(recv_buffer, response, strlen(response)) == 0))
    {

        fprintf(stderr,"HTTP OK received\n");
        //printf("\n%s", recv_buffer);
        //double usecs = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usecs
        //becomes Mb/s
    }
    else
    {
        fprintf(stderr, "%s", recv_buffer);
        free(recv_buffer);
        free(send_buffer);
        return -1;
    }
    free(recv_buffer);
    free(send_buffer);
    //shutdown(ServerSocket,1);
    return 0;
}