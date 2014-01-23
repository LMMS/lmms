#ifndef IIR_H
#define IIR_H

#include <ladspa-util.h>

/* header file for IIR framework */

typedef struct iir_stage iir_stage_t;
typedef struct iirf iirf_t;
// defines this to float if your brave and you'll see what happens..
#define gliirt float
#define CLAMP(x,mi,ma) ( (x < mi) ? mi : ( (x>ma) ? ma : x))
#ifndef MIN
#define MIN(a, b) ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a, b) ((a)>(b)?(a):(b))
#endif
/* alloc zeroed mem, malloc/calloc syntax. */
#define ALLOC(type) (type *)calloc(1, sizeof(type))
#define ALLOCN(n, type) (n == 0 ? NULL : (type *)calloc((n), sizeof(type)))

/* supported filter modes by lib */
#define IIR_STAGE_LOWPASS    0
#define IIR_STAGE_HIGHPASS   1
#define IIR_STAGE_BANDPASS   2
#define IIR_STAGE_BANDPASS_A 3

struct iir_stage {
	int np;		/* Number of poles */
	int mode;	/* Filter mode low/high/bandpass... */
	int availst;    /* Number of allocated stages */
	int nstages;	/* Number of active filterstages */
	int na;		/* number of a coefficients per stage */
	int nb;		/* number of b coefficients per stage */
	gliirt fc;	/* cutoff/center frequency */
	gliirt bw;	/* bandwidth for bandpass */
	gliirt ppr;	/* percent of ripple in passband */
	gliirt spr;	/* percent of ripple in stopband */
	gliirt **coeff;	/* Actual filter coefficients */
};

struct iirf {
	gliirt *iring;
	gliirt *oring;
	int	ipos;
	int	opos;
};
// allocate ringbuffers for iir calculation
static inline iirf_t* init_iirf_t(iir_stage_t* gt) {
	int i;
	iirf_t* iirf=ALLOCN(gt->availst,iirf_t);
	
	for(i=0;i<gt->availst;i++){
		iirf[i].iring=ALLOCN(gt->na,gliirt);
		iirf[i].oring=ALLOCN(gt->nb+1,gliirt);
		iirf[i].ipos=0;
		iirf[i].opos=0;
	}

	return iirf;
};

static inline void free_iirf_t(iirf_t* iirf, iir_stage_t* gt) {
	int i;
	for(i=0;i<gt->availst;i++){
		if (iirf[i].iring) free(iirf[i].iring);
		if (iirf[i].oring) free(iirf[i].oring);
	}
	if (iirf) free(iirf);
};

static inline void reset_iirf_t(iirf_t* iirf, iir_stage_t* gt, int n) {
	int i;
	for(i=0;i<n; ++i) {
		memset(iirf[i].iring, 0, sizeof(gliirt)*gt->na);
		memset(iirf[i].oring, 0, sizeof(gliirt)*(gt->nb+1));
	}
};

iir_stage_t *init_iir_stage(int mode, int nstages, int na, int nb);
void combine_iir_stages(int mode, iir_stage_t* gt, iir_stage_t *first, iir_stage_t *second, int upf, int ups);
void free_iir_stage(iir_stage_t *gt);
void calc_2polebandpass(iirf_t* iirf, iir_stage_t* gt, float fc, float bw, long sample_rate);
// for chebyshev we need iir stages with na=3, nb=2
// na are the forward coefficients
// nb are the recursive coefficients
int chebyshev(iirf_t* iirf, iir_stage_t* gt, int n, int mode, float fc, float pr);

/* calculate butterworth coefficients
 * coefficient calculation taken from http://musicdsp.org/showArchiveComment.php?ArchiveID=38
 * mode = 0 -> lowpass
 * mode !=0 -> highpass
 * 
 * f -> cutoff frequency
 * r -> resonance 
 */

static inline void butterworth_stage(iir_stage_t *gt, int mode, float f, float r, long sample_rate)
{
	float c, a1, a2, a3, b1, b2;
	
	/* lowpass coefficients */

	if (mode==0) {
		c = 1.0f / tan(M_PI * f / sample_rate ) ;

		a1 = 1.0f / ( 1.0f + r * c + c * c);
		a2 = 2.0f * a1;
		a3 = a1;
		b1 = -2.0f * ( 1.0f - c*c) * a1;
		b2 = -( 1.0f - r * c + c * c) * a1;
	} else {
	/* highpass coefficients */
		c = tan(M_PI * f / sample_rate );

		a1 = 1.0f / ( 1.0f + r * c + c * c);
		a2 = -2.0f*a1;
		a3 = a1;
		b1 = -2.0f * ( c*c - 1.0f) * a1;
		b2 = -( 1.0f - r * c + c * c) * a1;
	}
	
	gt->fc = f;
	gt->nstages = 1;
	
	gt->coeff[0][0] = a1;
	gt->coeff[0][1] = a2;
	gt->coeff[0][2] = a3;
	gt->coeff[0][3] = b1;
	gt->coeff[0][4] = b2;
};

/* process function */
static inline void iir_process_buffer(iirf_t* iirf, iir_stage_t* gt, const float *indata, float *outdata, const long numSampsToProcess, int add) {
	long pos;
	int i,nb,nt,j,z,ipos,opos;

	if(gt->nstages==0) {
		if (indata==outdata)
			return;
		memcpy(outdata, indata, numSampsToProcess*sizeof(float));
		return;
	}

	nb=gt->nb+1;
	nt=gt->na+gt->nb;

	ipos = iirf[0].ipos;
	opos = iirf[0].opos;

	if (add==0) 
		for(pos=0; pos<numSampsToProcess; pos++) {
			iirf[0].iring[ipos]=indata[pos];
			for(i=0;i<gt->nstages;i++){
				if (i>0)
					iirf[i].iring[ipos]=iirf[i-1].oring[opos];
				iirf[i].oring[opos]=0.0;
				/* y[n]=a0*x[n]+a1*x[n-1]+... */
				z=ipos;
				for(j=0;j<gt->na;j++){
					if(z==-1)
						z=gt->na-1;
					iirf[i].oring[opos]+=gt->coeff[i][j]*iirf[i].iring[z--];
				}
				/* y[n]=y[n]+b1*y[n-1]+b2*y[n-2]+... */
				z=opos-1;
				for(j=gt->na;j<nt;j++){
					if (z==-1)
						z=gt->nb;
					iirf[i].oring[opos]+=gt->coeff[i][j]*iirf[i].oring[z--];
				}
			}
			/* No matter if we process it in place */
			outdata[pos]=(float)iirf[gt->nstages-1].oring[opos];
			
			/* Adjust ringbuffers */
			ipos++;
			if (ipos==gt->na)
				ipos=0;
			opos++;
			if (opos==nb)
				opos=0;	
		}
	else
		for(pos=0; pos<numSampsToProcess; pos++) {
			iirf[0].iring[ipos]=indata[pos];
			for(i=0;i<gt->nstages;i++){
				if (i>0)
					iirf[i].iring[ipos]=iirf[i-1].oring[opos];
				iirf[i].oring[opos]=0.0;
				/* y[n]=a0*x[n]+a1*x[n-1]+... */
				z=ipos;
				for(j=0;j<gt->na;j++){
					if(z==-1)
						z=gt->na-1;
					iirf[i].oring[opos]+=gt->coeff[i][j]*iirf[i].iring[z--];
				}
				/* y[n]=y[n]+b1*y[n-1]+b2*y[n-2]+... */
				z=opos-1;
				for(j=gt->na;j<nt;j++){
					if (z==-1)
						z=gt->nb;
					iirf[i].oring[opos]+=gt->coeff[i][j]*iirf[i].oring[z--];
				}
			}
			/* Now it matters if we process it in place */
			outdata[pos]+=(float)iirf[gt->nstages-1].oring[opos];
			
			/* Adjust ringbuffers */
			ipos++;
			if (ipos==gt->na)
				ipos=0;
			opos++;
			if (opos==nb)
				opos=0;	
		}

	iirf[0].ipos = ipos;
	iirf[0].opos = opos;
};

/* process function for 3a and 2b coeffs */
static inline void iir_process_buffer_1s_5(iirf_t* iirf, iir_stage_t* gt, const float *indata, 
					   float *outdata, const long numSampsToProcess, int add) {
	long pos;

	if (add==0) 
		for(pos=0; pos<numSampsToProcess; pos++) {
			iirf[0].iring[0]=iirf[0].iring[1];
			iirf[0].iring[1]=iirf[0].iring[2];
			iirf[0].iring[2]=indata[pos];
			iirf[0].oring[0]=iirf[0].oring[1];
			iirf[0].oring[1]=iirf[0].oring[2];
			/* y[n]=a0*x[n]+a1*x[n-1]+... */
			/* y[n]=y[n]+b1*y[n-1]+b2*y[n-2]+... */
			iirf[0].oring[2] = flush_to_zero(gt->coeff[0][0]*iirf[0].iring[2] +
				gt->coeff[0][1]*iirf[0].iring[1] +
				gt->coeff[0][2]*iirf[0].iring[0] +
				gt->coeff[0][3]*iirf[0].oring[1] +
				gt->coeff[0][4]*iirf[0].oring[0]);
			outdata[pos]=(float)iirf[0].oring[2];
		}
	else
		for(pos=0; pos<numSampsToProcess; pos++) {
			iirf[0].iring[0]=iirf[0].iring[1];
			iirf[0].iring[1]=iirf[0].iring[2];
			iirf[0].iring[2]=indata[pos];
			iirf[0].oring[0]=iirf[0].oring[1];
			iirf[0].oring[1]=iirf[0].oring[2];
			/* y[n]=a0*x[n]+a1*x[n-1]+... */
			/* y[n]=y[n]+b1*y[n-1]+b2*y[n-2]+... */
			outdata[pos] += iirf[0].oring[2] = flush_to_zero(gt->coeff[0][0]*iirf[0].iring[2] +
				gt->coeff[0][1]*iirf[0].iring[1] +
				gt->coeff[0][2]*iirf[0].iring[0] +
				gt->coeff[0][3]*iirf[0].oring[1] +
				gt->coeff[0][4]*iirf[0].oring[0]);	
		}
};

/* process function */
static inline void iir_process_buffer_ns_5(iirf_t* iirf, iir_stage_t* gt, const float *indata, float *outdata, const long numSampsToProcess, int add) {
	long pos;
	int i;

	if (add==0) 
		for(pos=0; pos<numSampsToProcess; pos++) {
			iirf[0].iring[0]=iirf[0].iring[1];
			iirf[0].iring[1]=iirf[0].iring[2];
			iirf[0].iring[2]=indata[pos];

			iirf[0].oring[0]=iirf[0].oring[1];
			iirf[0].oring[1]=iirf[0].oring[2];
			/* y[n]=a0*x[n]+a1*x[n-1]+... */
			/* y[n]=y[n]+b1*y[n-1]+b2*y[n-2]+... */
			iirf[0].oring[2] = flush_to_zero(gt->coeff[0][0]*iirf[0].iring[2] +
				gt->coeff[0][1]*iirf[0].iring[1] +
				gt->coeff[0][2]*iirf[0].iring[0] +
				gt->coeff[0][3]*iirf[0].oring[1] +
				gt->coeff[0][4]*iirf[0].oring[0]);

			for(i=1;i<gt->nstages;i++){				
				iirf[i].iring[0]=iirf[i].iring[1];
				iirf[i].iring[1]=iirf[i].iring[2];				
                iirf[i].iring[2]=iirf[i-1].oring[2];

				iirf[i].oring[0]=iirf[i].oring[1];
				iirf[i].oring[1]=iirf[i].oring[2];
				/* y[n]=a0*x[n]+a1*x[n-1]+... */
				/* y[n]=y[n]+b1*y[n-1]+b2*y[n-2]+... */
				iirf[i].oring[2] = 
					flush_to_zero(gt->coeff[i][0]*iirf[i].iring[2] +
                        gt->coeff[i][1]*iirf[i].iring[1] +
                        gt->coeff[i][2]*iirf[i].iring[0] +
                        gt->coeff[i][3]*iirf[i].oring[1] +
                        gt->coeff[i][4]*iirf[i].oring[0]);				
			}
			/* No matter if we process it in place */
			outdata[pos]=(float)iirf[gt->nstages-1].oring[2];
		}
	else
		for(pos=0; pos<numSampsToProcess; pos++) {
			iirf[0].iring[0]=iirf[0].iring[1];
			iirf[0].iring[1]=iirf[0].iring[2];
			iirf[0].iring[2]=indata[pos];

			iirf[0].oring[0]=iirf[0].oring[1];
			iirf[0].oring[1]=iirf[0].oring[2];
			/* y[n]=a0*x[n]+a1*x[n-1]+... */
			/* y[n]=y[n]+b1*y[n-1]+b2*y[n-2]+... */
			iirf[0].oring[2] = flush_to_zero(gt->coeff[0][0]*iirf[0].iring[2] +
				gt->coeff[0][1]*iirf[0].iring[1] +
				gt->coeff[0][2]*iirf[0].iring[0] +
				gt->coeff[0][3]*iirf[0].oring[1] +
				gt->coeff[0][4]*iirf[0].oring[0]);

			for(i=1;i<gt->nstages;i++){				
				iirf[i].iring[0]=iirf[i].iring[1];
				iirf[i].iring[1]=iirf[i].iring[2];				
				iirf[i].iring[2]=iirf[i-1].oring[2];

				iirf[i].oring[0]=iirf[i].oring[1];
				iirf[i].oring[1]=iirf[i].oring[2];
				/* y[n]=a0*x[n]+a1*x[n-1]+... */
				/* y[n]=y[n]+b1*y[n-1]+b2*y[n-2]+... */
				iirf[i].oring[2] = flush_to_zero(
					gt->coeff[i][0]*iirf[i].iring[2] +
					gt->coeff[i][1]*iirf[i].iring[1] +
					gt->coeff[i][2]*iirf[i].iring[0] +
					gt->coeff[i][3]*iirf[i].oring[1] +
					gt->coeff[i][4]*iirf[i].oring[0]);				
			}
			/* No matter if we process it in place */
			outdata[pos]+=(float)iirf[gt->nstages-1].oring[2];
		}	
};

#endif 
