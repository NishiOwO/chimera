/*
 * net.c
 *
 * Copyright (C) 1993, 1994, John Kilburg.
 *
 * See copyright.h for details.
 */
#include "copyright.h"

#include "options.h"

#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>

#if ((defined(SYSV) || defined(SVR4)) && defined(sun))
#include <sys/file.h>
#endif
 
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "net.h"
#include "util.h"

#if defined(SYSV) || defined(SVR4)
# define bzero(dst,len) memset(dst,0,len)
# define bcopy(src,dst,len) memcpy(dst,src,len)
#endif /*SYSV*/

#ifdef TERM
#include "termnet.h"
#endif

/*
 * net_open
 *
 * open network connection to a host.  the regular version.
 *
 */
int
net_open(hostname, port)
char *hostname;
int port;
{
  int s;
  struct sockaddr_in addr;
  struct hostent *hp;
  extern int errno;

  bzero((char *)&addr, sizeof(addr));

  /* fix by Jim Rees so that numeric addresses are dealt with */
  if ((addr.sin_addr.s_addr = inet_addr(hostname)) == -1)
  {
    if ((hp = (struct hostent *)gethostbyname(hostname)) == NULL) return(-1);
    bcopy(hp->h_addr, (char *)&(addr.sin_addr), hp->h_length);
  }

  addr.sin_family = AF_INET;
  addr.sin_port = htons((unsigned short)port);

  s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (s < 0) return(-1);

#ifdef NONBLOCKING_CONNECT
  fcntl(s, F_SETFL, FNDELAY);
#endif

#ifndef SOCKS 
  if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
#else
  if (Rconnect(s, &addr, sizeof(addr)) < 0)
#endif
  {
    if (errno != EINPROGRESS) return(-1);
  }
  
  return(s);
}

/*
 * net_bind
 *
 * open a socket, bind a port to it, and listen for connections.
 */
int
net_bind(port)
int port;
{
  int s;
  struct sockaddr_in addr;

  s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (s < 0) return(-1);

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons((unsigned short)port);
  
  if (bind(s, (struct sockaddr *)&addr, sizeof(addr))) return(-1);

  if (listen(s, 1) < 0) return(-1);

  return(s);
}

/*
 * net_close
 *
 * close a network connection
 *
 */
void
net_close(s)
int s;
{
  close(s);

  return;
}

/*
 * net_gethostname
 *
 * Returns the full domain name of the local host.  Thanks Jim.
 */
char *
net_gethostname()
{
  char myname[100];
  struct hostent *hp;
  static char domain[BUFSIZ];

  if (gethostname(myname, sizeof(myname)) < 0) return(NULL);
  if ((hp = (struct hostent *)gethostbyname(myname)) == NULL) return(NULL);
  
  if (strlen(hp->h_name) < sizeof(domain))
  {
    strcpy(domain, hp->h_name);
    return(domain);
  }

  return(NULL);
}

/*
 * net_accept
 *
 */
int
net_accept(s)
int s;
{
  struct sockaddr_in sock;
  int len = sizeof(sock);

  return(accept(s, (struct sockaddr *)&sock, &len));
}
