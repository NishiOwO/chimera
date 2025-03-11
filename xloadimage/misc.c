/* misc.c:
 *
 * miscellaneous funcs
 *
 * jim frost 10.05.89
 *
 * Copyright 1989, 1990, 1991 Jim Frost.
 * See included file "copyright.h" for complete copyright information.
 */

#include "copyright.h"
#include "xloadimage.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <signal.h>

void memoryExhausted()
{
  fprintf(stderr,
	  "\n\nMemory has been exhausted; operation cannot continue (sorry).\n");
  exit(1);
}

char *
tail(path)
char *path;
{ int   s;
  char *t;

  t= path;
  for (s= 0; *(path + s) != '\0'; s++)
    if (*(path + s) == '/')
      t= path + s + 1;
  return(t);
}

/*
  findstr - public-domain implementation of standard C 'strstr' library
            function (renamed and slightly modified to avoid naming
            conflicts - jimf)

  last edit:	02-Sep-1990	D A Gwyn

  This is an original implementation based on an idea by D M Sunday,
  essentially the "quick search" algorithm described in CACM V33 N8.
  Unlike Sunday's implementation, this one does not wander past the
  ends of the strings (which can cause malfunctions under certain
  circumstances), nor does it require the length of the searched
  text to be determined in advance.  There are numerous other subtle
  improvements too.  The code is intended to be fully portable, but in
  environments that do not conform to the C standard, you should check
  the sections below marked "configure as required".  There are also
  a few compilation options, as follows:
*/

#define BYTE_MAX 255

#define EOS '\0'		/* C string terminator */

char *					/* returns -> leftmost occurrence,
					   or null pointer if not present */
findstr( s1, s2 )
     char	*s1;		/* -> string to be searched */
     char	*s2;		/* -> search-pattern string */
{
  register byte	*t;		/* -> text character being tested */
  register byte	*p;		/* -> pattern char being tested */
  register byte	*tx;		/* -> possible start of match */
  register unsigned int	m;      /* length of pattern */
  register byte	*top;		/* -> high water mark in text */
  unsigned int  shift[BYTE_MAX + 1];	/* pattern shift table */

  if ( s1 == NULL || s2 == NULL )
    return NULL;		/* certainly, no match is found! */

  /* Precompute shift intervals based on the pattern;
     the length of the pattern is determined as a side effect: */

  bzero(&shift[1], (BYTE_MAX * sizeof(unsigned int)));

  /* Note: shift[0] is undefined at this point (fixed later). */

  for ( m = 1, p = (byte *)s2; *p != EOS; ++m, ++p )
    shift[(byte)*p] = m;

  {
    register byte c;

    c = BYTE_MAX;
    do
      shift[c] = m - shift[c];
    while ( --c > 0 );
    /* Note: shift[0] is still undefined at this point. */
  }

  shift[0] = --m; 		/* shift[EOS]; important details! */

  /* Try to find the pattern in the text string: */

  for ( top = tx = (byte *)s1; ; tx += shift[*(top = t)] ) {
    for ( t = tx, p = (byte *)s2; ; ++t, ++p ) {
      if ( *p == EOS )       /* entire pattern matched */
	return (char *)tx;
      if ( *p != *t )
	break;
    }
    if ( t < top ) /* idea due to ado@elsie.nci.nih.gov */
      t = top;	   /* already scanned this far for EOS */
    do	{
      if ( *t == EOS )
	return NULL;	/* no match */
    } while ( ++t - tx != m );	/* < */
  }
}
