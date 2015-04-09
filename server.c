#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <time.h>

#define ISspace(x) isspace((int)(x))

#define SERVER_PORT 8180 
#define MAX_PENDING 1
#define BUFSIZE 9000 

//Programmer: Victor Rogers ====================================================
//Class: CS 360
//Assignment: Program 2 (SERVER)
//==============================================================================
void SIGHandler (int s); 
void serve_file (int s, const char * path);
void transmit_headers(int s, const char * path, int size);
void transmit_file(int s, FILE * file);
void not_found (int s);
void not_implemented(int s);
int get_line(int s, char *buf, int size);

int main(int argc, char *argv[]) {
  struct sigaction SIGINTHandler;
  SIGINTHandler.sa_handler = SIGHandler;
  sigemptyset(&SIGINTHandler.sa_mask);
  SIGINTHandler.sa_flags = 0;

  struct sockaddr_in sin;
  unsigned int s, new_s, len;
  int numchars;
  char buf[BUFSIZE];
  char requestType[255];
  char * docRoot;
  char path[512];
  char url[255];
  struct stat st;

  if (argc > 1) {
    docRoot = argv[1];
  } else {
    perror("Please enter a document directory as the second argument");
    exit(1);
  }

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
    
    //Accept Connection
    if ((new_s = accept(s, (struct sockaddr *)&sin, &len)) < 0) {
      perror("Error while accepting the connection");
      exit(1);	
    }
   
    numchars = get_line(new_s, buf, sizeof(buf));
    int i = 0; 
    int j = 0;
    
    while (!ISspace(buf[j]) && (i < sizeof(requestType) - 1)) {
      requestType[i] = buf[j];
      i++; j++;
    }
    requestType[i] = '\0';

    if (strcasecmp(requestType, "GET")) {
      not_implemented(new_s);
      close(new_s);
    }

    i = 0;
    while (ISspace(buf[j]) && (j < sizeof(buf)))
      j++;
    while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf))) {
      url[i] = buf[j];
      i++;
      j++;
    }
    url[i] = '\0';

    sprintf(path, "%s%s", docRoot, url);
    if (path[strlen(path) - 1] == '/')
      strcat(path, "index.html");
    if (stat(path, &st) == -1) {
      while ((numchars > 0) && strcmp("\n", buf))
        numchars = get_line(new_s, buf, sizeof(buf));
      not_found(new_s);
    }
    serve_file(new_s, path);
    close(new_s);
  }

  return 0;
}

void SIGHandler (int s) {
	printf("\nCaught signal: %d\nShutting down server.\n", s);
	exit(1);
}

void serve_file(int s, const char * path) {
  FILE * file = NULL;
  int filesize;
  int numchars = 1;
  char buf[BUFSIZE];

  buf[0] = 'A';
  buf[1] = '\0';
  while ((numchars > 0) && strcmp("\n", buf))
    numchars = get_line(s, buf, sizeof(buf));

  file = fopen(path, "r");
  if (file == NULL)
    not_found(s);
  else {
    fseek(file, 0, SEEK_END);
    filesize = ftell(file);
    fseek(file, 0, SEEK_SET);
    transmit_headers(s, path, filesize);
    transmit_file(s, file);
  }
  fclose(file);
}

void transmit_headers(int s, const char * path, int size) {
  char buf[BUFSIZE];
  char * ext;
  char * type;
  if ((ext = strrchr(path, '.')) != NULL) {
    if (strcmp(ext, ".jpg") == 0) 
      type = "image/jpeg";
    else
      type = "text/html";
  }

  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  send(s, buf, strlen(buf), 0);
  sprintf(buf, "Connection: keep-alive\r\n");
  send(s, buf, strlen(buf), 0);
  sprintf(buf, "Content-Type: %s\r\n", type);
  send(s, buf, strlen(buf), 0);
  sprintf(buf, "Content-Length: %d\r\n", size);
  send(s, buf, strlen(buf), 0);
  sprintf(buf, "Server: cs360httpd/1.0 (Unix)\r\n");
  send(s, buf, strlen(buf), 0);
  sprintf(buf, "\r\n");
  send(s, buf, strlen(buf), 0);
}

void transmit_file(int s, FILE * file) {
  char buf[BUFSIZE];

  fgets(buf, sizeof(buf), file);
  while (!feof(file)) {
    send(s, buf, strlen(buf), 0);
    fgets(buf, sizeof(buf), file);
  }
}

void not_found(int s) {
  char buf[BUFSIZE];

  sprintf(buf, "HTTP/1.0 404 Not Found\r\n");
  send(s, buf, strlen(buf), 0);
  sprintf(buf, "Connection: close\r\n");
  send(s, buf, strlen(buf), 0);
  sprintf(buf, "Content-Type: text/html\r\n");
  send(s, buf, strlen(buf), 0);
  sprintf(buf, "Server: cs360httpd/1.0 (Unix)\r\n");
  send(s, buf, strlen(buf), 0);
  sprintf(buf, "\r\n");
  send(s, buf, strlen(buf), 0);
  sprintf(buf, "<HTML><TITLE>404 Not Found</TITLE></HTML>\r\n");
  send(s, buf, strlen(buf), 0);
}

void not_implemented(int s) {
  char buf[BUFSIZE];
  sprintf(buf, "HTTP/1.0 501 Not Implemented\r\n");
  send(s, buf, strlen(buf), 0);
  sprintf(buf, "Connection: close\r\n");
  send(s, buf, strlen(buf), 0);
  sprintf(buf, "Content-Type: text/html\r\n");
  send(s, buf, strlen(buf), 0);
  sprintf(buf, "Server: cs360httpd/1.0 (Unix)\r\n");
  send(s, buf, strlen(buf), 0);
  sprintf(buf, "<HTML><TITLE>Not Implemented</TITLE></HTML>\r\n");
  send(s, buf, strlen(buf), 0);
}

int get_line(int s, char *buf, int size) {
  int i = 0;
  char c = '\0';
  int n;

  while ((i < size - 1) && (c != '\n')) {
    n = recv(s, &c, 1, 0);
    if (n > 0) {
      if (c == '\r') {
        n = recv(s, &c, 1, MSG_PEEK);
        if ((n > 0) && (c == '\n'))
          recv(s, &c, 1, 0);
        else
          c = '\n';
      }
      buf[i] = c;
      i++;
    }
    else
      c = '\n';
  }
  buf[i] = '\0';

  return(i);
}
