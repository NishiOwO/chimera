/* 
 * net.h
 *
 * Copyright (C) 1993, 1994, John Kilburg.
 *
 * See copyright.h for details.
 */

#ifndef _NO_PROTO
int net_open(char *, int);
void net_close(int);
char *net_gethostname();
int net_bind(int);
int net_accept(int);
#else
int net_open();
void net_close();
char *net_gethostname();
int net_bind();
int net_accept();
#endif

