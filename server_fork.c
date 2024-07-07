#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdint.h>
#include <time.h>


#define SERVER_PORT                "4358"
#define BACKLOG                   10
#define NUM_FDS                   4000

void error (char *msg);

char buffer[1024];

void trim (char *dest, char *src); 


long long int fact(int n){
    if(n>20){
        n=20;
    }
    long long int ans=1;
    int i=1;
    while(i<n){
        ans*=i;
        i++;
    }
    return ans;
}

void handle_client(int client_socket) {
    char buffer[1024];
    int bytes_received;
    uint64_t n;
    
    while (1) {
        bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            close(client_socket);
            exit(0);
        }

		char* endPtr;
		long data = strtol(buffer, &endPtr, 10);
		char ans[100];
		//printf("response is %lld ",fact(data));
		snprintf(ans, sizeof(ans), "%lld", fact(data));
		if (send (client_socket, ans, sizeof(ans), 0) == -1)
			error ("send");
        
    }
}


int main(){

	const char * const ident = "flight-time-server";

    openlog (ident, LOG_CONS | LOG_PID | LOG_PERROR, LOG_USER);
    syslog (LOG_USER | LOG_INFO, "%s", "Hello world!");
    
    struct addrinfo hints;
    memset(&hints, 0, sizeof (struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Stream socket */
    hints.ai_flags = AI_PASSIVE;    /* for wildcard IP address */

    struct addrinfo *result;
    int s; 
    if ((s = getaddrinfo (NULL, SERVER_PORT, &hints, &result)) != 0) {
        fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (s));
        exit (EXIT_FAILURE);
    }

    /* Scan through the list of address structures returned by 
       getaddrinfo. Stop when the the socket and bind calls are successful. */

    int listener, optval = 1;
    socklen_t length;
    struct addrinfo *rptr;
    for (rptr = result; rptr != NULL; rptr = rptr -> ai_next) {
        listener = socket (rptr -> ai_family, rptr -> ai_socktype,
                       rptr -> ai_protocol);
        if (listener == -1)
            continue;

        if (setsockopt (listener, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof (int)) == -1)
            error("setsockopt");

        if (bind (listener, rptr -> ai_addr, rptr -> ai_addrlen) == 0)  // Success
            break;

        if (close (listener) == -1)
            error ("close");
    }

    if (rptr == NULL) {               // Not successful with any address
        fprintf(stderr, "Not able to bind\n");
        exit (EXIT_FAILURE);
    }

    freeaddrinfo (result);

    // Mark socket for accepting incoming connections using accept
    if (listen (listener, BACKLOG) == -1)
        error ("listen");

	socklen_t addrlen;
    struct sockaddr_storage client_saddr;
    char str [INET6_ADDRSTRLEN];
    struct sockaddr_in  *ptr;
    struct sockaddr_in6  *ptr1;

	while(1){
		pid_t childpid;
		int newSocket = accept(listener, (struct sockaddr*)&client_saddr, &addrlen);
        
		if(newSocket < 0){
			exit(1);
		}
		
		childpid = fork();
		if (childpid == 0) {
            close(listener);
            handle_client(newSocket);
        } else if (childpid > 0) {
            close(newSocket);
        } else {
            perror("Fork failed");
            close(newSocket);
        }

	}

	close(listener);


	return 0;
}