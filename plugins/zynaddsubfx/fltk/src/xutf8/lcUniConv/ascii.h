/* $XFree86: xc/lib/X11/lcUniConv/ascii.h,v 1.3 2000/11/29 17:40:28 dawes Exp $ */

/*
 * ASCII
 */

static int
ascii_mbtowc (conv_t conv, ucs4_t *pwc, const unsigned char *s, int n)
{
  unsigned char c = *s;
  if (c < 0x80) {
    *pwc = (ucs4_t) c;
    return 1;
  }
  return RET_ILSEQ;
}

static int
ascii_wctomb (conv_t conv, unsigned char *r, ucs4_t wc, int n)
{
  if (wc < 0x0080) {
    *r = wc;
    return 1;
  }
  return RET_ILSEQ;
}
