/*
 * http.c
 *
 * Copyright (C) 1993, 1994, John Kilburg.
 *
 * See copyright.h for details.
 */
#include "copyright.h"

#include "options.h"

#include <stdio.h>
#include <sys/types.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "url.h"
#include "mime.h"
#include "document.h"
#include "http.h"
#include "net.h"
#include "input.h"
#include "auth.h"
#include "util.h"
#include "stringdb.h"

#define DEFAULT_HTTP_PORT 80

#ifndef _NO_PROTO
extern int DisplayTransferStatus(int);
#else
extern int DisplayTransferStatus();
#endif

/*
 * ParseHTTPRequest
 *
 */
Document *
ParseHTTPRequest(s)
int s;
{
  Document *d;
  char buffer[BUFSIZ];
  int error_code = 0;
  int tlen;
  int header_end = 0;
  MIMEField *m;

  d = CreateDocument();

  /*
   * Read HTTP request command.
   */
  if (ReadLine(s, buffer, sizeof(buffer)) == -1)
  {
    DestroyDocument(d);
    return(NULL);
  }

/*
 * Don't care what this is right now.
 *
  if (sscanf(buffer, "%s %s %s", command, url, proto) != 3)
  {
    ;
  }
*/

  /*
   * Read the MIME header.
   */
  while (1)
  {
    if (ReadLine(s, buffer, sizeof(buffer)) != 0) break;

    /*
     * End of header?
     */
    if (buffer[0] == '\r' || buffer[0] == '\n')
    {
      header_end = 1;
      break;
    }

    if ((m = CreateMIMEField(buffer)) != NULL)
    {
      ParseMIMEField(d, m);
      m->next = d->mflist;
      d->mflist = m;
    }
  }

  if (!header_end)
  {
    char *msg = GetFromStringDB("abort");

    DestroyDocument(d);

    return(BuildDocument(msg, strlen(msg), "text/html", 1, 0));
  }

  /*
   * This has to be done because a number of servers return the wrong
   * content-length.
   */
  d->text = ReadBuffer(s, &tlen, 0, d->len);
  d->len = tlen;
  d->status = error_code;

  return(d);
}

/*
 * ParseHTTPResponse
 *
 * This function sorts out the standard HTTP header (which looks like
 * a MIME header) and sorts out the error code and all that jazz.
 * This is a whole bunch of hard-coded MIME string fields and stuff
 * in here.
 */
Document *
ParseHTTPResponse(s)
int s;
{
  Document *d;
  char buffer[BUFSIZ];
  char *t;
  int error_code = 0;
  int tlen;
  int header_end = 0;
  MIMEField *m;

  d = CreateDocument();
  
  t = ReadBuffer(s, &tlen, 8, 8);
  if (t == NULL) return(NULL);
  
  if (strncmp(t, "HTTP/1.0", 8) != 0 && strncmp(t, "HTTP/1.1", 8) != 0)
  {
    char *x;
    int xlen;
    
    x = ReadBuffer(s, &xlen, 0, 0);
    if (x == NULL)
    {
      d->text = t;
      d->len = tlen;
    }
    else
    {
      d->len = tlen + xlen;
      d->text = alloc_mem(d->len + 1);
      strcpy(d->text, t);
      strcpy(d->text + tlen, x);
      free(t);
      free(x);
    }
    return(d);
  }
    
  free(t);
  
  if (ReadLine(s, buffer, sizeof(buffer)) == -1)
  {
    DestroyDocument(d);
    return(NULL);
  }
  
  sscanf (buffer, "%d", &error_code);

  /*
   * Read the MIME header.
   */
  while (1)
  {
    if (ReadLine(s, buffer, sizeof(buffer)) != 0) break;

    /*
     * End of header?
     */
    if (buffer[0] == '\r' || buffer[0] == '\n')
    {
      header_end = 1;
      break;
    }

    if ((m = CreateMIMEField(buffer)) != NULL)
    {
      ParseMIMEField(d, m);
      m->next = d->mflist;
      d->mflist = m;
    }
  }

  if (!header_end)
  {
    char *msg = GetFromStringDB("abort");

    DestroyDocument(d);

    return(BuildDocument(msg, strlen(msg), "text/html", 1, 0));
  }

  /*
   * This has to be done because a number of servers return the wrong
   * content-length.
   */
  d->text = ReadBuffer(s, &tlen, 0, d->len);
  d->len = tlen;
  d->status = error_code;

  return(d);
}

static char *format_get = "\
GET %s HTTP/1.0\n\
Host: %s\n\
User-Agent: %s\n\
Accept: */*\n\
%s\
%s\
\n";

static char *format_get2 = "\
GET %s?%s HTTP/1.0\n\
Host: %s\n\
User-Agent: %s\n\
Accept: */*\n\
%s\
%s\
\n";

static char *format_post = "\
POST %s HTTP/1.0\n\
Host: %s\n\
User-Agent: %s\n\
Accept: */*\n\
Content-length: %d\n\
Content-type: %s\n\
%s\
%s\
\n\
%s\
\n";

static char *format_get_crlf = "\
GET %s HTTP/1.0\r\n\
Host: %s\r\n\
User-Agent: %s\r\n\
Accept: */*\r\n\
%s\
%s\
\r\n";

static char *format_get2_crlf = "\
GET %s?%s HTTP/1.0\r\n\
Host: %s\r\n\
User-Agent: %s\r\n\
Accept: */*\r\n\
%s\
%s\
\r\n";

static char *format_post_crlf = "\
POST %s HTTP/1.0\r\n\
Host: %s\r\n\
User-Agent: %s\n\
Accept: */*\r\n\
Content-length: %d\r\n\
Content-type: %s\r\n\
%s\
%s\
\r\n\
%s\
\r\n";

/*
 * crap for uuencode
 */
static char six2pr[64] =
{
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z',
    '0','1','2','3','4','5','6','7','8','9','+','/'
};

/*
 * uuencode
 *
 * This is used to uuencode a string.  It pisses me off that this is even
 * needed.  The WWW guys LIKE complexity and huge code because it keeps
 * them employed (ah wild speculation is good!).
 *
 * I snarfed this code from some version of libwww.
 *
 * ACKNOWLEDGEMENT:
 *      This code is taken from rpem distribution, and was originally
 *      written by Mark Riordan.
 *
 * AUTHORS:
 *      MR      Mark Riordan    riordanmr@clvax1.cl.msu.edu
 *      AL      Ari Luotonen    luotonen@dxcern.cern.ch
 *
 */
static int
uuencode(bufin, nbytes, bufcoded)
unsigned char *bufin;
unsigned int nbytes;
char *bufcoded;
{
  /* ENC is the basic 1 character encoding function to make a char printing */
#define ENC(c) six2pr[c]
  
  register char *outptr = bufcoded;
  unsigned int i;
  
  for (i=0; i<nbytes; i += 3) 
  {
    *(outptr++) = ENC(*bufin >> 2);            /* c1 */
    *(outptr++) = ENC(((*bufin << 4) & 060) | ((bufin[1] >> 4) & 017)); /*c2*/
    *(outptr++) = ENC(((bufin[1] << 2) & 074) | ((bufin[2] >> 6) & 03));/*c3*/
    *(outptr++) = ENC(bufin[2] & 077);         /* c4 */
    
    bufin += 3;
  }
  
  /* If nbytes was not a multiple of 3, then we have encoded too
   * many characters.  Adjust appropriately.
   */
  if (i == nbytes + 1)
  {
    /* There were only 2 bytes in that last group */
    outptr[-1] = '=';
  }
  else if (i == nbytes + 2)
  {
    /* There was only 1 byte in that last group */
    outptr[-1] = '=';
    outptr[-2] = '=';
  }
  *outptr = '\0';

  return(outptr - bufcoded);
}

/*
 * build_auth_info
 *
 * Build the string needed for authorization.
 */
static char *
build_auth_info(type, username, password)
char *type;
char *username;
char *password;
{
  char *garbage;
  char *auth_info;
  static char *format = "Authorization: %s %s\n";

  garbage = alloc_mem(strlen(username) +
		      1 +
		      strlen(password != NULL ? password:"") +
		      1);
  strcpy(garbage, username);
  if (password != NULL) strcat(garbage, ":");
  if (password != NULL) strcat(garbage, password);

  if (strcmp(type, "Basic") == 0)
  {
    char *t;

    t = alloc_mem(strlen(garbage) * 2);
    uuencode(garbage, strlen(garbage), t);
    free(garbage);
    garbage = t;;
  }

  auth_info = alloc_mem(strlen(format) +
			strlen(type) +
			strlen(garbage) + 1);
  sprintf (auth_info, format,
	   type,
	   garbage);

  return(auth_info);
}

/* 
 * build_request
 *
 * Make a HTTP request string
 */
static char *
build_request(up, filename, extra_header, crlf)
URLParts *up;
char *filename;
char *extra_header;
int crlf;
{
  char *data_type;
  char *query;
  char *auth_info = NULL;
  char *format;
  char* c_host;

  c_host = malloc(strlen(up->hostname) + 10); /* not a good code but should work */
  if(up->port == 0){
    sprintf(c_host, "%s", up->hostname);
  }else{
    sprintf(c_host, "%s:%d", up->hostname, up->port);
  }

  if (up->data_type == NULL) data_type = "application/x-www-form-urlencoded";
  else data_type = up->data_type;

  if (filename == NULL) filename = up->filename;
  if (extra_header == NULL) extra_header = "";

  if (up->auth_type != NULL)
  {
    if (up->username != NULL)
    {
      auth_info = build_auth_info(up->auth_type, up->username, up->password);
    }
  }
  if (auth_info == NULL) auth_info = alloc_string("");

  if (up->request_data != NULL)
  {
    if (mystrcmp(up->method, "GET") == 0) /* GET ! */
    {
      format = crlf == 1 ? format_get2_crlf:format_get2;
      query = alloc_mem(strlen(filename) +
			strlen(format) +
			strlen(up->request_data) +
			strlen(extra_header) +
			strlen(auth_info) + 
			strlen(USER_AGENT) +
			strlen(c_host) + 1);
      sprintf (query,
	       format,
	       filename,
	       up->request_data,
	       c_host,
	       USER_AGENT,
	       extra_header,
	       auth_info);
    }
    else if (mystrcmp(up->method, "POST") == 0) /* POST ! */
    {
      format = crlf == 1 ? format_post_crlf:format_post;
      query = alloc_mem(strlen(filename) +
			strlen(format) +
			strlen(up->request_data) +
			strlen(data_type) +
			strlen(extra_header) +
			strlen(auth_info) +
			strlen(USER_AGENT) +
			strlen(c_host) + 1);
      sprintf (query,
	       format,
	       filename,
	       c_host,
	       USER_AGENT,
	       strlen(up->request_data),
	       data_type,
	       extra_header,
	       auth_info,
	       up->request_data);
    }
    else return(NULL);
  }
  else /* regular GET */
  {
    format = crlf == 1 ? format_get_crlf:format_get;
    query = alloc_mem(strlen(filename) +
		      strlen(format) +
		      strlen(extra_header) +
		      strlen(auth_info) + 
		      strlen(USER_AGENT) + 
		      strlen(c_host) + 1);
    sprintf (query,
	     format,
	     filename,
             c_host,
	     USER_AGENT,
	     extra_header,
	     auth_info);
  }

  free(auth_info);
  free(c_host);

  return(query);
}

/*
 * http_request
 *
 * Sends an HTTP request
 */
int
http_request(fd, up, type, reload)
int fd;
URLParts *up;
int type;
int reload;
{
  char *query;
  char *extra_header;

  if (type == 0)
  {
    if (reload != 0) extra_header = "Pragma: no-cache\n";
    else extra_header = "";

    query = build_request(up, NULL, extra_header, 1);
  }
  else if (type == 1)
  {
    char *filename;

    if (reload != 0) extra_header = "Pragma: no-cache\n";
    else extra_header = "";

    filename = MakeURL(up);
    query = build_request(up, filename, extra_header, 1);
    free(filename);
  }
  else
  {
    char *url;
    static char *format = "\
URI: %s\n\
X-protocol: %s\n\
X-hostname: %s\n\
X-port: %d\n\
X-filename: %s\n";

    url = MakeURL(up);
    extra_header = alloc_mem(strlen(url) * 5 + strlen(format) + 1);
    sprintf (extra_header,
	     format,
	     url,
	     up->protocol != NULL ? up->protocol:"",
	     up->hostname != NULL ? up->hostname:"",
	     up->port,
	     up->filename != NULL ? up->filename:"/");

    query = build_request(up, NULL, extra_header, 0);
    free(extra_header);
    free(url);
  }

  if (query == NULL) return(-1);

  if (WriteBuffer(fd, query, strlen(query)) < 0)
  {
    free(query);

    return(-1);
  }

  free(query);

  return(0);
}

/*
 * http_main
 *
 * Talk to an HTTP server.
 */
static Document *
http_main(up, rup, mtlist, reload)
URLParts *up;
URLParts *rup;
MIMEType *mtlist;
int reload;
{
  Document *d;
  int s;
  char *hostname;

  /*
   * Start chitchatting with the HTTP server.
   */
  hostname = up->hostname != NULL ? up->hostname:"www";
  s = net_open(hostname, up->port == 0 ? DEFAULT_HTTP_PORT:up->port);
  if (s < 0) return(NULL);

  if (http_request(s, rup != NULL ? rup:up, rup != NULL ? 1:0, reload) == -1)
  {
    net_close(s);
    return(NULL);
  }

  d = ParseHTTPResponse(s);

  net_close(s);

  if (d == NULL) return(NULL);
  else if (d->content == NULL)
  {
    char *content;

    if ((content = Ext2Content(mtlist, up->filename)) == NULL)
    {
      content = "text/html";
    }
    d->content = alloc_string(content);
  }

  if (up->request_data != NULL) d->cache = 0;

  return(d);
}

/*
 * http
 *
 * Frontend for regular HTTP
 */
Document *
http(up, mtlist)
URLParts *up;
MIMEType *mtlist;
{
  return(http_main(up, NULL, mtlist, 0));
}

/*
 * http_proxy
 *
 * Frontend for proxy HTTP
 */
Document *
http_proxy(up, rup, mtlist, reload)
URLParts *up;
URLParts *rup;
MIMEType *mtlist;
int reload;
{
  return(http_main(up, rup, mtlist, reload));
}
