/*
 * local.h
 *
 * Copyright (C) 1993, 1994, John Kilburg.
 *
 * See copyright.h for details.
 */

#ifndef _NO_PROTO
Document *file(URLParts *, MIMEType *);
Document *telnet(URLParts *, MIMEType *);
Document *tn3270(URLParts *, MIMEType *);
#else
Document *file();
Document *telnet();
Document *tn3270();
#endif


