/*
 * gopher.h
 *
 * Copyright (C) 1994, John Kilburg.
 *
 * See copyright.h for details.
 */

#ifndef _NO_PROTO
Document *gopherplus(URLParts *, MIMEType *);
Document *gopherplain(URLParts *, MIMEType *);
#else
Document *gopherplus();
Document *gopherplain();
#endif
