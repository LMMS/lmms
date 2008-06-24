/*
 * spectrum_analyzer.cpp - spectrum analyzer plugin
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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


#include "spectrum_analyzer.h"


#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"


extern "C"
{

plugin::descriptor PLUGIN_EXPORT spectrumanalyzer_plugin_descriptor =
{
	STRINGIFY_PLUGIN_NAME( PLUGIN_NAME ),
	"Spectrum Analyzer",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"plugin for using arbitrary VST-effects "
				"inside LMMS." ),
	"Tobias Doerffel <tobydox/at/users.sf.net>",
	0x0100,
	plugin::Effect,
	new pluginPixmapLoader( "logo" ),
	NULL
} ;

}



spectrumAnalyzer::spectrumAnalyzer( model * _parent,
			const descriptor::subPluginFeatures::key * _key ) :
	effect( &spectrumanalyzer_plugin_descriptor, _parent, _key ),
	m_saControls( this ),
	m_framesFilledUp( 0 ),
	m_energy( 0 )
{
	m_specBuf = (fftwf_complex *) fftwf_malloc( ( BUFFER_SIZE + 1 ) *
						sizeof( fftwf_complex ) );
	m_fftPlan = fftwf_plan_dft_r2c_1d( BUFFER_SIZE*2, m_buffer,
						m_specBuf, FFTW_MEASURE );
}




spectrumAnalyzer::~spectrumAnalyzer()
{
	fftwf_destroy_plan( m_fftPlan );
	fftwf_free( m_specBuf );
}



enum WINDOWS
{
        KAISER=1,
        RECTANGLE,
        HANNING,
        HAMMING
};


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
int hanming(float *timebuffer, int length, int type)
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
		timebuffer[i]=timebuffer[i]*(alpha+(1-alpha)*cos(2*M_PI*i/((float)length-1.0)));
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
		absspec_buffer[i]=(absspec_buffer[i]*absspec_buffer[i])/BUFFER_SIZE;
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
	float power=0;
	unsigned int i;
	
	if ( num_values<=0 )
		return -1;

	if( timesignal==NULL )
		return -1;
		
	for ( i=0; i<num_values; i++ )	
	{
		power+=timesignal[i]*timesignal[i];
	}
	
	return power;	
}


bool spectrumAnalyzer::processAudioBuffer( sampleFrame * _buf,
							const fpp_t _frames )
{
	if( !isEnabled() || !isRunning () )
	{
		return( FALSE );
	}

	fpp_t f = 0;
	if( _frames > BUFFER_SIZE )
	{
		m_framesFilledUp = 0;
		f = _frames - BUFFER_SIZE;
	}

	const int cm = m_saControls.m_channelMode.value();

	switch( cm )
	{
		case MergeChannels:
			for( ; f < _frames; ++f )
			{
				m_buffer[m_framesFilledUp] =
					( _buf[f][0] + _buf[f][1] ) * 0.5;
				++m_framesFilledUp;
			}
			break;
		case LeftChannel:
			for( ; f < _frames; ++f )
			{
				m_buffer[m_framesFilledUp] = _buf[f][0];
				++m_framesFilledUp;
			}
			break;
		case RightChannel:
			for( ; f < _frames; ++f )
			{
				m_buffer[m_framesFilledUp] = _buf[f][1];
				++m_framesFilledUp;
			}
			break;
	}

	if( m_framesFilledUp < BUFFER_SIZE )
	{
		return( isRunning() );
	}


//	hanming( m_buffer, BUFFER_SIZE, HAMMING );

	const sample_rate_t sr = engine::getMixer()->processingSampleRate();
	const int LOWEST_FREQ = 0;
	const int HIGHEST_FREQ = sr / 2;

	fftwf_execute( m_fftPlan );
	absspec( m_specBuf, m_absSpecBuf, BUFFER_SIZE+1 );
	if( m_saControls.m_linearSpec.value() )
	{
		compressbands( m_absSpecBuf, m_bands, BUFFER_SIZE+1,
			MAX_BANDS,
			LOWEST_FREQ*(BUFFER_SIZE+1)/(float)(sr/2),
			HIGHEST_FREQ*(BUFFER_SIZE+1)/(float)(sr/2) );
		m_energy = maximum( m_bands, MAX_BANDS ) / maximum( m_buffer, BUFFER_SIZE );
	}
	else
	{
		calc13octaveband31( m_absSpecBuf, m_bands, BUFFER_SIZE+1, sr/2.0);
		m_energy = signalpower( m_buffer, BUFFER_SIZE ) / maximum( m_buffer, BUFFER_SIZE );
	}


	m_framesFilledUp = 0;

	checkGate( 0 );

	return( isRunning() );
}





extern "C"
{

// neccessary for getting instance out of shared lib
plugin * PLUGIN_EXPORT lmms_plugin_main( model * _parent, void * _data )
{
	return( new spectrumAnalyzer( _parent,
		static_cast<const plugin::descriptor::subPluginFeatures::key *>(
								_data ) ) );
}

}

