/*
 * mime.h
 *
 * Copyright (C) 1994, John Kilburg.
 *
 * See copyright.h for details.
 */

typedef struct _mailcap
{
  char *ctype;
  char *command;
  struct _mailcap *next;
} MailCap;

typedef struct _mimetype
{
  char *content;
  char *ext;
  struct _mimetype *next;
} MIMEType;

typedef struct _mimefield
{
  char *name;
  char *value;
  struct _mimefield *next;
} MIMEField;

#ifndef _NO_PROTO
char *Ext2Content(MIMEType *, char *);
MIMEType *ReadMIMETypeFiles(char *);
MailCap *ReadMailCapFiles(char *);
int DisplayExternal(char *, char *, char *, MailCap *);
MIMEField *CreateMIMEField(char *);
void DestroyMIMEField(MIMEField *);
time_t ParseExpiresDate(char *);
#else
char *Ext2Content();
MIMEType *ReadMIMETypeFiles();
MailCap *ReadMailCapFiles();
int DisplayExternal();
MIMEField *CreateMIMEField();
void DestroyMIMEField();
time_t ParseExpiresDate();
#endif

