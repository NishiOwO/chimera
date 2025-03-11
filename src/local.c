/*
 * local.c
 *
 * Copyright (C) 1993, 1994, John Kilburg.
 *
 * See copyright.h for details.
 */
#include "copyright.h"

#include "options.h"

#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#if defined(SYSV) || defined(SVR4) || defined(__arm)
#include <dirent.h>
#define DIRSTUFF struct dirent
#else
#include <sys/dir.h>
#define DIRSTUFF struct direct
#endif

/* Jim Rees fix */
#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif

#define DEFAULT_TELNET_PORT 23

#include "url.h"
#include "mime.h"
#include "document.h"
#include "local.h"
#include "ftp.h"
#include "input.h"
#include "util.h"
#include "net.h"
#include "stringdb.h"

/*
 * LoadFile
 *
 * Load the contents of a file into memory.
 */
static Document *
LoadFile(filename, size, mtlist)
char *filename;
int size;
MIMEType *mtlist;
{
  FILE *fp;
  char *b;
  char *content;
  Document *d;

  fp = fopen(filename, "r");
  if (fp == NULL) return(NULL);

  b = (char *)alloc_mem(size + 1);
  if (fread(b, 1, size, fp) < size)
  {
    free(b);
    fclose(fp);
    return(NULL);
  }
  b[size] = '\0';

  fclose(fp);

  content = Ext2Content(mtlist, filename);
  if (content == NULL) content = "text/plain";
  d = BuildDocument(b, size, content, 0, 0);

  return(d);
}

/*
 * LoadDir
 *
 * Reads a local directory and converts it into HTML so the user can
 * navigate the local filesystem.
 */
static Document *
LoadDir(filename)
char *filename;
{
  DIR *dp;
  DIRSTUFF *de;
  int flen;
  int filelen;
  int formlen;
  char *f;
  static char *format = "<li> <a href=file:%s/%s> %s </a>\n";
  static char *rformat = "<li> <a href=file:%s%s> %s </a>\n";
  static char *header = "<title>Local Directory %s</title>\n<h1>Local Directory %s</h1>\n<ul>\n";

  filelen = strlen(filename);
  formlen = strlen(format);

  flen = strlen(header) + filelen * 2 + 1;
  f = (char *)alloc_mem(flen);
  sprintf (f, header, filename, filename);

  dp = opendir(filename);
  if (dp == NULL) return(NULL);

  while ((de = readdir(dp)) != NULL)
  {
    if (de->d_name[0] != '.')
    {
      flen += formlen + strlen(de->d_name) * 2 + filelen + 1;
      
      f = (char *)realloc_mem(f, flen);

      if (filename[strlen(filename) - 1] == '/')
      {
	sprintf(f + strlen(f), rformat, filename, de->d_name, de->d_name);
      }
      else
      {
	sprintf(f + strlen(f), format, filename, de->d_name, de->d_name);
      }
    }
  }

  closedir(dp);

  return(BuildDocument(f, strlen(f), "text/html", 0, 0));
}

/*
 * file
 *
 * Grabs a local file or to grab the file using FTP (if the hostname
 * is not localhost).
 */
Document *
file(up, mtlist)
URLParts *up;
MIMEType *mtlist;
{
  char *domain;
  struct stat s;
  char *filename;
  Document *d;
  char *hostname;

  hostname = up->hostname != NULL ? up->hostname:"localhost";
  if (mystrcmp(hostname, "localhost") != 0)
  {
    domain = net_gethostname(); /* gets the full domain name */
    if (domain == NULL) return(NULL);
    if (mystrcmp(domain, hostname) != 0)
    {
      char myhostname[256];

      gethostname(myhostname, sizeof(myhostname)); /* gets only hostname ? */
      if (mystrcmp(hostname, myhostname) != 0)
      {
	return(ftp(up, mtlist));
      }
    }
  }

  filename = FixFilename(up->filename);
  if (filename == NULL || stat(filename, &s) < 0)
  {
    char *msg = GetFromStringDB("localaccess");

    return(BuildDocument(msg, strlen(msg), "text/html", 1, 0));
  }

  if (S_ISDIR(s.st_mode)) d = LoadDir(filename);
  else d = LoadFile(filename, (int) s.st_size, mtlist);

  return(d);
}

/*
 * telnet_main
 *
 * handles telnet and tn3270 URLs
 */
static Document *
telnet_main(up, content)
URLParts *up;
char *content;
{
  char *username;
  char *password;
  char *t;
  int portno;
  static char *format = "%s %d %s %s\n";

  if (up->hostname == NULL) return(NULL);

  if (up->username == NULL) username = alloc_string("");
  else username = up->username;

  if (up->password == NULL) password = alloc_string("");
  else password = up->password;

  if (up->port == 0) portno = DEFAULT_TELNET_PORT;
  else portno = up->port;

  t = alloc_mem(strlen(format) +
		strlen(up->hostname) +
		12 +
		strlen(username) +
		strlen(password));
  sprintf (t, format, up->hostname, portno, username, password);

  return(BuildDocument(t, strlen(t), content, 0, 0));
}

/*
 * telnet
 *
 * handles telnet URLs
 */
Document * 
telnet(up, mtlist)
URLParts *up;
MIMEType *mtlist;
{
  return(telnet_main(up, "application/x-telnet"));
}

/*
 * tn3270
 *
 * handles tn3270 URLs
 */
Document *
tn3270(up, mtlist)
URLParts *up;
MIMEType *mtlist;
{
  return(telnet_main(up, "application/x-tn3270"));
}
