/*
 * ftp.c
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
#include "ftp.h"
#include "net.h"
#include "util.h"
#include "input.h"

#define DEFAULT_FTP_PORT 21

#ifndef _NO_PROTO
static char *ftp_dir(char *, int, char *, char *);
static int soak(int, char *);
extern int DisplayTransferStatus(int);
#else
static char *ftp_dir();
static int soak();
extern int DisplayTransferStatus();
#endif

/*
 * soak
 * 
 * This function is used to "soak up" the multiline BS that some
 * ftp servers spew (not that it is bad to have nice long
 * informational messages its just that I hate them from my
 * programmer's point of view).
 */
static int
soak(s, msg)
int s;
char *msg;
{
  char buffer[BUFSIZ];
  int code = -1;
  int rval;

  if (msg != NULL)
  {
    if (WriteBuffer(s, msg, strlen(msg)) == -1) return(999);
  }

  while ((rval = ReadLine(s, buffer, sizeof(buffer))) == 0)
  {
    if (code == -1) code = atoi(buffer);
    if (buffer[3] == ' ' && buffer[0] > ' ') break;
  }

  if (rval < 0 && code == -1) return(999);

  return(code);
}

/*
 * ftp
 *
 * Anonymous FTP interface
 *
 * This is getting to be quite large and that is without all of the
 * necessary error checking being done.
 */
Document *
ftp(up, mtlist)
URLParts *up;
MIMEType *mtlist;
{
  Document *doc;
  char *query;
  char buffer[BUFSIZ];
  char data_hostname[48];
  char *t;
  char *domain;
  char *uname;
  char *filename;
  int s, d;
  int isdir;
  int h0, h1, h2, h3, p0, p1, reply, n;
  int data_port;
  int tlen = 0;
  int size;
  char *hostname;
  static char *format = "PASS -%s@%s\r\n"; 
  static char *format2 = "RETR %s\r\n";
  static char *format3 = "CWD %s\r\n";
  static char *format4 = "SIZE %s\r\n";

  filename = up->filename;

  /*
   * Contact the ftp server on the usualy port.  Hardcoding ports is bad.
   */
  hostname = up->hostname != NULL ? up->hostname:"ftp";
  s = net_open(hostname, (up->port == 0 ? DEFAULT_FTP_PORT:up->port));
  if (s < 0) return(NULL);

  /*
   * wait for the connect() to succeed
   */
#ifdef NONBLOCKING_CONNECT
  if (WaitForConnect(s) < 0) return(NULL);
#endif

  /*
   * Take care of the greeting.
   */
  if (soak(s, NULL) >= 400) 
  {
    net_close(s);
    return(NULL);
  }

  /*
   * Send the user name
   */
  if (soak(s, "USER anonymous\r\n") >= 400)
  {
    net_close(s);
    return(NULL);
  }

  domain = net_gethostname();
  if (domain == NULL)
  {
    net_close(s);
    return(NULL);
  }

  /*
   * Send the password
   */
  if ((uname = getenv("EMAIL")) != NULL)
  {
    query = alloc_mem(strlen(uname) + 9);
    strcpy(query, "PASS -");
    strcat(query, uname);
    strcat(query, "\r\n");
  }
  else
  {
    if ((uname = getenv("USER")) == NULL) uname = "nobody";
    query = alloc_mem(strlen(uname) + 
			   strlen(domain) + strlen(format) + 1);
    sprintf(query, format, uname, domain);
  }

  if (soak(s, query) >= 400)
  {
    net_close(s);
    return(NULL);
  }

  free(query);

  /*
   * Set binary transfers
   */
  if (soak(s, "TYPE I\r\n") >= 400)
  {
    net_close(s);
    return(NULL);
  }

  /*
   * Set passive mode and grab the port information
   */
  if (WriteBuffer(s, "PASV\r\n", 6) == -1)
  {
    net_close(s);
    return(NULL);
  }

  if (ReadLine(s, buffer, sizeof(buffer)) == -1)
  {
    net_close(s);
    return(NULL);
  }

  n = sscanf(buffer, "%d %*[^(] (%d,%d,%d,%d,%d,%d)", &reply, &h0, &h1, &h2, &h3, &p0, &p1);
  if (n != 7 || reply != 227)
  {
    net_close(s);
    return(NULL);
  }

  sprintf (data_hostname, "%d.%d.%d.%d", h0, h1, h2, h3);

  /*
   * Open a data connection
   */
  data_port = (p0 << 8) + p1;
  d = net_open(data_hostname, data_port);
  if (d < 0)
  {
    net_close(s);
    return(NULL);
  }

  query = alloc_mem(strlen(filename) + strlen(format4) + 1);
  sprintf (query, format4, filename);
  if (WriteBuffer(s, query, strlen(query)) == -1)
  {
    net_close(s);
    net_close(d);
    free(query);
    return(NULL);
  }

  if (ReadLine(s, buffer, sizeof(buffer)) == 0)
  {
    sscanf(buffer, "%d %d", &reply, &size);
    if (reply != 213) size = 0;
  }
  free(query);

  /*
   * Try to retrieve the file
   */
  query = alloc_mem(strlen(filename) + strlen(format2) + 1);
  sprintf(query, format2, filename);
  reply = soak(s, query);
  if (reply == 9999)
  {    
    net_close(s);
    net_close(d);
    free(query);
    return(NULL);
  }
  free(query);

  /*
   * If the retrieve fails try to treat the file as a directory.
   * If the file is a directory then ask for a listing.
   */
  if (reply >= 400)
  {
    /*
     * Try to read the file as a directory.
     */
    query = alloc_mem(strlen(filename) + strlen(format3) + 1);
    sprintf (query, format3, filename);
    if (soak(s, query) >= 400)
    {
      net_close(d);
      net_close(s);
      free(query);
      return(NULL);
    }
    free(query);

    if (soak(s, "NLST\r\n") >= 400)
    {
      net_close(s);
      net_close(d);
      return(NULL);
    }

    isdir = 1;
  }
  else
  {
    isdir = 0;
  }

  /*
   * Read info from the FTP host
   */
  t = ReadBuffer(d, &tlen, size, size);

  net_close(d);
  net_close(s);

  if (t == NULL) return(NULL);

  /*
   * Do some processing on the information.  If it a directory do some
   * really special stuff.  If it is a file then do other stuff.  See
   * file.c for file processing.
   */
  if (t)
  {
    if (isdir)
    {
      char *ot = t;

      t = ftp_dir(hostname, (up->port == 0 ? DEFAULT_FTP_PORT:up->port),
		  filename, ot);
      free(ot);
      doc = BuildDocument(t, strlen(t), "text/html", 0, 1);
    }
    else
    {
      char *content;

      content = Ext2Content(mtlist, filename);
      if (content == NULL) content = "text/plain";
      doc = BuildDocument(t, tlen, content, 0, 1);
    }
  }

  return(doc);
}

/*
 * ftp_dir
 *
 * Process data from an ftp directory listing.  This converts the listing
 * to HTML with the proper anchors so that the user can click on each
 * entry and access that entry.
 *
 */
static char *
ftp_dir(hostname, portno, filename, t)
char *hostname;
int portno;
char *filename;
char *t;
{
  char *temp;
  char *fptr;
  char *p, *f = NULL;
  char buffer[BUFSIZ];
  char file[BUFSIZ];
  int flen = 0, lastflen = 0;
  static char *head = "<title>FTP directory %s on %s</title>\n<h1> FTP directory %s </h1>\n<ul>\n";
  static char *entry = "<li> <a href=ftp://%s:%d%s/%s> %s </a>\n";
  static char *trail = "</ul>";

  p = t;

  /*
   * First make a title line with the hostname and file name and all that
   * jazz.
   */
  temp = alloc_mem(strlen(filename) * 2 + strlen(hostname) +
			   strlen(head) + 1 + 10);
  sprintf (temp, head, filename, hostname, filename);
  flen = strlen(temp);
  f = (char *)alloc_mem(sizeof(char) * (flen + 1));
  strcpy(f, temp);
  free(temp);

  if (filename[0] == '/' && filename[1] == '\0') fptr = "";
  else fptr = filename;

  /*
   * The following lines should be directory entries.  If there are no
   * directory entries then this loop will just poop out.  There will
   * always be one iteration since we need a link back to the parent
   * directory.
   */
  while ((p = get_line(p, buffer, sizeof(buffer))) != NULL)
  {
    /*
     * First get the filename
     */
    sscanf(buffer, "%s", file);

    /*
     * Now craft a bit of HTML with an anchor for the directory entry
     * and add it to the end of the HTML buffer.  Use realloc to extend
     * the buffer and sprintf to put the formatted HTML in the newly
     * allocated space.
     */
    lastflen = flen;
    temp = alloc_mem(strlen(file) * 2 + strlen(hostname) +
			     strlen(fptr) + strlen(entry) + 10 + 1);
    sprintf (temp, entry, hostname, portno, fptr, file, file);
    flen += strlen(temp);
    f = (char *)realloc_mem(f, sizeof(char) * (flen + 1));
    strcpy(f + lastflen, temp);
    free(temp);
  }

  /*
   * Put the trailer on the end for completeness.  Heck, might as well
   * be a nice guy about it.
   */
  lastflen = flen;
  flen += strlen(trail);
  f = (char *)realloc_mem(f, sizeof(char) * (flen + 1));
  strcpy(f + lastflen, trail);

  return(f);
}
