/*
 *
 * lister.c
 *
 * Copyright (C) 1993, 1994, John Kilburg.
 *
 * See copyright.h for details.
 */
#include "copyright.h"

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <X11/cursorfont.h>

#include <X11/Xaw/List.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Viewport.h>

#include <X11/Xaw/Cardinals.h>

#include "lister.h"

#include "options.h"

static Widget lister = 0;
static Widget tlist = 0;
static Widget blist = 0;
static Widget tok = 0;
static Widget bok = 0;
static void (*tokfunc)();
static void (*bokfunc)();

/*
 * ListerSelect
 *
 * Called when the user clicks on an item in the list.
 */
static void
ListerSelect(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  void (*func)();
  int l;
  Widget ok;
  XawListReturnStruct *lr;
  static int tlast_selected = -1;
  static int blast_selected = -1;

  if (w == tlist)
  {
    func = tokfunc;
    l = tlast_selected;
    ok = tok;
  }
  else
  {
    func = bokfunc;
    l = blast_selected;
    ok = bok;
  }

  lr = XawListShowCurrent(w);
  if (lr != NULL)
  {
    if (lr->list_index == l)
    {
      (func)(ok, (XtPointer)w, NULL);
      l = -1;
    }
    else
    {
      l = lr->list_index;
    }
  }

  if (w == tlist) tlast_selected = l;
  else blast_selected = l;

  return;
}

/*
 * CreateLister
 *
 * The first command in clist is assumed to be the "OK" button and its
 * callback function is used for the callback when the user double
 * clicks on an item.
 */
void
CreateLister(title, parent, clist, itemlist, clist2, itemlist2, callback_data)
char *title;
Widget parent;
ListCommand *clist;
char **itemlist;
ListCommand *clist2;
char **itemlist2;
XtPointer callback_data;
{
  Widget paned, box, box2, box3, box4, box5, view;
  Arg args[4];
  Dimension width, height;
  Display *dpy;
  Window rw, cw;
  int rx, ry, wx, wy;
  int i;
  unsigned int mask;
  int dwidth, dheight;

  if (lister)
  {
    fprintf (stderr, "WARNING: nested listers\n");
    return;
  }
  
  lister = XtCreatePopupShell("Lister",
				 transientShellWidgetClass, parent,
				 NULL, 0);
  
  paned = XtCreateManagedWidget("listerpaned",
                                panedWidgetClass, lister,
                                NULL, 0);
  
  box = XtCreateManagedWidget("listerbox",
			      boxWidgetClass, paned,
			      NULL, 0);

  XtCreateManagedWidget(title,
			labelWidgetClass, box,
			NULL, 0);

  if (clist2)
  {
    box4 = XtCreateManagedWidget("listerbox4",
				 formWidgetClass, paned,
				 NULL, 0);
    
    box5 = XtCreateManagedWidget("listerbox5",
				 boxWidgetClass, paned,
				 NULL, 0);

    view = XtCreateManagedWidget ("listerview2",
				  viewportWidgetClass, box4,
				  NULL, 0);
    
    tlist = XtCreateManagedWidget ("listerlist2",
				   listWidgetClass, view,
				   NULL, 0);
    
    XawListChange(tlist, itemlist2, 0, 0, True);
    XtAddCallback(tlist, XtNcallback, (XtCallbackProc)ListerSelect, 0);
    
    for (i = 0; clist2[i].name != NULL; i++)
    {
      clist2[i].w = XtCreateManagedWidget(clist2[i].name,
					  commandWidgetClass, box5,
					  NULL, 0);
      XtAddCallback(clist2[i].w, XtNcallback, clist2[i].func,(XtPointer)tlist);
    }

    tok = clist2[0].w;
    tokfunc = clist2[0].func;
  }

  box2 = XtCreateManagedWidget("listerbox2",
			       formWidgetClass, paned,
			       NULL, 0);

  box3 = XtCreateManagedWidget("listerbox3",
			       boxWidgetClass, paned,
			       NULL, 0);

  view = XtCreateManagedWidget ("listerview",
                                viewportWidgetClass, box2,
                                NULL, 0);
  
  blist = XtCreateManagedWidget ("listerlist",
				 listWidgetClass, view,
				 NULL, 0);
  
  XawListChange(blist, itemlist, 0, 0, True);
  XtAddCallback(blist, XtNcallback, (XtCallbackProc)ListerSelect, 0);
 
  for (i = 0; clist[i].name != NULL; i++)
  {
    clist[i].w = XtCreateManagedWidget(clist[i].name,
				       commandWidgetClass, box3,
				       NULL, 0);
    XtAddCallback(clist[i].w, XtNcallback, clist[i].func, (XtPointer)blist);
  }
  bok = clist[0].w;
  bokfunc = clist[0].func;

  /*
   * not centered but close enough
   */
  XtRealizeWidget(lister);

  dpy = XtDisplay(parent);
  XQueryPointer(dpy, XtWindow(lister),
		&rw, &cw,
		&rx, &ry,
		&wx, &wy,
		&mask);

  XtSetArg(args[0], XtNwidth, &width);
  XtSetArg(args[1], XtNheight, &height);
  XtGetValues(lister, args, 2);
  
  dwidth = DisplayWidth(dpy, DefaultScreen(dpy));
  dheight = DisplayHeight(dpy, DefaultScreen(dpy));

  if (rx + width > dwidth)
  {
    rx -= rx + width - dwidth;
  }
  if (ry + height > dheight)
  {
    ry -= ry + height - dheight;
  }

  XtSetArg(args[0], XtNx, rx);
  XtSetArg(args[1], XtNy, ry);
  XtSetValues(lister, args, 2);

  XtPopup(lister, XtGrabNone);

  return;
}

/*
 * ChangeLister
 */
void
ChangeLister(slist, slist2)
char **slist;
char **slist2;
{
  XawListChange(blist, slist, 0, 0, True);
  XawListChange(tlist, slist2, 0, 0, True);

  return;
}

/*
 * DestroyLister
 */
void
DestroyLister()
{
  XtPopdown(lister);
  XtDestroyWidget(lister);
  lister = 0;
  return;
}

/*
 * RaiseLister
 */
void
RaiseLister()
{
  XRaiseWindow(XtDisplay(lister), XtWindow(lister));

  return;
}
