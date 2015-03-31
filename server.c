#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

#define SERVER_PORT 8181 
#define MAX_PENDING 1
#define BUFSIZE 256

//Programmer: Victor Rogers ====================================================
//Class: CS 360
//Assignment: Program 2 (SERVER)
//==============================================================================
//Description: This is a server that implements the "Knock Knock Protocol" as
//		described in the instruction document for the CS360 Program 1
//		assignment.
//		
//		NOTE: Please view the README for instruction on testing the
//		server without a client.
//==============================================================================

void SIGHandler (int s); 

int main() {
	struct sigaction SIGINTHandler;
	SIGINTHandler.sa_handler = SIGHandler;
	sigemptyset(&SIGINTHandler.sa_mask);
	SIGINTHandler.sa_flags = 0;


	struct sockaddr_in sin;
	unsigned int s, new_s, len;
	int bytes_recv, bytes_sent;
	char buf[BUFSIZE];

	//Address Data Structure =====================================================
	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(SERVER_PORT);
	//============================================================================

	//Open Socket ================================================================
	if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Error while creating socket");
		exit(1);	
	}
	//============================================================================

	//Bind socket to address =====================================================
	if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0) {
		perror("Error while binding the port");
		exit(1);	
	}
	//============================================================================

	//Listen for connections======================================================
	listen(s, MAX_PENDING);
	//============================================================================

	while(!sigaction(SIGINT, &SIGINTHandler, NULL)) {
		len = sizeof(sin);

		if ((new_s = accept(s, (struct sockaddr *)&sin, &len)) < 0) {
			perror("Error while accepting the connection");
			exit(1);	
		}

		//NOTE: Ignore warning - This assignment is on purpose 
		while (bytes_recv = recv(new_s, buf, sizeof(buf), 0)) {
			buf[bytes_recv] = '\0';	
			printf("Received: %.3s %.*s\n", buf, 50, buf + 3);
			
		}
	}
}

void SIGHandler (int s) {
	printf("\nCaught signal: %d\nShutting down server.\n", s);
	exit(1);
};
