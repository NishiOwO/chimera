/* xpixmap.c:
 *
 * This has been mangled to work with Chimera.  It has also been "fixed"
 * so that it handles XPM3 according to the document on ftp.x.org.
 * this code is really slow...probably because it asks the server about
 * every color.
 * john@cs.unlv.edu
 *
 * XPixMap format file read and identify routines.  these can handle any
 * "format 1" XPixmap file with up to BUFSIZ - 1 chars per pixel.  it's
 * not nearly as picky as it might be.
 *
 * unlike most image loading routines, this is X specific since it
 * requires X color name parsing.
 *
 * Copyright 1989 Jim Frost.  See included file "copyright.h" for complete
 * copyright information.
 */

#include "copyright.h"
#include "xloadimage.h"

/* SUPPRESS 530 */
/* SUPPRESS 560 */

static unsigned char *xpixmap_data;
static int xpixmap_datalen;
static int xpixmap_pos;

/*
 * read data
 */
static int
xpixmap_read(b, blen)
unsigned char *b;
int blen;
{
  if (xpixmap_datalen < blen + xpixmap_pos) return(0);

  memcpy(b, xpixmap_data + xpixmap_pos, blen);
  xpixmap_pos += blen;

  return(blen);
}

/*
 * xpixmap_gets
 */
static int
xpixmap_gets(b, blen)
unsigned char *b;
int blen;
{
  int i;
  unsigned char x;

  for (i = 0; i < blen - 1; i++)
  {
    if (xpixmap_read(&x, 1) == 0) return(0);
    if (x == '\r')
    {
      xpixmap_read(&x, 1);
      break;
    }
    else if (x == '\n') break;
    b[i] = x;
  }

  b[i] = '\0';

  return(i);
}

/*
 * xpixmap_getc
 */
static int
xpixmap_getc()
{
  unsigned char b;

  if (xpixmap_read(&b, 1) == 1) return((int)b);
  return(EOF);
}

Image *xpixmapLoad(Disp, Scrn, data, datalen, bg)
Display *Disp;
int Scrn;
unsigned char *data;
int datalen;
RGBColor *bg;
{
  char           buf[BUFSIZ];
  char           cval[BUFSIZ];
  char           key;
  char           pixel[BUFSIZ];
  unsigned int   value;
  unsigned int   w, h;    /* image dimensions */
  unsigned int   cpp;     /* chars per pixel */
  unsigned int   ncolors; /* number of colors */
  unsigned int   hx, hy;  /* hot spot */
  unsigned int   depth;   /* depth of image */
  char         **ctable;  /* color table */
  Image         *image;
  XColor         xcolor;
  unsigned int   a, x, y;
  int            c;
  byte          *dptr;

  /* read #defines until we have all that are necessary or until we
   * get an error
   */

  xpixmap_data = data;
  xpixmap_datalen = datalen;
  xpixmap_pos = 0;

  /* header */
  if (xpixmap_gets(buf, BUFSIZ - 1) == 0) return(NULL);
  if (strcmp(buf, "/* XPM */") != 0) return(NULL);

  /* declaration and beginning of assignment line */
  if (xpixmap_gets(buf, BUFSIZ - 1) == 0) return(NULL);

  /* values */
  for (;;)
  {
    if (xpixmap_gets(buf, BUFSIZ - 1) == 0) return(NULL);
    if (buf[0] == '"') break;
  }

  w = h = ncolors = cpp = hx = hy = 0;
  sscanf(buf, "\"%d %d %d %d %d %d\",", &w, &h, &ncolors, &cpp, &hx, &hy);
  if (w == 0 || h == 0 || ncolors == 0 || cpp == 0) return(NULL);

  for (depth= 1, value= 2; value < ncolors; value <<= 1, depth++)
    ;
  image = newRGBImage(w, h, depth);
  image->rgb.used = ncolors;

  /* colors */
  /* values */
  for (;;)
  {
    if (xpixmap_gets(buf, BUFSIZ - 1) == 0) return(NULL);
    if (buf[0] == '"') break;
  }

  ctable= (char **)lmalloc(sizeof(char *) * ncolors);
  xcolor.flags= DoRed | DoGreen | DoBlue;
  for (a = 0; a < ncolors; a++)
  {
    *(ctable + a)= (char *)lmalloc(cpp);
    buf[strlen(buf) - 2] = '\0';
    sscanf(buf + 1, "%s %s %s", pixel, &key, cval);
    strncpy(*(ctable + a), pixel, cpp);

    if (Disp)
    {
      if (!XParseColor(Disp, DefaultColormap(Disp, Scrn), cval, &xcolor))
      {
	return(NULL);
      }
      *(image->rgb.red + a) = xcolor.red;
      *(image->rgb.green + a) = xcolor.green;
      *(image->rgb.blue + a) = xcolor.blue;
    }

    if (xpixmap_gets(buf, BUFSIZ - 1) == 0) return(NULL);
  }

  /* pixels */

  dptr= image->data;
  for (y= 0; y < h; y++)
  {
    while (((c = xpixmap_getc()) != EOF) && (c != '"'))
	;

    for (x = 0; x < w; x++)
    {
      for (a = 0; a < cpp; a++)
      {
	if ((c = xpixmap_getc()) == '\\') c = xpixmap_getc();
	if (c == EOF) return(NULL);
	buf[a] = (char)c;
      }
      for (a = 0; a < ncolors; a++)
      {
	if (!strncmp(*(ctable + a), buf, cpp))
	    break;
      }
      if (a == ncolors) return(NULL);
      valToMem((unsigned long)a, dptr, image->pixlen);
      dptr += image->pixlen;
    }
    if ((c = xpixmap_getc()) != '"') return(NULL);
  }
  return(image);
}
