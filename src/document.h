/*
 * document.h
 *
 * Copyright (C) 1993, 1994, John Kilburg.
 *
 * See copyright.h for details.
 */

typedef struct _document
{
  URLParts *up;         /* the URL in parts */
  char *text;           /* the stuff in the document */
  int len;              /* the len of the data */
  char *content;        /* content-type might be NULL */
  char *ptext;          /* processed data (after pipe) */
  int plen;             /* length of processed data */
  char *pcontent;       /* processed content-type */
  int from_cache;       /* 1 if fetched from the cache */
  int cache;            /* 1 if the document should be cached */
  char *location;       /* the real location */
  char *encoding;       /* content encoding */
  char *tencoding;      /* tranfer encoding */
  int status;           /* HTTP status */
  char *auth_realm;     /* authentication realm */
  char *auth_type;      /* authentication type */
  MIMEField *mflist;    /* list of MIME fields for document */
  int nothing;          /* if non-zero then is a null document */
  time_t expires;       /* date/time of expiration */
} Document;

typedef struct _protocol
{
  char *proto;
  char *command;
  struct _protocol *next;
} Protocol;

#ifndef _NO_PROTO
Protocol *ReadProtocolFiles(char *);
Document *LoadDocument(URLParts *, Protocol *, MIMEType *, int);
void DestroyDocument(Document *);
Document *CreateDocument();
Document *BuildDocument(char *, int, char *, int, int);
void ParseMIMEField(Document *, MIMEField *);
#else
Protocol *ReadProtocolFiles();
Document *LoadDocument();
void DestroyDocument();
Document *CreateDocument();
Document *BuildDocument();
void ParseMIMEField();
#endif

