/*
 * filepop.c
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
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/MenuButton.h>

#include <X11/Xaw/Cardinals.h>

#include "util.h"
#include "filepop.h"

static Widget requester = 0;
static Widget save = 0;
static Widget print = 0;
static Widget mail = 0;
static Widget dismiss = 0;
static Widget clear = 0;
static void (*savefunc)();
static void (*printfunc)();
static void (*mailfunc)();
static int outtype = 2;
static Widget menubutton;
static Widget simplemenu = 0;

static void (*dismissfunc)();

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
  XtSetValues(save, args, 1);
  XtSetValues(print, args, 1);
  XtSetValues(mail, args, 1);

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

  XtPopdown(requester);
  XtDestroyWidget(requester);
  XtDestroyWidget(simplemenu);
  simplemenu = 0;
  requester = 0;

  return;
}

/*
 * MailFunc
 *
 * Called when the user presses return in the mail field.
 */
static void
MailFunc(w, event, params, num_params)
Widget w;
XEvent *event;
String *params;
Cardinal *num_params;
{
  String s;
  Arg args[1];
  char *t;

  XtSetArg(args[0], XtNstring, &s);
  XtGetValues(mail, args, 1);
  
  t = alloc_string(s);

  CancelFunc(dismiss, 0, 0);

  (*mailfunc)(t, outtype);
  
  free(t);

  return;
}

/*
 * SaveFunc
 *
 * Called when the user presses return in the save field.
 */
static void
SaveFunc(w, event, params, num_params)
Widget w;
XEvent *event;
String *params;
Cardinal *num_params;
{
  String s;
  Arg args[1];
  char *t;

  XtSetArg(args[0], XtNstring, &s);
  XtGetValues(save, args, 1);
  
  t = alloc_string(s);

  CancelFunc(dismiss, 0, 0);

  (*savefunc)(t, outtype);
  
  free(t);

  return;
}

/*
 * PrintFunc
 *
 * Called when the user presses return in the print field.
 */
static void
PrintFunc(w, event, params, num_params)
Widget w;
XEvent *event;
String *params;
Cardinal *num_params;
{
  String s;
  Arg args[1];
  char *t;

  XtSetArg(args[0], XtNstring, &s);
  XtGetValues(print, args, 1);

  t = alloc_string(s);

  CancelFunc(dismiss, 0, 0);

  (*printfunc)(t, outtype);

  free(t);

  return;
}

/*
 * bs-func
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

static void menucb();

static struct menu_entries
{
  Widget w;
  void (*proc)();
  char *name;
} menu_table[] =
{
{ 0, menucb, "plainmenuitem" },
{ 0, menucb, "prettymenuitem" },
{ 0, menucb, "psmenuitem" },
{ 0, menucb, "htmlmenuitem" },
{ 0, NULL, NULL },
};

static void
create_menu(parent)
Widget parent;
{
  int i;
  Arg args[2];
  char *label;

  XtSetArg(args[0], XtNmenuName, "menu");
  menubutton = XtCreateWidget("menubutton",
			      menuButtonWidgetClass, parent,
			      args, 1);

  simplemenu = XtCreatePopupShell("menu",
				  simpleMenuWidgetClass, menubutton,
				  NULL, 0);

  for (i = 0; menu_table[i].proc != NULL; i++)
  {
    menu_table[i].w = XtCreateManagedWidget(menu_table[i].name,
				     smeBSBObjectClass, simplemenu,
				     NULL, 0);
    XtAddCallback(menu_table[i].w, XtNcallback, menu_table[i].proc,
		  (XtPointer)parent);
  }

  XtSetArg(args[0], XtNlabel, &label);
  XtGetValues(menu_table[outtype].w, args, 1);
  XtSetArg(args[0], XtNlabel, label);
  XtSetValues(menubutton, args, 1);

  XtManageChild(menubutton);

  return;
}

static void
menucb(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  int i;

  for (i = 0; menu_table[i].proc != NULL; i++)
  {
    if (menu_table[i].w == w)
    {
      outtype = i;
      XtDestroyWidget(menubutton);
      XtDestroyWidget(simplemenu);
      create_menu((Widget)cldata);
      break;
    }
  }

  return;
}

static char *savetrans =
"Ctrl<Key>M:   save-func()\n\
Ctrl<Key>J:    save-func()\n\
<Key>Linefeed: save-func()\n\
<Key>Return:   save-func()\n\
<Key>Delete:   bs-func()\n";

static char *printtrans =
"Ctrl<Key>M:   print-func()\n\
Ctrl<Key>J:    print-func()\n\
<Key>Linefeed: print-func()\n\
<Key>Return:   print-func()\n\
<Key>Delete:   bs-func()\n";

static char *mailtrans =
"Ctrl<Key>M:   mail-func()\n\
Ctrl<Key>J:    mail-func()\n\
<Key>Linefeed: mail-func()\n\
<Key>Return:   mail-func()\n\
<Key>Delete:   bs-func()\n";

/*
 * The "main" function.
 */
void
FilePopup(parent, sfunc, sdef, pfunc, pdef, mfunc, mdef, dfunc)
Widget parent;
void (*sfunc)();
char *sdef;
void (*pfunc)();
char *pdef;
void (*mfunc)();
char *mdef;
void (*dfunc)();
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

  if (requester)
  {
    XRaiseWindow(XtDisplay(requester), XtWindow(requester));
    return;
  }

  savefunc = sfunc;
  printfunc = pfunc;
  mailfunc = mfunc;
  dismissfunc = dfunc;

  requester = XtCreatePopupShell("filepopup",
				 transientShellWidgetClass, parent,
				 NULL, 0);
  
  paned = XtCreateManagedWidget("paned",
                                panedWidgetClass, requester,
                                NULL, 0);

  form = XtCreateManagedWidget("sform",
			       formWidgetClass, paned,
			       NULL, 0);
    
  label = XtCreateManagedWidget("filename",
				labelWidgetClass, form,
				NULL, 0);

  save = XtCreateManagedWidget("save",
			       asciiTextWidgetClass, form,
			       NULL, 0);
  XtSetArg(args[0], XtNfromHoriz, label);
  XtSetValues(save, args, 1);
  /*save = XtNameToWidget(save, "text");*/
  if (sdef == NULL) XtSetArg(args[0], XtNstring, "");
  else XtSetArg(args[0], XtNstring, sdef);
  XtSetArg(args[1], XtNeditType, XawtextEdit);
  XtSetValues(save, args, 2);

  action_rec[0].proc = (XtActionProc)SaveFunc;
  action_rec[0].string = "save-func";
  action_rec[1].proc = (XtActionProc)BSFunc;
  action_rec[1].string = "bs-func";
  XtAppAddActions(XtWidgetToApplicationContext(save), action_rec, 2);
  XtOverrideTranslations(save, XtParseTranslationTable(savetrans));
  
  form = XtCreateManagedWidget("pform",
			       formWidgetClass, paned,
			       NULL, 0);
    
  label = XtCreateManagedWidget("printer",
				labelWidgetClass, form,
				NULL, 0);

  print = XtCreateManagedWidget("print",
				asciiTextWidgetClass, form,
				NULL, 0);
  XtSetArg(args[0], XtNfromHoriz, label);
  XtSetValues(print, args, 1);
  /*print = XtNameToWidget(print, "text");*/
  if (pdef == NULL) XtSetArg(args[0], XtNstring, "");
  else XtSetArg(args[0], XtNstring, pdef);
  XtSetArg(args[1], XtNeditType, XawtextEdit);
  XtSetValues(print, args, 2);

  action_rec[0].proc = (XtActionProc)PrintFunc;
  action_rec[0].string = "print-func";
  action_rec[1].proc = (XtActionProc)BSFunc;
  action_rec[1].string = "bs-func";
  XtAppAddActions(XtWidgetToApplicationContext(print), action_rec, 2);
  XtOverrideTranslations(print, XtParseTranslationTable(printtrans));

  form = XtCreateManagedWidget("mform",
			       formWidgetClass, paned,
			       NULL, 0);
    
  label = XtCreateManagedWidget("email",
				labelWidgetClass, form,
				NULL, 0);

  mail = XtCreateManagedWidget("mail",
				asciiTextWidgetClass, form,
				NULL, 0);
  XtSetArg(args[0], XtNfromHoriz, label);
  XtSetValues(mail, args, 1);
  /*mail = XtNameToWidget(mail, "text");*/
  if (mdef == NULL) XtSetArg(args[0], XtNstring, "");
  else XtSetArg(args[0], XtNstring, mdef);
  XtSetArg(args[1], XtNeditType, XawtextEdit);
  XtSetValues(mail, args, 2);

  action_rec[0].proc = (XtActionProc)MailFunc;
  action_rec[0].string = "mail-func";
  action_rec[1].proc = (XtActionProc)BSFunc;
  action_rec[1].string = "bs-func";
  XtAppAddActions(XtWidgetToApplicationContext(mail), action_rec, 2);
  XtOverrideTranslations(mail, XtParseTranslationTable(mailtrans));

  box = XtCreateManagedWidget("mbox",
			      boxWidgetClass, paned,
			      NULL, 0);
  create_menu(box);

  box = XtCreateManagedWidget("bbox",
			      boxWidgetClass, paned,
			      NULL, 0);

  w = XtCreateManagedWidget("savebutton",
			    commandWidgetClass, box,
			    NULL, 0);
  XtAddCallback(w, XtNcallback, SaveFunc, 0);

  w = XtCreateManagedWidget("printbutton",
			    commandWidgetClass, box,
			    NULL, 0);
  XtAddCallback(w, XtNcallback, PrintFunc, 0);

  w = XtCreateManagedWidget("mailbutton",
			    commandWidgetClass, box,
			    NULL, 0);
  XtAddCallback(w, XtNcallback, MailFunc, 0);

  clear = XtCreateManagedWidget("clearbutton",
				 commandWidgetClass, box,
				 NULL, 0);
  XtAddCallback(clear, XtNcallback, ClearFunc, 0);

  dismiss = XtCreateManagedWidget("dismissbutton",
				   commandWidgetClass, box,
				   NULL, 0);
  XtAddCallback(dismiss, XtNcallback, CancelFunc, (XtPointer)1);

  XtRealizeWidget(requester);

  dpy = XtDisplay(parent);
  XQueryPointer(dpy, XtWindow(requester),
		&rw, &cw,
		&rx, &ry,
		&wx, &wy,
		&mask);

  XtSetArg(args[0], XtNwidth, &width);
  XtSetArg(args[1], XtNheight, &height);
  XtGetValues(requester, args, 2);
  
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
  XtSetValues(requester, args, 2);

  XtPopup(requester, XtGrabNone);

  return;
}
