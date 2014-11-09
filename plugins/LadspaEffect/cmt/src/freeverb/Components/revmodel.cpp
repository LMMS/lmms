// Reverb model implementation
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain

#include "revmodel.h"

revmodel::revmodel( float sampleRatio ) :
		m_sampleRatio( sampleRatio )
{
	// Tie the components to their buffers
	combL[0].setbuffer(bufcombL1,static_cast<int>( combtuningL1 * m_sampleRatio ));
	combR[0].setbuffer(bufcombR1,static_cast<int>( combtuningR1 * m_sampleRatio ));
	combL[1].setbuffer(bufcombL2,static_cast<int>( combtuningL2 * m_sampleRatio ));
	combR[1].setbuffer(bufcombR2,static_cast<int>( combtuningR2 * m_sampleRatio ));
	combL[2].setbuffer(bufcombL3,static_cast<int>( combtuningL3 * m_sampleRatio ));
	combR[2].setbuffer(bufcombR3,static_cast<int>( combtuningR3 * m_sampleRatio ));
	combL[3].setbuffer(bufcombL4,static_cast<int>( combtuningL4 * m_sampleRatio ));
	combR[3].setbuffer(bufcombR4,static_cast<int>( combtuningR4 * m_sampleRatio ));
	combL[4].setbuffer(bufcombL5,static_cast<int>( combtuningL5 * m_sampleRatio ));
	combR[4].setbuffer(bufcombR5,static_cast<int>( combtuningR5 * m_sampleRatio ));
	combL[5].setbuffer(bufcombL6,static_cast<int>( combtuningL6 * m_sampleRatio ));
	combR[5].setbuffer(bufcombR6,static_cast<int>( combtuningR6 * m_sampleRatio ));
	combL[6].setbuffer(bufcombL7,static_cast<int>( combtuningL7 * m_sampleRatio ));
	combR[6].setbuffer(bufcombR7,static_cast<int>( combtuningR7 * m_sampleRatio ));
	combL[7].setbuffer(bufcombL8,static_cast<int>( combtuningL8 * m_sampleRatio ));
	combR[7].setbuffer(bufcombR8,static_cast<int>( combtuningR8 * m_sampleRatio ));
	allpassL[0].setbuffer(bufallpassL1,static_cast<int>( allpasstuningL1 * m_sampleRatio ));
	allpassR[0].setbuffer(bufallpassR1,static_cast<int>( allpasstuningR1 * m_sampleRatio ));
	allpassL[1].setbuffer(bufallpassL2,static_cast<int>( allpasstuningL2 * m_sampleRatio ));
	allpassR[1].setbuffer(bufallpassR2,static_cast<int>( allpasstuningR2 * m_sampleRatio ));
	allpassL[2].setbuffer(bufallpassL3,static_cast<int>( allpasstuningL3 * m_sampleRatio ));
	allpassR[2].setbuffer(bufallpassR3,static_cast<int>( allpasstuningR3 * m_sampleRatio ));
	allpassL[3].setbuffer(bufallpassL4,static_cast<int>( allpasstuningL4 * m_sampleRatio ));
	allpassR[3].setbuffer(bufallpassR4,static_cast<int>( allpasstuningR4 * m_sampleRatio ));

	// Set default values
	allpassL[0].setfeedback(0.5f);
	allpassR[0].setfeedback(0.5f);
	allpassL[1].setfeedback(0.5f);
	allpassR[1].setfeedback(0.5f);
	allpassL[2].setfeedback(0.5f);
	allpassR[2].setfeedback(0.5f);
	allpassL[3].setfeedback(0.5f);
	allpassR[3].setfeedback(0.5f);
	setwet(initialwet);
	setroomsize(initialroom);
	setdry(initialdry);
	setdamp(initialdamp);
	setwidth(initialwidth);
	setmode(initialmode);

	// Buffer will be full of rubbish - so we MUST mute them
	mute();
}

void revmodel::mute()
{
	int i;

	if (getmode() >= freezemode)
		return;

	for (i=0;i<numcombs;i++)
	{
		combL[i].mute();
		combR[i].mute();
	}
	for (i=0;i<numallpasses;i++)
	{
		allpassL[i].mute();
		allpassR[i].mute();
	}
}

void revmodel::processreplace(float *inputL, float *inputR, float *outputL, float *outputR, long numsamples, int skip)
{
	float outL,outR,input;
	int i;

	while(numsamples-- > 0)
	{
		outL = outR = 0;
		input = (*inputL + *inputR) * gain;

		// Accumulate comb filters in parallel
		for(i=0; i<numcombs; i++)
		{
			outL += combL[i].process(input);
			outR += combR[i].process(input);
		}

		// Feed through allpasses in series
		for(i=0; i<numallpasses; i++)
		{
			outL = allpassL[i].process(outL);
			outR = allpassR[i].process(outR);
		}

		// Calculate output REPLACING anything already there
		*outputL = outL*wet1 + outR*wet2 + *inputL*dry;
		*outputR = outR*wet1 + outL*wet2 + *inputR*dry;

		// Increment sample pointers, allowing for interleave (if any)
		inputL += skip;
		inputR += skip;
		outputL += skip;
		outputR += skip;
	}
}

void revmodel::processmix(float *inputL, float *inputR, float *outputL, float *outputR, long numsamples, int skip)
{
	float outL,outR,input;
	int i;

	while(numsamples-- > 0)
	{
		outL = outR = 0;
		input = (*inputL + *inputR) * gain;

		// Accumulate comb filters in parallel
		for(i=0; i<numcombs; i++)
		{
			outL += combL[i].process(input);
			outR += combR[i].process(input);
		}

		// Feed through allpasses in series
		for(i=0; i<numallpasses; i++)
		{
			outL = allpassL[i].process(outL);
			outR = allpassR[i].process(outR);
		}

		// Calculate output MIXING with anything already there
		*outputL += outL*wet1 + outR*wet2 + *inputL*dry;
		*outputR += outR*wet1 + outL*wet2 + *inputR*dry;

		// Increment sample pointers, allowing for interleave (if any)
		inputL += skip;
		inputR += skip;
		outputL += skip;
		outputR += skip;
	}
}

void revmodel::update()
{
// Recalculate internal values after parameter change

	int i;

	wet1 = wet*(width/2 + 0.5f);
	wet2 = wet*((1-width)/2);

	if (mode >= freezemode)
	{
		roomsize1 = 1;
		damp1 = 0;
		gain = muted;
	}
	else
	{
		roomsize1 = roomsize;
		damp1 = damp;
		gain = fixedgain;
	}

	for(i=0; i<numcombs; i++)
	{
		combL[i].setfeedback(roomsize1);
		combR[i].setfeedback(roomsize1);
	}

	for(i=0; i<numcombs; i++)
	{
		combL[i].setdamp(damp1);
		combR[i].setdamp(damp1);
	}
}

// The following get/set functions are not inlined, because
// speed is never an issue when calling them, and also
// because as you develop the reverb model, you may
// wish to take dynamic action when they are called.

void revmodel::setroomsize(float value)
{
	roomsize = (value*scaleroom) + offsetroom;
	update();
}

float revmodel::getroomsize()
{
	return (roomsize-offsetroom)/scaleroom;
}

void revmodel::setdamp(float value)
{
	damp = value*scaledamp;
	update();
}

float revmodel::getdamp()
{
	return damp/scaledamp;
}

void revmodel::setwet(float value)
{
	wet = value*scalewet;
	update();
}

float revmodel::getwet()
{
	return wet/scalewet;
}

void revmodel::setdry(float value)
{
	dry = value*scaledry;
}

float revmodel::getdry()
{
	return dry/scaledry;
}

void revmodel::setwidth(float value)
{
	width = value;
	update();
}

float revmodel::getwidth()
{
	return width;
}

void revmodel::setmode(float value)
{
	mode = value;
	update();
}

float revmodel::getmode()
{
	if (mode >= freezemode)
		return 1;
	else
		return 0;
}

//ends
