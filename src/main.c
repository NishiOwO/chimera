/*
 * main.c
 *
 * Copyright (C) 1993, 1994, John Kilburg.
 *
 * See copyright.h for details.
 */
#include "copyright.h"

#include "options.h"

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/types.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <X11/Xaw/Form.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/Scrollbar.h>

#include <X11/Xaw/Cardinals.h>

#include <X11/cursorfont.h>

#include "HTML.h"

#include "url.h"
#include "mime.h"
#include "document.h"
#include "cache.h"
#include "bookmark.h"
#include "convert.h"
#include "filepop.h"
#include "strpop.h"
#include "lister.h"
#include "inline.h"
#include "util.h"
#include "http.h"
#include "input.h"
#include "net.h"
#include "lang.h"
#include "stringdb.h"

#include "widget.h"

/*
 * convenience routines.  the ifdef is bogus.
 */
#ifndef _NO_PROTO
static Document *LoadDoc(URLParts *, int, int);
static void HandleDoc(Document *, int);
static void AddMessage(char *);
static void DisplayCurrent(void);
static void SetTitle(char *);
static void SetURL(URLParts *);
static void SetSensitive(int);
static void MyXtSetSensitive(Widget, int);
int DisplayTransferStatus(int, int);
static void ListBookmark();
static void ListBookmarkGroup();
static void BookmarkAdd();
static void BookmarkGroupAdd();
static void AddMark();
static void AddGroup();
static void DeleteBookmark();
static void DeleteBookmarkGroup();
static void CancelLister();
static void sigigh_handler();
static void Dismiss();
static void Save();
static void Mail();
static void InputHandler();
#else
static Document *LoadDoc();
static void HandleDoc();
static void AddMessage();
static void DisplayCurrent();
static void SetTitle();
static void SetURL();
static void SetSensitive();
static void MyXtSetSensitive();
int DisplayTransferStatus();
static void ListBookmark();
static void ListBookmarkGroup();
static void BookmarkAdd();
static void BookmarkGroupAdd();
static void AddMark();
static void AddGroup();
static void DeleteBookmark();
static void DeleteBookmarkGroup();
static void CancelLister();
static void sigigh_handler();
static void Dismiss();
static void Save();
static void Mail();
static void InputHandler();
#endif

static void           HomeAction();
static void           BackAction();
static void           ReloadAction();
static void           HelpAction();
static void           QuitAction();
static void           SourceAction();
void                  OpenAction();
void                  FileAction();
void                  SearchAction();
static void           BookmarkAction();
static void           OpenURL();

static HTMLRoot root;

static char defaultTranslations[] =
"\
<Message>WM_PROTOCOLS:quit()\n\
<Key>h: home()\n\
<Key>u: back()\n\
<Key>l: reload()\n\
<Key>?: help()\n\
<Key>q: quit()\n\
<Key>d: source()\n\
<Key>o: open()\n\
<Key>f: file()\n\
<Key>s: search()\n\
<Key>m: bookmark()\n\
";

#define offset(field) XtOffset(HTMLRoot *, field)
static XtResource       resource_list[] =
{
  { "convertFiles", "ConvertFiles", XtRString, sizeof(char *),
	offset(convertFiles), XtRImmediate, (XtPointer)CONVERT_FILES },
  { "homeURL", "HomeURL", XtRString, sizeof(char *),
	offset(homeURL), XtRImmediate, (XtPointer)HOME_URL },
  { "helpURL", "HelpURL", XtRString, sizeof(char *),
	offset(helpURL), XtRImmediate, (XtPointer)HELP_URL },
  { "showURL", "ShowURL", XtRBoolean, sizeof(Boolean),
	offset(showURL), XtRString, (XtPointer)SHOW_URL },
  { "showTitle", "ShowTitle", XtRBoolean, sizeof(Boolean),
	offset(showTitle), XtRString, (XtPointer)SHOW_TITLE },
  { "button1Box", "Button1Box", XtRString, sizeof(char *),
	offset(button1Box), XtRImmediate, (XtPointer)"quit, open, home, back, source, reload, file, help, bookmark, search, cancel" },
  { "button2Box", "Button2Box", XtRString, sizeof(char *),
	offset(button2Box), XtRImmediate, (XtPointer)NULL },
  { "printerName", "PrinterName", XtRString, sizeof(char *),
	offset(printerName), XtRImmediate, (XtPointer)PRINTER_NAME },
  { "keyTrans", "KeyTrans", XtRString, sizeof(char *),
        offset(keyTrans), XtRImmediate, (XtPointer)defaultTranslations },
  { "cacheOff", "CacheOff", XtRBoolean, sizeof(Boolean),
	offset(cacheOff), XtRString, (XtPointer)CACHE_OFF },
  { "cacheDir", "CacheDir", XtRString, sizeof(char *),
	offset(cacheDir), XtRImmediate, (XtPointer)CACHE_DIR },
  { "cacheTTL", "CacheTTL", XtRInt, sizeof(int),
	offset(cacheTTL), XtRInt, (XtPointer) (XtPointer)&root.cacheTTL },
  { "cacheSize", "CacheSize", XtRInt, sizeof(int),
	offset(cacheSize), XtRInt, (XtPointer) &root.cacheSize },
  { "cacheClean", "CacheClean", XtRBoolean, sizeof(Boolean),
	offset(cacheClean), XtRString, (XtPointer)CACHE_CLEAN },
  { "mimeTypeFiles", "MIMETypeFiles", XtRString, sizeof(char *),
	offset(mimeTypeFiles), XtRString, (XtPointer)MIME_TYPE_FILES },
  { "mailCapFiles", "MailCapFiles", XtRString, sizeof(char *),
	offset(mailCapFiles), XtRString, (XtPointer)MAIL_CAPS },
  { "bookmarkFile", "BookmarkFile", XtRString, sizeof(char *),
	offset(bookmarkFile), XtRString, (XtPointer)BOOKMARK_FILE },
  { "protocolFiles", "ProtocolFiles", XtRString, sizeof(char *),
	offset(protocolFiles), XtRString, (XtPointer)PROTOCOL_FILES },
  { "statusUpdate", "StatusUpdate", XtRInt, sizeof(int),
	offset(statusUpdate), XtRInt, (XtPointer)&root.statusUpdate },
  { "path", "Path", XtRString, sizeof(char *),
	offset(path), XtRString, (XtPointer)PATH },
  { "inPort", "InPort", XtRInt, sizeof(int),
	offset(inPort), XtRString, (XtPointer)"0" },
  { "languageDB", "LanguageDB", XtRString, sizeof(char *),
	offset(languageDB), XtRString, (XtPointer)NULL },
  { "httpProxy", "HTTPProxy", XtRString, sizeof(char *),
	offset(httpProxy), XtRString, (XtPointer)NULL },
  { "gopherProxy", "GopherProxy", XtRString, sizeof(char *),
	offset(gopherProxy), XtRString, (XtPointer)NULL },
  { "ftpProxy", "FTPProxy", XtRString, sizeof(char *),
	offset(ftpProxy), XtRString, (XtPointer)NULL },
  { "waisProxy", "WAISProxy", XtRString, sizeof(char *),
	offset(waisProxy), XtRString, (XtPointer)NULL },
  { "newsProxy", "NewsProxy", XtRString, sizeof(char *),
	offset(newsProxy), XtRString, (XtPointer)NULL },
  { "nntpProxy", "NNTPProxy", XtRString, sizeof(char *),
	offset(nntpProxy), XtRString, (XtPointer)NULL },
  { "urnProxy", "URNProxy", XtRString, sizeof(char *),
	offset(urnProxy), XtRString, (XtPointer)NULL },
  { "noProxy", "NoProxy", XtRString, sizeof(char *),
	offset(noProxy), XtRString, (XtPointer)NULL },
  { "allProxy", "AllProxy", XtRString, sizeof(char *),
	offset(allProxy), XtRString, (XtPointer)NULL },
  { "maxColors", "MaxColors", XtRInt, sizeof(int),
	offset(maxColors), XtRInt, (XtPointer)&root.maxColors },
  { "cacheInfoFiles", "CacheInfoFiles", XtRString, sizeof(char *),
	offset(cacheInfoFiles), XtRString, (XtPointer)CACHE_INFO_FILES },
  { "cacheIgnoreExpires", "CacheIgnoreExpires", XtRBoolean, sizeof(Boolean),
	offset(cacheIgnoreExpires),XtRString,(XtPointer)CACHE_IGNORE_EXPIRES },
};

static XtActionsRec actionsList[] =
{
  { "home",     (XtActionProc) HomeAction },
  { "back",     (XtActionProc) BackAction },
  { "reload",   (XtActionProc) ReloadAction },
  { "help",     (XtActionProc) HelpAction },
  { "quit",     (XtActionProc) QuitAction },
  { "source",   (XtActionProc) SourceAction },
  { "open",     (XtActionProc) OpenAction },
  { "file",     (XtActionProc) FileAction },
  { "search",   (XtActionProc) SearchAction },
  { "bookmark", (XtActionProc) BookmarkAction },
  { "openurl",  (XtActionProc) OpenURL },
};

/*
 * main
 *
 * this is where the whole thing kicks off.
 */
void
main(argc, argv)
int argc;
char **argv;
{
  Arg args[2];
  char *first;
  Document *d;
  URLParts *up, *tup;
  URLParts *default_url;
  char *msg;
  XtPointer inputmask;
  int netfd;
  Colormap cmap;
  char *base_url;
  char path[MAXPATHLEN + 1];
  char *cwd;
  Atom delete;
  extern char *fallback_resources[];

#ifdef SOCKS
  SOCKSinit(argv[0]);
#endif

  root.maxColors = 256; /* why? */
  root.statusUpdate = STATUS_UPDATE;
  root.cacheSize = CACHE_SIZE;
  root.cacheTTL = CACHE_TTL;

  root.toplevel = XtAppInitialize(&(root.appcon), "Chimera", NULL, 0,
				  &argc, argv, fallback_resources, NULL, 0);
  if (root.toplevel == 0) exit(1);

  /*
   * Grab up the resources.
   */
  XtGetApplicationResources(root.toplevel, &root, resource_list,
			    XtNumber(resource_list), NULL, 0);

  /*
   * Init the root information.
   */
  if (argc > 1) first = argv[1];
  else if ((first = getenv("WWW_HOME")) == NULL) first = root.homeURL;
  first = alloc_string(first);

  /*
   * Initialize the user interface code global variables.
   */
  root.dlist = NULL;
  root.listerup = False;
  root.watch = XCreateFontCursor(XtDisplay(root.toplevel), XC_watch);
  root.left_ptr = XCreateFontCursor(XtDisplay(root.toplevel), XC_left_ptr);
  root.searchstr = NULL;
  root.loadstr = NULL;
  root.savestr = NULL;
  root.printerstr = NULL;
  root.mailstr = NULL;
  root.bookmarkstr = NULL;
  root.savedoc = NULL;
  root.rflag = False;
  root.bslist = NULL;
  root.bgslist = NULL;
  root.rlist = NULL;
  root.authdoc = NULL;

  root.clist = ReadConvertFiles(root.convertFiles);
  root.blist = ReadBookmarkFile(root.bookmarkFile);
  if (root.blist == NULL)
  {
    fprintf (stderr, "Could not read bookmarks.  Check your configuration\n");
  }
  else
  {
    root.group = alloc_string(root.blist->name);
  }
  root.mclist = ReadMailCapFiles(root.mailCapFiles);
  root.mtlist = ReadMIMETypeFiles(root.mimeTypeFiles);
  root.plist = ReadProtocolFiles(root.protocolFiles);

  AddLanguage(root.languageDB);

  if (root.httpProxy != NULL) AddToStringDB("http_proxy", root.httpProxy);
  if (root.gopherProxy != NULL) AddToStringDB("gopher_proxy",root.gopherProxy);
  if (root.ftpProxy != NULL) AddToStringDB("ftp_proxy", root.ftpProxy);
  if (root.waisProxy != NULL) AddToStringDB("wais_proxy", root.waisProxy);
  if (root.newsProxy != NULL) AddToStringDB("news_proxy", root.newsProxy);
  if (root.nntpProxy != NULL) AddToStringDB("nntp_proxy", root.nntpProxy);
  if (root.urnProxy != NULL) AddToStringDB("urn_proxy", root.urnProxy);
  if (root.noProxy != NULL) AddToStringDB("no_proxy", root.noProxy);
  if (root.allProxy != NULL) AddToStringDB("all_proxy", root.allProxy);

  /*
   * Huh?  This is goofy.  We are trying to trap a bunch of signals
   * in case the user has set cleanUp to True so that cache files can
   * be cleaned.
   */
  signal(SIGINT, sigigh_handler);
  signal(SIGQUIT, sigigh_handler);
  signal(SIGHUP, sigigh_handler);
  signal(SIGTERM, sigigh_handler);
  signal(SIGPIPE, SIG_IGN);

  /*
   * Setup the translations stuff for the keyboard shortcuts.
   */
  XtAppAddActions(root.appcon,
		  actionsList, XtNumber(actionsList));
  root.trans = XtParseTranslationTable(root.keyTrans);

  /*
   * this is hiding in widget.c.
   */
  CreateWidgets(&root);

  XtRealizeWidget(root.toplevel);

  XtSetArg(args[0], XtNbackground, &root.bgcolor.pixel);
  XtSetArg(args[1], XtNcolormap, &cmap);
  XtGetValues(root.w, args, 2);
  XQueryColor(XtDisplay(root.w), cmap, &root.bgcolor);

  StartReaper();

  if (!root.cacheOff)
  {
    InitCache(root.cacheDir, root.cacheSize, root.cacheTTL,
	      root.cacheClean ? 1:0, root.cacheIgnoreExpires ? 1:0,
	      root.cacheInfoFiles);
  }

  /*
   * Grab the first document.  First, build a fake context URL in case the
   * user provides a relative URL.  Create a new URL based on the user
   * supplied URL and then fake context and try to load it.  If that
   * fails then supply a document that is just an error message.
   */
  if ((cwd = getcwd(path, sizeof(path))) != NULL)
  {
    base_url = alloc_mem(16 + strlen(cwd) + 1 + 1);
    strcpy(base_url, "file://localhost");
    strcat(base_url, cwd);
    strcat(base_url, "/");
  }
  else base_url = alloc_string("file://localhost/");

  default_url = ParseURL(base_url);
  free(base_url);

  if (default_url == NULL)
  {
    fprintf (stderr, GetFromStringDB("crash"));
    exit(1);
  }

  up = ParseURL(first);
  free(first);

  if (up == NULL) tup = NULL;
  else
  {
    tup = MakeURLParts(up, default_url);
    DestroyURLParts(up);
    DestroyURLParts(default_url);
  }

  if (tup == NULL)
  {
    msg = GetFromStringDB("nofirst");
    d = BuildDocument(msg, strlen(msg), "text/html", 1, 0);
  }
  else
  {
    d = LoadDoc(tup, 0, 0);
    if (d == NULL)
    {
      msg = GetFromStringDB("nofirst");
      d = BuildDocument(msg, strlen(msg), "text/html", 1, 0);
    }

    DestroyURLParts(tup);
  }

  HandleDoc(d, 0);

  if (root.urldisplay != 0)
  {
    XtOverrideTranslations(root.urldisplay,
			   XtParseTranslationTable("<Key>Return: openurl()"));
  }

  if (root.inPort > 0)
  {
    netfd = net_bind(root.inPort);
    if (netfd > 0)
    {
      inputmask = (XtPointer)XtInputReadMask;
      XtAppAddInput(root.appcon, netfd, inputmask, InputHandler, 0);
    }
  }

  delete = XInternAtom(XtDisplay(root.toplevel), "WM_DELETE_WINDOW", False);
  XSetWMProtocols (XtDisplay(root.toplevel), XtWindow(root.toplevel),
		   &delete, 1);

  XtAppMainLoop(root.appcon);
}

/*
 * InputHandler
 *
 * Handle incoming network connections
 */
static void
InputHandler(cldata, netfd, xid)
XtPointer cldata;
int *netfd;
XtInputId *xid;
{
  int fd;
  Document *d;

  fd = net_accept(*netfd);
  if (fd < 0) return;

  d = (Document *)ParseHTTPRequest(fd);

  close(fd);

  if (d != NULL)
  {
    if (d->location != NULL)
    {
      URLParts *up;
      
      up = ParseURL(d->location);
      
      DestroyDocument(d);
      
      if (up != NULL && IsAbsoluteURL(up))
      {
	d = LoadDoc(up, 0, 0);
	DestroyURLParts(up);
      }
    }

    if (d != NULL)
    {
      if (d->nothing != 0) DestroyDocument(d);
      else HandleDoc(d, 0);
    }
  }

  return;
}

/*
 * SetTitle
 *
 * Sets the string in the title widget.  Passing a NULL as a string
 * makes the title of the current document the title.
 */
static void
SetTitle(title)
char *title;
{
  Arg args[1];
  String t;
  char *cp;
  char *titlestr = NULL;

  if (root.showTitle)
  {
    if (title == NULL)
    {
      XtSetArg(args[0], WbNtitleText, &t);
      XtGetValues(root.w, args, 1);
      
      if (t == NULL) title = "";
      else title = t;
    }

    titlestr = alloc_string(title);
    for (cp = titlestr; *cp; cp++)
    {
      if (*cp == '\n') *cp = ' ';
    }

    XtSetArg(args[0], XtNstring, titlestr);
    XtSetValues(root.titledisplay, args, 1);

    free(titlestr);

    XFlush(XtDisplay(root.w));
  }

  return;
}

/*
 * SetURL
 *
 * Change the URL display
 */
static void
SetURL(up)
URLParts *up;
{
  Arg args[1];

  if (root.showURL)
  {
    char *url, *t = NULL;

    if (up == NULL) url = "";
    else t = url = MakeURL(up);
    XtSetArg(args[0], XtNstring, url);
    XtSetValues(root.urldisplay, args, 1);
    if (t != NULL) free(t);

    XFlush(XtDisplay(root.w));
  }

  return;
}

/*
 * DisplayCurrent
 *
 * Displays the current HTML screen.  It also changes the title display
 * and the URL display.
 */
static void
DisplayCurrent()
{
  Document *d;
  DocNode *t;
  Arg args[2];
  Widget view;
  char *anchor;

  if (root.dlist == NULL)
  {
    AddMessage("nofirst");
    return;
  }

  XDefineCursor(XtDisplay(root.toplevel), XtWindow(root.toplevel), root.watch);
  
  /*
   * Give the new scrollbar position.
   */
  XtSetArg(args[0], WbNverticalScrollBarPos, root.dlist->vpos);
  XtSetArg(args[1], WbNhorizontalScrollBarPos, root.dlist->hpos);
  XtSetValues(root.w, args, 2);

  SetTitle(GetFromStringDB("display"));
  SetURL(root.dlist->up);

  /*
   * Try to display the current document.  If there isn't one, see if the
   * URL is still around.  If not, try to load the previous document.
   */
  d = root.dlist->doc;
  if (d == NULL)
  {
    if (root.dlist->up != NULL)
    {
      d = LoadDoc(root.dlist->up, 0, 0);
      DestroyURLParts(root.dlist->up);
      if (d->up) root.dlist->up = DupURLParts(d->up);
      root.dlist->doc = d;
    }
    else
    {
      t = root.dlist;
      root.dlist = root.dlist->next;
      if (t->base != NULL) free(t->base);
      free(t);
      DisplayCurrent();
      return;
    }
  }

  /*
   * This should release the memory used for the images on the last page.
   */
  HTMLFreeImageInfo(root.w);

  root.cancelop = False;

  if (d->up != NULL && d->up->anchor != NULL) anchor = d->up->anchor;
  else anchor = NULL;

  if (d->pcontent != NULL && mystrcmp(d->pcontent, "text/html") == 0)
  {
    HTMLSetText(root.w, d->ptext, NULL, NULL, 0, anchor, NULL);
  }
  else
  {
    HTMLSetText(root.w, d->text, NULL, NULL, 0, anchor, NULL);
  }

  root.cancelop = False;

  /*
   * Screw around with the translations, cursor, URL display and the
   * title display.
   */
  XtSetArg(args[0], WbNview, &view);
  XtGetValues(root.w, args, 1);
  XtOverrideTranslations(view, root.trans);

  XDefineCursor(XtDisplay(root.toplevel), XtWindow(root.toplevel),
		root.left_ptr);
  
  SetTitle(NULL);
  SetURL(root.dlist->up);
  SetSensitive(True);

  return;
}

/*
 * LoadDoc
 *
 */
static Document *
LoadDoc(up, reload, cacheonly)
URLParts *up;
int reload;
int cacheonly;
{
  Document *d;
  URLParts *tup = NULL, *nup = NULL;

  if (root.dlist != NULL && reload == 0)
  {
    if (root.dlist->base != NULL)
    {
      URLParts *kup;

      kup = ParseURL(root.dlist->base);
      if (kup != NULL)
      {
	tup = nup = MakeURLParts(up, kup);
	DestroyURLParts(kup);
      }
      else nup = up;
    }
    else if (root.dlist->up != NULL)
    {
      tup = nup = MakeURLParts(up, root.dlist->up);
    }
    else nup = up;
  }
  else nup = up;

  if (!root.cacheOff && !reload) d = ReadCache(nup);
  else d = NULL;
  if (d == NULL && !cacheonly)
  {
    XDefineCursor(XtDisplay(root.toplevel), XtWindow(root.toplevel),
                  root.watch);
  
    SetURL(nup);
    SetTitle(GetFromStringDB("download"));
    
    d = LoadDocument(nup, root.plist, root.mtlist, reload);
    SetTitle("");

    XDefineCursor(XtDisplay(root.toplevel), XtWindow(root.toplevel),
		  root.left_ptr);
  }

  if (tup != NULL) DestroyURLParts(tup);

  return(d);
}

/*
 * Password
 *
 * Called when the user enters a string in the password popup.
 */
static void
Password(password)
char *password;
{
  Document *d;
  RealmInfo *r;
  Document *t;

  if (password == NULL) password = "";

  root.rflag = True;

  t = root.authdoc;
  root.authdoc = NULL;
  t->up->password = alloc_string(password);
  t->up->auth_type = alloc_string(t->auth_type);

  for (r = root.rlist; r; r = r->next)
  {
    if (mystrcmp(t->auth_type, r->up->auth_type) == 0 &&
	mystrcmp(t->auth_realm, r->name) == 0 &&
	mystrcmp(t->up->protocol, r->up->protocol) == 0 &&
	mystrcmp(t->up->hostname, r->up->hostname) == 0 &&
	t->up->port == r->up->port)
    {
      if (strcmp(t->up->username, r->up->username) != 0 ||
	  strcmp(t->up->password, r->up->password) != 0)
      {
	DestroyURLParts(r->up);
	r->up = DupURLParts(t->up);
      }
      break;
    }
  }

  if (r == NULL)
  {
    r = (RealmInfo *)alloc_mem(sizeof(RealmInfo));
    r->name = alloc_string(t->auth_realm);
    r->up = DupURLParts(t->up);
    r->next = root.rlist;
    root.rlist = r;
  }

  d = LoadDoc(t->up, 1, 0);
  HandleDoc(d, 0);

  DestroyDocument(t);

  SetSensitive(True);

  return;
}

/*
 * Username
 *
 * Called when the user enters a string in the username popup
 */
static void
Username(username)
char *username;
{
  char *msg = GetFromStringDB("enterpw");

  root.authdoc->up->username = alloc_string(username);

  StringPopup(root.w, Password, "", msg, Dismiss, 1, 1);

  return;
}

/*
 * CheckRealm
 */
static RealmInfo *
CheckRealm(d)
Document *d;
{
  RealmInfo *r;

  for (r = root.rlist; r; r = r->next)
  {
    if (mystrcmp(d->auth_type, r->up->auth_type) == 0 &&
	mystrcmp(d->auth_realm, r->name) == 0 &&
	mystrcmp(d->up->protocol, r->up->protocol) == 0 &&
	mystrcmp(d->up->hostname, r->up->hostname) == 0 &&
	d->up->port == r->up->port)
    {
      if (d->up->username != NULL) free(d->up->username);
      d->up->username = alloc_string(r->up->username);
      if (d->up->password != NULL) free(d->up->password);
      d->up->password = alloc_string(r->up->password);
      if (d->up->auth_type != NULL) free(d->up->auth_type);
      d->up->auth_type = alloc_string(r->up->auth_type);

      return(r);
    }
  }    

  return(NULL);
}

/*
 * HandleDoc
 */
static void
HandleDoc(d, download)
Document *d;
int download;
{
  Arg args[2];
  DocNode *dn = NULL;
  RealmInfo *r;
  Document *x;

  if (d->nothing != 0)
  {
    DestroyDocument(d);
    return;
  }

  /*
   * This is where chimera figures out whether or not authentication
   * information is needed.
   */
  if (d->up != NULL && d->status == 401)
  {
    if ((r = CheckRealm(d)) != NULL)
    {
      x = LoadDoc(d->up, 1, 0);
      if (x->status == 401)
      {
	r = NULL;
	DestroyDocument(x);
      }
      else HandleDoc(x, download);
    }

    if (r == NULL)
    {
      char *msg = GetFromStringDB("enterusername");
      root.authdoc = d;
      StringPopup(root.w, Username, "", msg, Dismiss, 1, 0);
      SetSensitive(False);
    }
    else DestroyDocument(d);

    return;
  }

  /*
   * Convert needs to be called in case there is a transfer or content
   * encoding.
   */
  if (d->ptext == NULL)
  {
    /*
     * This frightens me.  I know it is bad but I can't think of anything
     * I'd rather do right now.
     */
    x = ConvertDocument(d, root.clist, NULL, root.path);
    if (x != d) HandleDoc(x, 0);
  }
  
  if (!root.cacheOff) WriteCache(d);

  /*
   * Save the old scrollbar position.
   */
  if (root.dlist)
  {
    XtSetArg(args[0], WbNhorizontalScrollBarPos, &(root.dlist->hpos));
    XtSetArg(args[1], WbNverticalScrollBarPos, &(root.dlist->vpos));
    XtGetValues(root.w, args, 2);
  }

  if (download || (d->content == NULL && d->pcontent == NULL))
  {
    root.savedoc = d;
    StringPopup(root.w, Save, root.savestr,
		GetFromStringDB("enterfilename"), Dismiss, 1, 0);
  }
  else if ((d->content != NULL && mystrcmp(d->content, "text/html") == 0) ||
	   (d->pcontent != NULL && mystrcmp(d->pcontent, "text/html") == 0))
  {
    dn = (DocNode *)alloc_mem(sizeof(DocNode));
    dn->vpos = 0;
    dn->hpos = 0;
    dn->doc = d;
    dn->base = NULL;
    if (d->up != NULL) dn->up = DupURLParts(d->up);
    else dn->up = NULL;
    dn->next = root.dlist;
    root.dlist = dn;
  }
  else
  {
    char filename[L_tmpnam + 1];

    if (SaveData(d->text, d->len, tmpnam(filename)) != -1)
    {
      if (DisplayExternal(filename, d->content, root.path, root.mclist))
      {
	root.savedoc = d;
	StringPopup(root.w, Save, root.savestr,
		    GetFromStringDB("enterfilename"), Dismiss, 1, 0);
      }
      else
      {
	DestroyDocument(d);
	d = NULL;
      }
    }
    else
    {
      char *msg = GetFromStringDB("notsaveext");

      DestroyDocument(d);

      d = BuildDocument(msg, strlen(msg), "text/html", 1, 0);
      HandleDoc(d, 0);
      return;
    }
  }

  DisplayCurrent();

  return;
}

/*
 * AddMessage
 */
static void
AddMessage(text)
char *text;
{
  Document *d;
  char *value;

  value = GetFromStringDB(text);
  d = BuildDocument(value, strlen(value), "text/html", 1, 0);
  HandleDoc(d, 0);

  return;
}

/*
 * Anchor
 *
 * Called when the user clicks on an anchor
 */
void
Anchor(w, cldata, c)
Widget w;
XtPointer cldata;
WbAnchorCallbackData *c;
{
  URLParts *tup;
  Document *d;

  if (c == NULL || c->href == NULL) return;

  if (c->event)
  {
    tup = ParseURL(c->href);
    if (tup == NULL) return;

    switch (c->event->xbutton.button)
    {
      case 2:
        d = LoadDoc(tup, 0, 0);
	HandleDoc(d, 1);
        break;

      default:
	if (c->href[0] == '#')
	{
	  int id;
	  
	  id = HTMLAnchorToId(root.w, c->href + 1);
	  if (id > 0) HTMLGotoId(root.w, id);
	}
	else
	{
	  d = LoadDoc(tup, 0, 0);
	  HandleDoc(d, 0);
	}
    }

    DestroyURLParts(tup);
  }

  return;
}

/*
 * OpenDocument
 *
 * Called when the user clicks on the open button.
 *
 * This function grabs the text in the text widget which it assumes to
 * be a URL and tries to access the document.  It will not handle local
 * files.
 */
void
OpenDocument(url)
char *url;
{
  URLParts *up;
  Document *d;

  if (NullString(url))
  {
    AddMessage("emptyurl");
  }
  else
  {
    if (root.loadstr) free(root.loadstr);
    root.loadstr = alloc_string(url);

    up = ParseURL(url);
    if (up != NULL)
    {
      d = LoadDoc(up, 0, 0);
      HandleDoc(d, 0);
      DestroyURLParts(up);
    }
    else AddMessage("invalidurl");
  }

  return;
}

/*
 * OpenURL
 *
 * Called when the user presses RETURN in the urldisplay
 */
static void
OpenURL()
{
  Arg args[1];
  String s;
  Document *d;
  URLParts *up;

  XtSetArg(args[0], XtNstring, &s);
  XtGetValues(root.urldisplay, args, 1);

  /*
   * Make sure that the user has entered something in the text widget
   */
  if (s == NULL || s[0] == '\0')
  {
    AddMessage("emptyurl");
  }
  else
  {
    if (root.loadstr) free(root.loadstr);
    root.loadstr = alloc_string(s);

    up = ParseURL(s);
    if (up != NULL)
    {
      d = LoadDoc(up, 0, 0);
      HandleDoc(d, 0);
      DestroyURLParts(up);
    }
    else AddMessage("invalidurl");
  }

  return;
}

void
OpenAction()
{
  char *msg = GetFromStringDB("enterurl");

  StringPopup(root.load, OpenDocument, root.loadstr, msg, Dismiss, 1, 0);

  return;
}

/*
 * Home
 *
 * Called when the user clicks on the home button.
 *
 * The equivalent of clicking on "Back" until the first document is
 * visible.
 */
void
Home(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  DocNode *c, *t;

  c = root.dlist;
  if (c)
  {
    if (c->next != NULL) DestroyDocument(c->doc);

    while (c->next)
    {
      t = c;
      c = c->next;
      if (t->base != NULL) free(t->base);
      if (t->up != NULL) DestroyURLParts(t->up);
      free(t);
    }
    root.dlist = c;
    DisplayCurrent();
  }

  return;
}

static void
HomeAction()
{
  Arg args[1];
  Boolean sensitive;

  XtSetArg(args[0], XtNsensitive, &sensitive);
  XtGetValues(root.home, args, 1);
  if (sensitive) Home(root.home, &root, NULL);

  return;
}

/*
 * Dismiss
 *
 * Called when the dismiss button is pressed.
 */
static void
Dismiss()
{
  if (root.savedoc != NULL)
  {
    DestroyDocument(root.savedoc);
    root.savedoc = NULL;
  }

  if (root.authdoc != NULL)
  {
    DestroyDocument(root.authdoc);
    root.authdoc = NULL;
  }

  SetSensitive(True);

  return;
}

/*
 * Save
 *
 * Eventually called by the popup.c code.
 */
static void
Save(filename, otype)
char *filename;
int otype;
{
  char *text = NULL, *data;
  int len;

  /*
   * Make sure that the user has entered something in the text widget
   */
  if (NullString(filename))
  {
    AddMessage("emptyfilename");
    if (root.savedoc != NULL)
    {
      DestroyDocument(root.savedoc);
      root.savedoc = NULL;
    }
  }
  else
  {
    if (root.savestr) free(root.savestr);
    root.savestr = alloc_string(filename);

    if (root.savedoc != NULL)
    {
      data = root.savedoc->text;
      len = root.savedoc->len;
    }
    else if (otype == 3)
    {
      data = root.dlist->doc->text;
      len = root.dlist->doc->len;
    }
    else
    {
      data = text = HTMLGetText(root.w, otype);
      if (text == NULL) return;
      len = strlen(data);
    }  
    filename = FixFilename(filename); 
    if (filename == NULL)
    {
      AddMessage("invalidfilename");
      if (text != NULL) free(text);
      if (root.savedoc != NULL)
      {
	DestroyDocument(root.savedoc);
	root.savedoc = NULL;
      }
      return;
    }

    if (SaveData(data, len, filename) == -1)
    {
      AddMessage("notsaved");
      if (text != NULL) free(text);
      if (root.savedoc != NULL)
      {
	DestroyDocument(root.savedoc);
	root.savedoc = NULL;
      }
      return;
    }

    if (text != NULL) free(text);
    if (root.savedoc != NULL)
    {
      DestroyDocument(root.savedoc);
      root.savedoc = NULL;
    }
  }

  return;
}

/*
 * Back
 *
 * Called when the user clicks on the back button.  It moves back one
 * HTML frame.
 */
void
Back(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  DocNode *t;

  if (root.dlist->next == NULL) return;

  t = root.dlist;
  root.dlist = root.dlist->next;

  if (t->doc != NULL) DestroyDocument(t->doc);
  if (t->base != NULL) free(t->base);
  if (t->up != NULL) DestroyURLParts(t->up);
  free(t);

  DisplayCurrent();

  return;
}

static void
BackAction()
{
  Arg args[1];
  Boolean sensitive;

  XtSetArg(args[0], XtNsensitive, &sensitive);
  XtGetValues(root.back, args, 1);
  if (sensitive) Back(root.back, &root, NULL);

  return;
}

/*
 * Reload
 */
void
Reload(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  DocNode *t;
  Document *d;

  root.rflag = True;

  t = root.dlist;
  root.dlist = root.dlist->next;

  d = LoadDoc(t->up, 1, 0);
  HandleDoc(d, 0);

  if (t->doc != NULL) DestroyDocument(t->doc);
  if (t->up != NULL) DestroyURLParts(t->up);
  if (t->base != NULL) free(t->base);
  free(t);

  root.rflag = False;

  return;
}

static void
ReloadAction()
{
  Reload(root.reload, &root, NULL);
  return;
}

/*
 * Cancel
 */
void
Cancel(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  return;
}

/*
 * Help
 *
 * Shows the help file.
 */
void
Help(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  URLParts *up;
  Document *d;

  up = ParseURL(root.helpURL);
  if (up != NULL)
  {
    d = LoadDoc(up, 0, 0);
    HandleDoc(d, 0);
    DestroyURLParts(up);
  }
  else AddMessage("invalidurl");

  return;
}

static void
HelpAction()
{
  Help(root.help, &root, NULL);
  return;
}

/*
 * Search
 *
 * Called when the user clicks on the Search button
 */
void
Search(ss)
char *ss;
{
  static ElementRef start, end;
  Boolean wrap;

  if (NullString(ss))
  {
    AddMessage("emptystring");
  }
  else
  {
    if (root.searchstr != NULL)
    {
      if (mystrcmp(root.searchstr, ss) != 0)
      {
	start.id = 0;
	free(root.searchstr);
	root.searchstr = alloc_string(ss);
      }
      else
      {
	start.pos = end.pos;
      }
    }
    else
    {
      root.searchstr = alloc_string(ss);
      start.id = 0;
      start.pos = 0;
    }

    do
    {
      wrap = False;
      if (HTMLSearchText(root.w, ss, &start, &end, 0, 1) == 1)
      {
	HTMLSetSelection(root.w, &start, &end);
	HTMLGotoId(root.w, start.id);
      }
      else
      {
	if (start.pos == 0)
	{
	  AddMessage("searchfailed");
	}
	else
	{
	  start.id = 0;
	  start.pos = 0;
	  wrap = True;
	}
      }
    } while (wrap);
  }

  return;
}

void
SearchAction()
{
  char *value;

  value = GetFromStringDB("entersearch");
  StringPopup(root.search, Search, root.searchstr, value, Dismiss, 0, 0);

  return;
}

/*
 * Print
 *
 * Called when the user clicks on the print button.
 */
static void
Print(printer, otype)
char *printer;
int otype;
{
  char *text;
  int len;
  FILE *pp;
  char *prncmd;

  if (NullString(printer))
  {
    AddMessage("emptyprinter");
    return;
  }

  if (root.printerstr) free(root.printerstr);
  root.printerstr = alloc_string(printer);
  
  if (otype == 3)
  {
    text = root.dlist->doc->text;
    len = root.dlist->doc->len;
  }
  else
  {
    text = HTMLGetText(root.w, otype);
    if (text == NULL) return;
    len = strlen(text);
  }

  prncmd = alloc_mem(strlen(PRINT_COMMAND) + strlen(printer) + 1);
  sprintf (prncmd, PRINT_COMMAND, printer);


  pp = popen(prncmd, "w");
  free(prncmd);
  if (pp == NULL)
  {
    AddMessage("notppipe");
    if (otype != 3) free(text);
    return;
  }

  if (fwrite(text, 1, len, pp) < len)
  {
    AddMessage("notpdata");
    if (otype != 3) free(text);
    pclose(pp);
    return;
  }

  pclose(pp);
  if (otype != 3) free(text);

  return;
}

/*
 * Mail
 *
 * Called when the user clicks on the mail button.
 */
static void
Mail(email, otype)
char *email;
int otype;
{
  char *text;
  int len;
  FILE *pp;
  char *emailcmd;

  if (NullString(email))
  {
    AddMessage("emptyemail");
    return;
  }

  if (root.mailstr) free(root.mailstr);
  root.mailstr = alloc_string(email);
  
  if (otype == 3)
  {
    text = root.dlist->doc->text;
    len = root.dlist->doc->len;
  }
  else
  {
    text = HTMLGetText(root.w, otype);
    if (text == NULL) return;
    len = strlen(text);
  }

  emailcmd = alloc_mem(strlen(EMAIL_COMMAND) + strlen(email) + 1);
  sprintf (emailcmd, EMAIL_COMMAND, email);

  pp = popen(emailcmd, "w");
  free(emailcmd);
  if (pp == NULL)
  {
    AddMessage("notepipe");
    if (otype != 3) free(text);
    return;
  }

  if (fwrite(text, 1, len, pp) < len)
  {
    AddMessage("notedata");
    if (otype != 3) free(text);
    pclose(pp);
    return;
  }

  pclose(pp);
  if (otype != 3) free(text);

  return;
}

void
FileAction()
{
  if (NullString(root.printerstr))
  {
    if (getenv("PRINTER")) root.printerstr = alloc_string(getenv("PRINTER"));
    else root.printerstr = alloc_string(root.printerName);
  }

  FilePopup(root.file,
	    Save, root.savestr,
	    Print, root.printerstr,
	    Mail, root.mailstr,
	    Dismiss);

  return;
}

/*
 * sigiqh_handler
 */
static void
sigigh_handler()
{
  Quit(root.quit, &root, NULL);

  return;
}

/*
 * Quit
 *
 * Called when the user clicks on the quit button
 */
void
Quit(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  if (root.dlist && root.dlist->doc) DestroyDocument(root.dlist->doc);

  if (!root.cacheOff) CleanCache();

  exit(0);
}

static void
QuitAction()
{
  Quit(root.quit, &root, NULL);
  return;
}

/*
 * Source
 *
 * Called when the user clicks on the source button.  Allocates yet more
 * memory to put the current HTML in a document with <plaintext> at the
 * top.
 */
void
Source(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  Document *d;

  if (root.dlist != NULL && root.dlist->doc != NULL &&
      root.dlist->doc->text != NULL)
  {
    d = BuildDocument(root.dlist->doc->text, strlen(root.dlist->doc->text),
		   "text/plain", 1, 0);
    HandleDoc(d, 0);
  }

  return;
}

static void
SourceAction()
{
  Source(root.source, &root, NULL);
  return;
}

/*
 * CancelLister
 */
static void
CancelLister(w, listw, cbdata)
Widget w;
Widget listw;
XtPointer cbdata;
{
  DestroyLister();
  root.listerup = False;
  SetSensitive(True);

  return;
}

/*
 * DoLister
 *
 * Cranks up a lister and sets up the callback.
 */
void
DoLister(w, cbdata)
Widget w;
XtPointer cbdata;
{
  char *value;
  static ListCommand clist[] =
  {
    { "listok", ListBookmark },
    { "listcancel", CancelLister },
    { "listadd", BookmarkAdd },
    { "listdelete", DeleteBookmark },
    { NULL, NULL },
  };
  static ListCommand gclist[] =
  {
    { "groupok", ListBookmarkGroup },
    { "groupadd", BookmarkGroupAdd },
    { "groupdel", DeleteBookmarkGroup },
    { NULL, NULL },
  };

  if (root.blist == NULL)
  {
    AddMessage("bookconfig");
    return;
  }

  if (root.listerup == True)
  {
    RaiseLister();
    return;
  }

  if (root.bslist != NULL) free(root.bslist);
  root.bslist = MakeBookmarkList(root.blist, root.group);

  if (root.bgslist != NULL) free(root.bgslist);
  root.bgslist = MakeBookmarkGroupList(root.blist);

  root.listerup = True;
  SetSensitive(True);

  value = GetFromStringDB("Bookmarks");
  CreateLister(value, w, clist, root.bslist, gclist, root.bgslist, NULL);

  return;
}

/*
 * Link
 *
 * The link callback
 */
void
LinkCB(w, cldata, cbdata)
Widget w;
XtPointer cldata;
XtPointer cbdata;
{
  LinkInfo *li = (LinkInfo *)cbdata;

  if (li != NULL && li->href != NULL)
      root.dlist->base = alloc_string(li->href);
  else root.dlist->base = NULL;

  return;
}

/*
 * ImageResolve
 *
 * Called by the HTML widget to turn an image into something the widget
 * understands.  This treats images like documents.
 */
ImageInfo *
ImageResolve(w, url, delay)
Widget w;
char *url;
int delay;
{
  Document *d, *x;
  URLParts *tup;
  ImageInfo *i = NULL;
  Display *dpy;
  int screen;
  Visual *v;
  int depth;
  Arg args[1];
  int stype;
  RealmInfo *r;

  if (root.cancelop || url == NULL)
  {
    root.cancelop = False;
    return(NULL);
  }

  tup = ParseURL(url);
  if (tup == NULL) return(NULL);

  if (delay) d = LoadDoc(tup, 0, 1);
  else d = LoadDoc(tup, 0, 0);

  DestroyURLParts(tup);

  if (d == NULL) return(NULL);

  if (d->status == 401 && (r = CheckRealm(d)) != NULL)
  {
    x = LoadDoc(d->up, 0, 0);
    DestroyDocument(d);
    d = x;
  }

  if (!root.cacheOff) WriteCache(d);

  /*
   * Figure out if the screen is color, grayscale, or monochrome.
   */
  XtSetArg(args[0], XtNdepth, &depth);
  XtGetValues(root.toplevel, args, 1);
    
  if (depth > 1)
  {
    dpy = XtDisplay(root.toplevel);
    screen = DefaultScreen(dpy);
    v = DefaultVisual(dpy, screen);
    switch(v->class)
    {
      case StaticColor:
      case DirectColor:
      case TrueColor:
      case PseudoColor:
        stype = COLOR_DISPLAY;
	break;
      case StaticGray:
      case GrayScale:
	stype = GRAY_DISPLAY;
	break;
      default:
	stype = MONO_DISPLAY;
	break;
    }
  }
  else
  {
    stype = MONO_DISPLAY;
  }

  i = CreateImageInfo(root.toplevel, d, root.clist, root.path,
		      stype, root.maxColors, &root.bgcolor);

  DestroyDocument(d);

  return(i);
}

/*
 * DelayedImageResolve
 *
 * Called when the user clicks on a delayed image.
 */
ImageInfo *
DelayedImageResolve(w, url)
Widget w;
char *url;
{
  ImageInfo *i = NULL;
 
  XDefineCursor(XtDisplay(root.toplevel),
		XtWindow(root.toplevel), root.watch);

  i = ImageResolve(w, url, 0);

  XDefineCursor(XtDisplay(root.toplevel),
		XtWindow(root.toplevel), root.left_ptr);

  return(i);
}

/*
 * AddGroup
 *
 * Add Bookmark Group
 */
static void
AddGroup(group)
char *group;
{
  char *cp;

  if (NullString(group))
  {
    AddMessage("emptygroup");
    return;
  }

  for (cp = group; *cp; cp++)
  {
    if (isspace(*cp))
    {
      AddMessage("nowhite");
      return;
    }
  }

  root.blist = AddBookmarkGroup(root.blist, group);

  if (root.bslist != NULL) free(root.bslist);
  root.bslist = MakeBookmarkList(root.blist, group);

  if (root.bgslist != NULL) free(root.bgslist);
  root.bgslist = MakeBookmarkGroupList(root.blist);

  ChangeLister(root.bslist, root.bgslist);

  if (root.group != NULL) free(root.group);
  root.group = alloc_string(group);

  return;
}

/*
 * BookmarkGroupAdd
 */
static void
BookmarkGroupAdd()
{
  char *value;

  value = GetFromStringDB("entergroup");
  StringPopup(root.bookmark, AddGroup, NULL, value, Dismiss, 1, 0);

  return;
}

/*
 * BookmarkAdd
 */
static void
BookmarkAdd()
{
  Arg args[1];
  char *t;
  char *value;

  if (root.blist == NULL)
  {
    AddMessage("onegroup");
    return;
  }

  XtSetArg(args[0], WbNtitleText, &t);
  XtGetValues(root.w, args, 1);

  value = GetFromStringDB("enterbookmark");
  StringPopup(root.bookmark, AddMark, t, value, Dismiss, 1, 0);

  return;
}

static void
BookmarkAction()
{
  DoLister(root.bookmark, &root, NULL);

  return;
}

/*
 * AddMark
 *
 * Adds the current document to the bookmark list.  This is a callback
 * for a command widget.  Be careful.
 */
static void
AddMark(bookmark)
char *bookmark;
{
  if (NullString(bookmark))
  {
    AddMessage("emptybookmark");
    return;
  }

  AddBookmark(root.blist, root.group, MakeURL(root.dlist->up), bookmark);

  if (root.bslist != NULL) free(root.bslist);
  root.bslist = MakeBookmarkList(root.blist, root.group);

  if (root.bgslist != NULL) free(root.bgslist);
  root.bgslist = MakeBookmarkGroupList(root.blist);

  ChangeLister(root.bslist, root.bgslist);

  if (WriteBookmarkFile(root.bookmarkFile, root.blist) != 0)
  {
    AddMessage("savebook");
  }

  return;
}

/*
 * ListBookmarkGroup
 *
 * Selects the group picked by the user.
 */
static void
ListBookmarkGroup(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  XawListReturnStruct *lr;

  lr = XawListShowCurrent((Widget)cldata);
  if (lr != NULL)
  {
    if (root.bslist != NULL) free(root.bslist);
    root.bslist = MakeBookmarkList(root.blist, lr->string);
    
    if (root.bgslist != NULL) free(root.bgslist);
    root.bgslist = MakeBookmarkGroupList(root.blist);
    
    ChangeLister(root.bslist, root.bgslist);

    if (root.group) free(root.group);
    root.group = alloc_string(lr->string);
  }

  return;
}

/*
 * ListBoomark
 *
 * Goes to the bookmark selected by the user.  This is actually a
 * callback for a command widget.  See lister.c.
 */
static void
ListBookmark(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  Document *d;
  XawListReturnStruct *lr;
  char *url;

  lr = XawListShowCurrent((Widget)cldata);
  if (lr != NULL)
  {
    url = GetBookmarkURL(root.blist, root.group, lr->list_index);
    if (url != NULL)
    {
      URLParts *up;
      
      up = ParseURL(url);
      if (up != NULL)
      {
	d = LoadDoc(up, 0, 0);
	HandleDoc(d, 0);
	DestroyURLParts(up);
      }
      else AddMessage("invalidurl");
    }
  }

  return;
}

/*
 * DeleteBookmark
 *
 * Deletes the bookmark selected by the user.  This is actually the
 * callback for a command widget.  Look at lister.c.  Be careful.
 */
static void
DeleteBookmark(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  XawListReturnStruct *lr;

  lr = XawListShowCurrent((Widget)cldata);
  if (lr != NULL)
  {
    RemoveBookmark(root.blist, root.group, lr->list_index);
    
    if (root.bslist != NULL) free(root.bslist);
    root.bslist = MakeBookmarkList(root.blist, root.group);
    
    if (root.bgslist != NULL) free(root.bgslist);
    root.bgslist = MakeBookmarkGroupList(root.blist);
    
    ChangeLister(root.bslist, root.bgslist);
    
    if (WriteBookmarkFile(root.bookmarkFile, root.blist) != 0)
    {
      AddMessage("savebook");
    }
  }

  return;
}

/*
 * DeleteBookmarkGroup
 *
 * Deletes a bookmark group
 */
static void
DeleteBookmarkGroup(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  XawListReturnStruct *lr;

  lr = XawListShowCurrent((Widget)cldata);
  if (lr != NULL)
  {
    root.blist = RemoveBookmarkGroup(root.blist, lr->string);
    
    if (root.bgslist != NULL) free(root.bgslist);
    root.bgslist = MakeBookmarkGroupList(root.blist);  

    if (root.group != NULL)
    {
      free(root.group);
      root.group = NULL;
    }

    if (root.bgslist[0] != NULL)
    {
      if (root.bslist != NULL) free(root.bslist);
      root.bslist = MakeBookmarkList(root.blist, root.bgslist[0]);
      root.group = alloc_string(root.bgslist[0]);
    }

    ChangeLister(root.bslist, root.bgslist);
  }

  return;
}

/*
 * Prevents trouble.
 */
static void
MyXtSetSensitive(w, sensitive)
Widget w;
int sensitive;
{
  if (w) XtSetSensitive(w, (Boolean)sensitive);

  return;
}

/*
 * SetSensitive
 *
 * Figures out which buttons to make sensitive or insensitive.
 * Sensitive is a guideline which may be ignored.  Could probably
 * eliminate all of the if's here but it wouldn't look as cool.
 */
static void
SetSensitive(sensitive)
int sensitive;
{
  MyXtSetSensitive(root.load, sensitive);
  MyXtSetSensitive(root.search, sensitive);
  MyXtSetSensitive(root.bookmark, sensitive);
  MyXtSetSensitive(root.file, sensitive);
  MyXtSetSensitive(root.help, sensitive);
  MyXtSetSensitive(root.source, sensitive);

  MyXtSetSensitive(root.reload,
		   sensitive &&
		   (root.dlist != NULL && root.dlist->doc != NULL &&
		    root.dlist->doc->up != NULL ? True:False));

  if (root.dlist == NULL || root.dlist->next == NULL)
  {
    MyXtSetSensitive(root.back, False);
    MyXtSetSensitive(root.home, False);
  }
  else
  {
    MyXtSetSensitive(root.back, sensitive);
    MyXtSetSensitive(root.home, sensitive);
  }

  XFlush(XtDisplay(root.w));

  return;
}

/*
 * VisitTest
 *
 * Called by the HTML widget functions to determine if a link has been
 * visited.
 */
int
VisitTest(w, url)
Widget w;
char *url;
{
  URLParts *up;
  URLParts *r;

  if (root.dlist == NULL) return(0);

  up = ParseURL(url);
  if (up == NULL) return(0);

  if (root.dlist->base != NULL)
  {
    URLParts *bup;

    bup = ParseURL(root.dlist->base);
    if (bup == NULL)
    {
      DestroyURLParts(up);
      return(0);
    }
    r = MakeURLParts(up, bup);
    DestroyURLParts(bup);
  }
  else
  {
    r = MakeURLParts(up, root.dlist->up);
  }

  DestroyURLParts(up);

  if (r == NULL) return(0);

  if (IsCached(r))
  {
    DestroyURLParts(r);
    return(1);
  }

  DestroyURLParts(r);

  return(0);
}

/*
 * DisplayTransferStatus
 *
 * This is called by the data transfer functions to display the status of
 * a transfer.
 */
int
DisplayTransferStatus(n, max)
int n, max;
{
  static int count = 0;
  static char *byte2 = NULL;
  static char *byte1 = NULL;

  if (root.titledisplay != 0 &&
      n > 0 &&
      count++ % root.statusUpdate == 0)
  {
    char buffer[256];

    if (byte2 == NULL) byte2 = GetFromStringDB("byte2");
    if (byte1 == NULL) byte1 = GetFromStringDB("byte1");

    if (max > 0) sprintf (buffer, byte2, n, max);
    else sprintf (buffer, byte1, n);

    SetTitle(buffer);
  }

  if (root.cancel)
  {
    XEvent xe;

    if (XCheckWindowEvent(XtDisplay(root.cancel), XtWindow(root.cancel),
			  ButtonReleaseMask, &xe))
    {
      root.cancelop = True;
      return(1);
    }
  }

  return(0);
}

/*
 * SubmitForm
 *
 * Form callback for the HTML widget.
 */
void
SubmitForm(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  int i;
  char *n, *v;
  char *url;
  char **nlist = NULL, **vlist = NULL;
  char *sep = "";
  char *finfo = NULL;
  int flen;
  int oflen;
  Document *d;
  URLParts *up;
  WbFormCallbackData *formdata;
  char *method = NULL;

  formdata = (WbFormCallbackData *)cbdata;

  if (formdata == NULL) return;

  if (NullString(formdata->href)) url = MakeURL(root.dlist->up);
  else url = alloc_string(formdata->href);

  nlist = formdata->attribute_names;
  vlist = formdata->attribute_values;
  if (nlist != NULL && vlist != NULL)
  {
    /*
     * If there is only one attribute and it is named isindex then
     * treat this like a regular searchable index.  Otherwise
     * do form stuff.
     */
    if (formdata->attribute_count == 1 && mystrcmp("isindex", nlist[0]) == 0)
    {
      if (vlist[0] == NULL) finfo = alloc_string("");
      else finfo = EscapeURL((unsigned char *)vlist[0], 1);
    }
    else
    {
      finfo = NULL;
      flen = 0;
      method = formdata->method;
      for (i = 0; i < formdata->attribute_count; i++)
      {
	if (nlist[i] == NULL) continue;
	
	n = EscapeURL((unsigned char *)nlist[i], 0);
	
	if (vlist[i] != NULL) v = EscapeURL((unsigned char *)vlist[i], 0);
	else v = alloc_string("");

	/*
	 * length of URL + length of attribute name + length of
	 * attribute value + ampersand/question mark + equal sign + NULL
	 */
	oflen = flen;
	flen = flen + strlen(n) + strlen(v) + strlen(sep) + 1;
	if (finfo != NULL) finfo = realloc_mem(finfo, flen + 1);
	else finfo = alloc_mem(flen + 1);
	sprintf (finfo + oflen, "%s%s=%s", sep, n, v); 
	sep = "&";

	free(n);
	free(v);
      }
    }
  }

  up = ParseURL(url);
  free(url);
  if (up == NULL)
  {
    free(finfo);
    AddMessage("invalidaction");
    return;
  }

  up->method = method != NULL ? alloc_string(method):alloc_string("GET");
  up->request_data = finfo;

  d = LoadDoc(up, 0, 0);
  HandleDoc(d, 0);
  DestroyURLParts(up);

  return;
}
