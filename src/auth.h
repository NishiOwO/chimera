/*
 * auth.h
 *
 * Copyright (C) 1993, 1994, John Kilburg.
 *
 * See copyright.h for details.
 *
 */
#ifndef _NO_PROTO
void GetAuthInfo(char *);
int IsAuthentic(char *, char *);
int IsKAuthentic(char *);
#else
void GetAuthInfo();
int IsAuthentic();
int IsKAuthentic();
#endif
