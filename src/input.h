/*
 * input.h
 *
 * Copyright (C) 1994, John Kilburg.
 *
 * See copyright.h for details.
 */

#ifndef _NO_PROTO
int WaitForConnect(int);
int WriteBuffer(int, char *, int);
char *ReadBuffer(int, int *, int, int);
int ReadLine(int, char *, int);
char *ReadPipe(char *, char *, char *, int, char **, int *);
int PipeCommand(char *, int *, int);
#else
int WaitForConnect();
int WriteBuffer();
char *ReadBuffer();
int ReadLine();
char *ReadPipe();
int PipeCommand();
#endif
