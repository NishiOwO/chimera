/*
 * bookmark.c
 *
 * Copyright (C) 1993, 1994, John Kilburg.
 *
 * See copyright.h for details.
 */
#include "copyright.h"

#include "options.h"

#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <sys/param.h>

#include "bookmark.h"
#include "util.h"

/*
 * DestroyBookmark
 *
 * Deallocate a bookmark
 */
void
DestroyBookmark(b)
Bookmark *b;
{
  if (b->url) free(b->url);
  if (b->display) free(b->display);
  free(b);

  return;
}

/*
 * CreateBookmark
 */
static Bookmark *
CreateBookmark(url, display)
char *url, *display;
{
  Bookmark *b;

  b = (Bookmark *)alloc_mem(sizeof(Bookmark));
  b->url = alloc_string(url);
  b->display = alloc_string(display);
  b->next = NULL;

  return(b);
}

/*
 * CreateBookmarkGroup
 */
static BookmarkGroup *
CreateBookmarkGroup(name)
char *name;
{
  BookmarkGroup *bg;

  bg = (BookmarkGroup *)alloc_mem(sizeof(BookmarkGroup));
  bg->name = alloc_string(name);
  bg->next = NULL;
  bg->b = NULL;

  return(bg);
}

/*
 * AddBookmark
 */
void
AddBookmark(bglist, group, url, display)
BookmarkGroup *bglist;
char *group;
char *url;
char *display;
{
  Bookmark *b;

  for (; bglist; bglist = bglist->next)
  {
    if (strcmp(bglist->name, group) == 0)
    {
      b = CreateBookmark(url, display);
      b->next = bglist->b;
      bglist->b = b;
      return;
    }
  }

  return;
}

/*
 * ReadBookmarkFile
 *
 * Read bookmarks from a file.
 */
BookmarkGroup *
ReadBookmarkFile(filename)
char *filename;
{
  FILE *fp;
  char *cp;
  char buffer[BUFSIZ];
  char url[BUFSIZ];
  char display[BUFSIZ];
  char group[BUFSIZ];
  int v2 = 0;
  BookmarkGroup *bglist = NULL, *bg = NULL, *tbg = NULL;
  Bookmark *b, *tb = NULL;

  filename = FixFilename(filename);
  if (filename == NULL) return(NULL);

  fp = fopen(filename, "r");
  if (fp == NULL)
  {
    bglist = CreateBookmarkGroup("default");
    return(bglist);
  }

  if (fgets(buffer, sizeof(buffer), fp) == NULL)
  {
    fclose(fp);
    bglist = CreateBookmarkGroup("default");
    return(bglist);
  }

  if (strncmp(buffer, "chimera_bookmarkv2", 18) == 0) v2 = 1;
  else
  {
    bglist = CreateBookmarkGroup("default");
    sscanf(buffer, "%s %[^\n]", url, display);
    tb = bglist->b = CreateBookmark(url, display);
  }

  while (fgets(buffer, sizeof(buffer), fp))
  {
    if (v2)
    {
      sscanf(buffer, "%s", group);
      if (strcmp(group, "group") == 0)
      {
	cp = (char *)strchr(buffer, ' ');
	if (cp != NULL && *cp != '\0')
	{
	  sscanf(++cp, "%[^\n]", display);
	  bg = CreateBookmarkGroup(display);
	  if (bglist == NULL) bglist = bg;
	  else tbg->next = bg;
	  tbg = bg;
	  tb = NULL;
	}
      }
      else if (strcmp(group, "mark") == 0 && bg != NULL)
      {
	cp = (char *)strchr(buffer, ' ');
	if (cp != NULL && *cp != '\0')
	{
	  sscanf(++cp, "%s %[^\n]", url, display);
	  b = CreateBookmark(url, display);
	  if (bg->b == NULL) bg->b = b;
	  else tb->next = b;
	  tb = b;
	}
      }
    }
    else
    {
      sscanf(buffer, "%s %[^\n]", url, display);
      b = CreateBookmark(url, display);
      tb->next = b;
      tb = b;
    }
  }

  fclose(fp);

  if (bglist == NULL) bglist = CreateBookmarkGroup("default");

  return(bglist);
}

/*
 * WriteBookmarkFile
 */
int
WriteBookmarkFile(filename, bglist)
char *filename;
BookmarkGroup *bglist;
{
  FILE *fp;
  Bookmark *b;

  filename = FixFilename(filename);
  if (filename == NULL) return(-1);

  fp = fopen(filename, "w");
  if (fp == NULL) return(-1);

  fprintf (fp, "chimera_bookmarkv2\n");

  for (; bglist; bglist = bglist->next)
  {
    fprintf (fp, "group %s\n", bglist->name);
    for (b = bglist->b; b; b = b->next)
    {
      fprintf (fp, "mark %s %s\n", b->url, b->display);
    }
    if (feof(fp))
    {
      fclose(fp);
      return(-1);
    }
  }

  fclose(fp);

  return(0);
}

/*
 * RemoveBookmark
 *
 * Removes a bookmark from the bookmark list.
 */
void
RemoveBookmark(bglist, group, index)
BookmarkGroup *bglist;
char *group;
int index;
{
  int i;
  Bookmark *b, *tb;

  for (; bglist; bglist = bglist->next)
  {
    if (strcmp(bglist->name, group) == 0)
    {
      for (i = 0, b = bglist->b, tb = NULL; b; tb = b, b = b->next, i++)
      {
	if (i == index)
	{
	  if (tb == NULL) bglist->b = b->next;
	  else tb->next = b->next;
	  DestroyBookmark(b);
	  return;
	}
      }
    }
  }

  return;
}

/*
 * MakeBookmarkGroupList
 *
 * Makes an array of char pointers to the list of bookmark groups.
 */
char **
MakeBookmarkGroupList(bglist)
BookmarkGroup *bglist;
{
  char **list;
  BookmarkGroup *bg;
  int count, i;

  for (count = 0, bg = bglist; bg; count++, bg = bg->next)
      ;

  if (count == 0)
  {
    list = (char **)alloc_mem(sizeof(char *));
    list[0] = NULL;
    return(list);
  }

  list = (char **)alloc_mem(sizeof(char *) * (count + 1));
  for (bg = bglist, i = 0; bg && i < count; i++, bg = bg->next)
  {
    list[i] = bg->name;
  }
  list[i] = NULL;

  return(list);
}

/*
 * MakeBookmarkList
 *
 * Makes an array of char pointers to the list of bookmarks.
 */
char **
MakeBookmarkList(bglist, group)
BookmarkGroup *bglist;
char *group;
{
  int i, count;
  BookmarkGroup *bg;
  Bookmark *b;
  char **list;

  for (bg = bglist; bg; bg = bg->next)
  {
    if (strcmp(bg->name, group) == 0) break;
  }

  if (bg == NULL)
  {
    list = (char **)alloc_mem(sizeof(char *));
    list[0] = NULL;
    return(list);
  }

  for (b = bg->b, count = 0; b; count++, b = b->next)
      ;

  if (count == 0)
  {
    list = (char **)alloc_mem(sizeof(char *));
    list[0] = NULL;
    return(list);
  }

  count += 1;
  list = (char **)alloc_mem(sizeof(char *) * count);

  for (i = 0, b = bg->b; b && i < count; i++, b = b->next)
  {
    list[i] = b->display;
  }
  list[i] = NULL;

  return(list);
}

/*
 * GetBookmarkURL
 *
 * Returns the URL for a bookmark.
 */
char *
GetBookmarkURL(bglist, group, index)
BookmarkGroup *bglist;
char *group;
int index;
{
  int i;
  Bookmark *b;
 
  for (i = 0; bglist; bglist = bglist->next)
  {
    if (strcmp(bglist->name, group) == 0)
    {
      for (b = bglist->b, i = 0; b; i++, b = b->next)
      {
	if (i == index) return(b->url);
      }
    }
  }
  
  return(NULL);
}

/*
 * RemoveBookmarkGroup
 */
BookmarkGroup *
RemoveBookmarkGroup(bglist, group)
BookmarkGroup *bglist;
char *group;
{
  Bookmark *b, *tb;
  BookmarkGroup *bg, *tbg;

  for (bg = bglist, tbg = NULL; bg; tbg = bg, bg = bg->next)
  {
    if (strcmp(bg->name, group) == 0)
    {
      for (b = bg->b; b; )
      {
	tb = b;
	b = b->next;
	DestroyBookmark(tb);
      }

      if (tbg == NULL) bglist = bg->next;
      else tbg->next = bg->next;
      if (bg->name != NULL) free(bg->name);
      free(bg);
      break;
    }
  }

  return(bglist);
}

/*
 * AddBookmarkGroup
 */
BookmarkGroup *
AddBookmarkGroup(bglist, group)
BookmarkGroup *bglist;
char *group;
{
  BookmarkGroup *bg;

  bg = CreateBookmarkGroup(group);
  bg->next = bglist;
  bglist = bg;

  return(bglist);
}
