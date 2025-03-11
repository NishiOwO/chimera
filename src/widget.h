/*
 * widget.h
 *
 * Copyright (C) 1993, 1994, John Kilburg.
 *
 * See copyright.h for details.
 */ 

typedef struct _realminfo
{
  char *name;
  URLParts *up;
  int used;
  struct _realminfo *next;
} RealmInfo;

typedef struct _docnode
{
  Document *doc;
  int vpos, hpos;
  URLParts *up;
  char *base;
  struct _docnode *next;
} DocNode;

typedef struct _htmlroot
{
  /*
   * Xt variables.
   */
  XtAppContext appcon;
  Widget file;
  Widget toplevel;
  Widget w;
  Widget back;
  Widget load;
  Widget view;
  Widget help;
  Widget source;
  Widget urldisplay;
  Widget titledisplay;
  Widget bookmark;
  Widget reload;
  Widget home;
  Widget search;
  Widget cancel;
  Widget quit;
  
  /*
   * Context info stored between callbacks.  Yeek.
   */
  char *savestr;
  char *loadstr;
  char *printerstr;
  char *searchstr;
  char *bookmarkstr;
  char *mailstr;
  char *base;

  /*
   * Some other stuff
   */
  XtTranslations trans;
  Boolean cancelop;
  DocNode *dlist;
  Document *savedoc;
  Boolean listerup;
  char *keyTrans;
  Cursor left_ptr;
  Cursor watch;

  /*
   * Configuration file info
   */
  char *path;
  char *convertFiles;
  Convert *clist;
  char *mailCapFiles;
  MailCap *mclist;
  char *protocolFiles;
  Protocol *plist;
  char *mimeTypeFiles;
  MIMEType *mtlist;
  char *bookmarkFile;
  BookmarkGroup *blist;
  char *languageDB;
  char *cacheInfoFiles;

  /*
   * General resources.
   */
  int cacheTTL;
  int cacheSize;
  Boolean cacheOff;
  char *cacheDir;
  Boolean cacheClean;
  char *helpURL; /* location of the help page */
  char *homeURL; /* location of the home page */
  char *button1Box; /* list of widgets in the first button box */
  char *button2Box; /* list of widgets in the second button box */
  char *printerName; /* default printer */
  Boolean showURL; /* switch for the display of the current URL */
  Boolean showTitle; /* switch for the display of the current title */
  Boolean rflag; /* reload flag */
  char *group; /* current bookmark group */
  char **bslist; /* bookmark list */
  char **bgslist; /* bookmark group list */
  int statusUpdate; /* frequency of download status update */
  int inPort; /* the port that chimera listens on for data */
  char *httpProxy;
  char *gopherProxy;
  char *ftpProxy;
  char *waisProxy;
  char *nntpProxy;
  char *newsProxy;
  char *urnProxy;
  char *noProxy;
  char *allProxy;

  RealmInfo *rlist; /* list of realm authentication info */
  Document *authdoc; /* document that needs authentication */

  int maxColors; /* maximum colors per inline image */

  Boolean cacheIgnoreExpires;

  XColor bgcolor; /* background color */
} HTMLRoot;

/*
 * Callbacks
 *
 * It would be a pain to put args for these guys because I would
 * have to do funky things with the middle argument.
 */
void Quit();
void OpenDocument();
void Anchor();
void Home();
void Back();
void Help();
void Source();
void Reload();
void File();
void FileAction();
void OpenAction();
void SearchAction();
void DoLister();
void Search();
void Cancel();
ImageInfo *ImageResolve();
ImageInfo *DelayedImageResolve();
int VisitTest();
void SubmitForm();
void LinkCB();

#ifndef _NO_PROTO
void CreateWidgets(HTMLRoot *);
#else
void CreateWidgets();
#endif


