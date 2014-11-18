// Reverb model declaration
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain

#ifndef _revmodel_
#define _revmodel_

#include "comb.h"
#include "allpass.h"
#include "tuning.h"

const int maxSampleRatio = 18; // enough for largest possible samplerate, 8 * 96000

class revmodel
{
public:
					revmodel( float sampleRatio );
			void	mute();
			void	processmix(float *inputL, float *inputR, float *outputL, float *outputR, long numsamples, int skip);
			void	processreplace(float *inputL, float *inputR, float *outputL, float *outputR, long numsamples, int skip);
			void	setroomsize(float value);
			float	getroomsize();
			void	setdamp(float value);
			float	getdamp();
			void	setwet(float value);
			float	getwet();
			void	setdry(float value);
			float	getdry();
			void	setwidth(float value);
			float	getwidth();
			void	setmode(float value);
			float	getmode();
private:
			void	update();
private:
	float	gain;
	float	roomsize,roomsize1;
	float	damp,damp1;
	float	wet,wet1,wet2;
	float	dry;
	float	width;
	float	mode;
	
	float m_sampleRatio;

	// The following are all declared inline 
	// to remove the need for dynamic allocation
	// with its subsequent error-checking messiness

	// Comb filters
	comb	combL[numcombs];
	comb	combR[numcombs];

	// Allpass filters
	allpass	allpassL[numallpasses];
	allpass	allpassR[numallpasses];

	// Buffers for the combs
	float	bufcombL1[combtuningL1 * maxSampleRatio];
	float	bufcombR1[combtuningR1 * maxSampleRatio];
	float	bufcombL2[combtuningL2 * maxSampleRatio];
	float	bufcombR2[combtuningR2 * maxSampleRatio];
	float	bufcombL3[combtuningL3 * maxSampleRatio];
	float	bufcombR3[combtuningR3 * maxSampleRatio];
	float	bufcombL4[combtuningL4 * maxSampleRatio];
	float	bufcombR4[ combtuningR4 * maxSampleRatio ];
	float	bufcombL5[ combtuningL5 * maxSampleRatio ];
	float	bufcombR5[ combtuningR5 * maxSampleRatio ];
	float	bufcombL6[ combtuningL6 * maxSampleRatio ];
	float	bufcombR6[ combtuningR6 * maxSampleRatio ];
	float	bufcombL7[ combtuningL7 * maxSampleRatio ];
	float	bufcombR7[ combtuningR7 * maxSampleRatio ];
	float	bufcombL8[ combtuningL8 * maxSampleRatio ];
	float	bufcombR8[ combtuningR8 * maxSampleRatio ];

	// Buffers for the allpasses
	float	bufallpassL1[ allpasstuningL1 * maxSampleRatio ];
	float	bufallpassR1[ allpasstuningR1 * maxSampleRatio ];
	float	bufallpassL2[ allpasstuningL2 * maxSampleRatio ];
	float	bufallpassR2[ allpasstuningR2 * maxSampleRatio ];
	float	bufallpassL3[ allpasstuningL3 * maxSampleRatio ];
	float	bufallpassR3[ allpasstuningR3 * maxSampleRatio ];
	float	bufallpassL4[ allpasstuningL4 * maxSampleRatio ];
	float	bufallpassR4[ allpasstuningR4 * maxSampleRatio ];
};

#endif//_revmodel_

//ends
