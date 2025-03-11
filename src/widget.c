/*
 * widget.c
 *
 * Copyright (C) 1993, 1994, John Kilburg.
 *
 * Note that this code was actually written by Jim.Rees@umich.edu.
 *
 * See copyright.h for details.
 */
#include "copyright.h"

#include <stdio.h>
#include <sys/types.h>

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

#include "ScrollText.h"

#include "HTML.h"

#include "url.h"
#include "mime.h"
#include "document.h"
#include "bookmark.h"
#include "lister.h"
#include "util.h"
#include "convert.h"

#include "widget.h"

#include "options.h"

static struct ButtonTable {
  char *name;
  Widget w;
  void (*cb)();
} ButtonTable[] = {
  {"quit",      NULL, Quit},
  {"open",	NULL, OpenAction},
  {"home",	NULL, Home},
  {"back",	NULL, Back},
  {"source",	NULL, Source},
  {"reload",	NULL, Reload},
  {"file",      NULL, FileAction},
  {"help",	NULL, Help},
  {"cancel",    NULL, Cancel},
  {"bookmark",	NULL, DoLister},
  {"search",    NULL, SearchAction},
  {NULL,        NULL, NULL},
};

static void
AddButtons(r, box, list)
HTMLRoot *r;
Widget box;
char *list;
{
  char name[256];
  struct ButtonTable *btp;

  list = alloc_string(list);

  while (sscanf(list, " %[^,]", name) == 1)
  {
    /*
     * Find the listed button and create its widget
     */
    for (btp = &ButtonTable[0]; btp->name != NULL; btp++)
    {
      if (!mystrcmp(btp->name, name))
      {
	btp->w = XtCreateManagedWidget(btp->name,
				       commandWidgetClass,
				       box,
				       NULL, ZERO);
	XtAddCallback(btp->w, XtNcallback, btp->cb, r);
	break;
      }
    }

    /*
     * Fill in the entry in HTMLRoot for the widget.
     */
    if (!mystrcmp(name, "open"))
      r->load = btp->w;
    else if (!mystrcmp(name, "home"))
      r->home = btp->w;
    else if (!mystrcmp(name, "back"))
      r->back = btp->w;
    else if (!mystrcmp(name, "source"))
      r->source = btp->w;
    else if (!mystrcmp(name, "reload"))
      r->reload = btp->w;
    else if (!mystrcmp(name, "file"))
      r->file = btp->w;
    else if (!mystrcmp(name, "help"))
      r->help = btp->w;
    else if (!mystrcmp(name, "bookmark"))
      r->bookmark = btp->w;
    else if (!mystrcmp(name, "cancel"))
      r->cancel = btp->w;
    else if (!mystrcmp(name, "search"))
      r->search = btp->w;
    else if (!mystrcmp(name, "quit"))
      r->quit = btp->w;

    /*
     * Skip to the next comma-delimited item in the list
     */
    while (*list && *list != ',')
      list++;
    if (*list == ',')
      list++;
  }

  return;
}

/*
 * CreateWidgets
 *
 * Create the command and box widgets.
 */
void
CreateWidgets(r)
HTMLRoot *r;
{
  Widget paned, box, form;
  Arg args[5];

  /*
   * Main window pane
   */
  paned = XtCreateManagedWidget("paned",
                                panedWidgetClass, r->toplevel, 
                                NULL, ZERO);


  /*
   * Button pane(s)
   */
  if (r->button1Box && *r->button1Box)
  {
    box = XtCreateManagedWidget("box1", boxWidgetClass, paned, NULL, ZERO);
    AddButtons(r, box, r->button1Box);
  }

  if (r->button2Box && *r->button2Box)
  {
    box = XtCreateManagedWidget("box3", boxWidgetClass, paned, NULL, ZERO);
    AddButtons(r, box, r->button2Box);
  }


  /*
   * Third pane. Title display.
   */
  if (r->showTitle)
  {
    form = XtCreateManagedWidget("box5",
				formWidgetClass, paned,
				NULL, ZERO);
    
    XtCreateManagedWidget("titlelabel",
			  labelWidgetClass, form,
			  NULL, ZERO);
    
    r->titledisplay = XtCreateManagedWidget("titledisplay",
					    scrollingTextWidgetClass, form,
					    NULL, ZERO);
    r->titledisplay = XtNameToWidget(r->titledisplay, "text");
  }
  else
  {
    r->titledisplay = 0;
  }

  /*
   * Fourth pane.  URL display.
   */
  if (r->showURL)
  {
    form = XtCreateManagedWidget("box4",
				formWidgetClass, paned,
				NULL, ZERO);
    
    XtCreateManagedWidget("urllabel",
			  labelWidgetClass, form,
			  NULL, ZERO);
    
    r->urldisplay = XtCreateManagedWidget("urldisplay",
					  scrollingTextWidgetClass, form,
					  NULL, ZERO);
    r->urldisplay = XtNameToWidget(r->urldisplay, "text");
  }
  else
  {
    r->urldisplay = 0;
  }

  /*
   * Fifth pane, the HTML viewing area
   */
  r->w = XtCreateManagedWidget("html",
                              htmlWidgetClass, paned,
                              NULL, ZERO);
  XtAddCallback(r->w, WbNanchorCallback, Anchor, r);

  /*
   * Set some callbacks for the HTML widget.
   */
  XtSetArg(args[0], WbNresolveImageFunction, ImageResolve);
  XtSetArg(args[1], WbNpreviouslyVisitedTestFunction, VisitTest);
  XtSetArg(args[2], WbNresolveDelayedImage, DelayedImageResolve);
  XtSetValues(r->w, args, 3);

  XtAddCallback(r->w, WbNlinkCallback, LinkCB, (XtPointer)0);
  XtAddCallback(r->w, WbNsubmitFormCallback, SubmitForm, (XtPointer)0);

  return;
}


