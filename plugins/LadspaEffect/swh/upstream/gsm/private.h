/*
 * Copyright 1992 by Jutta Degener and Carsten Bormann, Technische
 * Universitaet Berlin.  See the accompanying file "COPYRIGHT" for
 * details.  THERE IS ABSOLUTELY NO WARRANTY FOR THIS SOFTWARE.
 */

/*$Header: /home/cvs/giga/ladspa-swh/gsm/private.h,v 1.1 2001/06/10 21:36:51 swh Exp $*/

#ifndef	PRIVATE_H
#define	PRIVATE_H

/* Added by Erik de Castro Lopo */
#define	NeedFunctionPrototypes	1
#define	SASR  
#define	USE_FLOAT_MUL
#define	FAST
#define	WAV49  
/* Added by Erik de Castro Lopo */



typedef short				word;		/* 16 bit signed int	*/
typedef int					longword;	/* 32 bit signed int	*/

typedef unsigned short		uword;		/* unsigned word	*/
typedef unsigned int		ulongword;	/* unsigned longword	*/

struct gsm_state {

	word		dp0[ 280 ];

	word		z1;		/* preprocessing.c, Offset_com. */
	longword	L_z2;		/*                  Offset_com. */
	int			mp;		/*                  Preemphasis	*/

	word		u[8];		/* short_term_aly_filter.c	*/
	word		LARpp[2][8]; 	/*                              */
	word		j;		/*                              */

	word        ltp_cut;        /* long_term.c, LTP crosscorr.  */
	word		nrp; /* 40 */	/* long_term.c, synthesis	*/
	word		v[9];		/* short_term.c, synthesis	*/
	word		msr;		/* decoder.c,	Postprocessing	*/

	char		verbose;	/* only used if !NDEBUG		*/
	char		fast;		/* only used if FAST		*/

	char		wav_fmt;	/* only used if WAV49 defined	*/
	unsigned char	frame_index;	/*            odd/even chaining	*/
	unsigned char	frame_chain;	/*   half-byte to carry forward	*/
};


#define	MIN_WORD	(-32767 - 1)
#define	MAX_WORD	  32767

#define	MIN_LONGWORD	(-2147483647 - 1)
#define	MAX_LONGWORD	  2147483647

#ifdef	SASR		/* flag: >> is a signed arithmetic shift right */
#undef	SASR
#define	SASR(x, by)	((x) >> (by))
#else
#define	SASR(x, by)	((x) >= 0 ? (x) >> (by) : (~(-((x) + 1) >> (by))))
#endif	/* SASR */

#include "proto.h"

/*
 *	Prototypes from add.c
 */
extern word	gsm_mult 		(word a, word b);
extern longword gsm_L_mult 	(word a, word b);
extern word	gsm_mult_r		(word a, word b);

extern word	gsm_div  		(word num, word denum);

extern word	gsm_add 		(word a, word b );
extern longword gsm_L_add 	(longword a, longword b );

extern word	gsm_sub 		(word a, word b);
extern longword gsm_L_sub 	(longword a, longword b);

extern word	gsm_abs 		(word a);

extern word	gsm_norm 		(longword a );

extern longword gsm_L_asl  	(longword a, int n);
extern word	gsm_asl 		(word a, int n);

extern longword gsm_L_asr  	(longword a, int n);
extern word	gsm_asr  		(word a, int n);

/*
 *  Inlined functions from add.h 
 */

/* 
 * #define GSM_MULT_R(a, b) (* word a, word b, !(a == b == MIN_WORD) *)	\
 *	(0x0FFFF & SASR(((longword)(a) * (longword)(b) + 16384), 15))
 */
#define GSM_MULT_R(a, b) /* word a, word b, !(a == b == MIN_WORD) */	\
	(SASR( ((longword)(a) * (longword)(b) + 16384), 15 ))

# define GSM_MULT(a,b)	 /* word a, word b, !(a == b == MIN_WORD) */	\
	(SASR( ((longword)(a) * (longword)(b)), 15 ))

# define GSM_L_MULT(a, b) /* word a, word b */	\
	(((longword)(a) * (longword)(b)) << 1)

# define GSM_L_ADD(a, b)	\
	( (a) <  0 ? ( (b) >= 0 ? (a) + (b)	\
		 : (utmp = (ulongword)-((a) + 1) + (ulongword)-((b) + 1)) \
		   >= MAX_LONGWORD ? MIN_LONGWORD : -(longword)utmp-2 )   \
	: ((b) <= 0 ? (a) + (b)   \
	          : (utmp = (ulongword)(a) + (ulongword)(b)) >= MAX_LONGWORD \
		    ? MAX_LONGWORD : utmp))

/*
 * # define GSM_ADD(a, b)	\
 * 	((ltmp = (longword)(a) + (longword)(b)) >= MAX_WORD \
 * 	? MAX_WORD : ltmp <= MIN_WORD ? MIN_WORD : ltmp)
 */
/* Nonportable, but faster: */

#define	GSM_ADD(a, b)	\
	((ulongword)((ltmp = (longword)(a) + (longword)(b)) - MIN_WORD) > \
		MAX_WORD - MIN_WORD ? (ltmp > 0 ? MAX_WORD : MIN_WORD) : ltmp)

# define GSM_SUB(a, b)	\
	((ltmp = (longword)(a) - (longword)(b)) >= MAX_WORD \
	? MAX_WORD : ltmp <= MIN_WORD ? MIN_WORD : ltmp)

# define GSM_ABS(a)	((a) < 0 ? ((a) == MIN_WORD ? MAX_WORD : -(a)) : (a))

/* Use these if necessary:

# define GSM_MULT_R(a, b)	gsm_mult_r(a, b)
# define GSM_MULT(a, b)		gsm_mult(a, b)
# define GSM_L_MULT(a, b)	gsm_L_mult(a, b)

# define GSM_L_ADD(a, b)	gsm_L_add(a, b)
# define GSM_ADD(a, b)		gsm_add(a, b)
# define GSM_SUB(a, b)		gsm_sub(a, b)

# define GSM_ABS(a)		gsm_abs(a)

*/

/*
 *  More prototypes from implementations..
 */
extern void Gsm_Coder P((
		struct gsm_state	* S,
		word	* s,	/* [0..159] samples		IN	*/
		word	* LARc,	/* [0..7] LAR coefficients	OUT	*/
		word	* Nc,	/* [0..3] LTP lag		OUT 	*/
		word	* bc,	/* [0..3] coded LTP gain	OUT 	*/
		word	* Mc,	/* [0..3] RPE grid selection	OUT     */
		word	* xmaxc,/* [0..3] Coded maximum amplitude OUT	*/
		word	* xMc	/* [13*4] normalized RPE samples OUT	*/));

extern void Gsm_Long_Term_Predictor P((		/* 4x for 160 samples */
		struct gsm_state * S,
		word	* d,	/* [0..39]   residual signal	IN	*/
		word	* dp,	/* [-120..-1] d'		IN	*/
		word	* e,	/* [0..40] 			OUT	*/
		word	* dpp,	/* [0..40] 			OUT	*/
		word	* Nc,	/* correlation lag		OUT	*/
		word	* bc	/* gain factor			OUT	*/));

extern void Gsm_LPC_Analysis P((
		struct gsm_state * S,
		word * s,	 /* 0..159 signals	IN/OUT	*/
	        word * LARc));   /* 0..7   LARc's	OUT	*/

extern void Gsm_Preprocess P((
		struct gsm_state * S,
		word * s, word * so));

extern void Gsm_Encoding P((
		struct gsm_state * S,
		word	* e,	
		word	* ep,	
		word	* xmaxc,
		word	* Mc,	
		word	* xMc));

extern void Gsm_Short_Term_Analysis_Filter P((
		struct gsm_state * S,
		word	* LARc,	/* coded log area ratio [0..7]  IN	*/
		word	* d	/* st res. signal [0..159]	IN/OUT	*/));

extern void Gsm_Decoder P((
		struct gsm_state * S,
		word	* LARcr,	/* [0..7]		IN	*/
		word	* Ncr,		/* [0..3] 		IN 	*/
		word	* bcr,		/* [0..3]		IN	*/
		word	* Mcr,		/* [0..3] 		IN 	*/
		word	* xmaxcr,	/* [0..3]		IN 	*/
		word	* xMcr,		/* [0..13*4]		IN	*/
		word	* s));		/* [0..159]		OUT 	*/

extern void Gsm_Decoding P((
		struct gsm_state * S,
		word 	xmaxcr,
		word	Mcr,
		word	* xMcr,  	/* [0..12]		IN	*/
		word	* erp)); 	/* [0..39]		OUT 	*/

extern void Gsm_Long_Term_Synthesis_Filtering P((
		struct gsm_state* S,
		word	Ncr,
		word	bcr,
		word	* erp,		/* [0..39]		  IN 	*/
		word	* drp)); 	/* [-120..-1] IN, [0..40] OUT 	*/

void Gsm_RPE_Decoding P((
	struct gsm_state *S,
		word xmaxcr,
		word Mcr,
		word * xMcr,  /* [0..12], 3 bits             IN      */
		word * erp)); /* [0..39]                     OUT     */

void Gsm_RPE_Encoding P((
		struct gsm_state * S,
		word    * e,            /* -5..-1][0..39][40..44     IN/OUT  */
		word    * xmaxc,        /*                              OUT */
		word    * Mc,           /*                              OUT */
		word    * xMc));        /* [0..12]                      OUT */

extern void Gsm_Short_Term_Synthesis_Filter P((
		struct gsm_state * S,
		word	* LARcr, 	/* log area ratios [0..7]  IN	*/
		word	* drp,		/* received d [0...39]	   IN	*/
		word	* s));		/* signal   s [0..159]	  OUT	*/

extern void Gsm_Update_of_reconstructed_short_time_residual_signal P((
		word	* dpp,		/* [0...39]	IN	*/
		word	* ep,		/* [0...39]	IN	*/
		word	* dp));		/* [-120...-1]  IN/OUT 	*/

/*
 *  Tables from table.c
 */
#ifndef	GSM_TABLE_C

extern word gsm_A[8], gsm_B[8], gsm_MIC[8], gsm_MAC[8];
extern word gsm_INVA[8];
extern word gsm_DLB[4], gsm_QLB[4];
extern word gsm_H[11];
extern word gsm_NRFAC[8];
extern word gsm_FAC[8];

#endif	/* GSM_TABLE_C */

/*
 *  Debugging
 */
#ifdef NDEBUG

#	define	gsm_debug_words(a, b, c, d)		/* nil */
#	define	gsm_debug_longwords(a, b, c, d)		/* nil */
#	define	gsm_debug_word(a, b)			/* nil */
#	define	gsm_debug_longword(a, b)		/* nil */

#else	/* !NDEBUG => DEBUG */

	extern void  gsm_debug_words     P((char * name, int, int, word *));
	extern void  gsm_debug_longwords P((char * name, int, int, longword *));
	extern void  gsm_debug_longword  P((char * name, longword));
	extern void  gsm_debug_word      P((char * name, word));

#endif /* !NDEBUG */

#include "unproto.h"

#endif	/* PRIVATE_H */
