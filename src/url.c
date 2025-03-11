/*
 * url.c
 *
 * Copyright (C) 1993, 1994, John Kilburg.
 *
 * See copyright.h for details.
 */
#include "copyright.h"

#include "options.h"

#include <stdio.h>
#include <ctype.h>
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
#include "util.h"

#define MYDUP(a, b)   a = (b != NULL ? alloc_string(b):NULL)

/*
 * StripSearchURL
 *
 * Takes the ?mumble off the end.
 */
void
StripSearchURL(url)
char *url;
{
  char *cp;

  for (cp = url; *cp; cp++)
  {
    if (*cp == '?')
    {
      *cp = '\0';
      break;
    }
  }

  return;
}

/*
 * EscapeURL
 *
 * Puts escape codes in URLs.  NOT complete.
 */
char *
EscapeURL(url, s2p)
unsigned char *url;
int s2p;
{
  unsigned char *cp;
  char *n, *s;
  static char *hex = "0123456789ABCDEF";

  /*
   * use a bit of memory so i don't have to mess around here
   */
  s = n = alloc_mem(strlen(url) * 3 + 2);
  if (s == NULL)
  {
    return(NULL);
  }

  for (cp = url; *cp; cp++, n++)
  {
    if (*cp == ' ' && s2p != 0)
    {
      *n = '+';
    }
    else if (*cp > 127 ||
	     (!isalpha(*cp) && !(*cp == '$' || *cp == '-' || *cp == '_'||
				 *cp == '.' || *cp == '\'' || *cp == '(' ||
				 *cp == ')' || *cp == ',' || *cp == '"' ||
				 *cp == '+' || *cp == '!' || *cp == '*')))
    {
      *n = '%';
      n++;
      *n = hex[*cp / 16];
      n++;
      *n = hex[*cp % 16];
    }
    else
    {
      *n = *cp;
    }
  }

  *n = '\0';

  return(s);
}

/*
 * UnescapeURL
 *
 * Converts the escape codes (%xx) into actual characters.  NOT complete.
 * Could do everthing in place I guess.
 */
char *
UnescapeURL(url)
char *url;
{
  char *cp, *n, *s;
  char hex[3];

  s = n = alloc_mem(strlen(url) + 2);
  if (n == NULL)
  {
    return(NULL);
  }

  for (cp = url; *cp; cp++, n++)
  {
    if (*cp == '%')
    {
      cp++;
      if (*cp == '%')
      {
	*n = *cp;
      }
      else
      {
	hex[0] = *cp;
	cp++;
	hex[1] = *cp;
	hex[2] = '\0';
	*n = (char)strtol(hex, NULL, 16);
      }
    }
    else
    {
      *n = *cp;
    }
  }

  *n = '\0';

  return(s);
}

/*
 * MakeURL
 */
char *
MakeURL(up)
URLParts *up;
{
  int len;
  char *u;
  char *delim;
  char *delim2;
  char *filename;
  char *hostname;
  char *protocol;

  if (up->protocol != NULL && strncmp(up->protocol, "urn", 3) == 0)
  {
    u = alloc_mem(strlen(up->filename) + 5);
    strcpy(u, "urn:");
    strcat(u, up->filename);
    return(u);
  }

  if (NullString(up->protocol)) protocol = "file";
  else protocol = up->protocol;
  
  if (NullString(up->hostname))
  {
    delim = "";
    hostname = "";
  }
  else
  {
    delim = "//";
    hostname = up->hostname;
  }

  if (NullString(up->filename)) filename = "/";
  else filename = up->filename;

  delim2 = "";

  len = strlen(protocol) + strlen(hostname) + strlen(filename) +
        strlen(delim) + 11;
  u = alloc_mem(len + 1);
  if (up->port == 0)
  {
    sprintf (u, "%s:%s%s%s%s", protocol, delim, hostname, delim2,
	     filename);
  }
  else
  {
    sprintf (u, "%s:%s%s:%d%s%s", protocol, delim, hostname, up->port,
           delim2, filename);
  }

  return(u);
}

/*
 * CreateURLParts
 *
 * Allocate URLParts and initialize to NULLs
 */
static URLParts *
CreateURLParts()
{
  URLParts *up;

  up = (URLParts *)alloc_mem(sizeof(URLParts));
  up->protocol = NULL;
  up->hostname = NULL;
  up->port = 0;
  up->filename = NULL;
  up->anchor = NULL;

  up->method = NULL;
  up->request_data = NULL;
  up->data_type = NULL;

  up->auth_type = NULL;
  up->username = NULL;
  up->password = NULL;

  return(up);
}
  
/*
 * MakeURLParts
 */
URLParts *
MakeURLParts(up1, up2)
URLParts *up1, *up2;
{
  URLParts *r;
  char *filename;

  /*
   * Special case for URNs.  Sucks.
   */
  if (up1->protocol != NULL && strncmp(up1->protocol, "urn", 3) == 0)
  {
    return(DupURLParts(up1));
  }

  r = CreateURLParts();

  /*
   * Deal with the protocol.  If it has a protocol then use it, otherwise
   * use the parent's protocol.  If the parent doesn't have a protocol for
   * some spooky reason then use "file".
   */
  if (up1->protocol == NULL)
  {
    if (up2->protocol != NULL) r->protocol = alloc_string(up2->protocol);
    else r->protocol = alloc_string("file");
  }
  else r->protocol = alloc_string(up1->protocol);

  /*
   * Deal with the hostname.  If it has a hostname then use it, otherwise
   * if the protocol used by the parent is the same then use the parent's
   * hostname.
   *
   * The port goes along with the hostname.
   */
  if (up1->hostname == NULL)
  {
    /*
     * r is used here because up1->protocol can be NULL but r->protocol
     * will always have the right thing.
     */
    if (up2->hostname != NULL && up2->protocol != NULL &&
        mystrcmp(r->protocol, up2->protocol) == 0)
    {
      r->hostname = alloc_string(up2->hostname);
      r->port = up2->port;
    }
  }
  else
  {
    r->hostname = alloc_string(up1->hostname);
    r->port = up1->port;
  }

  if (up1->filename == NULL)
  {
    r->filename = alloc_string("/");
  }
  if (up2->protocol == NULL || mystrcmp(r->protocol, up2->protocol) != 0)
  {
    r->filename = alloc_string(up1->filename);
  }
  else if (up1->filename[0] == '/' || up1->filename[0] == '~')
  {
    r->filename = alloc_mem(strlen(up1->filename) + 2);
    if (up1->filename[0] != '/')
    {
      r->filename[0] = '/';
      strcpy(r->filename + 1, up1->filename);
    }
    else
    {
      strcpy(r->filename, up1->filename);
    }
  }
  else
  {
    /*
     * This includes an attempt to deal with '..' and '.'
     */
    int i;
    int len, len2;
    int start;
    int backcount = 0;

    backcount = 0;
    start = 0;
    len = strlen(up1->filename);
    for (i = 0; i < len; i++)
    {
      if (strncmp(up1->filename + i, "../", 3) == 0)
      {
	start = i + 3;
	i += 2;
	backcount++;
      }
      else if (strncmp(up1->filename + i, "./", 2) == 0)
      {
	start = i + 2;
	i += 1;
      }
    }

    filename = alloc_string(up2->filename);
    for (i = strlen(filename) - 1; i >= 0; i--)
    {
      if (filename[i] == '/')
      {
	if (backcount-- == 0)
	{
	  filename[i + 1] = '\0';
	  break;
	}
      }
    }

    if (i == -1)
    {
      r->filename = alloc_string(up1->filename + start);
    }
    else
    {
      len2 = strlen(filename);
      r->filename = alloc_mem(len2 + strlen(up1->filename + start) + 1);
      strcpy(r->filename, filename);
      strcat(r->filename, up1->filename + start);
    }
    free(filename);
  }

  /*
   * Copy misc. fields.
   */
  MYDUP(r->method, up1->method);
  MYDUP(r->request_data, up1->request_data);
  MYDUP(r->data_type, up1->data_type);
  MYDUP(r->auth_type, up1->auth_type);
  MYDUP(r->username, up1->username);
  MYDUP(r->password, up1->password);

  return(r);
}

URLParts *
DupURLParts(up)
URLParts *up;
{
  URLParts *dp;


  dp = CreateURLParts();
  MYDUP(dp->method, up->method);
  MYDUP(dp->protocol, up->protocol);
  MYDUP(dp->hostname, up->hostname);
  dp->port = up->port;

  dp->filename = up->filename != NULL ?
      alloc_string(up->filename):alloc_string("/");

  MYDUP(dp->anchor, up->anchor);

  MYDUP(dp->request_data, up->request_data);
  MYDUP(dp->data_type, up->data_type);

  MYDUP(dp->auth_type, up->auth_type);
  MYDUP(dp->username, up->username);
  MYDUP(dp->password, up->password);

  return(dp);
}

/*
 * ParseURL
 *
 * Turns a URL into a URLParts structure
 *
 * The good stuff was written by Rob May <robert.may@rd.eng.bbc.co.uk>
 * and heavily mangled/modified by john to suite his own weird style.
 */
URLParts *
ParseURL(url)
char *url;
{
  URLParts *up;
  char *start;
  char *colon, *slash, *fslash;
  char *pound; /* link pound (#) sign */
  char *at; /* username/password @ */
  char *ucolon; /* username colon */
  char *pcolon; /* port number colon */

  up = CreateURLParts();

  /* skip leading white-space (if any)*/
  for (start = url; isspace(*start); start++)
      ;

  /* Lousy hack for URNs */
  if (mystrncmp(url, "urn:", 4) == 0)
  {
    up->protocol = alloc_string("urn");
    up->filename = alloc_string(url + 4);
    return(up);
  }

  colon = strchr(start, ':');
  slash = strchr(start, '/');

  /*
   * Check to see if there is a protocol.
   */
  if (colon != NULL && (slash == NULL || colon < slash))
  {
    up->protocol = alloc_mem(colon - start + 1);
    strncpy(up->protocol, start, colon - start);
    up->protocol[colon - start] = '\0';
  }

  /*
   * If there is a slash then sort out the hostname and filename.
   * If there is no slash then there is no hostname but there is a
   * filename.
   */
  if (slash != NULL)
  {
    /*
     * Check for leading //. If its there then there is a host string.
     */
    if ((*(slash + 1) == '/') && ((colon == NULL && slash == start) ||
	(colon != NULL && slash == colon + 1)))
    {
      /*
       * Check for filename at end of host string.
       */
      slash += 2;
      if ((fslash = strchr(slash, '/')) != NULL)
      {
	up->hostname = alloc_mem(fslash - slash + 1);
	strncpy(up->hostname, slash, fslash - slash);
	up->hostname[fslash - slash] = '\0';
	up->filename = alloc_string(fslash);
      }
      else
      { /* there is no filename */
	up->hostname = alloc_string(slash);
      }
    }
    else
    {
      /*
       * the rest is a filename because there is no // or it appears
       * after other characters
       */
      if (colon != NULL) up->filename = alloc_string(colon + 1);
      else up->filename = alloc_string(start);
    }
  }
  else
  {
    /*
     * No slashes at all so the rest must be a filename.
     */
    if (colon == NULL) up->filename = alloc_string(start);
    else up->filename = alloc_string(colon + 1);
  }

  /*
   * If there is a host string then divide it into
   * username:password@hostname:port as needed.
   */
  if (up->hostname != NULL)
  {
    /*
     * Look for username:password.
     */
    if ((at = strchr(up->hostname, '@')) != NULL)
    {
      char *mumble;

      up->username = alloc_mem(at - up->hostname + 1);
      strncpy(up->username, up->hostname, at - up->hostname);
      up->username[at - up->hostname] = '\0';

      mumble = alloc_string(at + 1);
      free(up->hostname);
      up->hostname = mumble;

      if ((ucolon = strchr(up->username, ':')) != NULL)
      {
	up->password = alloc_string(ucolon + 1);
	*ucolon = '\0';
      }
    }

    /*
     * Grab the port.
     */
    if ((pcolon = strchr(up->hostname, ':')) != NULL)
    {
      up->port = atoi(pcolon + 1);
      *pcolon = '\0';
    }
  }

  /*
   * Check the filename for a '#foo' string.
   */
  if (up->filename != NULL)
  {
    if ((pound = strchr(up->filename, '#')) != NULL)
    {
      *pound = '\0';
      up->anchor = alloc_string(pound + 1);
    }
  }
  else up->filename = alloc_string("/");

  return(up);
}


/*
 * DestroyURLParts
 *
 * Destroys a URLParts structure.
 */
void
DestroyURLParts(up)
URLParts *up;
{
  if (up->protocol) free(up->protocol);
  if (up->hostname) free(up->hostname);
  if (up->filename) free(up->filename);
  if (up->anchor) free(up->anchor);

  if (up->method) free(up->method);
  if (up->request_data) free(up->request_data);
  if (up->data_type) free(up->data_type);

  if (up->auth_type) free(up->auth_type);
  if (up->username) free(up->username);
  if (up->password) free(up->password);

  free(up);

  return;
}

/*
 * IsAbsoluteURL
 *
 * Returns 1 if the URLParts given is absolute, 0 otherwise.
 */
int
IsAbsoluteURL(up)
URLParts *up;
{
  if (up->protocol == NULL) return(0);

  return(1);
}
