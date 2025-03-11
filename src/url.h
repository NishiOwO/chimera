/*
 * url.h
 *
 * Copyright (C) 1993, 1994, John Kilburg.
 *
 * See copyright.h for details.
 */

typedef struct
{
  char *protocol;
  char *hostname;
  int port;
  char *filename;
  char *anchor;
  char *method;
  char *request_data;
  char *data_type;
  char *auth_type;
  char *username;
  char *password;
} URLParts;

#define NullString(s)	(s == NULL || *s == '\0')

/*
 * Prototypes
 */

#ifndef _NO_PROTO
char *FixURL(char *);
char *MakeURL(URLParts *);
URLParts *MakeURLParts(URLParts *, URLParts *);
URLParts *ParseURL(char *);
void DestroyURLParts(URLParts *);
void StripSearchURL(char *);
void AddVisitedURL(char *);
int IsVisitedURL(char *);
char *UnescapeURL(char *);
char *EscapeURL(unsigned char *, int);
URLParts *DupURLParts(URLParts *);
int IsAbsoluteURL(URLParts *);
#else
char *FixURL();
URLParts *ParseURL();
void DestroyURLParts();
char *MakeURL();
URLParts *MakeURLParts();
void StripSearchURL();
void AddVisitedURL();
int IsVisitedURL();
char *UnescapeURL();
char *EscapeURL();
URLParts *DupURLParts();
int IsAbsoluteURL();
#endif
