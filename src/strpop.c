/*
 * strpop.c
 *
 * Copyright (C) 1994, John Kilburg.
 *
 * See copyright.h for details.
 */
#include "copyright.h"

#include "options.h"

#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <X11/cursorfont.h>

#include "ScrollText.h"

#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Form.h>

#include <X11/Xaw/Cardinals.h>

#include "util.h"
#include "strpop.h"

/*
 * for StringPopup
 */
static Widget sreq = 0;
static Widget field = 0;
static Widget dismiss = 0;
static Widget clear = 0;
static void (*fieldfunc)();
static void (*dismissfunc)();
static int popdown_on_field;

/*
 * ClearFunc
 *
 * Clears string in the requester
 */
static void
ClearFunc(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  Arg args[1];

  XtSetArg(args[0], XtNstring, "");
  XtSetValues(field, args, 1);

  return;
}

/*
 * CancelFunc
 *
 * Called when the user clicks on the cancel button
 */
static void
CancelFunc(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  if (cldata && dismissfunc) (*dismissfunc)();

  XtPopdown(sreq);
  XtDestroyWidget(sreq);
  sreq = 0;

  return;
}

/*
 * FieldFunc
 *
 * Called when the user presses return in the string field.
 */
static void
FieldFunc(w, event, params, num_params)
Widget w;
XEvent *event;
String *params;
Cardinal *num_params;
{
  String s;
  Arg args[1];
  char *t;

  XtSetArg(args[0], XtNstring, &s);
  XtGetValues(field, args, 1);

  t = alloc_string(s);

  if (popdown_on_field)
  {
    CancelFunc(dismiss, 0, 0);
  }

  (*fieldfunc)(t);

  free(t);

  return;
}

/*
 * BSFunc
 *
 * Called when the user pressed backspace
 */
static void
BSFunc(w, event, params, num_params)
Widget w;
XEvent *event;
String *params;
Cardinal *num_params;
{
  XawTextPosition begin, end;

  XawTextGetSelectionPos(w, &begin, &end);
  if (begin == end)
  {
    /* no selection, do a normal backspace */
    XtCallActionProc(w, "delete-previous-character", event,
		     params, *num_params);
  }
  else
  {
    XtCallActionProc(w, "kill-selection", event, params, *num_params);
  }

  return;
}

static char *fieldtrans =
"Ctrl<Key>M:   field-func()\n\
Ctrl<Key>J:    field-func()\n\
<Key>Linefeed: field-func()\n\
<Key>Return:   field-func()\n\
<Key>Delete:   bs-func()\n\
<Key>BackSpace: bs-func()\n";

/*
 * The "main" function.
 */
void
StringPopup(parent, sfunc, sdef, message, dfunc, pdof, inv)
Widget parent;
void (*sfunc)();
char *sdef;
char *message;
void (*dfunc)();
int pdof; /* if 1 then the popup only goes away when user clicks on dismiss */
int inv; /* if 1 then text is invisible */
{
  Widget paned, form, label, box, w;
  Arg args[4];
  Dimension width, height;
  Display *dpy;
  Window rw, cw;
  int rx, ry, wx, wy;
  unsigned int mask;
  int dwidth, dheight;
  XtActionsRec action_rec[2];
  Pixel bg;

  if (sreq)
  {
    XRaiseWindow(XtDisplay(sreq), XtWindow(sreq));
    return;
  }

  fieldfunc = sfunc;
  dismissfunc = dfunc;
  popdown_on_field = pdof;

  sreq = XtCreatePopupShell("stringpopup",
			    transientShellWidgetClass, parent,
			    NULL, 0);
  
  paned = XtCreateManagedWidget("paned",
                                panedWidgetClass, sreq,
                                NULL, 0);

  form = XtCreateManagedWidget("form",
			       formWidgetClass, paned,
			       NULL, 0);
    
  label = XtCreateManagedWidget(message,
				labelWidgetClass, form,
				NULL, 0);

  field = XtCreateManagedWidget("field",
				asciiTextWidgetClass, form,
				NULL, 0);

  if (inv)
  {
    XtSetArg(args[0], XtNbackground, &bg);
    XtGetValues(field, args, 1);
  }

  XtSetArg(args[0], XtNfromVert, label);
  XtSetValues(field, args, 1);
/*  field = XtNameToWidget(field, "text");*/
  if (sdef == NULL) XtSetArg(args[0], XtNstring, "");
  else XtSetArg(args[0], XtNstring, sdef);
  XtSetArg(args[1], XtNeditType, XawtextEdit);
  if (inv) XtSetArg(args[2], XtNforeground, bg);
  XtSetValues(field, args, inv != 0 ? 3:2);

  action_rec[0].proc = (XtActionProc)FieldFunc;
  action_rec[0].string = "field-func";
  action_rec[1].proc = (XtActionProc)BSFunc;
  action_rec[1].string = "bs-func";
  XtAppAddActions(XtWidgetToApplicationContext(field), action_rec, 2);
  XtOverrideTranslations(field, XtParseTranslationTable(fieldtrans));

  box = XtCreateManagedWidget("bbox",
			      boxWidgetClass, paned,
			      NULL, 0);
  
  w = XtCreateManagedWidget("OK",
			    commandWidgetClass, box,
			    NULL, 0);
  XtAddCallback(w, XtNcallback, FieldFunc, 0);
  
  clear = XtCreateManagedWidget("Clear",
				 commandWidgetClass, box,
				 NULL, 0);
  XtAddCallback(clear, XtNcallback, ClearFunc, 0);
  
  dismiss = XtCreateManagedWidget("Dismiss",
				   commandWidgetClass, box,
				   NULL, 0);
  XtAddCallback(dismiss, XtNcallback, CancelFunc, (XtPointer)1);
  
  XtRealizeWidget(sreq);
  
  dpy = XtDisplay(parent);
  XQueryPointer(dpy, XtWindow(sreq),
		&rw, &cw,
		&rx, &ry,
		&wx, &wy,
		&mask);

  XtSetArg(args[0], XtNwidth, &width);
  XtSetArg(args[1], XtNheight, &height);
  XtGetValues(sreq, args, 2);
  
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

  XtSetArg(args[0], XtNx, rx - 5);
  XtSetArg(args[1], XtNy, ry - 5);
  XtSetValues(sreq, args, 2);

  XtPopup(sreq, XtGrabNone);

  XtSetKeyboardFocus(sreq, field);

  return;
}
