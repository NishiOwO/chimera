/*
 * stringdb.c
 *
 * Copyright (C) 1993, 1994, John Kilburg.
 *
 * See copyright.h for details.
 */
#include "copyright.h"

#include <stdio.h>

#include "stringdb.h"
#include "util.h"
#include "options.h"

static StringDB *head = NULL;

/*
 * AddListToStringDB
 *
 * Adds a preallocated list to the string DB
 */
void
AddListToStringDB(list)
StringDB *list;
{
  StringDB *l, *tl;

  for (l = list, tl = NULL; l; tl = l, l = l->next)
      ;

  if (tl != NULL) tl->next = head;
  head = list;

  return;
}

/*
 * AddToStringDB
 *
 * Adds a new entry to the string DB
 */
void
AddToStringDB(name, value)
char *name;
char *value;
{
  StringDB *n;

  n = (StringDB *)alloc_mem(sizeof(StringDB));
  n->name = alloc_string(name);
  n->value = alloc_string(value);
  n->next = head;
  head = n;

  return;
}

/*
 * GetFromStringDB
 *
 * Returns a value from a name
 */
char *
GetFromStringDB(name)
char *name;
{
  StringDB *c;

  for (c = head; c; c = c->next)
  {
    if (strcmp(c->name, name) == 0) return(c->value);
  }

  return(name);
}

/*
 * GetFromStringDB
 *
 * Returns a value from a name
 */
char *
NGetFromStringDB(name)
char *name;
{
  StringDB *c;

  for (c = head; c; c = c->next)
  {
    if (strcmp(c->name, name) == 0) return(c->value);
  }

  return(NULL);
}
