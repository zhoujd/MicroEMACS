#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<stdarg.h>
#include	<stddef.h>
#include	<wchar.h>


#include "estruct.h"

typedef unsigned char uchar;

/*
 * Kinds of undo information.
 */
typedef enum UKIND
{
  UUNUSED = 0,			/* Entry is unused		*/
  UMOVE,			/* Move to (line #, offset)	*/
  UINSERT,			/* Insert string		*/
  UDELETE,			/* Delete string		*/
} UKIND;

#define NLMOVE	0		/* C-M moves to next line if at */
        /* eol and next line is blank   */
#define CVMVAS	1		/* C-V, M-V work in pages.      */
#define BACKUP	1		/* Make backup file.            */

/*
 * Universal.
 */
#define FALSE	0		/* False, no, bad, etc.         */
#define TRUE	1		/* True, yes, good, etc.        */
#define ABORT	2		/* Death, ^G, abort, etc.       */

#define KRANDOM 0x0080		/* A "no key" code.             */

#define eprintf printf

#define b_mark b_ring.m_ring[0] /* Current "mark" position	*/
#define firstline(bp) (lforw((bp)->b_linep))
#define lastline(bp)  (lback((bp)->b_linep))

extern BUFFER *curbp;
extern WINDOW *curwp;

int ldelete (int n, int kflag);		/* Delete n bytes at dot.	*/
int insertwithnl (const char *s, int len);
int linsert2 (int n, int c, char *s);	/* Insert char(s) at dot	*/
