/* 
 *           flight-time-server.c: record and provide time of a
 *                                 flight from the airport
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/select.h>
#include <ctype.h>
#include <stdint.h>
#include <time.h>


#define SERVER_PORT                "4358"


#define BACKLOG                   10

void error (char *msg);

char buffer[1024];

void trim (char *dest, char *src); 
void error (char *msg);

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

int main (int argc, char **argv)
{
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
    fd_set fds, readfds;
    FD_ZERO (&fds);
    FD_SET (listener, &fds);
    int fdmax = listener;
    struct sockaddr_storage client_saddr;
    char str [INET6_ADDRSTRLEN];
    struct sockaddr_in  *ptr;
    struct sockaddr_in6  *ptr1;
    struct tnode *root = NULL;

    while (1) {
        readfds = fds;
        // monitor readfds for readiness for reading
        if (select (fdmax + 1, &readfds, NULL, NULL, NULL) == -1)
            error ("select");
        
        // Some sockets are ready. Examine readfds
        for (int fd = 0; fd < (fdmax + 1); fd++) {
            if (FD_ISSET (fd, &readfds)) {  // fd is ready for reading 
                if (fd == listener) {  // request for new connection
                    addrlen = sizeof (struct sockaddr_storage);
                    int fd_new;
                    if ((fd_new = accept (listener, (struct sockaddr *) &client_saddr, &addrlen)) == -1)
                        error ("accept");
                    FD_SET (fd_new, &fds); 
                    if (fd_new > fdmax) 
                        fdmax = fd_new;

                    // print IP address of the new client
                    if (client_saddr.ss_family == AF_INET) {
                        ptr = (struct sockaddr_in *) &client_saddr;
                        inet_ntop (AF_INET, &(ptr -> sin_addr), str, sizeof (str));
                    }
                    else if (client_saddr.ss_family == AF_INET6) {
                        ptr1 = (struct sockaddr_in6 *) &client_saddr;
	                inet_ntop (AF_INET6, &(ptr1 -> sin6_addr), str, sizeof (str));
                    }
                    else
                    {
                        ptr = NULL;
                        fprintf (stderr, "Address family is neither AF_INET nor AF_INET6\n");
                    }
                    if (ptr) 
                        syslog (LOG_USER | LOG_INFO, "%s %s", "Connection from client", str);
                
                }
                else  // data from an existing connection, receive it
                {
                    int bytes_received;
                    char buffer[1024];
                    bytes_received = recv(fd, buffer, sizeof(buffer), 0);
                    
   
                    if (bytes_received== -1)
                        error ("recv");
                    else if (bytes_received == 0) {
                        // connection closed by client
                        fprintf (stderr, "Socket %d closed by client\n", fd);
                        if (close (fd) == -1)
                            error ("close");
                        FD_CLR (fd, &fds);
                    }
                    else 
                    {
                        // data from client
                        char* endPtr;
                        long data = strtol(buffer, &endPtr, 10);
                        char ans[100];
                        //printf("response is %lld ",fact(data));
                        snprintf(ans, sizeof(ans), "%lld", fact(data));
                        if (send (fd, ans, sizeof(ans), 0) == -1)
                            error ("send");

                        
                        

                    }
                }
            } // if (fd == ...
        } // for
    } // while (1)

    exit (EXIT_SUCCESS);
} // main


void error (char *msg)
{
    perror (msg);
    exit (1);
}

// trim: leading and trailing whitespace of string
void trim (char *dest, char *src)
{
    if (!src || !dest)
       return;

    int len = strlen (src);

    if (!len) {
        *dest = '\0';
        return;
    }
    char *ptr = src + len - 1;

    // remove trailing whitespace
    while (ptr > src) {
        if (!isspace (*ptr))
            break;
        ptr--;
    }

    ptr++;

    char *q;
    // remove leading whitespace
    for (q = src; (q < ptr && isspace (*q)); q++)
        ;

    while (q < ptr)
        *dest++ = *q++;

    *dest = '\0';
}