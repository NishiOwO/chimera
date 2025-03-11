/*
 * gopher.c
 *
 * Copyright (C) 1993, 1994, John Kilburg.
 *
 * See copyright.h for details.
 */
#include "copyright.h"

#include "options.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "url.h"
#include "mime.h"
#include "document.h"
#include "gopher.h"
#include "net.h"
#include "util.h"
#include "input.h"
#include "stringdb.h"

#define DEFAULT_GOPHER_PORT 70

#ifndef _NO_PROTO
static char *gopher_to_html(char *, int, char *, char *, char *);
extern int DisplayTransferStatus(int);
#else
static char *gopher_to_html();
extern int DisplayTransferStatus();
#endif

/*
 * gopher_main
 *
 * Request a document from a gopher/gopher+ server.  Converts directories
 * into HTML files so that subdirectories can be accessed.
 *
 * This code needs a rewrite.  I figured out basically how gopher
 * works and then figured the rest out by telnet'ing to a gopher
 * port and trying stuff out.  This is not good.  Also, gopher/gopher+
 * URLs are not handled very well.  I added my own bogus gopher+
 * access method.
 *
 */
Document *
gopher_main(up, mtlist, plus)
URLParts *up;
MIMEType *mtlist;
int plus;
{
  char *t = NULL;
  char buffer[BUFSIZ];
  char bcontent[BUFSIZ];
  char *content = NULL;
  int s;
  int tlen;
  int filelen;
  int portno;
  int type;
  char *ot;
  char *filename;
  Document *d;
  char *tfilename;
  char *hostname;

  /*
   * The gopher server WANTS spaces.
   */ 
  filename = UnescapeURL(up->filename);
  if (filename == NULL) return(NULL);

  if (filename[0] == '/' && filename[1] == '\0')
  {
    free(filename);
    filename = alloc_string("");
    type = '1';
  }
  else if (filename[0] == '/')
  {
    type = filename[1];
    tfilename = alloc_string(filename + 2);
    free(filename);
    filename = tfilename;
  }
  else type = filename[0];

  filelen = strlen(filename);

  /*
   * If the type is index then check to see if the user has entered
   * a search string.  If not then return and ask for a query string
   * using the <isindex> command.
   */
  switch (type)
  {
    case '7':
      if (up->request_data == NULL)
      {
	char *msg = GetFromStringDB("g<index>");

	d = BuildDocument(msg, strlen(msg), "text/html", 1, 0);
	free(filename);
	d->cache = 0;
	return(d);
      }
      else
      {
        tfilename = alloc_mem(filelen + strlen(up->request_data) + 4);
	strcpy(tfilename, filename);
        strcat(tfilename, "\t");
        strcat(tfilename, up->request_data);
        strcat(tfilename, "\r\n");
      }
      break;

    default:
      tfilename = alloc_mem(filelen + 3);
      strcpy(tfilename, filename);
      strcat(tfilename, "\r\n");
  }

  if (up->port == 0) portno = DEFAULT_GOPHER_PORT;
  else portno = up->port;

  hostname = up->hostname != NULL ? up->hostname:"gopher";
  s = net_open(hostname, portno);
  if (s < 0)
  {
    free(tfilename);
    free(filename);
    return(NULL);
  }

  if (WriteBuffer(s, tfilename, strlen(tfilename)) < 0)
  {
    free(tfilename);
    free(filename);
    net_close(s);
    return(NULL);
  }

  free(tfilename);

  /*
   * Read info from the Gopher host
   */
  t = ReadBuffer(s, &tlen, 0, 0);
  if (t == NULL)
  {
    free(filename);
    return(NULL);
  }

  /*
   * Now reconnect and grab the gopher+ information.  This is really
   * kind of hokey but then so is gopher+.
   */
  if (plus &&
      type != '1' &&
      type != '8' &&
      type != '7' &&
      type != 'T')
  {
    s = net_open(hostname, portno);
    if (s >= 0)
    {
      /*
       * Tack on "\t!\r\n"
       */
      tfilename = alloc_mem(filelen + 5);
      strcpy(tfilename, filename);
      strcat(tfilename, "\t!\r\n");

      if (WriteBuffer(s, tfilename, filelen + 5) < 0)
      {
	net_close(s);
	free(tfilename);
	free(filename);
	free(t);
	return(NULL);
      }

      free(tfilename);

      /*
       * Grab up the document information.
       */
      while (ReadLine(s, buffer, sizeof(buffer)) == 0)
      {
	if (strncmp("+VIEWS:", buffer, 7) == 0)
	{
	  if (ReadLine(s, bcontent, sizeof(bcontent)) == 0)
	  {
	    content = bcontent + 1;
	    break;
	  }
	}
      }

      net_close(s);
    }
  }

  /*
   * Do some processing on the information.  If it a directory do some
   * really special stuff.  If it is a file then do other stuff.  See
   * file.c for file processing.
   */
  if (t)
  {
    if (t[tlen - 3] == '.' && t[tlen - 2] == '\r' && t[tlen - 1] == '\n')
    {
      tlen -= 3;
      t[tlen] = '\0';
    }
    else
    {
      t[tlen] = '\0';
    }

    if (t[0] == '\0')
    {
      char *msg = GetFromStringDB("nodata");

      d = BuildDocument(msg, strlen(msg), "text/html", 1, 0);
      free(filename);

      return(d);
    }

    switch (type)
    {
      case '8': /* telnet.  this is scary */
        fprintf (stderr, "Gopher telnet bug.\n");
	break;

      case 'T': /* TN3270 */
	fprintf (stderr, "Gopher TN3270 bug.\n");
	break;

      /*
       * Convert these guys to HTML.
       */
      case '1': /* directories */
	ot = t;
        t = gopher_to_html(hostname, portno, filename,
			   up->request_data, ot);
	free(ot);
	d = BuildDocument(t, strlen(t), "text/html", 0, 1);
	break;

      case '7': /* indexes */
	ot = t;
        t = gopher_to_html(hostname, portno, filename,
			   up->request_data, ot);
	free(ot);
	d = BuildDocument(t, strlen(t), "text/html", 0, 0);
	d->cache = 0;
	break;

      /*
       * Everything else
       */
      default:
	/*
	 * Check for an error.  Don't mess with it further.
	 */
	if (plus && t[0] == '-') free(t);
	else
	{
	  /*
	   * probably should not hardcode this.  oh well.  gopher sucks
	   * and will hopefully go away soon anyways.  hopefully everyone
	   * has managed to start running a gopher+ server so this shouldn't
	   * be needed.
	   */
	  if (content == NULL)
	  {
	    content = Ext2Content(mtlist, filename);
	    if (content == NULL)
	    {
	      if (type == '0') content = "text/plain";
	      else if (type == '1') content = NULL;
	      else if (type == '2') content = "text/plain";
	      else if (type == '3') content = NULL;
	      else if (type == '4') content = "application/mac-binhex40";
	      else if (type == '5') content = NULL;
	      else if (type == '7') content = "text/html";
	      else if (type == '9') content = NULL;
	      else if (type == 's') content = "audio/basic";
	      else if (type == 'e') content = NULL;
	      else if (type == 'I') content = "image/";
	      else if (type == 'M') content = "multipart/mixed";
	      else if (type == 'c') content = NULL;
	      else if (type == 'g') content = "image/gif";
	      else if (type == 'h') content = "text/html";
	      else content = "text/plain";

	      d = BuildDocument(t, tlen, content, 0, 1);
	    }
	    else
	    {
	      d = BuildDocument(t, tlen, content, 0, 1);
	    }
	  }
	  else
	  {
	    d = BuildDocument(t, tlen, content, 0, 1);
	  }
	}
	break;
    }
  }

  free(filename);

  return(d);
}

/*
 * gopherplus
 *
 * This is the gopher+ frontend to gopher_main
 */
Document *
gopherplus(up, mtlist)
URLParts *up;
MIMEType *mtlist;
{
  return(gopher_main(up, mtlist, 1));
}

/*
 * gopherplain
 *
 * The plain gopher frontend to gopher_main
 */
Document *
gopherplain(up, mtlist)
URLParts *up;
MIMEType *mtlist;
{
  return(gopher_main(up, mtlist, 0));
}

#define GOPHER_DISPLAY 0
#define GOPHER_SELECTOR 1
#define GOPHER_HOST 2
#define GOPHER_PORT 3
#define GOPHER_EXTRA 4
#define GOPHER_FIELD_COUNT 5

/*
 * gopher_to_html
 *
 * Converts gopher directory information into HTML.
 *
 * Much fixing provided by R. Stewart Ellis.
 */
static char *
gopher_to_html(hostname, portno, filename, ext, t)
char *hostname;
int portno;
char *filename;
char *ext;
char *t;
{
  /*
   * What's with the big strings?!?
   */
  char gf[GOPHER_FIELD_COUNT][BUFSIZ]; /* fields in gopher line */
  char buffer[BUFSIZ];
  char *p;
  char *f = NULL;
  char *form;
  char *junk = NULL;
  char *stuff;
  char *cp;
  char gtype;
  int i;
  int gfno;
  int flen;
  static char *header = "<title>Gopher directory %s on %s</title><h1>Gopher directory %s on %s</h1><ul>\n";
  static char *header2 = "<title>Gopher search for %s</title><h1>Gopher search  for %s</h1><ul>\n";
  static char *format  = "<li><a href=gopher://%s:%s/%c%s>%s</a>\n";
  static char *eformat = "<li><a href=gopherp://%s:%s/%c%s>%s</a>\n";
  static char *trailer = "</ul>";
  static char *telform = "<li><a href=telnet://%s@%s:%s/>Telnet to %s</a>\n";
  static char *tnform  = "<li><a href=tn3270://%s@%s:%s/>TN3270 to %s</a>\n"; 

  switch (filename[0])
  {
    case '7':
      f = alloc_mem(strlen(header2) + strlen(ext) * 2 + 1);
      sprintf (f, header2, ext, ext);
      break;

    default:
      if (filename[0] == '\0') stuff = "/";
      else stuff = filename;

      f = alloc_mem(strlen(header) +
			   strlen(stuff) * 2 +
			   strlen(hostname) * 2 + 1);
      sprintf (f, header, stuff, hostname, stuff, hostname);
      break;
  }
  flen = strlen(f);

  /*
   * Now grab up each gopher line and convert it to an HTML menu item.
   */
  p = t;
  while ((p = get_line(p, buffer, sizeof(buffer))) != NULL)
  {
    /*
     * Make sure that the fields are blank in case someone leaves a
     * field out.
     */
    for (gfno = 0; gfno < GOPHER_FIELD_COUNT; gfno++)
    {
      gf[gfno][0] = '\0';
    }

    /*
     * Separate the TAB separated fields.
     */
    gtype = buffer[0];
    for (i = 0, cp = buffer + 1, gfno = 0;
	 *cp && gfno < GOPHER_FIELD_COUNT;
	 cp++)
    {
      if (*cp == '\t')
      {
	gf[gfno][i] = '\0';
	i = 0;
	gfno++;
      }
      else
      {
	gf[gfno][i++] = *cp;
      }
    }
    gf[gfno][i] = '\0';

    /*
     * Check for empty fields.
     */
    if (gf[GOPHER_HOST][0] == '\0') strcpy(gf[GOPHER_HOST], hostname);

    /*
     * Select the proper form.
     */
    switch (gtype)
    {
      case '8':
        form = telform;
	stuff = gf[GOPHER_SELECTOR];
        break;

      case 'T':
	form = tnform;
	stuff = gf[GOPHER_SELECTOR];
	break;

      default:
	if (gtype != '7' &&
	    gf[GOPHER_EXTRA][0] == '+' && gf[GOPHER_SELECTOR][0] != '1')
	{
	  form = eformat;
	}
	else
	{
	  form = format;
	}
	/*
	 * Make sure the weird characters are escaped.
	 */
	junk = EscapeURL((unsigned char *)gf[GOPHER_SELECTOR], 0);
	if (junk != NULL) stuff = junk;
	else stuff = gf[GOPHER_SELECTOR];
    }
    
    /*
     * Now make a bit of HTML for the gopher line.
     */
    flen += strlen(form) + strlen(gf[GOPHER_HOST]) + strlen(stuff) +
	strlen(gf[GOPHER_DISPLAY]) + strlen(gf[GOPHER_PORT]) + 2;
    
    f = (char *)realloc_mem(f, flen);

    if (gtype != '8' && gtype != 'T')
    {
      sprintf(f + strlen(f), form,
	      gf[GOPHER_HOST],
	      gf[GOPHER_PORT],
	      gtype,
	      stuff,
	      gf[GOPHER_DISPLAY]);
    }
    else
    {
      sprintf(f + strlen(f), form,
              stuff,
	      gf[GOPHER_HOST],
	      gf[GOPHER_PORT],
	      gf[GOPHER_DISPLAY]);
    }
    
    if (junk)
    {
      free(junk);
      junk = NULL;
    }
  }

  /*
   * Put a trailer on the HTML document.
   */ 
  flen += strlen(trailer) + 1;
  f = (char *)realloc_mem(f, flen);
  strcpy(f + strlen(f), trailer);

  return(f);
}
