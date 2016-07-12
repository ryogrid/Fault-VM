#include <sys/types.h>	/* socket(), wait4() */
#include <sys/socket.h>	/* socket() */
#include <netinet/in.h>	/* struct sockaddr_in */
#include <sys/resource.h> /* wait4() */
#include <sys/wait.h>	/* wait4() */
#include <netdb.h>	/* getnameinfo() */

/* for server */
extern	int tcp_bind( int portno );	/* return socket file descriptor*/
extern  int tcp_accept(int sock_fd);   /* return socket */
extern	int tcp_open_stream( int sock, FILE **inp, FILE **outp ); /* create in/out stream with socket from accept function or tcp_connect function*/

/* for client */
extern  int tcp_connect( char *server, int portno ); /* return socket */

/* utility */
extern	void tcp_print_host( int portno );   /* print information about binding host and port */
extern	void tcp_print_peeraddr( int sock );  /* print information abount accepted connection with socket from accept function */
extern  void tcp_set_timeout(int sock,int sec);

#define BUFFERSIZE      1024
