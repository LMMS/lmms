/* $XFree86: xc/lib/X11/lcUniConv/iso8859_1.h,v 1.3 2000/11/29 17:40:30 dawes Exp $ */

/*
 * ISO-8859-1
 */

static int
iso8859_1_mbtowc (conv_t conv, ucs4_t *pwc, const unsigned char *s, int n)
{
  unsigned char c = *s;
  *pwc = (ucs4_t) c;
  return 1;
}

static int
iso8859_1_wctomb (conv_t conv, unsigned char *r, ucs4_t wc, int n)
{
  if (wc < 0x0100) {
    *r = wc;
    return 1;
  }
  return RET_ILSEQ;
}
