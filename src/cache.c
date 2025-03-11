/*
 * cache.c
 *
 * Copyright (C) 1994, John Kilburg.
 *
 * See copyright.h for details.
 */
#include "copyright.h"

#include "options.h"

#include <stdio.h>
#include <time.h>

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
#include <sys/stat.h>
#include <sys/param.h>
#include <fcntl.h>

#if defined(SYSV) || defined(SVR4)|| defined(__arm)
#include <dirent.h>
#define DIRSTUFF struct dirent
#else
#include <sys/dir.h>
#define DIRSTUFF struct direct
#endif

#include "url.h"
#include "mime.h"
#include "document.h"
#include "http.h"
#include "util.h"
#include "md5.h"
#include "cache.h"

static CacheInfo *cache_list = NULL;
static CacheInfo default_cache;
static int ignore_expires;

/*
 * HasExpired
 */
static int
HasExpired(expires, ttl, mtime)
time_t expires, ttl, mtime;
{
  time_t now;

  if (ignore_expires != 0) expires = 0;

  now = time((time_t *)0);
  if ((expires > 0 && now > expires) ||
      (now - mtime > ttl && ttl > 0))
  {
    return(1);
  }
  return(0);
}

/*
 * FindCacheEntry
 */
static CacheEntry *
FindCacheEntry(ci, filename)
CacheInfo *ci;
char *filename;
{
  CacheEntry *c;

  for (c = ci->clist; c; c = c->next)
  {
    if (strcmp(filename, c->path) == 0) return(c);
  }

  return(NULL);
}

/*
 * FindCacheInfo
 */
static CacheInfo *
FindCacheInfo(domain)
char *domain;
{
  CacheInfo *ci;
  int dlen;
  int elen;

  dlen = strlen(domain);
  for (ci = cache_list; ci; ci = ci->next)
  {
    if (ci->domain[0] == '*') return(ci);
    else
    {
      elen = strlen(ci->domain);
      if (elen <= dlen &&
	  mystrncmp(ci->domain, domain + (dlen - elen), elen) == 0)
      {
	return(ci);
      }
    }
  }

  if (ci == NULL) ci = &default_cache;

  return(ci);
}

/*
 * InsertCacheInfo
 */
static void
InsertCacheEntry(ci, path, atime, mtime, size, expires)
CacheInfo *ci;
char *path;
time_t atime;
time_t mtime;
off_t size;
time_t expires;
{
  CacheEntry *n, *c, *t;

  n = (CacheEntry *)alloc_mem(sizeof(CacheEntry));
  n->path = alloc_string(path);
  n->atime = atime;
  n->mtime = mtime;
  n->size = size;
  n->prev = NULL;
  n->next = NULL;
  n->expires = expires;

  t = NULL;
  c = ci->clist;
  while (c)
  {
    if (c->atime > atime)
    {
      if (t != NULL) t->next = n;
      else ci->clist = n;
      n->next = c;
      n->prev = t;
      return;
    }
 
    t = c;
    c = c->next;
  }

  if (t != NULL) t->next = n;
  else ci->clist = n;
  n->prev = t;

  return;
}

/*
 * DeleteCacheEntry
 */
static void
DeleteCacheEntry(ci, c)
CacheInfo *ci;
CacheEntry *c;
{
  unlink(c->path);
  ci->size -= c->size;

  if (c->prev == NULL) ci->clist = c->next;
  else c->prev->next = c->next;

  if (c->next != NULL) c->next->prev = c->prev;

  free(c->path);
  free(c);

  return;
}

/*
 * ReadCacheInfoFile
 */
static void
ReadCacheInfoFile(filename)
char *filename;
{
  FILE *fp, *fp2;
  char buffer[BUFSIZ];
  char path[MAXPATHLEN + 1], domain[BUFSIZ];
  int max, ttl, clean;
  CacheInfo *ci, *ti;
  DIR *dp;
  struct stat s;
  time_t expires = 0;
  MIMEField *mf;
  DIRSTUFF *de;

  fp = fopen(filename, "r");
  if (fp == NULL) return;

  while (fgets(buffer, sizeof(buffer), fp) != NULL)
  {
    if (buffer[0] == '#' || buffer[0] == '\n') continue;

    if (sscanf(buffer, "%s %s %d %d %d",
	       path, domain, &max, &ttl, &clean) == 5)
    {
      ci = (CacheInfo *)alloc_mem(sizeof(CacheInfo));
      ci->dir = alloc_string(path);
      ci->ttl = (time_t)ttl;
      ci->max_size = (off_t)max;
      ci->domain = alloc_string(domain);
      ci->cleanup = clean;
      ci->size = (off_t)0;
      ci->clist = NULL;
      ci->next = NULL;
      if (cache_list == NULL) cache_list = ci;
      else ti->next = ci;
      ti = ci;

      /*
       * Scan the cache directory for cache files.
       */
      dp = opendir(ci->dir);
      if (dp != NULL)
      {
	while ((de = readdir(dp)) != NULL)
	{
	  /*
	   * Is the prefix right?
	   */
	  if (strncmp("ccf", de->d_name, 3) == 0)
	  {
	    /*
	     * Get the fullpath and filename.
	     */
	    sprintf (path, "%s/%s", ci->dir, de->d_name);
	    if (stat(path, &s) != 0) continue;

	    /*
	     * Now scan the header and look for the Expires field.
	     */
	    expires = 0;
	    fp2 = fopen(path, "r");
	    if (fp2 != NULL)
	    {
	      while (fgets(buffer, sizeof(buffer), fp2) != NULL)
	      {
		if (buffer[0] == '\n') break;
		
		if ((mf = CreateMIMEField(buffer)) != NULL)
		{
		  if (mystrcmp("expires", mf->name) == 0)
		  {
		    expires = ParseExpiresDate(mf->value);
		    DestroyMIMEField(mf);
		    break;
		  }
		  else DestroyMIMEField(mf);
		}
	      }
	      fclose(fp2);
	    }

	    /*
	     * If the document has not expired then add it to the list
	     * otherwise remove the file.
	     */
	    if (!HasExpired(expires, ci->ttl, s.st_mtime))
	    {
	      InsertCacheEntry(ci, path, s.st_atime, s.st_mtime,
			       s.st_size, expires);
	      ci->size += s.st_size;
	    }
	    else unlink(path);
	  }
	}    
	closedir(dp);
      }
    }
  }

  fclose(fp);

  return;
}

/*
 * InitCache
 */
void
InitCache(cachedir, max, ttl, cleanup, ignore, cachelist)
char *cachedir;
int max, ttl, cleanup;
int ignore;
char *cachelist;
{
  char *f, *filename;

  ignore_expires = ignore;

  f = cachelist;
  while ((filename = mystrtok(f, ':', &f)) != NULL)
  {
    filename = FixFilename(filename);
    if (filename != NULL) ReadCacheInfoFile(filename);
  }

  default_cache.dir = alloc_string(cachedir);
  default_cache.domain = alloc_string("*");
  default_cache.max_size = (off_t)max;
  default_cache.ttl = (time_t)ttl;
  default_cache.cleanup = cleanup;
  default_cache.size = (off_t)0;
  default_cache.clist = NULL;
  default_cache.next = NULL;

  return;
}

/*
 * urlToCacheName
 *
 * uses MD5 to make a name for the cache.
 */
static char *
urlToCacheName(ci, up)
CacheInfo *ci;
URLParts *up;
{
  static char name[MAXPATHLEN + 1];
  struct MD5Context context;
  unsigned char digest[16];
  unsigned char hex[33];
  unsigned char *url;
  unsigned char *cp, *ep, *dp;

  url = (unsigned char *)MakeURL(up);

  MD5Init(&context);
  MD5Update(&context, url, strlen(url));
  MD5Final(digest, &context);
  free(url);

  cp = hex;
  for (dp = digest, ep = digest + (sizeof(digest) / sizeof(digest[0]));
           dp < ep; cp += 2)
  {
    (void)sprintf(cp, "%02x", *dp++ & 0xff);
  }
  *cp = '\0';

  /* chop it off to have reasonable length file names */
  hex[11] = '\0';

  sprintf(name, "%s/ccf%s", ci->dir, hex);

  return(name);
}

/*
 * IsCached
 *
 * Returns 1 if the specified URL is in the cache, otherwise returns 0.
 */
int
IsCached(up)
URLParts *up;
{
  CacheEntry *c;
  CacheInfo *ci;
  char *filename;
  struct stat s;

  if (up->hostname == NULL) return(0);

  ci = FindCacheInfo(up->hostname);
  if (ci == NULL) return(0);

  filename = urlToCacheName(ci, up);

  if (stat(filename, &s) == -1) return(0);
  
  /*
   * If the document is too old then don't use it.
   */
  c = FindCacheEntry(ci, filename);
  if (c == NULL)
  {
    unlink(filename);
    return(0);
  }

  if (HasExpired(c->expires, ci->ttl, c->mtime))
  {
    DeleteCacheEntry(ci, c);
    return(0);
  }

  return(1);
}

/*
 * FlushCache
 */
static int
FlushCache(ci)
CacheInfo *ci;
{
  if (ci->clist != NULL)
  {
    DeleteCacheEntry(ci, ci->clist);
    return(1);
  }

  return(0);
}

/*
 * WriteCache
 */
void
WriteCache(d)
Document *d;
{
  FILE *fp;
  char *u;
  char *filename;
  int size = 0;
  CacheInfo *ci;
  MIMEField *mf;
  time_t now;

  if (d->cache && d->up != NULL && !d->from_cache &&
      !HasExpired(d->expires, 0, 0) && d->up->hostname != NULL)
  {
    ci = FindCacheInfo(d->up->hostname);
    if (ci == NULL) return;

    filename = urlToCacheName(ci, d->up);

    size = d->len + (d->ptext != NULL ? d->plen:0);
    
    while (ci->max_size &&
	   ((size + ci->size) > ci->max_size))
    {
      if (!FlushCache(ci)) return;
    }

    fp = fopen(filename, "w");
    if (fp == NULL) return;
    
    u = MakeURL(d->up);
    fprintf (fp, "X-URL: %s\n", u);
    free(u);

    if (d->content) fprintf (fp, "Content-type: %s\n", d->content);
    else fprintf (fp, "Content-type: x-unknown/x-unknown\n");
    fprintf (fp, "Content-length: %d\n", d->len);

    if (d->ptext)
    {
      fprintf (fp, "X-PContent-length: %d\n", d->plen);
      if (d->pcontent) fprintf (fp, "X-PContent-type: %s\n", d->pcontent);
      else fprintf (fp, "X-PContent-type: x-unknown/x-unknown\n");
    }

    for (mf = d->mflist; mf; mf = mf->next)
    {
      if (mystrcmp(mf->name, "content-type") != 0 &&
	  mystrcmp(mf->name, "content-length") != 0)
      {
	fprintf (fp, "%s: %s\n", mf->name, mf->value);
      }
    }

    fprintf (fp, "\n");

    if (fwrite(d->text, 1, d->len, fp) < d->len)
    {
      fclose(fp);
      unlink(filename);
      return;
    }
    if (d->ptext != NULL)
    {
      if (fwrite(d->ptext, 1, d->plen, fp) < d->plen)
      {
	fclose(fp);
	unlink(filename);
	return;
      }
    }
    
    fclose(fp);

    ci->size += size;

    now = time((time_t *)0);
    InsertCacheEntry(ci, filename, now, now, size, d->expires);
  }
  
  return;
}

/*
 * CleanCache
 */
void
CleanCache()
{
  CacheEntry *c, *t;
  CacheInfo *ci, *ti;

  ci = cache_list;
  while (ci)
  {
    c = ci->clist;
    while (c)
    {
      t = c;
      c = c->next;
      if (t->path != NULL)
      {
	if (ci->cleanup != 0) unlink(t->path);
	free(t->path);
      }
      free(t);
    }

    ti = ci;
    ci = ci->next;
    if (ti->dir) free(ti->dir);
    if (ti->domain) free(ti->domain);
    free(ti);
  }

  c = default_cache.clist;
  while (c)
  {
    t = c;
    c = c->next;
    if (t->path != NULL)
    {
      if (default_cache.cleanup != 0) unlink(t->path);
      free(t->path);
    }
    free(t);
  }
  if (default_cache.dir != NULL) free(default_cache.dir);
  if (default_cache.domain != NULL) free(default_cache.domain);

  return;
}


/*
 * ReadCache
 */
Document *
ReadCache(up)
URLParts *up;
{
  char *filename;
  Document *d;
  FILE *fp;
  char buffer[BUFSIZ];
  MIMEField *m;
  CacheInfo *ci;
  CacheEntry *c;

  if (up->request_data != NULL || up->hostname == NULL) return(NULL);

  ci = FindCacheInfo(up->hostname);
  if (ci == NULL) return(NULL);

  filename = urlToCacheName(ci, up);

  fp = fopen(filename, "r");
  if (fp == NULL) return(NULL);
  
  /*
   * If the document is too old then don't use it.
   */
  c = FindCacheEntry(ci, filename);
  if (c == NULL)
  {
    fclose(fp);
    unlink(filename);
    return(NULL);
  }

  if (HasExpired(c->expires, ci->ttl, c->mtime))
  {
    fclose(fp);
    DeleteCacheEntry(ci, c);
    return(NULL);
  }

  d = CreateDocument();

  /*
   * Read MIME header
   */
  while (fgets(buffer, sizeof(buffer), fp) != NULL)
  {
    if (buffer[0] == '\n') break;

    if ((m = CreateMIMEField(buffer)) != NULL)
    {
      ParseMIMEField(d, m);
      m->next = d->mflist;
      d->mflist = m;
    }
  }

  /*
   * If cache length is zero then flush and return NULL to force a reload.
   */
  if (d->len == 0)
  {
    fclose(fp);
    DestroyDocument(d);
    DeleteCacheEntry(ci, c);
    return(NULL);
  }

  /*
   * Read the contents.
   */
  d->text = (char *)alloc_mem(d->len + 1);
  if (fread(d->text, 1, d->len, fp) < d->len)
  {
    fclose(fp);
    DestroyDocument(d);
    DeleteCacheEntry(ci, c);
    return(NULL);
  }
  d->text[d->len] = '\0';

  /*
   * Read processed contents if any.
   */
  if (d->plen > 0)
  {
    d->ptext = (char *)alloc_mem(d->plen + 1);
    if (fread(d->ptext, 1, d->plen, fp) < d->plen)
    {
      free(d->ptext);
      d->ptext = NULL;
      d->plen = 0;
      if (d->pcontent != NULL)
      {
	free(d->pcontent);
	d->pcontent = NULL;
      }
    }
  }

  fclose(fp);

  d->from_cache = 1;

  return(d);
}
