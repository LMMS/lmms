/*
 * BitInvader.cpp - instrument which uses a usereditable wavetable
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

#include <cmath>
#include <QDomElement>

#include "BitInvader.h"
#include "AudioEngine.h"
#include "base64.h"
#include "Engine.h"
#include "Graph.h"
#include "InstrumentTrack.h"
#include "Knob.h"
#include "LedCheckBox.h"
#include "NotePlayHandle.h"
#include "PixmapButton.h"
#include "Song.h"
#include "lmms_math.h"

#include "embed.h"
#include "plugin_export.h"

namespace lmms
{


static const int wavetableSize = 200;
static const float defaultNormalizationFactor = 1.0f;

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT bitinvader_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"BitInvader",
	QT_TRANSLATE_NOOP( "PluginBrowser",
				"Customizable wavetable synthesizer" ),
	"Andreas Brandmaier <andreas/at/brandmaier/dot/de>",
	0x0100,
	Plugin::Type::Instrument,
	new PluginPixmapLoader( "logo" ),
	{},
	nullptr,
} ;

}


BSynth::BSynth( float * _shape, NotePlayHandle * _nph, bool _interpolation,
				float _factor, const sample_rate_t _sample_rate ) :
	sample_index( 0 ),
	sample_realindex( 0 ),
	nph( _nph ),
	sample_rate( _sample_rate ),
	interpolation( _interpolation)
{
	sample_shape = new float[wavetableSize];
	for (int i=0; i < wavetableSize; ++i)
	{
		float buf = _shape[i] * _factor;

		/* Double check that normalization has been performed correctly,
		i.e., the absolute value of all samples is <= 1.0 if _factor
		is different to the default normalization factor. If there is
		a value > 1.0, clip the sample to 1.0 to limit the range. */
		if ((_factor != defaultNormalizationFactor) && (std::abs(buf) > 1.0f))
		{
			buf = (buf < 0) ? -1.0f : 1.0f;
		}
		sample_shape[i] = buf;
	}
}


BSynth::~BSynth()
{
	delete[] sample_shape;
}


sample_t BSynth::nextStringSample( float sample_length )
{
	auto sample_step = static_cast<float>(sample_length / (sample_rate / nph->frequency()));

	// check overflow
	while (sample_realindex >= sample_length) {
		sample_realindex -= sample_length;
	}

	const auto currentRealIndex = sample_realindex;
	const auto currentIndex = static_cast<int>(sample_realindex);
	sample_realindex += sample_step;

	if (!interpolation)
	{
		sample_index = currentIndex;
		return sample_shape[sample_index];
	}

	const auto nextIndex = currentIndex < sample_length - 1 ? currentIndex + 1 : 0;
	return std::lerp(sample_shape[currentIndex], sample_shape[nextIndex], fraction(currentRealIndex));
}	

/***********************************************************************
*
*	class BitInvader
*
*	lmms - plugin 
*
***********************************************************************/


BitInvader::BitInvader( InstrumentTrack * _instrument_track ) :
	Instrument( _instrument_track, &bitinvader_plugin_descriptor ),
	m_sampleLength(wavetableSize, 4, wavetableSize, 1, this, tr("Sample length")),
	m_graph(-1.0f, 1.0f, wavetableSize, this),
	m_interpolation(false, this, tr("Interpolation")),
	m_normalize(false, this, tr("Normalize"))
{
	m_graph.setWaveToSine();
	lengthChanged();

	connect( &m_sampleLength, SIGNAL( dataChanged() ),
			this, SLOT( lengthChanged() ), Qt::DirectConnection );

	connect( &m_graph, SIGNAL( samplesChanged( int, int ) ),
			this, SLOT( samplesChanged( int, int ) ) );
}







void BitInvader::saveSettings( QDomDocument & _doc, QDomElement & _this )
{

	// Save plugin version
	_this.setAttribute( "version", "0.1" );

	// Save sample length
	m_sampleLength.saveSettings( _doc, _this, "sampleLength" );

	// Save sample shape base64-encoded
	QString sampleString;
	base64::encode((const char *)m_graph.samples(),
		wavetableSize * sizeof(float), sampleString);
	_this.setAttribute( "sampleShape", sampleString );
	

	// save LED normalize 
	m_interpolation.saveSettings( _doc, _this, "interpolation" );
	
	// save LED 
	m_normalize.saveSettings( _doc, _this, "normalize" );
}




void BitInvader::loadSettings( const QDomElement & _this )
{
	// Clear wavetable before loading a new
	m_graph.clear();

	// Load sample length
	m_sampleLength.loadSettings( _this, "sampleLength" );

	int sampleLength = (int)m_sampleLength.value();

	// Load sample shape
	int size = 0;
	char * dst = 0;
	base64::decode( _this.attribute( "sampleShape"), &dst, &size );

	m_graph.setLength(size / sizeof(float));
	m_graph.setSamples(reinterpret_cast<float*>(dst));
	m_graph.setLength(sampleLength);
	delete[] dst;

	// Load LED normalize 
	m_interpolation.loadSettings( _this, "interpolation" );
	// Load LED 
	m_normalize.loadSettings( _this, "normalize" );

}




void BitInvader::lengthChanged()
{
	m_graph.setLength( (int) m_sampleLength.value() );

	normalize();
}




void BitInvader::samplesChanged( int _begin, int _end )
{
	normalize();
	//engine::getSongEditor()->setModified();
}




void BitInvader::normalize()
{
	// analyze
	float max = std::numeric_limits<float>::epsilon();
	const float* samples = m_graph.samples();
	for(int i=0; i < m_graph.length(); i++)
	{
		const float f = std::abs(samples[i]);
		if (f > max) { max = f; }
	}
	m_normalizeFactor = 1.0 / max;
}




QString BitInvader::nodeName() const
{
	return( bitinvader_plugin_descriptor.name );
}




void BitInvader::playNote( NotePlayHandle * _n,
						SampleFrame* _working_buffer )
{
	if (!_n->m_pluginData)
	{
		float factor = !m_normalize.value() ? defaultNormalizationFactor : m_normalizeFactor;
		_n->m_pluginData = new BSynth(
					const_cast<float*>( m_graph.samples() ),
					_n,
					m_interpolation.value(), factor,
				Engine::audioEngine()->outputSampleRate() );
	}

	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();

	auto ps = static_cast<BSynth*>(_n->m_pluginData);
	for( fpp_t frame = offset; frame < frames + offset; ++frame )
	{
		_working_buffer[frame] = SampleFrame(ps->nextStringSample(m_graph.length()));
	}

	applyRelease( _working_buffer, _n );
}




void BitInvader::deleteNotePluginData( NotePlayHandle * _n )
{
	delete static_cast<BSynth *>( _n->m_pluginData );
}




gui::PluginView * BitInvader::instantiateView( QWidget * _parent )
{
	return( new gui::BitInvaderView( this, _parent ) );
}




namespace gui
{


BitInvaderView::BitInvaderView( Instrument * _instrument,
					QWidget * _parent ) :
	InstrumentViewFixedSize( _instrument, _parent )
{
	setAutoFillBackground( true );
	QPalette pal;

	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	setPalette( pal );
	
	m_sampleLengthKnob = new Knob( KnobType::Dark28, this );
	m_sampleLengthKnob->move( 6, 201 );
	m_sampleLengthKnob->setHintText( tr( "Sample length" ), "" );

	m_graph = new Graph( this, Graph::Style::Nearest, 204, 134 );
	m_graph->move(23,59);	// 55,120 - 2px border
	m_graph->setAutoFillBackground( true );
	m_graph->setGraphColor( QColor( 255, 255, 255 ) );

	m_graph->setToolTip(tr("Draw your own waveform here "
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
	m_sinWaveBtn->setToolTip(
			tr( "Sine wave" ) );

	m_triangleWaveBtn = new PixmapButton( this, tr( "Triangle wave" ) );
	m_triangleWaveBtn->move( 131 + 14, 205 );
	m_triangleWaveBtn->setActiveGraphic(
		embed::getIconPixmap( "triangle_wave_active" ) );
	m_triangleWaveBtn->setInactiveGraphic(
		embed::getIconPixmap( "triangle_wave_inactive" ) );
	m_triangleWaveBtn->setToolTip(
			tr( "Triangle wave" ) );

	m_sawWaveBtn = new PixmapButton( this, tr( "Saw wave" ) );
	m_sawWaveBtn->move( 131 + 14*2, 205  );
	m_sawWaveBtn->setActiveGraphic( embed::getIconPixmap(
						"saw_wave_active" ) );
	m_sawWaveBtn->setInactiveGraphic( embed::getIconPixmap(
						"saw_wave_inactive" ) );
	m_sawWaveBtn->setToolTip(
			tr( "Saw wave" ) );

	m_sqrWaveBtn = new PixmapButton( this, tr( "Square wave" ) );
	m_sqrWaveBtn->move( 131 + 14*3, 205 );
	m_sqrWaveBtn->setActiveGraphic( embed::getIconPixmap(
					"square_wave_active" ) );
	m_sqrWaveBtn->setInactiveGraphic( embed::getIconPixmap(
					"square_wave_inactive" ) );
	m_sqrWaveBtn->setToolTip(
			tr( "Square wave" ) );

	m_whiteNoiseWaveBtn = new PixmapButton( this,
					tr( "White noise" ) );
	m_whiteNoiseWaveBtn->move( 131 + 14*4, 205 );
	m_whiteNoiseWaveBtn->setActiveGraphic(
		embed::getIconPixmap( "white_noise_wave_active" ) );
	m_whiteNoiseWaveBtn->setInactiveGraphic(
		embed::getIconPixmap( "white_noise_wave_inactive" ) );
	m_whiteNoiseWaveBtn->setToolTip(
			tr( "White noise" ) );

	m_usrWaveBtn = new PixmapButton( this, tr( "User-defined wave" ) );
	m_usrWaveBtn->move( 131 + 14*5, 205 );
	m_usrWaveBtn->setActiveGraphic( embed::getIconPixmap(
						"usr_wave_active" ) );
	m_usrWaveBtn->setInactiveGraphic( embed::getIconPixmap(
						"usr_wave_inactive" ) );
	m_usrWaveBtn->setToolTip(
			tr( "User-defined wave" ) );

	m_smoothBtn = new PixmapButton( this, tr( "Smooth waveform" ) );
	m_smoothBtn->move( 131 + 14*6, 205 );
	m_smoothBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
						"smooth_active" ) );
	m_smoothBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
						"smooth_inactive" ) );
	m_smoothBtn->setToolTip(
			tr( "Smooth waveform" ) );


	m_interpolationToggle = new LedCheckBox( "Interpolation", this,
							tr( "Interpolation" ), LedCheckBox::LedColor::Yellow );
	m_interpolationToggle->move( 131, 221 );


	m_normalizeToggle = new LedCheckBox( "Normalize", this,
							tr( "Normalize" ), LedCheckBox::LedColor::Green );
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




void BitInvaderView::modelChanged()
{
	auto b = castModel<BitInvader>();

	m_graph->setModel( &b->m_graph );
	m_sampleLengthKnob->setModel( &b->m_sampleLength );
	m_interpolationToggle->setModel( &b->m_interpolation );
	m_normalizeToggle->setModel( &b->m_normalize );

}




void BitInvaderView::sinWaveClicked()
{
	m_graph->model()->clearInvisible();
	m_graph->model()->setWaveToSine();
	Engine::getSong()->setModified();
}




void BitInvaderView::triangleWaveClicked()
{
	m_graph->model()->clearInvisible();
	m_graph->model()->setWaveToTriangle();
	Engine::getSong()->setModified();
}




void BitInvaderView::sawWaveClicked()
{
	m_graph->model()->clearInvisible();
	m_graph->model()->setWaveToSaw();
	Engine::getSong()->setModified();
}




void BitInvaderView::sqrWaveClicked()
{
	m_graph->model()->clearInvisible();
	m_graph->model()->setWaveToSquare();
	Engine::getSong()->setModified();
}




void BitInvaderView::noiseWaveClicked()
{
	m_graph->model()->clearInvisible();
	m_graph->model()->setWaveToNoise();
	Engine::getSong()->setModified();
}




void BitInvaderView::usrWaveClicked()
{
	QString fileName = m_graph->model()->setWaveToUser();
	if (!fileName.isEmpty())
	{
		m_usrWaveBtn->setToolTip(fileName);
		m_graph->model()->clearInvisible();
		Engine::getSong()->setModified();
	}
}




void BitInvaderView::smoothClicked()
{
	m_graph->model()->smooth();
	Engine::getSong()->setModified();
}




void BitInvaderView::interpolationToggled( bool value )
{
	m_graph->setGraphStyle( value ? Graph::Style::Linear : Graph::Style::Nearest);
	Engine::getSong()->setModified();
}




void BitInvaderView::normalizeToggled( bool value )
{
	Engine::getSong()->setModified();
}


} // namespace gui


extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model *m, void * )
{
	return( new BitInvader( static_cast<InstrumentTrack *>( m ) ) );
}


}


} // namespace lmms
