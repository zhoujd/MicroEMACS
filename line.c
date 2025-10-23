/*
 * The functions in this file are a general set of line management utilities.
 * They are the only routines that touch the text. They also touch the buffer
 * and window structures, to make sure that the necessary updating gets done.
 * There are routines in this file that handle the kill buffer too. It isn't
 * here for any good reason.
 *
 * Note that this code only updates the dot and mark values in the window
 * list. Since all the code acts on the current window, the buffer that we are
 * editing must be being displayed, which means that "b_nwnd" is non zero,
 * which means that the dot and mark values in the buffer headers are
 * nonsense
 */

#include <stdlib.h>		/* malloc(3) */
#include "estruct.h"
#include "edef.h"
#include <string.h>
#include <stdio.h>

extern void mlwrite ();
extern int backchar (int f, int n);

LINE* lalloc (int used);
void lfree (LINE *lp);
void lchange (int flag);
int linsert(int n, int c, char *s);
int lnewline ();
int ldelete (int n, int kflag);
int ldelnewline ();
void kdelete ();
int kinsert (int c);
int kremove (int n);

#define NBLOCK	16		/* Line block chunk size */
#define KBLOCK	1024		/* Kill buffer block size */

char *kbufp = NULL;		/* Kill buffer data */
unsigned long kused = 0;	/* # of bytes used in KB */
unsigned long ksize = 0;	/* # of bytes allocated in KB */

/*
 * This routine allocates a block of memory large enough to hold a LINE
 * containing "used" characters. The block is always rounded up a bit. Return
 * a pointer to the new block, or NULL if there isn't any memory left. Print a
 * message in the message line if no space.
 */
LINE* lalloc (int used)
{
  LINE *lp;
  int size;
  //char *malloc ();

  size = (used + NBLOCK - 1) & ~(NBLOCK - 1);
  if (size == 0)	       /* Assume that an empty */
    size = NBLOCK;	       /* line is for type-in */
  if ((lp = (LINE *) malloc (sizeof (LINE) + size)) == NULL)
    {
      mlwrite ("Cannot allocate %d bytes", size);
      return (NULL);
    }
  lp->l_size = size;
  lp->l_used = used;
  return (lp);
}

/*
 * Delete line "lp". Fix all of the links that might point at it (they are
 * moved to offset 0 of the next line. Unlink the line from whatever buffer it
 * might be in. Release the memory. The buffers are updated too; the magic
 * conditions described in the above comments don't hold here
 */
void lfree (LINE *lp)
{
  BUFFER *bp;
  WINDOW *wp;

  wp = wheadp;
  while (wp != NULL)
    {
      if (wp->w_linep == lp)
	wp->w_linep = lp->l_fp;
      if (wp->w_dotp == lp)
	{
	  wp->w_dotp = lp->l_fp;
	  wp->w_doto = 0;
	}
      if (wp->w_markp == lp)
	{
	  wp->w_markp = lp->l_fp;
	  wp->w_marko = 0;
	}
      wp = wp->w_wndp;
    }
  bp = bheadp;
  while (bp != NULL)
    {
      if (bp->b_nwnd == 0)
	{
	  if (bp->b_dotp == lp)
	    {
	      bp->b_dotp = lp->l_fp;
	      bp->b_doto = 0;
	    }
	  if (bp->b_markp == lp)
	    {
	      bp->b_markp = lp->l_fp;
	      bp->b_marko = 0;
	    }
	}
      bp = bp->b_bufp;
    }
  lp->l_bp->l_fp = lp->l_fp;
  lp->l_fp->l_bp = lp->l_bp;
  free ((char *) lp);
}

/*
 * This routine gets called when a character is changed in place in the
 * current buffer. It updates all of the required flags in the buffer and
 * window system. The flag used is passed as an argument; if the buffer is
 * being displayed in more than 1 window we change EDIT t HARD. Set MODE if
 * the mode line needs to be updated (the "*" has to be set).
 */
void lchange (int flag)
{
  WINDOW *wp;

  if (curbp->b_nwnd != 1)      /* Ensure hard */
    flag = WFHARD;
  if ((curbp->b_flag & BFCHG) == 0)
    {			       /* First change, so */
      flag |= WFMODE;	       /* update mode lines */
      curbp->b_flag |= BFCHG;
    }
  wp = wheadp;
  while (wp != NULL)
    {
      if (wp->w_bufp == curbp)
	wp->w_flag |= flag;
      wp = wp->w_wndp;
    }
}


/*
 * Insert a newline into the buffer at the current location of dot in the
 * current window. The funny ass-backwards way it does things is not a botch;
 * it just makes the last line in the file not a special case. Return TRUE if
 * everything works out and FALSE on error (memory allocation failure). The
 * update of dot and mark is a bit easier then in the above case, because the
 * split forces more updating.
 */
int lnewline ()
{
  WINDOW *wp;
  char *cp1, *cp2;
  LINE *lp1, *lp2;
  int doto;

  lchange (WFHARD);
  lp1 = curwp->w_dotp;	       /* Get the address and */
  doto = curwp->w_doto;	       /* offset of "." */
  if ((lp2 = lalloc (doto)) == NULL)	/* New first half line */
    return (FALSE);
  cp1 = &lp1->l_text[0];       /* Shuffle text around */
  cp2 = &lp2->l_text[0];
  while (cp1 != &lp1->l_text[doto])
    *cp2++ = *cp1++;
  cp2 = &lp1->l_text[0];
  while (cp1 != &lp1->l_text[lp1->l_used])
    *cp2++ = *cp1++;
  lp1->l_used -= doto;
  lp2->l_bp = lp1->l_bp;
  lp1->l_bp = lp2;
  lp2->l_bp->l_fp = lp2;
  lp2->l_fp = lp1;
  wp = wheadp;		       /* Windows */
  while (wp != NULL)
    {
      if (wp->w_linep == lp1)
	wp->w_linep = lp2;
      if (wp->w_dotp == lp1)
	{
	  if (wp->w_doto < doto)
	    wp->w_dotp = lp2;
	  else
	    wp->w_doto -= doto;
	}
      if (wp->w_markp == lp1)
	{
	  if (wp->w_marko < doto)
	    wp->w_markp = lp2;
	  else
	    wp->w_marko -= doto;
	}
      wp = wp->w_wndp;
    }
  return (TRUE);
}

/*
 * This function deletes "n" bytes, starting at dot. It understands how do
 * deal with end of lines, etc. It returns TRUE if all of the characters were
 * deleted, and FALSE if they were not (because dot ran into the end of the
 * buffer. The "kflag" is TRUE if the text should be put in the kill buffer.
 */
int ldelete (int n, int kflag)
{
  LINE *dotp;
  WINDOW *wp;
  char *cp1, *cp2;
  int doto, chunk;

  while (n != 0)
    {
      dotp = curwp->w_dotp;
      doto = curwp->w_doto;
      if (dotp == curbp->b_linep) /* Hit end of buffer */
	return (FALSE);
      chunk = dotp->l_used - doto; /* Size of chunk */
      if (chunk > n)
	chunk = n;
      if (chunk == 0)
	{			/* End of line, merge */
	  lchange (WFHARD);
	  if (ldelnewline () == FALSE
	      || (kflag != FALSE && kinsert ('\n') == FALSE))
	    return (FALSE);
	  --n;
	  continue;
	}
      lchange (WFEDIT);
      cp1 = &dotp->l_text[doto]; /* Scrunch text */
      cp2 = cp1 + chunk;
      if (kflag != FALSE)
	{			/* Kill? */
	  while (cp1 != cp2)
	    {
	      if (kinsert (*cp1) == FALSE)
		return (FALSE);
	      ++cp1;
	    }
	  cp1 = &dotp->l_text[doto];
	}
      while (cp2 != &dotp->l_text[dotp->l_used])
	*cp1++ = *cp2++;
      dotp->l_used -= chunk;
      wp = wheadp;		/* Fix windows */
      while (wp != NULL)
	{
	  if (wp->w_dotp == dotp && wp->w_doto >= doto)
	    {
	      wp->w_doto -= chunk;
	      if (wp->w_doto < doto)
		wp->w_doto = doto;
	    }
	  if (wp->w_markp == dotp && wp->w_marko >= doto)
	    {
	      wp->w_marko -= chunk;
	      if (wp->w_marko < doto)
		wp->w_marko = doto;
	    }
	  wp = wp->w_wndp;
	}
      n -= chunk;
    }
  return (TRUE);
}

/*
 * Delete a newline. Join the current line with the next line. If the next
 * line is the magic header line always return TRUE; merging the last line
 * with the header line can be thought of as always being a successful
 * operation, even if nothing is done, and this makes the kill buffer work
 * "right". Easy cases can be done by shuffling data around. Hard cases
 * require that lines be moved about in memory. Return FALSE on error and TRUE
 * if all looks ok. Called by "ldelete" only.
 */
int ldelnewline ()
{
  LINE *lp1, *lp2, *lp3;
  WINDOW *wp;
  char *cp1, *cp2;

  lp1 = curwp->w_dotp;
  lp2 = lp1->l_fp;
  if (lp2 == curbp->b_linep)
    {			       /* At the buffer end */
      if (lp1->l_used == 0)	 /* Blank line */
	lfree (lp1);
      return (TRUE);
    }
  if (lp2->l_used <= lp1->l_size - lp1->l_used)
    {
      cp1 = &lp1->l_text[lp1->l_used];
      cp2 = &lp2->l_text[0];
      while (cp2 != &lp2->l_text[lp2->l_used])
	*cp1++ = *cp2++;
      wp = wheadp;
      while (wp != NULL)
	{
	  if (wp->w_linep == lp2)
	    wp->w_linep = lp1;
	  if (wp->w_dotp == lp2)
	    {
	      wp->w_dotp = lp1;
	      wp->w_doto += lp1->l_used;
	    }
	  if (wp->w_markp == lp2)
	    {
	      wp->w_markp = lp1;
	      wp->w_marko += lp1->l_used;
	    }
	  wp = wp->w_wndp;
	}
      lp1->l_used += lp2->l_used;
      lp1->l_fp = lp2->l_fp;
      lp2->l_fp->l_bp = lp1;
      free ((char *) lp2);
      return (TRUE);
    }
  if ((lp3 = lalloc (lp1->l_used + lp2->l_used)) == NULL)
    return (FALSE);
  cp1 = &lp1->l_text[0];
  cp2 = &lp3->l_text[0];
  while (cp1 != &lp1->l_text[lp1->l_used])
    *cp2++ = *cp1++;
  cp1 = &lp2->l_text[0];
  while (cp1 != &lp2->l_text[lp2->l_used])
    *cp2++ = *cp1++;
  lp1->l_bp->l_fp = lp3;
  lp3->l_fp = lp2->l_fp;
  lp2->l_fp->l_bp = lp3;
  lp3->l_bp = lp1->l_bp;
  wp = wheadp;
  while (wp != NULL)
    {
      if (wp->w_linep == lp1 || wp->w_linep == lp2)
	wp->w_linep = lp3;
      if (wp->w_dotp == lp1)
	wp->w_dotp = lp3;
      else if (wp->w_dotp == lp2)
	{
	  wp->w_dotp = lp3;
	  wp->w_doto += lp1->l_used;
	}
      if (wp->w_markp == lp1)
	wp->w_markp = lp3;
      else if (wp->w_markp == lp2)
	{
	  wp->w_markp = lp3;
	  wp->w_marko += lp1->l_used;
	}
      wp = wp->w_wndp;
    }
  free ((char *) lp1);
  free ((char *) lp2);
  return (TRUE);
}

/*
 * Delete all of the text saved in the kill buffer. Called by commands when a
 * new kill context is being created. The kill buffer array is released, just
 * in case the buffer has grown to immense size. No errors.
 */
void kdelete ()
{
  if (kbufp != NULL)
    {
      free ((char *) kbufp);
      kbufp = NULL;
      kused = 0;
      ksize = 0;
    }
}

/*
 * Convert a Unicode character c to UTF-8, writing the
 * characters to s; s must be at least 6 bytes long.
 * Return the number of bytes in the UTF-8 string.
 */
int
uputc (wchar_t c, uchar *s)
{
  if (c < 0x80)
    {
      s[0] = c;
      return 1;
    }
  if (c >= 0x80 && c <= 0x7ff)
    {
      s[0] = 0xc0 | ((c >> 6) & 0x1f);
      s[1] = 0x80 | (c & 0x3f);
      return 2;
    }
  if (c >= 0x800 && c <= 0xffff)
    {
      s[0] = 0xe0 | ((c >> 12) & 0x0f);
      s[1] = 0x80 | ((c >>  6) & 0x3f);
      s[2] = 0x80 | (c & 0x3f);
      return 3;
    }
  if (c >= 0x10000 && c <= 0x1fffff)
    {
      s[0] = 0xf0 | ((c >> 18) & 0x07);
      s[1] = 0x80 | ((c >> 12) & 0x3f);
      s[2] = 0x80 | ((c >>  6) & 0x3f);
      s[3] = 0x80 | (c & 0x3f);
      return 4;
    }
  if (c >= 0x200000 && c <= 0x3ffffff)
    {
      s[0] = 0xf8 | ((c >> 24) & 0x03);
      s[1] = 0x80 | ((c >> 18) & 0x3f);
      s[2] = 0x80 | ((c >> 12) & 0x3f);
      s[3] = 0x80 | ((c >>  6) & 0x3f);
      s[4] = 0x80 | (c & 0x3f);
      return 5;
    }
  if (c >= 0x4000000 && c <= 0x7fffffff)
    {
      s[0] = 0xfc | ((c >> 30) & 0x01);
      s[1] = 0x80 | ((c >> 24) & 0x3f);
      s[2] = 0x80 | ((c >> 18) & 0x3f);
      s[3] = 0x80 | ((c >> 12) & 0x3f);
      s[4] = 0x80 | ((c >>  6) & 0x3f);
      s[5] = 0x80 | (c & 0x3f);
      return 6;
    }
  /* Error */
  s[0] = c;
  return 1;
}

/*
 * Adjust line positions in wp->w_ring to account for an insertion of nchars characters
 * at position *oldpos, where newlp is the line that replaced oldpos.p.
 */
static void
adjustforinsert (const POS *oldpos, LINE *newlp, int nchars, WINDOW *wp)
{
  int i;

  for (i = 0; i < wp->w_ring.m_count; i++)
    {
      POS *pos = &wp->w_ring.m_ring[i];

      if (pos->p == oldpos->p)
	{
	  pos->p = newlp;
	  if (pos->o > oldpos->o)
	    pos->o += nchars;
	}
    }
}

/*
 * Return number of UTF-8 characters in the string s of length n.
 */
int
unslen (const uchar *s, int n)
{
  int len = 0;
  const uchar *end = s + n;

  while (s < end)
    {
      s += uclen (s);
      len++;
    }
  return len;
}

/*
 * Return the byte offset of the nth UTF-8 character in the string s.
 */
int
uoffset (const uchar *s, int n)
{
  const uchar *start = s;
  while (n > 0)
    {
       s += uclen (s);
      --n;
    }
  return s - start;
}


/*
 * Insert a character to the kill buffer, enlarging the buffer if there isn't
 * any room. Always grow the buffer in chunks, on the assumption that if you
 * put something in the kill buffer you are going to put more stuff there too
 * later. Return TRUE if all is well, and FALSE on errors.
 */
int kinsert (int c)
{
  //char *realloc ();
  //char *malloc ();
  char *nbufp;

  if (kused == ksize)
    {
      if (ksize == 0)	       /* first time through? */
	nbufp = malloc (KBLOCK); /* alloc the first block */
      else		       /* or re allocate a bigger block */
	nbufp = realloc (kbufp, ksize + KBLOCK);
      if (nbufp == NULL)	       /* abort if it fails */
	return (FALSE);
      kbufp = nbufp;	       /* point our global at it */
      ksize += KBLOCK;	       /* and adjust the size */
    }
  kbufp[kused++] = c;
  return (TRUE);
}

/*
 * This function gets characters from the kill buffer. If the character index
 * "n" is off the end, it returns "-1". This lets the caller just scan along
 * until it gets a "-1" back.
 */
int kremove (int n)
{
  if (n >= kused)
    return (-1);
  else
    return (kbufp[n] & 0xFF);
}

/*
 * If the string pointer "s" is NULL,
 * insert "n" copies of the Uncode character "c", converted to UTF-8
 * at the current location of dot.  Otherwise,
 * insert "n" bytes from the UTF-8 string "s" at the current
 * location of dot.  In the easy case,
 * all that happens is the text is stored in the line.
 * In the hard case, the line has to be reallocated.
 * When the window list is updated, take special
 * care; I screwed it up once. You always update dot
 * in the current window. You update mark, and a
 * dot in another window, if it is greater than
 * the place where you did the insert. Return TRUE
 * if all is well, and FALSE on errors.
 */
int
linsert(int n, int c, char *s)
{
  WINDOW *wp;
  LINE *lp2;
  LINE *lp3;
  POS dot;
  int chars, bytes, offset, buflen;
  char buf[6];

  if (checkreadonly () == FALSE)
    return FALSE;
  lchange (WFEDIT);
  dot = curwp->w_dot;		/* Save current line.	*/

  /* Get byte offset of insertion point. */
  offset = wloffset (dot.p, dot.o);

  if (s != NULL)
    {
      /* Set chars to number of characters in UTF-8 string.
       * Set bytes to number of bytes in the string.
       */
      chars = unslen ((uchar *)s, n);
      bytes = n;
      saveundo (UINSERT, NULL, 1, chars, bytes, s);
    }
 else
    {
      /* Convert character to UTF-8, store in buf. Set chars to
       * total number of characters to insert.  Set bytes to total number
       * of bytes to insert.
       */
      buflen = uputc (c, (uchar *)buf);
      chars = n;
      bytes = n * buflen;
      saveundo (UINSERT, NULL, n, 1, buflen, buf);
    }

  if (dot.p == curbp->b_linep)
    {				/* At the end: special  */
      if (offset != 0)
	{
	  eprintf ("bug: linsert");
	  return (FALSE);
	}
      if ((lp2 = lalloc (bytes)) == NULL)	/* Allocate new line    */
	return (FALSE);
      lp3 = dot.p->l_bp;		/* Previous line        */
      lp3->l_fp = lp2;			/* Link in              */
      lp2->l_fp = dot.p;
      dot.p->l_bp = lp2;
      lp2->l_bp = lp3;
    }
  else if (dot.p->l_used + bytes > dot.p->l_size)
    {					/* Hard: reallocate     */
      if ((lp2 = lalloc (dot.p->l_used + bytes)) == NULL)
	return (FALSE);
      memcpy (&lp2->l_text[0], &dot.p->l_text[0], offset);
      memcpy (&lp2->l_text[offset + bytes], &dot.p->l_text[offset],
	      dot.p->l_used - offset);	/* make room            */
      dot.p->l_bp->l_fp = lp2;
      lp2->l_fp = dot.p->l_fp;
      dot.p->l_fp->l_bp = lp2;
      lp2->l_bp = dot.p->l_bp;
      free ((char *) dot.p);
    }
  else
    {				/* Easy: in place       */
      lp2 = dot.p;		/* Pretend new line     */
      memmove (&dot.p->l_text[offset + bytes], &dot.p->l_text[offset],
	       dot.p->l_used - offset);	/* make room            */
      lp2->l_used += bytes;		/* bump length up       */
    }

  if (s == NULL)		/* fill or copy?        */
    {
      int i, o;

      for (i = 0, o = offset; i < chars; i++, o += buflen)
	memcpy (&lp2->l_text[o], buf, buflen);	/* fill the characters  */
    }
  else
    memcpy (&lp2->l_text[offset], s, bytes);	/* copy the characters  */

  ALLWIND (wp)
  {				/* Update windows       */
    if (wp->w_linep == dot.p)
      wp->w_linep = lp2;
    if (wp->w_savep == dot.p)
      wp->w_savep = lp2;
    if (wp->w_dot.p == dot.p)
      {
	wp->w_dot.p = lp2;
	if (wp == curwp || wp->w_dot.o > dot.o)
	  wp->w_dot.o += chars;
      }
    adjustforinsert (&dot, lp2, chars, wp);
  }
  return (TRUE);
}


/*
 * Insert the string s containing len bytes,
 * but treat \n characters properly, i.e., as the starts of
 * new lines instead of raw characters.  Return TRUE
 * if successful, or FALSE if an error occurs.
 */
int
insertwithnl (const char *s, int len)
{
  int status = TRUE;
  const char *end = s + len;

  while (status == TRUE && s < end)
    {
      const char *nl = memchr (s, '\n', end - s);
      if (nl == NULL)
	{
	  status = linsert (end - s, 0, (char *) s);
	  s = end;
	}
      else
	{
	  if (nl != s)
	    {
	      status = linsert (nl - s, 0, (char *) s);
	      if (status != TRUE)
		break;
	    }
	  status = lnewline ();
	  s = nl + 1;
	}
    }
  return status;
}

