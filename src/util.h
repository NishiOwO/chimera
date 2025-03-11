/*
 * util.h
 *
 * Copyright (C) 1993, 1994, John Kilburg.
 *
 * See copyright.h for details.
 */

/*
 * Just in case.
 */
#ifndef L_tmpnam
#define L_tmpnam 1024
#endif

#ifndef _NO_PROTO
char *get_line(char *, char *, int);
char *alloc_string(char *);
char *realloc_mem(char *, int);
int mystrcmp(char *, char *);
int mystrncmp(char *, char *, int);
char *FixFilename(char *);
char *alloc_mem(int);
int SaveData(char *, int, char *);
void StartReaper();
char *mystrtok(char *, int, char **);
char *CreateCommandLine(char *, char *, char *);
#ifdef NEED_STRSTR
char *strstr(char *, char *);
#endif
#ifdef NEED_GETCWD
char *getcwd(char *, int);
#endif
#else
char *get_line();
char *alloc_string();
int mystrcmp();
int mystrncmp();
char *FixFilename();
char *alloc_mem();
char *realloc_mem();
int SaveData();
void StartReaper();
char *mystrtok();
char *CreateCommandLine();
#ifdef NEED_STRSTR
char *strstr();
#endif
#ifdef NEED_GETCWD
char *getcwd();
#endif
#endif
