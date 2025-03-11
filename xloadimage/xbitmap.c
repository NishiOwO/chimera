/* xbitmap.c:
 *
 * mangled by john to work with chimera.
 * john@cs.unlv.edu
 *
 * at one time this was XRdBitF.c.  it bears very little resemblence to it
 * now.  that was ugly code.  this is cleaner, faster, and more reliable
 * in most cases.
 *
 * jim frost 10.06.89
 *
 * Copyright, 1987, Massachusetts Institute of Technology
 *
 * Copyright 1989 Jim Frost.  See included file "copyright.h" for complete
 * copyright information.
 */

#include "mit.cpyrght"
#include "copyright.h"
#include <ctype.h>
#include "image.h"

/* SUPPRESS 560 */

#define MAX_SIZE 255

static short        HexTable[256];  /* conversion value */
static unsigned int Initialized= 0; /* easier to fill in at run time */

static unsigned char *bitmap_data;
static int          bitmap_pos;
static int          bitmap_datalen;

#define b0000 0 /* things make more sense if you see them by bit */
#define b0001 1
#define b0010 2
#define b0011 3
#define b0100 4
#define b0101 5
#define b0110 6
#define b0111 7
#define b1000 8
#define b1001 9
#define b1010 10
#define b1011 11
#define b1100 12
#define b1101 13
#define b1110 14
#define b1111 15

#define HEXSTART -1
#define HEXDELIM -2
#define HEXBAD   -3

/*
 * bitmap_read
 *
 * "read" bitmap data
 */
static int
bitmap_read(b, blen)
unsigned char *b;
int blen;
{
  if (bitmap_datalen < blen + bitmap_pos) return(0);

  memcpy(b, bitmap_data + bitmap_pos, blen);

  bitmap_pos += blen;

  return(blen);
}

/*
 * bitmap_gets
 */
static int
bitmap_gets(b, blen)
unsigned char *b;
int blen;
{
  int i;
  unsigned char x;

  for (i = 0; i < blen - 1; i++)
  {
    if (bitmap_read(&x, 1) == 0) return(0);
    if (x == '\r')
    {
      bitmap_read(&x, 1);
      break;
    }
    else if (x == '\n') break;
    b[i] = x;
  }

  b[i] = '\0';

  return(i);
}

/*
 * bitmap_getc
 */
static int
bitmap_getc()
{
  unsigned char b;

  if (bitmap_read(&b, 1) == 0) return(EOF);

  return((int)b);
}

/* build a hex digit value table with the bits inverted
 */

static void initHexTable()
{ int a;

  for (a= 0; a < 256; a++)
    HexTable[a]= HEXBAD;

  HexTable['0']= b0000;
  HexTable['1']= b1000;
  HexTable['2']= b0100;
  HexTable['3']= b1100;
  HexTable['4']= b0010;
  HexTable['5']= b1010;
  HexTable['6']= b0110;
  HexTable['7']= b1110;
  HexTable['8']= b0001;
  HexTable['9']= b1001;
  HexTable['A']= b0101; HexTable['a']= HexTable['A'];
  HexTable['B']= b1101; HexTable['b']= HexTable['B'];
  HexTable['C']= b0011; HexTable['c']= HexTable['C'];
  HexTable['D']= b1011; HexTable['d']= HexTable['D'];
  HexTable['E']= b0111; HexTable['e']= HexTable['E'];
  HexTable['F']= b1111; HexTable['f']= HexTable['F'];
  HexTable['x']= HEXSTART;
  HexTable['\r']= HEXDELIM;
  HexTable['\n']= HEXDELIM;
  HexTable['\t']= HEXDELIM;
  HexTable[' ']= HEXDELIM;
  HexTable[',']= HEXDELIM;
  HexTable['}']= HEXDELIM;

  Initialized = 1;
}

/* read a hex value and return its value
 */

static int
nextInt()
{
  int c;
  int value= 0;
  int shift= 0;
    
  for (;;)
  {
    c = bitmap_getc();
    if (c == EOF) return(-1);
    else
    {
      c = HexTable[c & 0xff];
      switch(c)
      {
	case HEXSTART:
	  shift = 0; /* reset shift counter */
	  break;
        case HEXDELIM:
	  if (shift == 4) return(value << 4);
	  if (shift) return(value);
	  break;
        case HEXBAD:
	  return(-1);
        default:
	  value += (c << shift);
	  shift += 4;
      }
    }
  }
}

Image *xbitmapLoad(data, datalen, bg)
unsigned char *data;
int datalen;
RGBColor *bg;
{
  Image        *image;
  char          line[MAX_SIZE];
  char          name_and_type[MAX_SIZE];
  char         *type;
  int           value;
  unsigned int  linelen;
  unsigned int  x, y;
  unsigned int  w = 0, h = 0;
  byte         *dataptr;

  bitmap_data = data;
  bitmap_datalen = datalen;
  bitmap_pos = 0;

  if (!Initialized) initHexTable();

  /* get width/height values */

  while (bitmap_gets(line, MAX_SIZE))
  {
    if (strlen(line) == MAX_SIZE - 1) return(NULL);

    /* width/height/hot_x/hot_y scanning
     */
    if (sscanf(line,"#define %s %d", name_and_type, &value) == 2)
    {
      if (!(type = rindex(name_and_type, '_'))) type = name_and_type;
      else type++;

      if (!strcmp("width", type)) w = (unsigned int)value;
      else if (!strcmp("height", type)) h = (unsigned int)value;
    }

    /* check for start of data
     */
    if ((sscanf(line,"static unsigned char %s = {", name_and_type) == 1)
	|| (sscanf(line, "static char %s = {", name_and_type) == 1))
    {
      break;
    }
  }

  if (!w || !h) return(NULL);

  image = newBitImage(w, h);

  /* read bitmap data
   */
  linelen = (w / 8) + (w % 8 ? 1 : 0); /* internal line length */
  dataptr = image->data;
  for (y = 0; y < h; y++)
  {
    for (x = 0; x < linelen; x++)
    {
      if ((value = nextInt()) >= 0) *(dataptr++) = value;
    }
  }
  
  return(image);
}
