/*
 * cache.h
 *
 * Copyright (C) 1994, John Kilburg.
 *
 * See copyright.h for details.
 */

typedef struct _cache_entry
{
  char *path; /* path of cache file */
  time_t atime; /* access time */
  off_t size; /* size of data sort of */
  time_t expires; /* expires time */
  time_t mtime; /* modified time */
  struct _cache_entry *prev, *next;
} CacheEntry;

typedef struct _cache_info
{
  char *dir;
  char *domain;
  off_t max_size;
  time_t ttl;
  int cleanup;
  off_t size;
  CacheEntry *clist;
  struct _cache_info *next;
} CacheInfo;

#ifndef _NO_PROTO
int IsCached(URLParts *up);
void InitCache(char *, int, int, int, int, char *);
Document *ReadCache(URLParts *);
void WriteCache(Document *);
void CleanCache();
#else
int IsCached();
void InitCache();
Document *ReadCache();
void WriteCache();
void CleanCache();
#endif
