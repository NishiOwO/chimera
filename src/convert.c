/*
 * convert.c
 *
 * Copyright (C) 1993, 1994, John Kilburg.
 *
 * See copyright.h for details.
 */
#include "copyright.h"

#include "options.h"

#include <stdio.h>
#include <fcntl.h>
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
#include "convert.h"
#include "util.h"
#include "input.h"
#include "http.h"
#include "stringdb.h"

#ifdef _NO_PROTO
static int PlainTextToHTML();
#else
static int PlainTextToHTML(char *, int, char **, int *);
#endif

static Convert converters[] =
{
  { "*", "text/plain", "text/html", NULL, PlainTextToHTML },
  { NULL, NULL, NULL, NULL, NULL },
};

/*
 * ReadConvertDB
 *
 * Reads in the convert entries in a list of files separated by colons.
 * For example,
 *
 * ~/.chimera_convert:~john/lib/convert:/local/infosys/lib/convert
 */
Convert *
ReadConvertFiles(filelist)
char *filelist;
{
  Convert *t = NULL, *c, *clist = NULL;
  char *f;
  char *filename;
  char buffer[BUFSIZ];
  char use[BUFSIZ];
  char incontent[BUFSIZ];
  char outcontent[BUFSIZ];
  char command[BUFSIZ];
  FILE *fp;

  f = filelist;
  while ((filename = mystrtok(f, ':', &f)) != NULL)
  {
    filename = FixFilename(filename);
    if (filename == NULL) continue;

    fp = fopen(filename, "r");
    if (fp == NULL) continue;
    
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
      if (buffer[0] == '#' || buffer[0] == '\n') continue;

      if (sscanf(buffer, "%s %s %s %[^\n]",
		 use, incontent, outcontent, command) == 4)
      {
	c = (Convert *)alloc_mem(sizeof(Convert));
	c->use = alloc_string(use);
	c->incontent = alloc_string(incontent);
	c->outcontent = alloc_string(outcontent);
	c->command = mystrcmp(command, "none") == 0 ?
	    NULL:alloc_string(command);
	c->func = NULL;
	c->next = NULL;
	
	if (clist) t->next = c;
	else clist = c;
	t = c;
      }
    }
    
    fclose(fp);
  }

  return(clist);
}

/*
 * GetConvert
 *
 * Returns an entry from the content file that matches the access method
 * and content-type.
 */
static Convert *
GetConvert(clist, use, incontent)
Convert *clist;
char *use;
char *incontent;
{
  Convert *c;
  int flen, elen;
  int i;

  flen = strlen(incontent);
  c = clist;
  while (c)
  {
    if (c->use[0] == '*' || mystrcmp(use, c->use) == 0)
    {
      if (c->incontent[0] == '*')
      {
	return(c);
      }
      else
      {
	elen = strlen(c->incontent);
	if (elen <= flen && mystrncmp(c->incontent, incontent, elen) == 0)
	{
	  return(c);
	}
      }
    }
    c = c->next;
  }

  for (i = 0; converters[i].use != NULL; i++)
  {
    if (converters[i].use[0] == '*' || mystrcmp(use, converters[i].use) == 0)
    {
      if (converters[i].incontent[0] == '*')
      {
	return(converters + i);
      }
      else
      {
	elen = strlen(converters[i].incontent);
	if (elen <= flen &&
	    mystrncmp(converters[i].incontent, incontent, elen) == 0)
	{
	  return(converters + i);
	}
      }
    }
  }

  return(NULL);
}

/*
 * PlainTextToHTML
 */
static int
PlainTextToHTML(t, tlen, h, hlen)
char *t;
int tlen;
char **h;
int *hlen;
{
  *hlen = tlen + 12;
  *h = alloc_mem(*hlen);
  strcpy(*h, "<plaintext>");
  strcat(*h, t);

  return(0);
}

/*
 * ConvertDocument
 *
 * Converts a document according to instructions in the 'content file'.
 * The content-type is used to identify the type of file and pick the
 * the correct entry in the content file.  The access method is also
 * used (http, ftp, cimg, ...)
 */
Document *
ConvertDocument(d, clist, altuse, path)
Document *d;
Convert *clist;
char *altuse;
char *path;
{
  Convert *c;
  char *incontent;
  char *use;
  char *data;
  int datalen;
  char *errormsg = NULL;
  Document *x;

  /* 
   * Documents without a content-type are unknown.
   */ 
  if (d->content) incontent = d->content;
  else incontent = "x-unknown/x-unknown";

  /*
   * Internal messages need this.
   */
  if (d->up == NULL) use = "none";
  else if (d->up->protocol == NULL) use = "none";
  else use = d->up->protocol;

  /*
   * Deal with transfer encodings
   */
  if (d->tencoding != NULL)
  {
    c = GetConvert(clist, d->tencoding, incontent);
    if (c != NULL && c->command != NULL)
    {
      errormsg = ReadPipe(c->command, path,
			  d->text, d->len,
			  &data, &datalen);
      if (errormsg)
      {
	x = BuildDocument(errormsg, strlen(errormsg), "text/html", 0, 0);
	x->cache = 0;
	return(x);
      }
      else
      {
	free(d->text);
	d->text = data;
	d->len = datalen;
      }
      free(d->tencoding);
      d->tencoding = NULL;
    }
  }

  /*
   * Deal with content encodings
   */ 
  if (d->encoding != NULL)
  {
    c = GetConvert(clist, d->encoding, incontent);
    if (c != NULL && c->command != NULL)
    {
      errormsg = ReadPipe(c->command, path,
			  d->text, d->len,
			  &data, &datalen);
      if (errormsg)
      {
	x = BuildDocument(errormsg, strlen(errormsg), "text/html", 0, 0);
	x->cache = 0;
	return(x);
      }
      else
      {
	free(d->text);
	d->text = data;
	d->len = datalen;
      }
      free(d->encoding);
      d->encoding = NULL;
    }
  }

  /*
   * Figure out how to convert the document.
   */
  if (altuse != NULL) c = GetConvert(clist, altuse, incontent);
  else c = GetConvert(clist, use, incontent);
  if (c != NULL)
  {
    if (c->func != NULL)
    {
      if ((c->func)(d->text, d->len, &data, &datalen) != 0)
      {
	errormsg = GetFromStringDB("convfail");
      }
      else errormsg = NULL;
    }
    else if (c->command != NULL)
    {
      errormsg = ReadPipe(c->command, path,
			  d->text, d->len,
			  &data, &datalen);
    }
    else return(d);

    if (errormsg)
    {
      x = BuildDocument(errormsg, strlen(errormsg), "text/html", 0, 0);
      x->cache = 0;
      return(x);
    }
    else
    {
      d->ptext = data;
      d->plen = datalen;
      if (d->pcontent != NULL) free(d->pcontent);
      d->pcontent = alloc_string(c->outcontent);
    }
  }

  return(d);
}
