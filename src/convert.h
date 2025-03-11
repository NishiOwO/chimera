/*
 * file.h
 *
 * Copyright (C) 1993, 1994, John Kilburg.
 *
 * See copyright.h for details.
 */

typedef struct _convert
{
  char *use;
  char *incontent;
  char *outcontent;
  char *command;
  int (*func)();
  struct _convert *next;
} Convert;

#ifndef _NO_PROTO
Document *ConvertDocument(Document *, Convert *, char *, char *);
Convert *ReadConvertFiles(char *);
#else
Document *ConvertDocument();
Convert *ReadConvertFiles();
#endif
