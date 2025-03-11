/*
 * http.h
 *
 * Copyright (C) 1994, John Kilburg.
 *
 * See copyright.h for details.
 */

#ifndef _NO_PROTO
Document *ParseHTTPResponse(int);
Document *ParseHTTPRequest(int);
Document *http(URLParts *, MIMEType *);
Document *http_proxy(URLParts *, URLParts *, MIMEType *, int);
int http_request(int, URLParts *, int, int);
#else
Document *ParseHTTPResponse();
Document *ParseHTTPRequest();
Document *http();
Document *http_proxy();
int http_request();
#endif
