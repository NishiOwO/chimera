/*
 * stringdb.h
 *
 * Copyright (C) 1993, 1994, John Kilburg.
 *
 * See copyright.h for details.
 */

typedef struct _stringdb
{
  char *name;
  char *value;
  struct _stringdb *next;
} StringDB;

#ifndef _NO_PROTO
void AddListToStringDB(StringDB *);
void AddToStringDB(char *, char *);
char *GetFromStringDB(char *);
char *NGetFromStringDB(char *);
#else
void AddListToStringDB();
void AddToStringDB();
char *GetFromStringDB();
char *NGetFromStringDB();
#endif
