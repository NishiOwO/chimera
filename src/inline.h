/*
 * inline.h
 *
 * Copyright (C) 1993, 1994, John Kilburg.
 *
 * See copyright.h for details.
 */

#define COLOR_DISPLAY 0
#define GRAY_DISPLAY 1
#define MONO_DISPLAY 2

#ifndef _NO_PROTO
ImageInfo *CreateImageInfo(Widget, Document *, Convert *, char *, int, int, XColor *);
#else
ImageInfo *CreateImageInfo();
#endif
