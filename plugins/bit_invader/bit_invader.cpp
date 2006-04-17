/*
 * bit_invader.cpp - instrument which uses a usereditable wavetable
 *
 * Copyright (c) 2006 Andreas Brandmaier <andy/at/brandmaier/dot/de>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include "qt3support.h"

#ifdef QT4

#include <QtGui/QPainter>
#include <Qt/QtXml>
#include <QtGui/QDropEvent>

#else

#include <qpainter.h>
#include <qbitmap.h>
#include <qdom.h>
#include <qfileinfo.h>
#include <qdom.h>
#include <qmap.h>
#include <qcanvas.h>
#include <qlabel.h>

#endif

#include <iostream>
#include <cstdlib>
#include <ctime>
#include "math.h"

using namespace std;


#include "bit_invader.h"
#include "instrument_track.h"
#include "note_play_handle.h"
#include "templates.h"
#include "buffer_allocator.h"
#include "knob.h"
#include "pixmap_button.h"
#include "tooltip.h"
#include "song_editor.h"
#include "oscillator.h"
#include "sample_buffer.h"
#include "base64.h"

#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"

extern "C"
{

plugin::descriptor bitinvader_plugin_descriptor =
{
	STRINGIFY_PLUGIN_NAME( PLUGIN_NAME ),
	"BitInvader",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"Rough & Dirty Wavetable Synthesizer." ),
	"Andreas Brandmaier <andreas/at/brandmaier/dot/de>",
	0x0100,
	plugin::INSTRUMENT,
	new QPixmap( PLUGIN_NAME::getIconPixmap( "logo" ) )
} ;

}

QPixmap * bitInvader::s_artwork = NULL;


bSynth::bSynth(float* shape, int length, float _pitch, bool _interpolation, float factor, const sample_rate_t _sample_rate )
{

	interpolation = _interpolation;

	// init variables

	sample_length = length;
	sample_shape = new float[sample_length];
	for (int i=0; i < length; i++)
	{
		sample_shape[i] = shape[i] * factor;
	}


	sample_index = 0;
	sample_realindex = 0;
	

	sample_step = static_cast<float>( sample_length / ( _sample_rate /
		 						_pitch ) );
	

}


bSynth::~bSynth()
{
	delete[] sample_shape;
}

sample_t bSynth::nextStringSample( void )
{

	
	// check overflow
	while (sample_realindex >= sample_length) {
		sample_realindex -= sample_length;
	}

	sample_t sample;

	if (interpolation) {

	// find position in shape 
	int a = static_cast<int>(sample_realindex);	
	int b;
	if (a < (sample_length-1)) {
		b = static_cast<int>(sample_realindex+1);
	} else {
		b = 0;
	}
	
	// Nachkommaanteil
	float frac = sample_realindex - static_cast<int>(sample_realindex);
	
	sample = sample_shape[a]*(1-frac) + sample_shape[b]*(frac);

	} else {
		// No interpolation
		sample_index = static_cast<int>(sample_realindex);	
		sample = sample_shape[sample_index];
	}
	
	// progress in shape
	sample_realindex += sample_step;

//	cout << sample_index << "\t";
	
	return sample;
}	

/***********************************************************************
*
*	class BitInvader
*
*	lmms - plugin 
*
***********************************************************************/


bitInvader::bitInvader( instrumentTrack * _channel_track ) :
	instrument( _channel_track,
			&bitinvader_plugin_descriptor ),
	specialBgHandlingWidget( PLUGIN_NAME::getIconPixmap( "artwork" ) )
{


	m_graph = NULL;
	normalize = false;
	interpolation = false;
	
	if( s_artwork == NULL )
	{
		s_artwork = new QPixmap( PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	}


	m_sampleLengthKnob = new knob( knobDark_28, this, tr( "Samplelength" ),
									eng() );
	m_sampleLengthKnob->setRange( 8, 128, 1 );
 	m_sampleLengthKnob->setInitValue( 128 );
	m_sampleLengthKnob->move( 10, 120 );
	m_sampleLengthKnob->setHintText( tr( "Sample Length" ) + " ", "" );

	connect( m_sampleLengthKnob, SIGNAL( valueChanged( float ) ),
		this, SLOT ( sampleSizeChanged( float ) ) 
		);

	m_interpolationToggle = new ledCheckBox( "Interpolation", this, eng() );
	m_interpolationToggle->move( 55,80 );
	
	 connect( m_interpolationToggle, SIGNAL( toggled( bool ) ),
			this, SLOT ( interpolationToggle( bool ) ) );

	m_normalizeToggle = new ledCheckBox( "Normalize", this, eng() );
	m_normalizeToggle->move( 55, 100 );
	
	connect( m_normalizeToggle, SIGNAL( toggled( bool ) ),
			this, SLOT ( normalizeToggle( bool ) ) );


	m_graph = new graph( this, eng() );
	m_graph->move(53,118);	// 55,120 - 2px border
	m_graph->setCursor( QCursor( Qt::CrossCursor ) );

	toolTip::add( m_graph, tr ( "Draw your own waveform here"
				"by dragging your mouse onto this graph"
	));

	QPixmap p = PLUGIN_NAME::getIconPixmap("wavegraph3") ;

	m_graph->setBackground( p );

	connect( m_graph, SIGNAL ( sampleSizeChanged( float ) ), 
		this, SLOT (sampleSizeChanged( float ) ) );
	
	connect( m_graph, SIGNAL ( sampleChanged( void ) ),
		this, SLOT ( sampleChanged( void ) ) );
	
		sinWaveBtn = new pixmapButton( this, eng() );
		sinWaveBtn->move( 188, 120 );
		sinWaveBtn->setActiveGraphic( embed::getIconPixmap(
							"sin_wave_active" ) );
		sinWaveBtn->setInactiveGraphic( embed::getIconPixmap(
							"sin_wave_inactive" ) );
		toolTip::add( sinWaveBtn,
				tr( "Click here if you want a sine-wave for "
						"current oscillator." ) );

		triangleWaveBtn = new pixmapButton( this, eng() );
		triangleWaveBtn->move( 188, 136 );
		triangleWaveBtn->setActiveGraphic(
			embed::getIconPixmap( "triangle_wave_active" ) );
		triangleWaveBtn->setInactiveGraphic(
			embed::getIconPixmap( "triangle_wave_inactive" ) );
		toolTip::add( triangleWaveBtn,
				tr( "Click here if you want a triangle-wave "
						"for current oscillator." ) );

		sawWaveBtn = new pixmapButton( this, eng() );
		sawWaveBtn->move( 188, 152 );
		sawWaveBtn->setActiveGraphic( embed::getIconPixmap(
							"saw_wave_active" ) );
		sawWaveBtn->setInactiveGraphic( embed::getIconPixmap(
							"saw_wave_inactive" ) );
		toolTip::add( sawWaveBtn,
				tr( "Click here if you want a saw-wave for "
						"current oscillator." ) );

		sqrWaveBtn = new pixmapButton( this, eng() );
		sqrWaveBtn->move( 188, 168 );
		sqrWaveBtn->setActiveGraphic( embed::getIconPixmap(
						"square_wave_active" ) );
		sqrWaveBtn->setInactiveGraphic( embed::getIconPixmap(
						"square_wave_inactive" ) );
		toolTip::add( sqrWaveBtn,
				tr( "Click here if you want a square-wave for "
						"current oscillator." ) );

		whiteNoiseWaveBtn = new pixmapButton( this, eng() );
		whiteNoiseWaveBtn->move( 188, 184 );
		whiteNoiseWaveBtn->setActiveGraphic(
			embed::getIconPixmap( "white_noise_wave_active" ) );
		whiteNoiseWaveBtn->setInactiveGraphic(
			embed::getIconPixmap( "white_noise_wave_inactive" ) );
		toolTip::add( whiteNoiseWaveBtn,
				tr( "Click here if you want a white-noise for "
						"current oscillator." ) );

		usrWaveBtn = new pixmapButton( this, eng() );
		usrWaveBtn->move( 188, 200 );
		usrWaveBtn->setActiveGraphic( embed::getIconPixmap(
							"usr_wave_active" ) );
		usrWaveBtn->setInactiveGraphic( embed::getIconPixmap(
							"usr_wave_inactive" ) );
		toolTip::add( usrWaveBtn,
				tr( "Click here if you want a user-defined "
				"wave-shape for current oscillator." ) );


		connect( sinWaveBtn, SIGNAL (clicked ( void ) ),
			this, SLOT ( sinWaveClicked( void ) ) );
		connect( triangleWaveBtn, SIGNAL ( clicked ( void ) ),
			this, SLOT ( triangleWaveClicked( void ) ) );
		connect( sawWaveBtn, SIGNAL (clicked ( void ) ),
			this, SLOT ( sawWaveClicked( void ) ) );
		connect( sqrWaveBtn, SIGNAL ( clicked ( void ) ),
			this, SLOT ( sqrWaveClicked( void ) ) );
		connect( whiteNoiseWaveBtn, SIGNAL ( clicked ( void ) ),
			this, SLOT ( noiseWaveClicked( void ) ) );
		connect( usrWaveBtn, SIGNAL ( clicked ( void ) ),
			this, SLOT ( usrWaveClicked( void ) ) );
		


		smoothBtn = new pixmapButton( this, eng() );
		smoothBtn->move( 55, 225 );
		smoothBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"smooth" ) );
		smoothBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"smooth" ) );
		smoothBtn->setChecked( TRUE );
		toolTip::add( smoothBtn,
				tr( "Click here to "
						"smooth waveform." ) );

		connect( smoothBtn, SIGNAL ( clicked ( void ) ),
			this, SLOT ( smoothClicked( void ) ) );		


#ifdef QT4
	setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	setPalette( pal );
#else
	setErasePixmap( PLUGIN_NAME::getIconPixmap( "artwork" ) );
#endif


	sample_length = 128;
	sample_shape = new float[128];
	m_graph->setSamplePointer( sample_shape, sample_length );
	emit( sinWaveClicked() );
	

}


/*
void bitInvader::paintEvent( QPaintEvent * )
{
#ifdef QT4
	QPainter p( this );
#else
	QPixmap pm( rect().size() );
	pm.fill( this, rect().topLeft() );

	QPainter p( &pm, this );
#endif
	p.drawPixmap( 0, 0, *s_artwork );

	p.setPen( QColor( 255, 255, 255 ) );
//	p.setPen( QColor( 0xFF, 0xAA, 0x00 ) );

//	p.drawLine(0,0,200,200);

#ifndef QT4
	bitBlt( this, rect().topLeft(), &pm );
#endif


}
*/

void bitInvader::sinWaveClicked( void )
{
	// generate a Sinus wave using static oscillator-method
        for (int i=0; i < sample_length; i++)
        {
	    sample_shape[i] = oscillator::sinSample( i/static_cast<float>(sample_length) );
        }
	
	sampleChanged();
}

void bitInvader::triangleWaveClicked( void )
{
	// generate a Triangle wave using static oscillator-method
    for (int i=0; i < sample_length; i++)
        {
	    sample_shape[i] = oscillator::triangleSample( i/static_cast<float>(sample_length) );
        }
	
	sampleChanged();

}


void bitInvader::sawWaveClicked( void )
{
	// generate a Saw wave using static oscillator-method
	for (int i=0; i < sample_length; i++)
    {
	    sample_shape[i] = oscillator::sawSample( i/static_cast<float>(sample_length) );
    }
        
    sampleChanged();
}

void bitInvader::sqrWaveClicked( void )
{
	// generate a Sqr wave using static oscillator-method
	for (int i=0; i < sample_length; i++)
    {
	    sample_shape[i] = oscillator::squareSample( i/static_cast<float>(sample_length) );
    }
        
    sampleChanged();

}

void bitInvader::noiseWaveClicked( void )
{
	// generate a Noise wave using static oscillator-method
	for (int i=0; i < sample_length; i++)
    {
	    sample_shape[i] = oscillator::noiseSample( i/static_cast<float>(sample_length) );
    }
        
    sampleChanged();

}

void bitInvader::usrWaveClicked( void )
{
	// zero sample_shape
	for (int i = 0; i < sample_length; i++)
	{
		sample_shape[i] = 0;
	}

	// load user shape
	sampleBuffer buffer( eng() );
	QString af = buffer.openAudioFile();
	if ( af != "" )
	{
		buffer.setAudioFile( af );
		
		// copy buffer data
		sample_length = min( sample_length, static_cast<int>(buffer.frames()) );		 
		for ( int i = 0; i < sample_length; i++ )
		{
			sample_shape[i] = (float)*buffer.data()[i];
		}
	}
	
	sampleChanged();
		
}



/*

	deprecated code
	
	was replaced by static oscillator methods

void bitInvader::sinWaveClicked( void )
{
	// generate sample data
        for (int i=0; i < sample_length; i++)
        {
          // sin(x)
          sample_shape[i] = sinf(i * 6.2831853 / sample_length);
        }

        sampleChanged();
}

void bitInvader::triangleWaveClicked( void )
{
	int half_sample_length = sample_length / 2;

	if ((sample_length % 2) == 0) {

		for (int i=0; i < half_sample_length; i++)
       		{
		  // triangle
       		  sample_shape[i] = (((float)i) / half_sample_length * 2) - 1;
       		}
	        for (int i=half_sample_length; i < sample_length; i++)
	        {
	          // triangle
	          sample_shape[i] = - (((float)(i-half_sample_length)) / half_sample_length * 2) + 1;
	        }

        } else {
		for (int i=0; i < half_sample_length; i++)
       		{
		  // triangle
       		  sample_shape[i] = (((float)i) / half_sample_length * 2) - 1;
       		}
       		sample_shape[half_sample_length] = 1;
	        for (int i=half_sample_length+1; i < sample_length; i++)
	        {
	          // triangle
	          sample_shape[i] = - (((float)(i-half_sample_length)) / half_sample_length * 2) + 1;
	        }

        }

	sampleChanged();        
}

void bitInvader::sawWaveClicked( void )
{
	for (int i=0; i < sample_length; i++)
        {
          // saw
          sample_shape[i] = (((float)i) / sample_length * 2) - 1;
        }
        
        sampleChanged();
}

void bitInvader::sqrWaveClicked( void )
{
	int half_sample_length = sample_length / 2;

		for (int i=0; i < half_sample_length; i++)
       		{
		  // triangle
       		  sample_shape[i] = 1;
       		}
	        for (int i=half_sample_length; i < sample_length; i++)
	        {
	          // triangle
	          sample_shape[i] = -1;
	        }

	sampleChanged();                                
                                

}

void bitInvader::noiseWaveClicked( void)
{
	srand(time(NULL));

	for (int i=0; i < sample_length; i++)
	{
		sample_shape[i]= ((float)rand() / RAND_MAX * 2.0) - 1.0;
	}
	
	sampleChanged();	                                
	                                
}


*/


bitInvader::~bitInvader()
{
}




void bitInvader::saveSettings( QDomDocument & _doc, QDomElement & _this )
{

	// Save plugin version
	_this.setAttribute( "version", "0.1" );

	// Save sample length
	_this.setAttribute( "sampleLength", QString::number( sample_length ) );

	// Save sample shape base64-encoded
	QString sampleString;
	base64::encode( (const char *)sample_shape, 
		sample_length * sizeof(float), sampleString );
	_this.setAttribute( "sampleShape", sampleString );
	

	// save LED normalize 
	_this.setAttribute( "interpolation",
					m_interpolationToggle->isChecked() );
	
	// save LED 
	_this.setAttribute( "normalize", m_normalizeToggle->isChecked() );

}




void bitInvader::loadSettings( const QDomElement & _this )
{
	// Load sample length
	sample_length = _this.attribute( "sampleLength" ).toInt() ;

	// Load knobs  (fires change SIGNAL?)
	m_sampleLengthKnob->setValue( static_cast<float>(sample_length) );

	// Load sample shape
	delete[] sample_shape;
	sample_shape = new float[sample_length];
	int size = 0;
	char * dst = 0;
	base64::decode( _this.attribute( "sampleShape"), &dst, &size );
	memcpy( sample_shape, dst, tMin<int>( size, sample_length *
							sizeof( float ) ) );

	delete[] dst;
    	m_graph->setSamplePointer( sample_shape, sample_length );

	// Load LED normalize 
	m_interpolationToggle->setChecked( _this.attribute(
						"interpolation" ).toInt() );
	// Load LED 
	m_normalizeToggle->setChecked( _this.attribute( "normalize" ).toInt() );
	update();

//	songEditor::inst()->setModified();

}

void bitInvader::interpolationToggle( bool value )
{
      	interpolation = value;

	eng()->getSongEditor()->setModified();
}
        
void bitInvader::normalizeToggle( bool value )
{
       	normalize = value;

	eng()->getSongEditor()->setModified();

}


QString bitInvader::nodeName( void ) const
{
	return( bitinvader_plugin_descriptor.name );
}


void bitInvader::smoothClicked( void )
{
	// store values in temporary array
	float* temp = new float[sample_length];
	memcpy( temp, sample_shape, sizeof( float ) * sample_length );

	// Smoothing
	sample_shape[0] = ( temp[0]+temp[sample_length-1] ) * 0.5f;
	for ( int i=1; i < sample_length; i++)
	{
		sample_shape[i] = (temp[i-1] + temp[i]) * 0.5f; 	
	}


	// Clean up
	delete[] temp;
	
	// paint
	update();
	m_graph->update();

	eng()->getSongEditor()->setModified();

}




void bitInvader::playNote( notePlayHandle * _n )
{
	if ( _n->totalFramesPlayed() == 0 )
	{
	
		float freq = getInstrumentTrack()->frequency( _n );
		
		float factor;
		if (!normalize) {
			factor = 1.0; 
		} else {
			factor = normalizeFactor;
		}
		
		_n->m_pluginData = new bSynth( sample_shape, sample_length,freq
					, interpolation, factor,
					eng()->getMixer()->sampleRate() );
	}

	const fpab_t frames = eng()->getMixer()->framesPerAudioBuffer();
	sampleFrame * buf = bufferAllocator::alloc<sampleFrame>( frames );

	bSynth * ps = static_cast<bSynth *>( _n->m_pluginData );
	for( fpab_t frame = 0; frame < frames; ++frame )
	{
		const sample_t cur = ps->nextStringSample();
		for( Uint8 chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
		{
			buf[frame][chnl] = cur;
		}
	}

	getInstrumentTrack()->processAudioBuffer( buf, frames, _n );

	bufferAllocator::free( buf );
}




void bitInvader::deleteNotePluginData( notePlayHandle * _n )
{
	delete static_cast<bSynth *>( _n->m_pluginData );
}


void bitInvader::sampleSizeChanged( float _new_sample_length )
{
	int new_sample_length = static_cast<int>(_new_sample_length);

	// ** grow array
	if (new_sample_length > sample_length) {

		// store values in temporary array
		float* temp = new float[sample_length];
		for (int i=0; i < sample_length; i++)
		{
			temp[i] = sample_shape[i];
		}
	
		// reinitialize sample array
		delete[] sample_shape;
		sample_shape = new float[new_sample_length];
		for (int i=0; i < new_sample_length; i++)
		{
			sample_shape[i] = 0;
		}
		
		// fill in old values
		for (int i=0; i < sample_length; i++)
		{
			sample_shape[i] = temp[i];
		}
		
		delete[] temp;
		sample_length = new_sample_length;
		
	}

	// ** shrink array
	if (new_sample_length < sample_length) {

		sample_length = new_sample_length;	
	
	}

               
  	// update sample graph        
       	m_graph->setSamplePointer( sample_shape, sample_length );

	// set Song modified
	eng()->getSongEditor()->setModified();

}                                                
                                               
void bitInvader::sampleChanged()
{

	// analyze
	float max = 0;
	for (int i=0; i < sample_length; i++)
	{
		if (fabsf(sample_shape[i]) > max) { max = fabs(sample_shape[i]); }
	}
	normalizeFactor = 1.0 / max;


	// update
        if (m_graph != NULL) {
             m_graph->update();
	}

	eng()->getSongEditor()->setModified();
                                
}



extern "C"
{

// neccessary for getting instance out of shared lib
plugin * lmms_plugin_main( void * _data )
{
	return( new bitInvader( static_cast<instrumentTrack *>( _data ) ) );
}


}


