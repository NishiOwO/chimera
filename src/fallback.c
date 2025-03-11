/*
 * fallback.c
 *
 * Copyright (C) 1993, 1994, John Kilburg.
 *
 * See copyright.h for details.
 */ 
#include <stdio.h>

#include "copyright.h"

/*
 * I got sick of this being in main.c
 */
char *fallback_resources[] =
{
  /* Resources for everything */
  "*background:                moccasin",
  "*showGrip:                  false",

  /* resources for general widget types */
  "*Scrollbar.background:      burlywood2",
  "*Command.background:        burlywood2",
  "*MenuButton.background:     burlywood2",
  "*SimpleMenu.background:     burlywood2",
  "*SmeBSB.background:         burlywood2",
  "*Box.orientation:           horizontal",
  "*Label.borderWidth:         0",

  "*form.resizeable:           true",
  "*form.borderWidth:          0",

  "*allowHoriz:       true",
  "*allowVert:        true",

  /* Labels for main window commands */
  "*file.label:                File",
  "*open.label:                Open",
  "*bookmark.label:            Bookmark",
  "*quit.label:                Quit",
  "*reload.label:              Reload",
  "*help.label:                Help",
  "*source.label:              Source",
  "*home.label:                Home",
  "*back.label:                Back",
  "*search.label:              Search",
  "*cancel.label:              Cancel",

  /* resources for specific widgets */
  "*html.height:               600",
  "*html.width:                600",
  "*html.horizontalScrollOnTop: True",
  "*html.activeAnchorBG:       moccasin",
  "*html.anchorUnderlines:     1",
  "*html.visitedAnchorUnderlines: 1",
  "*html.dashedVisitedAnchorUnderlines: true",
  "*html.autoSize:             true",

  /* resources for the lister popup */
  "*Lister*listok.label:       Open",
  "*Lister*listdelete.label:   Delete",
  "*Lister*listadd.label:      Add",
  "*Lister*listcancel.label:   Dismiss",
  "*Lister*groupok.label:      Open Group",
  "*Lister*groupdel.label:     Delete Group",
  "*Lister*groupadd.label:     Add Group",
  "*Lister*defaultColumns:     1",
  "*Lister*forceColumns:       true",

  "*Lister*listerlist.borderWidth: 0",
  "*Lister*listerview.width:   500",
  "*Lister*listerview.height:  200",
  "*Lister*listerview.borderWidth: 0",
  "*Lister*listerbox2.borderWidth: 0",
  "*Lister*listerbox2.defaultDistance: 0",

  "*Lister*listerlist2.borderWidth: 0",
  "*Lister*listerview2.width:  500",
  "*Lister*listerview2.height: 100",
  "*Lister*listerview2.borderWidth: 0",
  "*Lister*listerbox4.borderWidth: 0",
  "*Lister*listerbox4.defaultDistance: 0",

  "*urllabel.label:            URL   :",
  "*urllabel.left:             ChainLeft",
  "*urllabel.right:            ChainLeft",
  "*urldisplay.left:           ChainLeft",
  "*urldisplay.right:          ChainRight",
  "*urldisplay.fromHoriz:      urllabel",
  "*urldisplay*sensitive:      true",
  "*urldisplay.width:          500",
  "*urldisplay*editType:       edit",

  "*titlelabel.label:          Title :",
  "*titlelabel.left:           ChainLeft",
  "*titlelabel.right:          ChainLeft",
  "*titledisplay.left:         ChainLeft",
  "*titledisplay.right:        ChainRight",
  "*titledisplay.fromHoriz:    titlelabel",
  "*titledisplay*sensitive:    false",
  "*titledisplay.width:        500",
  "*titledisplay*displayCaret: False",
  "*titledisplay*editType:     read",

  "*filepopup*filename.label: Filename :",
  "*filepopup*filename.left: ChainLeft",
  "*filepopup*filename.right: ChainLeft",
  "*filepopup*save.width: 300",
  "*filepopup*save.left: ChainLeft",
  "*filepopup*save.right: ChainRight",
  "*filepopup*savebutton.label: Save",

  "*filepopup*printer.label:  Printer  :",
  "*filepopup*printer.left:  ChainLeft",
  "*filepopup*printer.right:  ChainLeft",
  "*filepopup*print.left:  ChainLeft",
  "*filepopup*print.right:  ChainRight",
  "*filepopup*printbutton.label: Print",

  "*filepopup*email.label: Email    :",
  "*filepopup*email.left:  ChainLeft",
  "*filepopup*email.right:  ChainLeft",
  "*filepopup*mail.left:  ChainLeft",
  "*filepopup*mail.right:  ChainRight",
  "*filepopup*mailbutton.label: Mail",

  "*filepopup*clearbutton.label: Clear",
  "*filepopup*dismissbutton.label: Dismiss",

  "*filepopup*plainmenuitem.label: Plain Text",
  "*filepopup*prettymenuitem.label: Pretty Text",
  "*filepopup*psmenuitem.label: Postscript",
  "*filepopup*htmlmenuitem.label: HTML",

  "*stringpopup*field.width: 300",

  "*Label.font: -*-lucidatypewriter-medium-r-normal-*-*-120-*-*-*-*-iso8859-1",
  "*Command.font: -*-lucida-bold-r-normal-sans-*-120-*-*-*-*-iso8859-1",
  "*MenuButton.font: -*-lucida-bold-r-normal-sans-*-120-*-*-*-*-iso8859-1",
  "*SimpleMenu.font: -*-lucida-bold-r-normal-sans-*-120-*-*-*-*-iso8859-1",
  "*SmeBSB.font:       -*-lucida-bold-r-normal-sans-*-120-*-*-*-*-iso8859-1",
  "*printsave.font:     -*-lucida-bold-r-normal-sans-*-120-*-*-*-*-iso8859-1",
  "*getstring.font:     -*-lucida-bold-r-normal-sans-*-120-*-*-*-*-iso8859-1",
  "*Lister.font:        -*-lucida-bold-r-normal-sans-*-120-*-*-*-*-iso8859-1",
  "*UserTextInput.font: -*-lucida-bold-r-normal-sans-*-120-*-*-*-*-iso8859-1",
  "*urldisplay.font:    -*-lucida-bold-r-normal-sans-*-120-*-*-*-*-iso8859-1",
  "*titledisplay.font:  -*-lucida-bold-r-normal-sans-*-120-*-*-*-*-iso8859-1",

  "*header1Font:   -*-helvetica-bold-r-normal-*-*-240-*-*-*-*-iso8859-1",
  "*header2Font:   -*-helvetica-bold-r-normal-*-*-180-*-*-*-*-iso8859-1",
  "*header3Font:   -*-helvetica-bold-r-normal-*-*-140-*-*-*-*-iso8859-1",
  "*header4Font:   -*-helvetica-bold-r-normal-*-*-120-*-*-*-*-iso8859-1",
  "*header5Font:   -*-helvetica-bold-r-normal-*-*-100-*-*-*-*-iso8859-1",
  "*header6Font:   -*-helvetica-bold-r-normal-*-*-80-*-*-*-*-iso8859-1",
  "*font:          -*-helvetica-medium-r-normal-*-*-120-*-*-*-*-iso8859-1",
  "*boldFont:      -*-helvetica-bold-r-normal-*-*-120-*-*-*-*-iso8859-1",
  "*italicFont:    -*-helvetica-medium-o-normal-*-*-120-*-*-*-*-iso8859-1",
  "*addressFont:   -*-helvetica-medium-o-normal-*-*-140-*-*-*-*-iso8859-1",

  NULL /* I'm bitter about this because I have to include stdio.h which
          seems like worthlessness */
};
