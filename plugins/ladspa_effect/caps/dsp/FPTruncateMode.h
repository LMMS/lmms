/* Copyright 2001-4 tim goetze <tim@quitte.de> -- see 'COPYING'. */

/* Sets the FP rounding mode to 'truncate' in the constructor
 * and loads the previous FP conrol word in the destructor. 
 *
 * By directly using the machine instruction to convert float to int
 * we avoid the performance hit that loading the control word twice for
 * every (int) cast causes on i386.
 *
 * On other architectures this is a no-op.
 */

#ifndef _DSP_FP_TRUNCATE_MODE_H_
#define _DSP_FP_TRUNCATE_MODE_H_

#ifdef __i386__
	#define fstcw(i) \
		__asm__ __volatile__ ("fstcw %0" : "=m" (i))

	#define fldcw(i) \
		__asm__ __volatile__ ("fldcw %0" : : "m" (i))

	/* gcc chokes on __volatile__ sometimes. */
	#define fistp(f,i) \
		__asm__ ("fistpl %0" : "=m" (i) : "t" (f) : "st")
#else /* ! __i386__ */
	#define fstcw(i)
	#define fldcw(i)

	#define fistp(f,i) \
			i = (int) f
#endif

namespace DSP {
	
class FPTruncateMode
{
	public:
		int cw0, cw1; /* fp control word */

		FPTruncateMode()
			{
				fstcw (cw0);
				cw1 = cw0 | 0xC00;
				fldcw (cw1);
			}

		~FPTruncateMode()
			{
				fldcw (cw0);
			}
};

} /* namespace DSP */

#endif /* _DSP_FP_TRUNCATE_MODE_H_ */
