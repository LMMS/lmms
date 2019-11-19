/*
 * bit_invader.cpp - instrument which uses a usereditable wavetable
 *
 * Copyright (c) 2006-2008 Andreas Brandmaier <andy/at/brandmaier/dot/de>
 * 
 * This file is part of LMMS - https://lmms.io
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


#include <QDomElement>

#include "bit_invader.h"
#include "base64.h"
#include "Engine.h"
#include "Graph.h"
#include "InstrumentTrack.h"
#include "Knob.h"
#include "LedCheckbox.h"
#include "Mixer.h"
#include "NotePlayHandle.h"
#include "Oscillator.h"
#include "PixmapButton.h"
#include "ToolTip.h"
#include "Song.h"
#include "interpolation.h"

#include "embed.h"

#include "plugin_export.h"

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT bitinvader_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"BitInvader",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"Customizable wavetable synthesizer" ),
	"Andreas Brandmaier <andreas/at/brandmaier/dot/de>",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
} ;

}


bSynth::bSynth( float * _shape, NotePlayHandle * _nph, bool _interpolation,
				float _factor, const sample_rate_t _sample_rate ) :
	sample_index( 0 ),
	sample_realindex( 0 ),
	nph( _nph ),
	sample_rate( _sample_rate ),
	interpolation( _interpolation)
{
	sample_shape = new float[200];
	for (int i=0; i < 200; ++i)
	{
		sample_shape[i] = _shape[i] * _factor;
	}
}


bSynth::~bSynth()
{
	delete[] sample_shape;
}


sample_t bSynth::nextStringSample( float sample_length )
{
	float sample_step = 
		static_cast<float>( sample_length / ( sample_rate / nph->frequency() ) );

	
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
		const float frac = fraction( sample_realindex );
		
		sample = linearInterpolate( sample_shape[a], sample_shape[b], frac );

	} else {
		// No interpolation
		sample_index = static_cast<int>(sample_realindex);	
		sample = sample_shape[sample_index];
	}
	
	// progress in shape
	sample_realindex += sample_step;

	return sample;
}	

/***********************************************************************
*
*	class BitInvader
*
*	lmms - plugin 
*
***********************************************************************/


bitInvader::bitInvader( InstrumentTrack * _instrument_track ) :
	Instrument( _instrument_track, &bitinvader_plugin_descriptor ),
	m_sampleLength( 128, 4, 200, 1, this, tr( "Sample length" ) ),
	m_graph( -1.0f, 1.0f, 200, this ),
	m_interpolation( false, this ),
	m_normalize( false, this )
{
		
	lengthChanged();

	m_graph.setWaveToSine();

	connect( &m_sampleLength, SIGNAL( dataChanged( ) ),
			this, SLOT( lengthChanged( ) ) );

	connect( &m_graph, SIGNAL( samplesChanged( int, int ) ),
			this, SLOT( samplesChanged( int, int ) ) );

}




bitInvader::~bitInvader()
{
}




void bitInvader::saveSettings( QDomDocument & _doc, QDomElement & _this )
{

	// Save plugin version
	_this.setAttribute( "version", "0.1" );

	// Save sample length
	m_sampleLength.saveSettings( _doc, _this, "sampleLength" );

	// Save sample shape base64-encoded
	QString sampleString;
	base64::encode( (const char *)m_graph.samples(),
		m_graph.length() * sizeof(float), sampleString );
	_this.setAttribute( "sampleShape", sampleString );
	

	// save LED normalize 
	m_interpolation.saveSettings( _doc, _this, "interpolation" );
	
	// save LED 
	m_normalize.saveSettings( _doc, _this, "normalize" );
}




void bitInvader::loadSettings( const QDomElement & _this )
{
	// Load sample length
	m_sampleLength.loadSettings( _this, "sampleLength" );

	int sampleLength = (int)m_sampleLength.value();

	// Load sample shape
	int size = 0;
	char * dst = 0;
	base64::decode( _this.attribute( "sampleShape"), &dst, &size );

	m_graph.setLength( sampleLength );
	m_graph.setSamples( (float*) dst );
	delete[] dst;

	// Load LED normalize 
	m_interpolation.loadSettings( _this, "interpolation" );
	// Load LED 
	m_normalize.loadSettings( _this, "normalize" );

}




void bitInvader::lengthChanged()
{
	m_graph.setLength( (int) m_sampleLength.value() );

	normalize();
}




void bitInvader::samplesChanged( int _begin, int _end )
{
	normalize();
	//engine::getSongEditor()->setModified();
}




void bitInvader::normalize()
{
	// analyze
	float max = 0;
	const float* samples = m_graph.samples();
	for(int i=0; i < m_graph.length(); i++)
	{
		const float f = fabsf( samples[i] );
		if (f > max) { max = f; }
	}
	m_normalizeFactor = 1.0 / max;
}




QString bitInvader::nodeName() const
{
	return( bitinvader_plugin_descriptor.name );
}




void bitInvader::playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer )
{
	if ( _n->totalFramesPlayed() == 0 || _n->m_pluginData == NULL )
	{
	
		float factor;
		if( !m_normalize.value() )
		{
			factor = 1.0f;
		}
		else
		{
			factor = m_normalizeFactor;
		}

		_n->m_pluginData = new bSynth(
					const_cast<float*>( m_graph.samples() ),
					_n,
					m_interpolation.value(), factor,
				Engine::mixer()->processingSampleRate() );
	}

	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();

	bSynth * ps = static_cast<bSynth *>( _n->m_pluginData );
	for( fpp_t frame = offset; frame < frames + offset; ++frame )
	{
		const sample_t cur = ps->nextStringSample( m_graph.length() );
		for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
		{
			_working_buffer[frame][chnl] = cur;
		}
	}

	applyRelease( _working_buffer, _n );

	instrumentTrack()->processAudioBuffer( _working_buffer, frames + offset, _n );
}




void bitInvader::deleteNotePluginData( NotePlayHandle * _n )
{
	delete static_cast<bSynth *>( _n->m_pluginData );
}




PluginView * bitInvader::instantiateView( QWidget * _parent )
{
	return( new bitInvaderView( this, _parent ) );
}







bitInvaderView::bitInvaderView( Instrument * _instrument,
					QWidget * _parent ) :
	InstrumentViewFixedSize( _instrument, _parent )
{
	setAutoFillBackground( true );
	QPalette pal;

	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	setPalette( pal );
	
	m_sampleLengthKnob = new Knob( knobDark_28, this );
	m_sampleLengthKnob->move( 6, 201 );
	m_sampleLengthKnob->setHintText( tr( "Sample length" ), "" );

	m_graph = new Graph( this, Graph::NearestStyle, 204, 134 );
	m_graph->move(23,59);	// 55,120 - 2px border
	m_graph->setAutoFillBackground( true );
	m_graph->setGraphColor( QColor( 255, 255, 255 ) );

	ToolTip::add( m_graph, tr ( "Draw your own waveform here "
				"by dragging your mouse on this graph."
	));


	pal = QPalette();
	pal.setBrush( backgroundRole(), 
			PLUGIN_NAME::getIconPixmap("wavegraph") );
	m_graph->setPalette( pal );


	m_sinWaveBtn = new PixmapButton( this, tr( "Sine wave" ) );
	m_sinWaveBtn->move( 131, 205 );
	m_sinWaveBtn->setActiveGraphic( embed::getIconPixmap(
						"sin_wave_active" ) );
	m_sinWaveBtn->setInactiveGraphic( embed::getIconPixmap(
						"sin_wave_inactive" ) );
	ToolTip::add( m_sinWaveBtn,
			tr( "Sine wave" ) );

	m_triangleWaveBtn = new PixmapButton( this, tr( "Triangle wave" ) );
	m_triangleWaveBtn->move( 131 + 14, 205 );
	m_triangleWaveBtn->setActiveGraphic(
		embed::getIconPixmap( "triangle_wave_active" ) );
	m_triangleWaveBtn->setInactiveGraphic(
		embed::getIconPixmap( "triangle_wave_inactive" ) );
	ToolTip::add( m_triangleWaveBtn,
			tr( "Triangle wave" ) );

	m_sawWaveBtn = new PixmapButton( this, tr( "Saw wave" ) );
	m_sawWaveBtn->move( 131 + 14*2, 205  );
	m_sawWaveBtn->setActiveGraphic( embed::getIconPixmap(
						"saw_wave_active" ) );
	m_sawWaveBtn->setInactiveGraphic( embed::getIconPixmap(
						"saw_wave_inactive" ) );
	ToolTip::add( m_sawWaveBtn,
			tr( "Saw wave" ) );

	m_sqrWaveBtn = new PixmapButton( this, tr( "Square wave" ) );
	m_sqrWaveBtn->move( 131 + 14*3, 205 );
	m_sqrWaveBtn->setActiveGraphic( embed::getIconPixmap(
					"square_wave_active" ) );
	m_sqrWaveBtn->setInactiveGraphic( embed::getIconPixmap(
					"square_wave_inactive" ) );
	ToolTip::add( m_sqrWaveBtn,
			tr( "Square wave" ) );

	m_whiteNoiseWaveBtn = new PixmapButton( this,
					tr( "White noise" ) );
	m_whiteNoiseWaveBtn->move( 131 + 14*4, 205 );
	m_whiteNoiseWaveBtn->setActiveGraphic(
		embed::getIconPixmap( "white_noise_wave_active" ) );
	m_whiteNoiseWaveBtn->setInactiveGraphic(
		embed::getIconPixmap( "white_noise_wave_inactive" ) );
	ToolTip::add( m_whiteNoiseWaveBtn,
			tr( "White noise" ) );

	m_usrWaveBtn = new PixmapButton( this, tr( "User-defined wave" ) );
	m_usrWaveBtn->move( 131 + 14*5, 205 );
	m_usrWaveBtn->setActiveGraphic( embed::getIconPixmap(
						"usr_wave_active" ) );
	m_usrWaveBtn->setInactiveGraphic( embed::getIconPixmap(
						"usr_wave_inactive" ) );
	ToolTip::add( m_usrWaveBtn,
			tr( "User-defined wave" ) );

	m_smoothBtn = new PixmapButton( this, tr( "Smooth waveform" ) );
	m_smoothBtn->move( 131 + 14*6, 205 );
	m_smoothBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
						"smooth_active" ) );
	m_smoothBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
						"smooth_inactive" ) );
	ToolTip::add( m_smoothBtn,
			tr( "Smooth waveform" ) );


	m_interpolationToggle = new LedCheckBox( "Interpolation", this,
							tr( "Interpolation" ), LedCheckBox::Yellow );
	m_interpolationToggle->move( 131, 221 );


	m_normalizeToggle = new LedCheckBox( "Normalize", this,
							tr( "Normalize" ), LedCheckBox::Green );
	m_normalizeToggle->move( 131, 236 );
	
	
	connect( m_sinWaveBtn, SIGNAL (clicked () ),
			this, SLOT ( sinWaveClicked() ) );
	connect( m_triangleWaveBtn, SIGNAL ( clicked () ),
			this, SLOT ( triangleWaveClicked() ) );
	connect( m_sawWaveBtn, SIGNAL (clicked () ),
			this, SLOT ( sawWaveClicked() ) );
	connect( m_sqrWaveBtn, SIGNAL ( clicked () ),
			this, SLOT ( sqrWaveClicked() ) );
	connect( m_whiteNoiseWaveBtn, SIGNAL ( clicked () ),
			this, SLOT ( noiseWaveClicked() ) );
	connect( m_usrWaveBtn, SIGNAL ( clicked () ),
			this, SLOT ( usrWaveClicked() ) );
	
	connect( m_smoothBtn, SIGNAL ( clicked () ),
			this, SLOT ( smoothClicked() ) );		

	connect( m_interpolationToggle, SIGNAL( toggled( bool ) ),
			this, SLOT ( interpolationToggled( bool ) ) );

	connect( m_normalizeToggle, SIGNAL( toggled( bool ) ),
			this, SLOT ( normalizeToggled( bool ) ) );

}




void bitInvaderView::modelChanged()
{
	bitInvader * b = castModel<bitInvader>();

	m_graph->setModel( &b->m_graph );
	m_sampleLengthKnob->setModel( &b->m_sampleLength );
	m_interpolationToggle->setModel( &b->m_interpolation );
	m_normalizeToggle->setModel( &b->m_normalize );

}




void bitInvaderView::sinWaveClicked()
{
	m_graph->model()->clearInvisible();
	m_graph->model()->setWaveToSine();
	Engine::getSong()->setModified();
}




void bitInvaderView::triangleWaveClicked()
{
	m_graph->model()->clearInvisible();
	m_graph->model()->setWaveToTriangle();
	Engine::getSong()->setModified();
}




void bitInvaderView::sawWaveClicked()
{
	m_graph->model()->clearInvisible();
	m_graph->model()->setWaveToSaw();
	Engine::getSong()->setModified();
}




void bitInvaderView::sqrWaveClicked()
{
	m_graph->model()->clearInvisible();
	m_graph->model()->setWaveToSquare();
	Engine::getSong()->setModified();
}




void bitInvaderView::noiseWaveClicked()
{
	m_graph->model()->clearInvisible();
	m_graph->model()->setWaveToNoise();
	Engine::getSong()->setModified();
}




void bitInvaderView::usrWaveClicked()
{
	QString fileName = m_graph->model()->setWaveToUser();
	if (!fileName.isEmpty())
	{
		ToolTip::add(m_usrWaveBtn, fileName);
		m_graph->model()->clearInvisible();
		Engine::getSong()->setModified();
	}
}




void bitInvaderView::smoothClicked()
{
	m_graph->model()->smooth();
	Engine::getSong()->setModified();
}




void bitInvaderView::interpolationToggled( bool value )
{
	m_graph->setGraphStyle( value ? Graph::LinearStyle : Graph::NearestStyle);
	Engine::getSong()->setModified();
}




void bitInvaderView::normalizeToggled( bool value )
{
	Engine::getSong()->setModified();
}




extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model *m, void * )
{
	return( new bitInvader( static_cast<InstrumentTrack *>( m ) ) );
}


}
