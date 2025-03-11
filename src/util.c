/*
 * util.c
 *
 * Copyright (C) 1993, 1994, John Kilburg.
 *
 * See copyright.h for details.
 */
#include "copyright.h"

#include "options.h"

#include <stdio.h>
#include <pwd.h>
#include <errno.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <sys/param.h>

#include <ctype.h>

#include "util.h"

/*
 * OutOfMemory
 *
 * This function is called when a memory allocation fails.
 */
static void
OutOfMemory()
{
  fprintf (stderr, "Memory allocation failed.  Game Over.\n");
  exit(1);
  return;
}

/*
 * mymalloc
 *
 */
static char *
mymalloc(len)
int len;
{
  char *s;

  s = (char *)malloc(len);
  if (s == NULL) OutOfMemory();

/*
  memset(s, 1, len);
*/

  return(s);
}

/*
 * myrealloc
 *
 */
static char *
myrealloc(s, len)
char *s;
int len;
{
  s = (char *)realloc(s, len);
  if (s == NULL) OutOfMemory();

  return(s);
}

/*
 * alloc_mem
 *
 * allocate memory
 */
char *
alloc_mem(len)
int len;
{
  return((char *)mymalloc(len));
}

/*
 * realloc_mem
 */
char *
realloc_mem(s, len)
char *s;
int len;
{
  return((char *)myrealloc(s, len));
}

/*
 * alloc_string
 *
 * Allocates space for a string and copies the string to the space
 */
char *
alloc_string(str)
char *str;
{
  char *s;

  s = (char *)mymalloc(sizeof(char) * (strlen(str) + 1));

  strcpy(s, str);

  return(s);
}

/*
 * get_line
 *
 * Returns the next line from a text buffer.  A NULL is placed in
 * len - 1 in the return string.
 */
char *
get_line(t, buffer, len)
char *t;
char *buffer;
int len;
{
  char *j;
  int i;

  if (t == NULL) return(NULL);
  if (*t == '\0') return(NULL);

  for (j = t, i = 0; *j; j++, i++)
  {
    if (*j == '\r') /* nobody ever puts this without \n ? */
    {
      j += 2;
      break;
    }
    if (*j == '\n')
    {
      j++;
      break;
    }
    else if (*j == '\0')
    {
      break;
    }

    if (i < len) buffer[i] = *j;
  }
  buffer[(i < len - 1 ? i:len - 1)] = '\0';

  return(j);
}

/*
 * mystrncmp
 */
int
mystrncmp(s1, s2, len)
char *s1, *s2;
int len;
{
  int i;
  char c1 = 0, c2 = 0;

  for (i = 0; i < len && *(s1 + i) && *(s2 + i); i++)
  {
    c1 = *(s1 + i);
    c2 = *(s2 + i);
    c1 = (isupper(c1) ? tolower(c1):c1);
    c2 = (isupper(c2) ? tolower(c2):c2);
    if (c1 != c2) return(c1 - c2);
  }

  if (i == len)
  {
    c1 = 0;
    c2 = 0;
  }
  else
  {
    c1 = *(s1 + i);
    c2 = *(s2 + i);
    c1 = (isupper(c1) ? tolower(c1):c1);
    c2 = (isupper(c2) ? tolower(c2):c2);
  }

  return(c1 - c2);
}

/*
 * mystrcmp
 */
int
mystrcmp(s1, s2)
char *s1, *s2;
{
  int i;
  char c1 = 0, c2 = 0;

  for (i = 0; *(s1 + i) && *(s2 + i); i++)
  {
    c1 = *(s1 + i);
    c2 = *(s2 + i);
    c1 = (isupper(c1) ? tolower(c1):c1);
    c2 = (isupper(c2) ? tolower(c2):c2);
    if (c1 != c2) return(c1 - c2);
  }

  c1 = *(s1 + i);
  c2 = *(s2 + i);
  c1 = (isupper(c1) ? tolower(c1):c1);
  c2 = (isupper(c2) ? tolower(c2):c2);

  return(c1 - c2);
}

/*
 * FixFilename
 *
 * The only thing this does right now is to handle the '~' stuff.
 */
char *
FixFilename(filename)
char *filename;
{
  struct passwd *p;
  char *cp, *cp2;
  char username[BUFSIZ];
  char *fname;
  char *home;
  static char newfilename[MAXPATHLEN + 1];

  if (filename[0] == '~') fname = filename;
  else if (filename[0] == '/' && filename[1] == '~') fname = filename + 1;
  else fname = NULL;

  if (fname != NULL)
  {
    if (fname[1] == '/')
    {
      if ((home = getenv("HOME")) == NULL) return(NULL);
      cp = fname + 1;
    }
    else
    {
      for (cp = fname + 1, cp2 = username; *cp && *cp != '/'; cp++, cp2++)
      {
	*cp2 = *cp;
      }
      *cp2 = '\0';

      p = getpwnam(username);
      if (p == NULL) return(NULL);
      home = p->pw_dir;
    }

    if (strlen(home) + strlen(cp) > MAXPATHLEN) return(NULL);
    strcpy(newfilename, home);
    strcat(newfilename, cp);
  }
  else
  {
    strcpy(newfilename, filename);
  }

  return(newfilename);
}

/*
 * SaveData
 *
 * Save some memory to a file.  Returns 0 if successful, -1 otherwise.
 */
int
SaveData(data, datalen, filename)
char *data;
int datalen;
char *filename;
{
  FILE *fp;

  fp = fopen(filename, "w");
  if (fp == NULL) return(-1);

  if (fwrite(data, 1, datalen, fp) < datalen)
  {
    unlink(filename);
    fclose(fp);
    return(-1);
  }

  fclose(fp);

  return(0);
}

/*
 * ReapChild
 *
 * This code grabs the status from an exiting child process.  We really
 * don't care about the status, we just want to keep zombie's from
 * cropping up.
 */
static void
ReapChild()
{
#if defined(WNOHANG) && !defined(SYSV) && !defined(SVR4)
  int pid;
#endif
  extern int errno;
  int old_errno = errno;

 /*
  * It would probably be better to use the POSIX mechanism here,but I have not 
  * checked into it.  This gets us off the ground with SYSV.  RSE@GMI
  */
#if defined(WNOHANG) && !defined(SYSV) && !defined(SVR4)
#ifdef __linux__
  int st;
#else
  union wait st;
#endif

  do 
  {
    errno = 0;
    pid = wait3(&st, WNOHANG, 0);
  }
  while (pid <= 0 && errno == EINTR);
#else
  int st;

  wait(&st);
#endif
  StartReaper();
  errno = old_errno;

  return;
}

/*
 * StartReaper
 *
 * This code inits the code which reaps child processes that where
 * fork'd off for external viewers.
 */
void
StartReaper()
{
#ifdef SIGCHLD
  signal(SIGCHLD, ReapChild);
#else
  signal(SIGCLD, ReapChild);
#endif

  return;
}

/*
 * mystrtok
 *
 */
char *
mystrtok(s, c, cdr)
char *s;
int c;
char **cdr;
{
  char *cp, *cp2;
  static char str[BUFSIZ];

  if (s == NULL) return(NULL);

  for (cp = s, cp2 = str; ; cp++)
  {
    if (*cp == '\0') break;
    else if (*cp == c)
    {
      for (cp++; *cp == c; )
      {
	cp++;
      }
      break;
    }
    else *cp2++ = *cp;
  }

  *cp2 = '\0';

  if (*cp == '\0') *cdr = NULL;
  else *cdr = cp;

  return(str);
}

/*
 * CreateCommandLine
 */
char *
CreateCommandLine(command, path, filename)
char *command, *path, *filename;
{
  char *cmdline;

  cmdline = alloc_mem(strlen(command) + strlen(filename) + 1);
  sprintf (cmdline, command, filename);

  if (path != NULL && path[0] != '\0')
  {
#ifdef NOSEMICOLON
    char *format = "PATH=$PATH:%s %s";
#else
    char *format = "PATH=$PATH:%s; export PATH; %s";
#endif
    char *t = cmdline;

    cmdline = alloc_mem(strlen(format) + strlen(path) + strlen(t) + 1);
    sprintf (cmdline, format, path, t);
    free(t);
  }

  return(cmdline);
}

#ifdef NEED_STRSTR
/* 
 *      Source code for the "strstr" library routine.
 *
 * Copyright (c) 1988-1993 The Regents of the University of California.
 * All rights reserved.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

/*
 *----------------------------------------------------------------------
 *
 * strstr --
 *
 *      Locate the first instance of a substring in a string.
 *
 * Results:
 *      If string contains substring, the return value is the
 *      location of the first matching instance of substring
 *      in string.  If string doesn't contain substring, the
 *      return value is 0.  Matching is done on an exact
 *      character-for-character basis with no wildcards or special
 *      characters.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

/* snarfed from tcl */

char *
strstr(string, substring)
    register char *string;      /* String to search. */
    char *substring;            /* Substring to try to find in string. */
{
    register char *a, *b;

    /* First scan quickly through the two strings looking for a
     * single-character match.  When it's found, then compare the
     * rest of the substring.
     */

    b = substring;
    if (*b == 0) {
        return string;
    }
    for ( ; *string != 0; string += 1) {
        if (*string != *b) {
            continue;
        }
        a = string;
        while (1) {
            if (*b == 0) {
                return string;
            }
            if (*a++ != *b++) {
                break;
            }
        }
        b = substring;
    }
    return (char *) 0;
}
#endif

#ifdef NEED_GETCWD
/* 
 * getcwd.c --
 *
 *	This file provides an implementation of the getcwd procedure
 *	that uses getwd, for systems with getwd but without getcwd.
 *
 * Copyright (c) 1993 The Regents of the University of California.
 * All rights reserved.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

/* snarfed from tcl mangled for no reason by john */

extern char *getwd();

char *
getcwd(buf, size)
char *buf;		/* Where to put path for current directory. */
int size;		/* Number of bytes at buf. */
{
    char realBuffer[MAXPATHLEN+1];
    int length;

    if (getwd(realBuffer) == NULL) return(NULL);
    length = strlen(realBuffer);
    if (length >= size) return(NULL);
    strcpy(buf, realBuffer);
    return buf;
}
#endif
