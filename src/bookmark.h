/*
 * bookmark.h
 *
 * Copyright (C) 1993, 1994, John Kilburg.
 *
 * See copyright.h for details.
 */

typedef struct _bookmark
{
  char *url;
  char *display;
  struct _bookmark *next;
} Bookmark;

typedef struct _bookmarkgroup
{
  char *name;
  Bookmark *b;
  struct _bookmarkgroup *next;
} BookmarkGroup;

#ifndef _NO_PROTO
BookmarkGroup *ReadBookmarkFile(char *);
int WriteBookmarkFile(char *, BookmarkGroup *);
void AddBookmark(BookmarkGroup *, char *, char *, char *);
void RemoveBookmark(BookmarkGroup *, char *, int);
char **MakeBookmarkGroupList(BookmarkGroup *);
char **MakeBookmarkList(BookmarkGroup *, char *);
char *GetBookmarkURL(BookmarkGroup *, char *, int);
BookmarkGroup *RemoveBookmarkGroup(BookmarkGroup *, char *);
BookmarkGroup *AddBookmarkGroup(BookmarkGroup *, char *);
#else
BookmarkGroup *ReadBookmarkFile();
int WriteBookmarkFile();
void AddBookmark();
void RemoveBookmark();
char **MakeBookmarkGroupList();
char **MakeBookmarkList();
char *GetBookmarkURL();
BookmarkGroup *RemoveBookmarkGroup();
BookmarkGroup *AddBookmarkGroup();
#endif
