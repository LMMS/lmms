/*
 * fft_helpers.cpp - some functions around FFT analysis
 *
 * Copyright (c) 2008-2012 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of LMMS - http://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#include "fft_helpers.h"

#include "lmms_math.h"


/* returns biggest value from abs_spectrum[spec_size] array

   returns -1 on error
*/
float maximum(float *abs_spectrum, unsigned int spec_size)
{
	float maxi=0;
	unsigned int i;
	
	if ( abs_spectrum==NULL )
		return -1;
		
	if (spec_size<=0)
		return -1;

	for ( i=0; i<spec_size; i++ )
	{
		if ( abs_spectrum[i]>maxi )
			maxi=abs_spectrum[i];
	}
	
	return maxi;
}


/* apply hanning or hamming window to channel

	returns -1 on error */
int hanming(float *timebuffer, int length, WINDOWS type)
{
	int i;
	float alpha;
	
	if ( (timebuffer==NULL)||(length<=0) )
		return -1;
	
	switch (type)
	{
		case HAMMING: alpha=0.54; break;
		case HANNING: 
		default: alpha=0.5; break;
	}
	
	for ( i=0; i<length; i++ )
	{
		timebuffer[i]=timebuffer[i]*(alpha+(1-alpha)*cos(2*F_PI*i/((float)length-1.0)));
	}	

	return 0;
}


/* compute absolute values of complex_buffer, save to absspec_buffer
   take care that - compl_len is not bigger than complex_buffer!
                  - absspec buffer is big enough!

      returns 0 on success, else -1          */
int absspec(fftwf_complex *complex_buffer, float *absspec_buffer, int compl_length)
{
	int i;
	
	if ( (complex_buffer==NULL)||(absspec_buffer==NULL) )
		return -1;
	if ( compl_length<=0 )
		return -1;

	for (i=0; i<compl_length; i++)
	{
		absspec_buffer[i]=(float )sqrt(complex_buffer[i][0]*complex_buffer[i][0] + complex_buffer[i][1]*complex_buffer[i][1]);
	}
		
	return 0;
}


/* build fewer subbands from many absolute spectrum values
   take care that - compressedbands[] array num_new elements long
                  - num_old > num_new
 
     returns 0 on success, else -1          */
int compressbands(float *absspec_buffer, float *compressedband, int num_old, int num_new, int bottom, int top)
{
	float ratio;
	int i, usefromold;
	float j;
	float j_min, j_max;
	
	if ( (absspec_buffer==NULL)||(compressedband==NULL) )
		return -1;
		
	if ( num_old<num_new )
		return -1;
		
	if ( (num_old<=0)||(num_new<=0) )
		return -1;

	if ( bottom<0 )
		bottom=0;

	if ( top>=num_old )
		top=num_old-1;

	usefromold=num_old-(num_old-top)-bottom;

	ratio=(float)usefromold/(float)num_new;

	// foreach new subband
	for ( i=0; i<num_new; i++ )
	{
		compressedband[i]=0;
		
		j_min=(i*ratio)+bottom;

		if ( j_min<0 )
			j_min=bottom;
			
		j_max=j_min+ratio;

		for ( j=(int)j_min; j<=j_max; j++ ) 
		{
			compressedband[i]+=absspec_buffer[(int)j];
		}
	}

	return 0;
}


int calc13octaveband31(float *absspec_buffer, float *subbands, int num_spec, float max_frequency)
{
static const int onethirdoctavecenterfr[] = {20, 25, 31, 40, 50, 63, 80, 100, 125, 160, 200, 250, 315, 400, 500, 630, 800, 1000, 1250, 1600, 2000, 2500, 3150, 4000, 5000, 6300, 8000, 10000, 12500, 16000, 20000};
	int i, j;
	float f_min, f_max, frequency, bandwith;
	int j_min, j_max=0;
	float fpower;


	if ( (absspec_buffer==NULL)||(subbands==NULL) )
		return -1;

	if ( num_spec<31 )
		return -1;

	if ( max_frequency<=0 )
		return -1;

	/*** energy ***/
	fpower=0;
	for ( i=0; i<num_spec; i++ )
	{
		absspec_buffer[i]=(absspec_buffer[i]*absspec_buffer[i])/FFT_BUFFER_SIZE;
		fpower=fpower+(2*absspec_buffer[i]);
	}
	fpower=fpower-(absspec_buffer[0]); //dc not mirrored


	/*** for each subband: sum up power ***/
	for ( i=0; i<31; i++ )
	{
		subbands[i]=0;
		
		// calculate bandwith for subband
		frequency=onethirdoctavecenterfr[i];

		bandwith=(pow(2, 1.0/3.0)-1)*frequency;
		
		f_min=frequency-bandwith/2.0;
		f_max=frequency+bandwith/2.0;

		j_min=(int)(f_min/max_frequency*(float)num_spec);		

		j_max=(int)(f_max/max_frequency*(float)num_spec);


		if ( (j_min<0)||(j_max<0) )
		{
			fprintf(stderr, "Error: calc13octaveband31() in %s line %d failed.\n", __FILE__, __LINE__);
			return -1;
		}

		for ( j=j_min; j<=j_max; j++ )
		{
			if( j_max<num_spec )
				subbands[i]+=absspec_buffer[j];
		}

	} //for
	

	return 0;
}

/* compute power of finite time sequence
   take care num_values is length of timesignal[]

      returns power on success, else -1          */
float signalpower(float *timesignal, int num_values)
{
	if ( num_values<=0 )
		return -1;

	if( timesignal==NULL )
		return -1;
		
	float power=0;
	for ( int i=0; i<num_values; i++ )	
	{
		power+=timesignal[i]*timesignal[i];
	}
	
	return power;	
}

