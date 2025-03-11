/*
 *
 * lister.h
 *
 * Copyright (C) 1993, 1994, John Kilburg.
 *
 * See copyright.h for details.
 */

typedef struct _listcommand
{
  char *name;
  void (*func)();
  Widget w;
} ListCommand;

/*
 * Prototypes
 */
#ifndef _NO_PROTO
void CreateLister(char *, Widget, ListCommand *, char **, ListCommand *, char **, XtPointer);
void DestroyLister();
void RaiseLister();
void ChangeLister(char **, char **);
#else
void CreateLister();
void DestroyLister();
void RaiseLister();
void ChangeLister();
#endif




