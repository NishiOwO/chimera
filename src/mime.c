/*
 * mime.c
 *
 * Copyright (C) 1994, John Kilburg.
 *
 * See copyright.h for details.
 */
#include "copyright.h"

#include "options.h"

#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "url.h"
#include "mime.h"
#include "document.h"
#include "util.h"

static MIMEType mhlist[] =
{
  { "text/html", "html" },
  { "text/html", "htm" },
  { "text/plain", "txt" },
  { "image/gif", "gif" },
  { "image/xbm", "xbm" },
  { "image/x-xpimap", "xpm" },
  { "image/x-portable-anymap", "pnm" },
  { "image/x-portable-bitmap", "pbm" },
  { "image/x-portable-graymap", "pgm" },
  { "image/x-portable-pixmap", "ppm" },
  { "image/jpeg", "jpg" },
  { "image/jpeg", "jpeg" },
  { "image/tiff", "tiff" },
  { "image/png", "png" },
  { "image/x-fits", "fit" },
  { "image/x-fits", "fits" },
  { "image/x-fits", "fts" },
  { "audio/basic", "au" },
  { "audio/basic", "snd" },
  { "text/x-compress-html", "html.Z" },
  { "text/x-gzip-html", "html.gz" },
  { "application/postscript", "ps" },
  { "application/x-dvi", "dvi" },
  { "application/x-gzip", "gz" },
  { "application/x-compress", "Z" },
  { "application/x-tar", "tar" },
  { "video/mpeg", "mpeg" },
  { "video/mpeg", "mpg" },
  { NULL, NULL },
};

/*
 * ReadMimeTypes
 */
MIMEType *
ReadMIMETypeFiles(filelist)
char *filelist;
{
  FILE *fp;
  char *f;
  char *cp;
  char buffer[256];
  char content[256];
  char exts[256];
  MIMEType *t = NULL, *c, *mlist = NULL;
  char *filename, *e;

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
      
      if (sscanf(buffer, "%s %[^\n]", content, exts) == 2)
      {
	cp = exts;
	while ((e = mystrtok(cp, ' ', &cp)) != NULL)
	{
	  c = (MIMEType *)alloc_mem(sizeof(MIMEType));
	  c->content = alloc_string(content);
	  c->ext = alloc_mem(strlen(e) + 2);
	  strcpy(c->ext, ".");
	  strcat(c->ext, e);
	  c->next = NULL;
	  if (t != NULL) t->next = c;
	  else mlist = c;
	  t = c;
	}
      }
    }
    
    fclose(fp);
  }

  return(mlist);
}

/*
 * Ext2Content
 */
char *
Ext2Content(mlist, ext)
MIMEType *mlist;
char *ext;
{
  int elen, flen;
  MIMEType *c;
  int i;

  flen = strlen(ext);
  for (c = mlist; c; c = c->next)
  {
    elen = strlen(c->ext);
    if (elen > 0)
    {
      if (elen <= flen &&
	  mystrncmp(ext + flen - elen, c->ext, elen) == 0)
      {
	return(c->content);
      }
    }
  }

  for (i = 0; mhlist[i].content != NULL; i++)
  {
    elen = strlen(mhlist[i].ext);
    if (elen > 0)
    {
      if (elen <= flen &&
	  mystrncmp(ext + flen - elen, mhlist[i].ext, elen) == 0)
      {
	return(mhlist[i].content);
      }
    }
  }

  return(NULL);
}

static MailCap mclist[] =
{
  { "application/x-telnet", "xterm -e dotelnet %s" },
  { "application/x-tn3270", "xterm -e do3270 %s" },
  { "text/x-dvi", "xdvi %s" },
  { "application/postscript", "ghostview %s" },
  { "application/x-dvi", "xdvi %s" },
  { "image/x-fits", "saoimage %s" },
  { "image/*", "xv %s" },
  { "video/mpeg", "mpeg_play -loop %s" },
  { "audio/basic", "playaudio %s" },
  { NULL, NULL },
};

/*
 * ReadMailCapFiles
 *
 * Read a list of mailcap files.
 */
MailCap *
ReadMailCapFiles(filelist)
char *filelist;
{
  FILE *fp;
  char *f;
  char buffer[256];
  char ctype[256];
  char command[256];
  char junk[256];
  MailCap *c, *t = NULL, *caplist = NULL;
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
      
      if (sscanf(buffer, "%[^;]%[;]%[^;\n]", ctype, junk, command) == 3)
      {
	c = (MailCap *)alloc_mem(sizeof(MailCap));
	c->ctype = alloc_string(ctype);
	c->command = alloc_string(command);
	c->next = NULL;
	if (t != NULL) t->next = c;
	else caplist = c;
	t = c;
      }
    }

    fclose(fp);
  }

  return(caplist);
}


/*
 * start_viewer
 */
static int
start_viewer(path, command, filename)
char *path;
char *command;
char *filename;
{
  char *cmdline;

  cmdline = CreateCommandLine(command, path, filename);

  if (fork() == 0)
  {
    signal(SIGPIPE, SIG_DFL);
    system(cmdline);
    unlink(filename);
    exit(0);
  }
  free(cmdline);

  return(0);
}

/*
 * FindViewer
 */
static int
FindViewer(c, content, path, filename)
MailCap *c;
char *content;
char *path;
char *filename;
{
  int len, dlen;

  len = strlen(c->ctype);
  if (c->ctype[0] != '*')
  {
    if (c->ctype[len - 1] == '*') len--;
    if (mystrncmp(c->ctype, content, len) == 0)
    {
      return(start_viewer(path, c->command, filename));
    }
  }
  else
  {
    dlen = strlen(content);
    if (dlen >= len)
    {
      if (mystrncmp(c->ctype + 1, content + (dlen - len), len - 1) == 0)
      {
	return(start_viewer(path, c->command, filename));
      }
    }
  }

  return(-1);
}

/*
 * DisplayExternal
 *
 * Cranks up a viewer appropriate for the documents content-type.
 *
 * Returns 0 for success, -1 for failure.  Success doesn't neccesarily
 * mean that the viewer actually worked.
 */
int
DisplayExternal(filename, content, path, caplist)
char *filename;
char *content;
char *path;
MailCap *caplist;
{
  MailCap *c;
  int rval;
  int i;

  for (c = caplist; c; c = c->next)
  {
    if ((rval = FindViewer(c, content, path, filename)) != -1) return(rval);
  }

  for (i = 0; mclist[i].ctype != NULL; i++)
  {
    if ((rval = FindViewer(mclist + i, content, path, filename)) != -1)
    {
      return(rval);
    }
  }

  return(-1);
}

/*
 * ParseExpiresDate
 */
time_t
ParseExpiresDate(s)
char *s;
{
  int i;
  time_t expires;
  struct tm tm;
  char buffer[BUFSIZ];
  char month[4], zone[4];
  static char *ms[] =
  { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct",
    "Nov", "Dec" };

  memset(&tm, 0, sizeof(tm));
  if (sscanf(s, "%s %d %s %d %d:%d:%d %s",
	     buffer, &tm.tm_mday, month, &tm.tm_year,
	     &tm.tm_hour, &tm.tm_min, &tm.tm_sec, zone) == 8)
  {
    if (tm.tm_year > 1900) tm.tm_year -= 1900;
    for (i = 0; i < sizeof(ms) / sizeof(ms[0]); i++)
    {
      if (strcmp(month, ms[i]) == 0)
      {
	expires = mktime(&tm);
	break;
      }
    }
  }
  
  return(expires);
}

/*
 * DestroyMIMEField
 */
void
DestroyMIMEField(m)
MIMEField *m;
{
  if (m->name != NULL) free(m->name);
  if (m->value != NULL) free(m->value);
  free(m);

  return;
}

/*
 * CreateMIMEField
 *
 * This is a disaster when it comes to parsing MIME stuff.
 */
MIMEField *
CreateMIMEField(line)
char *line;
{
  char field[BUFSIZ];
  char junk[BUFSIZ];
  char data[BUFSIZ];
  MIMEField *m;

  if (sscanf(line, "%[^:] %[:] %[^\r\n]", field, junk, data) != 3)
  {
    return(NULL);
  }

  m = (MIMEField *)alloc_mem(sizeof(MIMEField));
  m->next = NULL;
  m->name = alloc_string(field);
  m->value = alloc_string(data);

  return(m);
}
