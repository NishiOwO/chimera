/* pbm.c:
 *
 * portable bit map (pbm) format images
 *
 * jim frost 09.27.89
 *
 * patched by W. David Higgins (wdh@mkt.csd.harris.com) to support
 * raw-format PBM files.
 *
 * patched by Ian MacPhedran (macphed@dvinci.usask.ca) to support
 * PGM and PPM files (03-July-1990)
 */

#include "image.h"
#include "pbm.h"

/* SUPPRESS 558 */

static char *pbm_data;
static int pbm_datalen;
static int pbm_pos;

static int          IntTable[256];
static unsigned int Initialized= 0;

#define NOTINT  -1
#define COMMENT -2
#define SPACE   -3
#define NEWLINE -4

#define BADREAD    0 /* read error */
#define NOTPBM     1 /* not a pbm file */
#define PBMNORMAL  2 /* pbm normal type file */
#define PBMCOMPACT 3 /* pbm compacty type file */
#define PBMRAWBITS 4 /* pbm raw bits type file */
#define PGMNORMAL  5 /* pgm normal type file */
#define PGMRAWBITS 6 /* pgm raw bytes type file */
#define PPMNORMAL  7 /* ppm normal type file */
#define PPMRAWBITS 8 /* ppm raw bytes type file */

static void initializeTable()
{ unsigned int a;

  for (a= 0; a < 256; a++)
    IntTable[a]= NOTINT;
  IntTable['#']= COMMENT;
  IntTable['\n']= NEWLINE;
  IntTable['\r']= IntTable['\t']= IntTable[' ']= SPACE;
  IntTable['0']= 0;
  IntTable['1']= 1;
  IntTable['2']= 2;
  IntTable['3']= 3;
  IntTable['4']= 4;
  IntTable['5']= 5;
  IntTable['6']= 6;
  IntTable['7']= 7;
  IntTable['8']= 8;
  IntTable['9']= 9;
  Initialized= 1;
}

/*
 * read data
 */
static int
pbm_read(b, blen)
unsigned char *b;
int blen;
{
  if (pbm_datalen < blen + pbm_pos) return(0);

  memcpy(b, pbm_data + pbm_pos, blen);
  pbm_pos += blen;

  return(blen);
}

static int
pbm_getc()
{
  unsigned char b;

  if (pbm_read(&b, 1) == 0) return(EOF);
  return((int)b);
}

static int
pbmReadChar()
{
  int c;

  if ((c = pbm_getc()) == EOF) return(-1);

  if (IntTable[c] == COMMENT)
  {
    do
    {
      if ((c = pbm_getc()) == EOF) return(-1);
    } while (IntTable[c] != NEWLINE);
  }

  return(c);
}

static int
pbmReadInt()
{
  int c, value;

  for (;;)
  {
    c = pbmReadChar();
    if (c < 0) return(-1);
    if (IntTable[c] >= 0) break;
  }

  value= IntTable[c];
  for (;;)
  {
    c = pbmReadChar();
    if (c < 0) return(-1);
    if (IntTable[c] < 0) return(value);
    value = (value * 10) + IntTable[c];
  }
}

static int isPBM(width, height, maxval)
int *width, *height, *maxval;
{
  byte buf[4];

  if (! Initialized) initializeTable();

  if (pbm_read(buf, 2) != 2)
    return(NOTPBM);
  if (memToVal((byte *)buf, 2) == memToVal((byte *)"P1", 2)) {
    if (((*width= pbmReadInt()) < 0) || ((*height= pbmReadInt()) < 0))
      return(NOTPBM);
    *maxval = 1;
    return(PBMNORMAL);
  }
  if (memToVal((byte *)buf, 2) == memToVal((byte *)"P4", 2)) {
    if (((*width= pbmReadInt()) < 0) || ((*height= pbmReadInt()) < 0))
      return(NOTPBM);
    *maxval = 1;
    return(PBMRAWBITS);
  }
  if (memToVal(buf, 2) == 0x2a17) {
    if (pbm_read(buf, 4) != 4)
      return(NOTPBM);
    *width= memToVal((byte *)buf, 2);
    *height= memToVal((byte *)(buf + 2), 2);
    *maxval = 1;
    return(PBMCOMPACT);
  }
  if (memToVal(buf, 2) == memToVal((byte *)"P2", 2)) {
    if (((*width= pbmReadInt()) < 0) || ((*height= pbmReadInt()) < 0))
      return(NOTPBM);
    *maxval = pbmReadInt();
    return(PGMNORMAL);
  }
  if (memToVal(buf, 2) == memToVal((byte *)"P5", 2)) {
    if (((*width= pbmReadInt()) < 0) || ((*height= pbmReadInt()) < 0))
      return(NOTPBM);
    *maxval = pbmReadInt();
    return(PGMRAWBITS);
  }
  if (memToVal(buf, 2) == memToVal((byte *)"P3", 2)) {
    if (((*width= pbmReadInt()) < 0) || ((*height= pbmReadInt()) < 0))
      return(NOTPBM);
    *maxval = pbmReadInt();
    return(PPMNORMAL);
  }
  if (memToVal(buf, 2) == memToVal((byte *)"P6", 2)) {
    if (((*width= pbmReadInt()) < 0) || ((*height= pbmReadInt()) < 0))
      return(NOTPBM);
    *maxval = pbmReadInt();
    return(PPMRAWBITS);
  }
  return(NOTPBM);
}

Image *
pbmLoad(data, datalen, bg)
char *data;
int datalen;
RGBColor *bg;
{
  Image        *image;
  int           pbm_type;
  int           x, y;
  int           width, height, maxval, depth;
  unsigned int  linelen;
  byte          srcmask, destmask;
  byte         *destptr, *destline;
  int           src, size;
  int           red, grn, blu;

  pbm_data = data;
  pbm_datalen = datalen;
  pbm_pos = 0;

  pbm_type= isPBM(&width, &height, &maxval);
  if (pbm_type == NOTPBM) return(NULL);

  switch (pbm_type) {
  case PBMNORMAL:
    image = newBitImage(width, height);
    linelen = (width / 8) + (width % 8 ? 1 : 0);
    destline = image->data;
    for (y = 0; y < height; y++) {
      destptr = destline;
      destmask = 0x80;
      for (x = 0; x < width; x++) {
	do {
	  if ((src = pbmReadChar()) < 0) return(image);
	  if (IntTable[src] == NOTINT) return(image);
	} while (IntTable[src] < 0);
	
	switch (IntTable[src]) {
	case 1:
	  *destptr |= destmask;
	case 0:
	  if (! (destmask >>= 1)) {
	    destmask= 0x80;
	    destptr++;
	  }
	  break;
	default:
	  return(image);
	}
      }
      destline += linelen;
    }
    break;

  case PBMRAWBITS:
    image = newBitImage(width, height);
    destline = image->data;
    linelen = (width + 7) / 8;
    srcmask = 0;	       /* force initial read */
    for (y = 0; y < height; y++) {
      destptr = destline;
      destmask = 0x80;
      if (srcmask != 0x80) {
        srcmask = 0x80;
	src = pbm_getc();
	if (src == EOF) return(image);
      }
      for (x= 0; x < width; x++) {
	if (src & srcmask)
	  *destptr |= destmask;
	if (! (destmask >>= 1)) {
	  destmask= 0x80;
	  destptr++;
	}
	if (! (srcmask >>= 1)) {
	  srcmask= 0x80;
	  src = pbm_getc();
	  if (src == EOF) return(image);
	}
      }
      destline += linelen;
    }
    break;
 
  case PBMCOMPACT:
    image = newBitImage(width, height);
    destline = image->data;
    linelen = (width / 8) + (width % 8 ? 1 : 0);
    srcmask= 0x80;
    destmask= 0x80;
    src = pbm_getc();
    if (src == EOF) return(image);
    for (y= 0; y < height; y++) {
      destptr= destline;
      destmask= 0x80;
      for (x= 0; x < width; x++) {
	if (src & srcmask)
	  *destptr |= destmask;
	if (! (destmask >>= 1)) {
	  destmask= 0x80;
	  destptr++;
	}
	if (! (srcmask >>= 1)) {
	  srcmask= 0x80;
	  src = pbm_getc();
	  if (src == EOF) return(image);
	}
      }
      destline += linelen;
    }
    break;
  case PGMRAWBITS:
    depth= colorsToDepth(maxval);
    if (depth > 8)
      image = newTrueImage(width, height);
    else {
      image = newRGBImage(width, height, depth);
      for (y = 0; y <= maxval; y++)
	{ /* As in sunraster.c, use simple ramp for grey scale */
	  *(image->rgb.red + y) = PM_SCALE(y, maxval, 0xffff);
	  *(image->rgb.green + y) = PM_SCALE(y, maxval, 0xffff);
	  *(image->rgb.blue + y) = PM_SCALE(y, maxval, 0xffff);
	}
      image->rgb.used = maxval+1;
    }
    size= height * width;

    switch (image->type) {
    case IRGB:

      /* read in the image in a chunk
       */

      if (pbm_read(image->data, size) != size) {
	freeImage(image);
	return(NULL);
      }
      break;

    case ITRUE:
      destptr = image->data;
      for (y = 0; y < size; y++) {
	if ((src = pbm_getc()) == EOF) {
	  freeImage(image);
	  return(NULL);
	}
	src = PM_SCALE(src, maxval, 0xff);
	*(destptr++) = src; /* red */
	*(destptr++) = src; /* green */
	*(destptr++) = src; /* blue */
      }
      break;
    }
    break;
  case PGMNORMAL:
    depth= colorsToDepth(maxval);
    if (depth > 8)
      image= newTrueImage(width, height);
    else {
      image= newRGBImage(width, height, depth);
      for (y= 0; y <= maxval; y++)
	{ /* As in sunraster.c, use simple ramp for grey scale */
	  *(image->rgb.red + y) = PM_SCALE(y, maxval, 0xffff);
	  *(image->rgb.green + y) = PM_SCALE(y, maxval, 0xffff);
	  *(image->rgb.blue + y) = PM_SCALE(y, maxval, 0xffff);
	}
      image->rgb.used = maxval+1;
    }
    destptr= image->data;
    size= height * width;
    for (y= 0; y < size; y++) {
      if ((src = pbmReadInt()) < 0) return(image);
      else {
	if (TRUEP(image)) {
	  src= PM_SCALE(src, maxval, 0xff);
	  *(destptr++) = src; /* red */
	  *(destptr++) = src; /* green */
	  *(destptr++) = src; /* blue */
	}
	else
	  *(destptr++) = src;
      }
    }
    break;

  case PPMRAWBITS:
    image = newTrueImage(width, height);
    size = height * width * 3;
    if (pbm_read(image->data, size) != size) {
      freeImage(image);
      return(NULL);
    }
    destptr = image->data;
    for (y = 0; y < size; y++) {
      *destptr = PM_SCALE(*destptr, maxval, 0xff);
      destptr++;
    }
    break;
  case PPMNORMAL:
    image= newTrueImage(width, height);
    size= height * width;
    destptr= image->data;
    for (y= 0; y < size; y++) {
      if (((red= pbmReadInt()) == EOF) ||
	  ((grn= pbmReadInt()) == EOF) ||
	  ((blu= pbmReadInt()) == EOF))
	{
	  return(image);
	}
      *(destptr++)= PM_SCALE(red, maxval, 0xff);
      *(destptr++)= PM_SCALE(grn, maxval, 0xff);
      *(destptr++)= PM_SCALE(blu, maxval, 0xff);
    }
    break;
  }
  return(image);
}
