/* 
 *           flight-time-server.c: record and provide time of a
 *                                 flight from the airport
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdint.h>
#include <time.h>

#define FLIGHT_NUM_SIZE            15

#define SERVER_PORT                "4358"
#define STORE_FLIGHT               1
#define FLIGHT_TIME_STORED         2
#define FLIGHT_TIME                3
#define FLIGHT_TIME_RESULT         4
#define FLIGHT_NOT_FOUND           5
#define ERROR_IN_INPUT             9

#define BACKLOG                   10
#define NUM_FDS                    5

#define MAX_EVENTS                4000

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

    int efd;
    if ((efd = epoll_create1 (0)) == -1)
	error ("epoll_create1");
    struct epoll_event ev, ep_event [MAX_EVENTS];

    ev.events = EPOLLIN;
    ev.data.fd = listener;
    if (epoll_ctl (efd, EPOLL_CTL_ADD, listener, &ev) == -1)
	error ("epoll_ctl");

    int nfds = 0;

    socklen_t addrlen;
    struct sockaddr_storage client_saddr;
    char str [INET6_ADDRSTRLEN];
    struct sockaddr_in  *ptr;
    struct sockaddr_in6  *ptr1;
    struct tnode *root = NULL;

    while (1) {
        // monitor readfds for readiness for reading
	if ((nfds = epoll_wait (efd, ep_event, MAX_EVENTS,  -1)) == -1)
	    error ("epoll_wait");

        // Some sockets are ready. Examine readfds
        for (int i = 0; i < nfds; i++) {

	    if 	((ep_event [i].events & EPOLLIN) == EPOLLIN) {
                if (ep_event [i].data.fd == listener) {  // request for new connection
                    addrlen = sizeof (struct sockaddr_storage);
                    int fd_new;
                    if ((fd_new = accept (listener, (struct sockaddr *) &client_saddr, &addrlen)) == -1)
                        error ("accept");
                    // add fd_new to epoll
                    ev.events = EPOLLIN;
                    ev.data.fd = fd_new;
                    if (epoll_ctl (efd, EPOLL_CTL_ADD, fd_new, &ev) == -1)
	                error ("epoll_ctl");
	
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
                    
                    ssize_t numbytes = recv (ep_event [i].data.fd, buffer, sizeof (buffer), 0);
   
                    if (numbytes == -1)
                        error ("recv");
                    else if (numbytes == 0) {
                        // connection closed by client
                        fprintf (stderr, "Socket %d closed by client\n", ep_event [i].data.fd);
			            // delete fd from epoll
                        if (epoll_ctl (efd, EPOLL_CTL_DEL, ep_event [i].data.fd, &ev) == -1)
	                        error ("epoll_ctl");
                        if (close (ep_event [i].data.fd) == -1)
                            error ("close");
                    }
                    else 
                    {
                        // data from client
                        char* endPtr;
                        long data = strtol(buffer, &endPtr, 10);
                        char ans[100];
                        //printf("response is %lld ",fact(data));
                        snprintf(ans, sizeof(ans), "%lld", fact(data));
                        if (send (ep_event [i].data.fd, ans, sizeof(ans), 0) == -1)
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
