/*
 * document.c
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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "url.h"
#include "mime.h"
#include "document.h"
#include "cache.h"
#include "http.h"
#include "gopher.h"
#include "ftp.h"
#include "local.h"
#include "util.h"
#include "input.h"
#include "stringdb.h"

/*
 * Tells which functions to call for each access.
 */
struct name
{
  char *name;
  Document *(*func)();
  char *proxy;
};

static struct name protocols[] = 
{
  { "ftp", ftp, "ftp_proxy" },
  { "http", http, "http_proxy" },
  { "gopher", gopherplain, "gopher_proxy" },
  { "gopherp", gopherplus, "gopher_proxy" },
  { "file", file, NULL },
  { "telnet", telnet, NULL },
  { "tn3270", tn3270, NULL },
  { "wais", NULL, "wais_proxy" },
  { "news", NULL, "news_proxy" },
  { "nntp", NULL, "nntp_proxy" },
  { "urn", NULL, "urn_proxy" },
  { NULL, NULL, NULL },
};

/*
 * ReadProtocolFiles
 *
 * Read a list of protocol files and return a list.
 */
Protocol *
ReadProtocolFiles(filelist)
char *filelist;
{
  FILE *fp;
  char buffer[BUFSIZ];
  char proto[BUFSIZ];
  char command[BUFSIZ];
  Protocol *p, *plist = NULL, *t = NULL;
  char *f;
  char *filename;

  f = filelist;
  while ((filename = mystrtok(f, ':', &f)) != NULL)
  {
    filename = FixFilename(filename);
    if (filename == NULL) continue;

    fp = fopen(filename, "r");
    if (fp == NULL) continue;
    
    while (fgets(buffer, sizeof(buffer), fp))
    {
      if (buffer[0] == '#' || buffer[0] == '\n') continue;

      if (sscanf(buffer, "%s %[^\n]", proto, command) == 2)
      {
	p = (Protocol *)alloc_mem(sizeof(Protocol));
	p->proto = alloc_string(proto);
	p->command = alloc_string(command);
	p->next = NULL;
	if (t != NULL) t->next = p;
	else plist = p;
	t = p;
      }
    }

    fclose(fp);
  }

  return(plist);
}

/*
 * ExternalProtocol
 *
 * This function handles executing other programs to talk to services
 * to get information.  Allows new protocols without changing chimera.
 */
Document *
ExternalProtocol(up, plist)
URLParts *up;
Protocol *plist;
{
  Document *d;
  Protocol *p;
  int pipe[2];

  p = plist;
  while (p)
  {
    if (mystrcmp(up->protocol, p->proto) == 0)
    {
      if (PipeCommand(p->command, pipe, 1) == -1)
      {
	char *msg = GetFromStringDB("nopipe");
	char *t;
	int tlen;

	tlen = strlen(msg) + strlen(p->command);
	t = alloc_mem(tlen + 1);
	strcpy(t, msg);
	strcat(t, p->command);

        d = BuildDocument(t, tlen, "text/html", 0, 0);
      }
      else
      {
	if (http_request(pipe[0], up, 2, 0) == -1)
	{
	  close(pipe[0]);
	  close(pipe[1]);

	  d = NULL;
	}
	else
	{
	  d = ParseHTTPResponse(pipe[1]);

	  close(pipe[1]);
	  close(pipe[0]);
	}
      }

      return(d);
    }

    p = p->next;
  }

  return(NULL);
}

/*
 * ProxyProtocol
 *
 * Handles proxy protocols.
 */
Document *
ProxyProtocol(up, mtlist, reload)
URLParts *up;
MIMEType *mtlist;
int reload;
{
  int i, plen, hlen;
  Document *d = NULL;
  URLParts *rup;
  char *pproxy = NULL;
  char *no_proxy;
  char *cp, *ph;

  if (!IsAbsoluteURL(up))
  {
    char *msg = GetFromStringDB("absurl");

    return(BuildDocument(msg, strlen(msg), "text/html", 1, 0));
  }

  for (i = 0; protocols[i].name != NULL; i++)
  {
    if (mystrcmp(up->protocol, protocols[i].name) == 0)
    {
      if (protocols[i].proxy != NULL)
      {
	if ((pproxy = getenv(protocols[i].proxy)) == NULL)
	{
	  pproxy = NGetFromStringDB(protocols[i].proxy);
	  if (pproxy == NULL)
	  {
	    if ((pproxy = getenv("all_proxy")) == NULL)
	    {
	      pproxy = NGetFromStringDB("all_proxy");
	      if (pproxy == NULL) continue;
	    }
	  }
	}
	pproxy = alloc_string(pproxy);

	if ((no_proxy = getenv("no_proxy")) != NULL)
	{
	  hlen = strlen(up->hostname);
	  cp = no_proxy;
	  while ((ph = mystrtok(cp, ',', &cp)) != NULL)
	  {
	    plen = strlen(ph);
	    if (plen <= hlen &&
		mystrncmp(ph, up->hostname + (hlen - plen), plen) == 0)
	    {
	      free(pproxy);
	      return(NULL);
	    }
	  }
	}

	if ((rup = ParseURL(pproxy)) != NULL)
	{	
	  char *msg = GetFromStringDB("absrurl");

	  if (IsAbsoluteURL(rup)) d = http_proxy(rup, up, mtlist, reload);
	  else d = BuildDocument(msg, strlen(msg), "text/html", 1, 0);
	  
	  DestroyURLParts(rup);
	}

	free(pproxy);
      }

      break;
    }
  }

  return(d);
}

/*
 * LocalProtocol
 *
 * Handle protocols that have been implemented in the chimera source.
 */
static Document *
LocalProtocol(up, mtlist)
URLParts *up;
MIMEType *mtlist;
{
  int i;
  Document *d = NULL;

  if (!IsAbsoluteURL(up))
  {
    char *msg = GetFromStringDB("absurl");
 
    return(BuildDocument(msg, strlen(msg), "text/html", 1, 0));
  }

  for (i = 0; protocols[i].name != NULL; i++)
  {
    if (protocols[i].func != NULL &&
	strcmp(protocols[i].name, up->protocol) == 0)
    {
      d = (protocols[i].func)(up, mtlist);
      break;
    }
  }

  return(d);
}

/*
 * CreateDocument
 *
 * Allocates the structure used for documents.  Could do other
 * init stuff, too.  This function should always be called to create
 * a new document.  It provides initialization for the document which
 * is good if an old part of the program doesn't know about new
 * document attributes.
 */
Document *
CreateDocument()
{
  Document *d;

  d = (Document *)alloc_mem(sizeof(Document));
  d->up = NULL;

  d->text = NULL;
  d->len = 0;
  d->content = NULL;

  d->ptext = NULL;
  d->plen = 0;
  d->pcontent = NULL;

  d->from_cache = 0;
  d->cache = 1;

  d->encoding = NULL;
  d->tencoding = NULL;
  d->location = NULL;

  d->status = 0;

  d->auth_realm = NULL;
  d->auth_type = NULL;

  d->mflist = NULL;

  d->nothing = 0;

  d->expires = 0;

  return(d);
}

/*
 * DestroyDocument
 */
void
DestroyDocument(d)
Document *d;
{
  MIMEField *m, *t;

  if (d->up) DestroyURLParts(d->up);

  if (d->text) free(d->text);
  if (d->content) free(d->content);

  if (d->ptext) free(d->ptext);
  if (d->pcontent) free(d->pcontent);

  if (d->encoding) free(d->encoding);
  if (d->tencoding) free(d->tencoding);
  if (d->location) free(d->location);

  if (d->auth_realm) free(d->auth_realm);
  if (d->auth_type) free(d->auth_type);

  for (m = d->mflist; m; )
  {
    t = m;
    m = m->next;
    DestroyMIMEField(t);
  }

  free(d);

  return;
}

/*
 * BuildDocument
 *
 * Do some of the grunge work to make a document.
 */
Document *
BuildDocument(text, len, content, doalloc, cache)
char *text;
int len;
char *content;
int doalloc;
int cache;
{
  Document *d;

  d = CreateDocument();
  if (doalloc && text != NULL)
  {
    d->text = alloc_mem(len + 1);
    memcpy(d->text, text, len);
    d->text[len] = '\0';
  }
  else
  {
    d->text = text;
  }
  d->len = (text == NULL ? 0:len);

  if (content != NULL) d->content = alloc_string(content);
  else d->content = NULL;

  d->cache = cache;

  return(d);
}

/*
 * LoadDocument
 */
Document *
LoadDocument(up, plist, mtlist, reload)
URLParts *up;
Protocol *plist;
MIMEType *mtlist;
int reload;
{
  Document *d;

  /*
   * Call a function to grab the URL.
   */
  if ((d = ExternalProtocol(up, plist)) != NULL) ;
  else if ((d = ProxyProtocol(up, reload)) != NULL) ;
  else d = LocalProtocol(up, mtlist);

  /*
   * Now figure out what we've got and do something with it.
   */
  if (d == NULL)
  {
    char *msg = GetFromStringDB("noload");

    d = BuildDocument(msg, strlen(msg), "text/html", 1, 0);
  }
  else
  {
    if (d->location != NULL) /* alternate location given ? */
    {
      char *url;

      url = MakeURL(up);
      if (strcmp(url, d->location) == 0) /* document loop ? */
      {
	char *msg = GetFromStringDB("loops");

	DestroyDocument(d);
	d = BuildDocument(msg, strlen(msg), "text/html", 1, 0);
      }
      else /* document doesn't loop (well at least not directly) */
      {
	URLParts *tup = NULL;

	tup = ParseURL(d->location);
	if (tup != NULL && IsAbsoluteURL(tup))
	{
	  if (up->username != NULL && up->password != NULL &&
	      up->auth_type != NULL)
	  {
	    tup->username = alloc_string(up->username);
	    tup->password = alloc_string(up->password);
	    tup->auth_type = alloc_string(up->auth_type);
	  }
	  DestroyDocument(d);
	  if (reload == 0) d = ReadCache(tup);
	  if (reload == 1 || d == NULL)
	      d = LoadDocument(tup, plist, mtlist, reload);
	}
	else /* bad relocation URL given */
	{
	  char *msg = GetFromStringDB("invlocation");
	  
	  DestroyDocument(d);
	  d = BuildDocument(msg, strlen(msg), "text/html", 1, 0);
	}

	if (tup != NULL) DestroyURLParts(tup);
      }
    }
    else if (d->text == NULL)
    {
      char *msg = GetFromStringDB("noload");

      DestroyDocument(d);
      d = BuildDocument(msg, strlen(msg), "text/html", 1, 0);
    }
    else /* if the document seems to be OK then copy the URL stuff over */
    {
      d->up = DupURLParts(up);
    }
  }

  return(d);
}

/*
 * ParseMIMEField
 *
 * Picks out the fields that Chimera document's know about for speed.
 */
void
ParseMIMEField(d, m)
Document *d;
MIMEField *m;
{
  char *field, *data;

  if (m == NULL || m->name == NULL || m->value == NULL) return;

  field = m->name;
  data = m->value;

  /*
   * Pick out the MIME fields that we understand and do stuff with
   * the data.
   */
  if (mystrncmp("content-type", field, 12) == 0)
  {
    int i;
    d->content = alloc_string(data);
    for(i = 0; d->content[i] != 0; i++){
	    if(d->content[i] == ';'){
		    d->content[i] = 0;
		    break;
	    }
    }
  }
  else if (mystrncmp("x-pcontent-type", field, 15) == 0)
  {
    d->pcontent = alloc_string(data);
  }
  else if (mystrncmp("content-length", field, 14) == 0)
  {
    d->len = atoi(data);
  }
  else if (mystrncmp("x-pcontent-length", field, 17) == 0)
  {
    d->plen = atoi(data);
  }
  else if (mystrncmp("x-url", field, 5) == 0)
  {
    if (d->up == NULL) d->up = ParseURL(data);
  }
  else if (mystrncmp("content-transfer-encoding", field, 8) == 0)
  {
    d->tencoding = alloc_string(data);
  }
  else if (mystrncmp("content-encoding", field, 8) == 0)
  {
    d->encoding = alloc_string(data);
  }
  else if (mystrncmp("expires", field, 7) == 0)
  {
    d->expires = ParseExpiresDate(data);
  }
  else if (mystrncmp("location", field, 8) == 0 ||
	   mystrncmp("uri", field, 3) == 0)
  {
    d->location = alloc_string(data);
  } 
  else if (mystrncmp("pragma", field, 6) == 0)
  {
    char *list;
    char *option;
    
    list = data;
    while ((option = mystrtok(list, ' ', &list)) != NULL)
    {
      if (strcmp(option, "no-cache") == 0) d->cache = 0;
      else if (strcmp(option, "nothing") == 0) d->nothing = 1;
    }
  }
  else if (mystrncmp("www-authenticate", field, 16) == 0)
  {
    char *list;
    char *t;
    
#ifndef CACHE_AUTH
    d->cache = 0;
#endif
    
    list = data;
    if ((t = mystrtok(list, ' ', &list)) != NULL)
    {
      d->auth_type = alloc_string(t);
    }
    
    if ((t = mystrtok(list, '=', &list)) != NULL)
    {
      if (list != NULL) d->auth_realm = alloc_string(list);
    }
  }

  return;
}
