/* gif.c:
 *
 * Mangled by John Kilburg (john@cs.unlv.edu) for use with chimera.
 *
 * adapted from code by kirk johnson (tuna@athena.mit.edu).  most of this
 * code is unchanged. -- jim frost 12.31.89
 *
 * gifin.c
 * kirk johnson
 * november 1989
 *
 * routines for reading GIF files
 *
 * Copyright 1989 Kirk L. Johnson (see the included file
 * "kljcpyrght.h" for complete copyright information)
 */

#include "image.h"
#include "gif.h"
#include "kljcpyrght.h"

/****
 **
 ** local #defines
 **
 ****/

#define PUSH_PIXEL(p)                                       \
{                                                           \
  if (pstk_idx == PSTK_SIZE)                                \
    ;                                                       \
  else                                                      \
    pstk[pstk_idx++] = (p);                                 \
}

/****
 **
 ** local variables
 **
 ****/

static int interlace_start[4]= { /* start line for interlacing */
  0, 4, 2, 1
};

static int interlace_rate[4]= { /* rate at which we accelerate vertically */
  8, 8, 4, 2
};

static BYTE file_open  = 0;     /* status flags */
static BYTE image_open = 0;

static int  root_size;          /* root code size */
static int  clr_code;           /* clear code */
static int  eoi_code;           /* end of information code */
static int  code_size;          /* current code size */
static int  code_mask;          /* current code mask */
static int  prev_code;          /* previous code */

/*
 * NOTE: a long is assumed to be at least 32 bits wide
 */
static long work_data;          /* working bit buffer */
static int  work_bits;          /* working bit count */

static BYTE buf[256];           /* byte buffer */
static int  buf_cnt;            /* byte count */
static int  buf_idx;            /* buffer index */

static int table_size;          /* string table size */
static int prefix[STAB_SIZE];   /* string table : prefixes */
static int extnsn[STAB_SIZE];   /* string table : extensions */

static BYTE pstk[PSTK_SIZE];    /* pixel stack */
static int  pstk_idx;           /* pixel stack pointer */


/****
 **
 ** global variables
 **
 ****/

static int  gifin_pos;                 /* current read position */
static int  gifin_datalen;             /* length of input data */
static char *gifin_data;               /* address of data */
static int  gifin_rast_width;          /* raster width */
static int  gifin_rast_height;         /* raster height */
static BYTE gifin_g_cmap_flag;         /* global colormap flag */
static int  gifin_g_pixel_bits;        /* bits per pixel, global colormap */
static int  gifin_g_ncolors;           /* number of colors, global colormap */
static BYTE gifin_g_cmap[3][256];      /* global colormap */
static int  gifin_bg_color;            /* background color index */
static int  gifin_color_bits;          /* bits of color resolution */
static int  gifin_transparent;         /* transparent color index */

static int  gifin_img_left;            /* image position on raster */
static int  gifin_img_top;             /* image position on raster */
static int  gifin_img_width;           /* image width */
static int  gifin_img_height;          /* image height */
static BYTE gifin_l_cmap_flag;         /* local colormap flag */
static int  gifin_l_pixel_bits;        /* bits per pixel, local colormap */
static int  gifin_l_ncolors;           /* number of colors, local colormap */
static BYTE gifin_l_cmap[3][256];      /* local colormap */
static BYTE gifin_interlace_flag;      /* interlace image format flag */

/*
 * read GIF data
 */
static int
gif_read(b, blen)
unsigned char *b;
int blen;
{
  if (gifin_datalen < blen + gifin_pos) return(0);

  memcpy(b, gifin_data + gifin_pos, blen);
  gifin_pos += blen;

  return(blen);
}

/*
 * open a GIF file, using s as the input stream
 */
static int gifin_open_file()
{
  /* make sure there isn't already a file open */
  if (file_open)
    return GIFIN_ERR_FAO;

  /* remember that we've got this file open */
  file_open = 1;

  /* check GIF signature */
  if (gif_read(buf, GIF_SIG_LEN) != GIF_SIG_LEN)
    return GIFIN_ERR_EOF;

  buf[GIF_SIG_LEN] = '\0';
  if ((strcmp((char *) buf, GIF_SIG) != 0) &&
      (strcmp((char *) buf, GIF_SIG_89) != 0))
    return GIFIN_ERR_BAD_SIG;

  /* read screen descriptor */
  if (gif_read(buf, GIF_SD_SIZE) != GIF_SD_SIZE)
    return GIFIN_ERR_EOF;

  /* decode screen descriptor */
  gifin_rast_width   = (buf[1] << 8) + buf[0];
  gifin_rast_height  = (buf[3] << 8) + buf[2];
  gifin_g_cmap_flag  = (buf[4] & 0x80) ? 1 : 0;
  gifin_color_bits   = ((buf[4] & 0x70) >> 4) + 1;
  gifin_g_pixel_bits = (buf[4] & 0x07) + 1;
  gifin_bg_color     = buf[5];

  if (buf[6] != 0) 
    return GIFIN_ERR_BAD_SD;

  /* load global colormap */
  if (gifin_g_cmap_flag)
  {
    gifin_g_ncolors = (1 << gifin_g_pixel_bits);

    if (gifin_load_cmap(gifin_g_cmap, gifin_g_ncolors) != GIFIN_SUCCESS)
      return GIFIN_ERR_EOF;
  }
  else
  {
    gifin_g_ncolors = 0;
  }

  /* done! */
  return GIFIN_SUCCESS;
}


/*
 * open next GIF image in the input stream; returns GIFIN_SUCCESS if
 * successful. if there are no more images, returns GIFIN_DONE. (might
 * also return various GIFIN_ERR codes.)
 */

static int gifin_open_image()
{
  int i;
  int separator;
  unsigned char b;

  /* make sure there's a file open */
  if (!file_open)
    return GIFIN_ERR_NFO;

  /* make sure there isn't already an image open */
  if (image_open)
    return GIFIN_ERR_IAO;

  /* remember that we've got this image open */
  image_open = 1;

  /* skip over any extension blocks */
  do
  {
    gif_read(&b, 1);
    separator = (int)b;
    if (separator == GIF_EXTENSION)
    {
      if (gifin_skip_extension() != GIFIN_SUCCESS)
        return GIFIN_ERR_EOF;
    }
  }
  while (separator == GIF_EXTENSION);

  /* check for end of file marker */
  if (separator == GIF_TERMINATOR)
    return GIFIN_DONE;

  /* make sure we've got an image separator */
  if (separator != GIF_SEPARATOR)
    return GIFIN_ERR_BAD_SEP;

  /* read image descriptor */
  if (gif_read(buf, GIF_ID_SIZE) != GIF_ID_SIZE)
    return GIFIN_ERR_EOF;

  /* decode image descriptor */
  gifin_img_left       = (buf[1] << 8) + buf[0];
  gifin_img_top        = (buf[3] << 8) + buf[2];
  gifin_img_width      = (buf[5] << 8) + buf[4];
  gifin_img_height     = (buf[7] << 8) + buf[6];
  gifin_l_cmap_flag    = (buf[8] & 0x80) ? 1 : 0;
  gifin_interlace_flag = (buf[8] & 0x40) ? 1 : 0;
  gifin_l_pixel_bits   = (buf[8] & 0x07) + 1;

  /* load local colormap */
  if (gifin_l_cmap_flag)
  {
    gifin_l_ncolors = (1 << gifin_l_pixel_bits);

    if (gifin_load_cmap(gifin_l_cmap, gifin_l_ncolors) != GIFIN_SUCCESS)
      return GIFIN_ERR_EOF;
  }
  else
  {
    gifin_l_ncolors = 0;
  }

  /* initialize raster data stream decoder */
  gif_read(&b, 1);
  root_size = (int)b;
  clr_code  = 1 << root_size;
  eoi_code  = clr_code + 1;
  code_size = root_size + 1;
  code_mask = (1 << code_size) - 1;
  work_bits = 0;
  work_data = 0;
  buf_cnt   = 0;
  buf_idx   = 0;

  /* initialize string table */
  for (i=0; i<STAB_SIZE; i++)
  {
    prefix[i] = NULL_CODE;
    extnsn[i] = i;
  }

  /* initialize pixel stack */
  pstk_idx = 0;

  /* done! */
  return GIFIN_SUCCESS;
}

/*
 * try to read next pixel from the raster, return result in *pel
 */

static int gifin_get_pixel(pel)
     int *pel;
{
  int  code;
  int  first;
  int  place;

  /* decode until there are some pixels on the pixel stack */
  while (pstk_idx == 0)
  {
    /* load bytes until we have enough bits for another code */
    while (work_bits < code_size)
    {
      if (buf_idx == buf_cnt)
      {
        /* read a new data block */
        if (gifin_read_data_block() != GIFIN_SUCCESS)
          return GIFIN_ERR_EOF;

        if (buf_cnt == 0)
          return GIFIN_ERR_EOD;
      }

      work_data |= ((long) buf[buf_idx++]) << work_bits;
      work_bits += 8;
    }

    /* get the next code */
    code        = work_data & code_mask;
    work_data >>= code_size;
    work_bits  -= code_size;

    /* interpret the code */
    if (code == clr_code)
    {
      /* reset decoder stream */
      code_size  = root_size + 1;
      code_mask  = (1 << code_size) - 1;
      prev_code  = NULL_CODE;
      table_size = eoi_code + 1;
    }
    else if (code == eoi_code)
    {
      /* Ooops! no more pixels */
      return GIFIN_ERR_EOF;
    }
    else if (prev_code == NULL_CODE)
    {
      gifin_push_string(code);
      prev_code = code;
    }
    else
    {
      if (code < table_size)
      {
        first = gifin_push_string(code);
      }
      else
      {
        place = pstk_idx;
        PUSH_PIXEL(NULL_CODE);
        first = gifin_push_string(prev_code);
        pstk[place] = first;
      }

      gifin_add_string(prev_code, first);
      prev_code = code;
    }
  }

  /* pop a pixel off the pixel stack */
  *pel = (int) pstk[--pstk_idx];

  /* done! */
  return GIFIN_SUCCESS;
}

#if 0
/*
 * close an open GIF image
 */

static int gifin_close_image()
{
  /* make sure there's an image open */
  if (!image_open)
    return GIFIN_ERR_NIO;

  /* skip any remaining raster data */
  do
  {
    if (gifin_read_data_block() != GIFIN_SUCCESS)
      return GIFIN_ERR_EOF;
  }
  while (buf_cnt > 0);

  /* mark image as closed */
  image_open = 0;

  /* done! */
  return GIFIN_SUCCESS;
}
#endif

/*
 * close an open GIF file
 */

static int gifin_cleanup()
{
  /* make sure there's a file open */
  if (!file_open)
    return GIFIN_ERR_NFO;

  /* mark file (and image) as closed */
  file_open  = 0;
  image_open = 0;

  /* done! */
  return GIFIN_SUCCESS;
}

/*
 * load a colormap from the input stream
 */

static int gifin_load_cmap(cmap, ncolors)
     BYTE cmap[3][256];
     int  ncolors;
{
  int i;

  for (i=0; i<ncolors; i++)
  {
    if (gif_read(buf, 3) != 3)
      return GIFIN_ERR_EOF;
    
    cmap[GIF_RED][i] = buf[GIF_RED];
    cmap[GIF_GRN][i] = buf[GIF_GRN];
    cmap[GIF_BLU][i] = buf[GIF_BLU];
  }

  /* done! */
  return GIFIN_SUCCESS;
}
 
/*
 * skip an extension block in the input stream
 */

static int gifin_skip_extension()
{
  unsigned char b;
  unsigned char gce[5];

  /* get the extension function byte */
  gif_read(&b, 1);
  if (b == 0xf9)
  {
    /*
     * This code is based on stuff I read in giftrans - john
     * giftrans by Andreas Ley <ley@rz.uni-karlsruhe.de>
     */
    gif_read(&b, 1); /* get the size */
    buf_cnt = (int)b;
    if (buf_cnt > 0)
    {
      gif_read(gce, buf_cnt);
      if (gce[0] & 0x1) gifin_transparent = gce[3];
      /* skip any remaining raster data */
      while (buf_cnt > 0)
      {
	if (gifin_read_data_block() != GIFIN_SUCCESS)
	    return GIFIN_ERR_EOF;
      }
    }
  }
  else
  {
    /* skip any remaining raster data */
    do
    {
      if (gifin_read_data_block() != GIFIN_SUCCESS)
	  return GIFIN_ERR_EOF;
    }
    while (buf_cnt > 0);
  }

  /* done! */
  return GIFIN_SUCCESS;
}

/*
 * read a new data block from the input stream
 */

static int gifin_read_data_block()
{
  unsigned char b;

  /* read the data block header */
  gif_read(&b, 1);
  buf_cnt = (int)b;

  /* read the data block body */
  if (gif_read(buf, buf_cnt) != buf_cnt)
    return GIFIN_ERR_EOF;

  buf_idx = 0;

  /* done! */
  return GIFIN_SUCCESS;
}

/*
 * push a string (denoted by a code) onto the pixel stack
 * (returns the code of the first pixel in the string)
 */

static int gifin_push_string(code)
     int code;
{
  int rslt;

  while (prefix[code] != NULL_CODE)
  {
    PUSH_PIXEL(extnsn[code]);
    code = prefix[code];
  }

  PUSH_PIXEL(extnsn[code]);
  rslt = extnsn[code];

  return rslt;
}

/*
 * add a new string to the string table
 */

static void gifin_add_string(p, e)
int p;
int e;
{
  prefix[table_size] = p;
  extnsn[table_size] = e;

  if ((table_size == code_mask) && (code_size < 12))
  {
    code_size += 1;
    code_mask  = (1 << code_size) - 1;
  }

  table_size += 1;

  return;
}

Image *
gifLoad(data, datalen, bg)
char *data;
int datalen;
RGBColor *bg;
{
  Image *image;
  int x, y, pixel, pass, scanlen;
  byte *pixptr, *pixline;

  gifin_data = data;
  gifin_datalen = datalen;
  gifin_pos = 0;
  gifin_transparent = -1;

  if ((gifin_open_file() != GIFIN_SUCCESS) || /* read GIF header */
      (gifin_open_image() != GIFIN_SUCCESS)) {  /* read image header */
    file_open  = 0;
    image_open = 0;
    return(NULL);
  }
  image = newRGBImage(gifin_img_width, gifin_img_height, (gifin_l_cmap_flag ?
							 gifin_l_pixel_bits :
							 gifin_g_pixel_bits));
  for (x = 0; x < gifin_g_ncolors; x++) {
    if (x == gifin_transparent)
    {
      image->rgb.red[x] = bg->red;
      image->rgb.green[x] = bg->green;
      image->rgb.blue[x] = bg->blue;
    }
    else
    {
      image->rgb.red[x] = gifin_g_cmap[GIF_RED][x] << 8;
      image->rgb.green[x] = gifin_g_cmap[GIF_GRN][x] << 8;
      image->rgb.blue[x] = gifin_g_cmap[GIF_BLU][x] << 8;
    }
  }
  image->rgb.used = gifin_g_ncolors;

  /* if image has a local colormap, override global colormap
   */

  if (gifin_l_cmap_flag) {
    for (x = 0; x < image->rgb.size; x++) {
      if (x == gifin_transparent)
      {
	image->rgb.red[x] = bg->red;
	image->rgb.green[x] = bg->green;
	image->rgb.blue[x] = bg->blue;
      }
      else
      {
	image->rgb.red[x] = gifin_g_cmap[GIF_RED][x] << 8;
	image->rgb.green[x] = gifin_g_cmap[GIF_GRN][x] << 8;
	image->rgb.blue[x] = gifin_g_cmap[GIF_BLU][x] << 8;
      }
    }
    image->rgb.used = gifin_l_ncolors;
  }

  /* interlaced image -- futz with the vertical trace.  i wish i knew what
   * kind of drugs the GIF people were on when they decided that they
   * needed to support interlacing.
   */

  if (gifin_interlace_flag) {
    scanlen = image->height * image->pixlen;

    /* interlacing takes four passes to read, each starting at a different
     * vertical point.
     */

    for (pass = 0; pass < 4; pass++) {
      y = interlace_start[pass];
      scanlen = image->width * image->pixlen * interlace_rate[pass];
      pixline = image->data + (y * image->width * image->pixlen);
      while (y < gifin_img_height) {
	pixptr = pixline;
	for (x = 0; x < gifin_img_width; x++) {
	  if (gifin_get_pixel(&pixel) != GIFIN_SUCCESS) {
	    y = gifin_img_height; x = gifin_img_width;
	  }
	  valToMem(pixel, pixptr, image->pixlen);
	  pixptr += image->pixlen;
	}
	y += interlace_rate[pass];
	pixline += scanlen;
      }
    }
  }

  /* not an interlaced image, just read in sequentially
   */

  else {
    pixptr = image->data;
    for (y = 0; y < gifin_img_height; y++)
      for (x = 0; x < gifin_img_width; x++) {
	if (gifin_get_pixel(&pixel) != GIFIN_SUCCESS) {
	  y = gifin_img_height; x = gifin_img_width;
	}
	valToMem(pixel, pixptr, image->pixlen);
	pixptr += image->pixlen;
      }
  }
  gifin_cleanup();
  return(image);
}
