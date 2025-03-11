/*
 * lang.c
 *
 * Copyright (C) 1993, 1994, John Kilburg.
 *
 * See copyright.h for details.
 */
#include "copyright.h"

#include <stdio.h>

#include "lang.h"
#include "stringdb.h"
#include "util.h"
#include "options.h"

/*
 * English/American strings for error/status messages.
 */
static StringDB lang[] =
{
  { "badurl", "<h1>Error</h1>Invalid URL.", lang + 1 },
  { "nodoc", "<h1>Error</h1>Reached DisplayCurrent() without a document.", lang + 2 },
  { "download", "Downloading document...", lang + 3 },
  { "enterpw", "Enter password", lang + 4 },
  { "enterfilename", "Enter filename", lang + 5 },
  { "enterusername", "Enter username", lang + 6 },
  { "emptyurl", "<h1>Error</h1>Empty URL.  Document not loaded.", lang + 7 },
  { "display", "Displaying document...", lang + 8 },
  { "emptyfilename", "<h1>Error</h1>Empty filename.  Document not saved.", lang + 9 },
  { "invalidfilename", "<h1>Error</h1>Invalid filename.  Document not saved.", lang + 10 },
  { "notsaved", "<h1>Error</h1>There was an error while the document was being saved.  Could not save document.", lang + 11 },
  { "invalidurl", "<h1>Error</h1>Invalid URL.", lang + 12 },
  { "emptystring", "<h1>Error</h1>Empty search string.", lang + 13 },
  { "searchfailed", "<h1>Error</h1>Search failed.", lang + 14 },
  { "entersearch", "Enter search string", lang + 15 },
  { "emptyprinter", "<h1>Error</h1>Empty printer name.  Document not printed.", lang + 16 },
  { "notppipe", "<h1>Error</h1>Could not open a pipe to the print command.", lang + 17 },
  { "notpdata", "<h1>Error</h1>Could not send data to the print command.", lang + 18 },
  { "emptyemail", "<h1>Error</h1>Empty email address.  Email not sent.", lang + 19 },
  { "notepipe", "<h1>Error</h1>Could not open a pipe to the mail command. Email not sent.", lang + 20 },
  { "notedata", "<h1>Error</h1>Could not send data to the mail command.  Email not sendt.", lang + 21 },
  { "bookconfig", "<h1>Info</h1>It is possible that there is a problem with your bookmark configuration.  Bookmark list not available.", lang + 22 },
  { "savebook", "<h1>Info</h1>There was an error while the bookmark file was being written.  Make sure that you have enough diskspace to save your bookmarks.", lang + 23 },
  { "invalidaction", "<h1>Error</h1>Invalid ACTION (URL) in FORM.", lang + 24 },
  { "emptygroup", "<h1>Info</h1>Empty group name.  Group not added.", lang + 25 },
  { "nowhite", "<h1>Info</h1>Group names cannot have whitespace in them.", lang + 26 },
  { "entergroup", "Enter group name", lang + 27 },
  { "onegroup", "<h1>Error</h1>There must be at least one bookmark group.", lang + 28 },
  { "emptybookmark", "<h1>Info</h1>Empty bookmark name.  Bookmark not added.", lang + 29 },
  { "emptygroup", "<h1>Info</h1>Empty group name.  Group not added.", lang + 30 },
  { "Bookmarks", "Bookmarks", lang + 31 },
  { "notsaveext", "<h1>Error</h1>Could not save data for external viewer.  Check to make sure that you have enough temporary diskspace.", lang + 32 },
  { "abort", "<h1>Info/Error</h1>The transfer was either aborted by the user or an error occurred during the download.", lang + 33 },
  { "g<index>", "<h1>Search</h1>Enter a search specification. <isindex>", lang + 34 },
  { "nodata", "<h1>Info</h1>No data available.", lang + 35 },
  { "localaccess", "<h1>Error</h1>Could not access local file.  Either the file does not exist or you do not have permission to access it", lang + 36 },
  { "absurl", "<h1>Error</h1>Invalid URL.  You must specify an absolute URL.", lang + 37 },
  { "nopipe", "<h1>Error</h1>Could not open pipe to ", lang + 38 },
  { "absrurl", "<h1>Error</h1>The proxy URL must be an absolute URL.", lang + 39 },
  { "noload", "<h1>Error</h1>Could not load document.", lang + 40 },
  { "loops", "<h1>Info</h1>Document loops to itself.", lang + 41 },
  { "invlocation", "<h1>Error</h1>Invalid relocation in reply header.", lang + 42 },
  { "nosave", "<h1>Error</h1>Could not save temporary file.  Make sure that you have enough temporary disk space.", lang + 43 },
  { "nopipedata", "<h1>Error</h1>No data read from ", lang + 44 },
  { "noexec", "<h1>Error</h1>Could not execute ", lang + 45 },
  { "nofirst", "<h1>Error</h1>Could not load the first document because the document did not exist or the URL was incorrect.", lang + 46 },
  { "crash", "Crash and burn on default URL.  Send email to john@cs.unlv.edu\n", lang + 47 },
  { "needpw", "<h1>Info</h1>This document requires a password.", lang + 48 },
  { "byte2", "%d bytes out of %d loaded", lang + 49 },
  { "byte1", "%d bytes loaded", lang + 50 },
  { "convfail", "<h1>Error</h1>Internal conversion of document failed.", lang + 51 },
  { "enterurl", "Enter URL", lang + 52 },
  { "nothing", "nothing", NULL },
};

/*
 * AddLanguage
 *
 * Adds the language array or a string DB file to the string database
 */
void
AddLanguage(filename)
char *filename;
{
  FILE *fp;
  char buffer[BUFSIZ];
  char name[BUFSIZ];
  char value[BUFSIZ];

  if (filename == NULL) AddListToStringDB(lang);
  else
  {
    filename = FixFilename(filename);
    if (filename == NULL) return; /* exit quietly ... */

    fp = fopen(filename, "r");
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
      sscanf(buffer, "%[^:]:%[^\n]", name, value);
      AddToStringDB(name, value);
    }

    fclose(fp);
  }

  return;
}
