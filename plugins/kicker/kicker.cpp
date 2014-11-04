/*
 * kicker.cpp - drum synthesizer
 *
 * Copyright (c) 2006-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2014 Hannu Haahti <grejppi/at/gmail.com>
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


#include <QtXml/QDomDocument>
#include <QtGui/QPainter>

#include "kicker.h"
#include "engine.h"
#include "InstrumentTrack.h"
#include "knob.h"
#include "NotePlayHandle.h"
#include "KickerOsc.h"

#include "embed.cpp"


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT kicker_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Kicker",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"Versatile drum synthesizer" ),
	"Tobias Doerffel <tobydox/at/users.sf.net>",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
} ;

}


kickerInstrument::kickerInstrument( InstrumentTrack * _instrument_track ) :
	Instrument( _instrument_track, &kicker_plugin_descriptor ),
	m_startFreqModel( 150.0f, 5.0f, 1000.0f, 1.0f, this, tr( "Start frequency" ) ),
	m_endFreqModel( 40.0f, 5.0f, 1000.0f, 1.0f, this, tr( "End frequency" ) ),
	m_decayModel( 440.0f, 5.0f, 5000.0f, 1.0f, 5000.0f, this, tr( "Length" ) ),
	m_distModel( 0.8f, 0.0f, 100.0f, 0.1f, this, tr( "Distortion Start" ) ),
	m_distEndModel( 0.8f, 0.0f, 100.0f, 0.1f, this, tr( "Distortion End" ) ),
	m_gainModel( 1.0f, 0.1f, 5.0f, 0.05f, this, tr( "Gain" ) ),
	m_envModel( 0.163f, 0.01f, 1.0f, 0.001f, this, tr( "Envelope Slope" ) ),
	m_noiseModel( 0.0f, 0.0f, 1.0f, 0.01f, this, tr( "Noise" ) ),
	m_clickModel( 0.4f, 0.0f, 1.0f, 0.05f, this, tr( "Click" ) ),
	m_slopeModel( 0.06f, 0.001f, 1.0f, 0.001f, this, tr( "Frequency Slope" ) ),
	m_startNoteModel( true, this, tr( "Start from note" ) ),
	m_endNoteModel( false, this, tr( "End to note" ) ),
	m_versionModel( 0, 0, KICKER_PRESET_VERSION, this, "" )
{
}




kickerInstrument::~kickerInstrument()
{
}




void kickerInstrument::saveSettings( QDomDocument & _doc,
							QDomElement & _this )
{
	m_startFreqModel.saveSettings( _doc, _this, "startfreq" );
	m_endFreqModel.saveSettings( _doc, _this, "endfreq" );
	m_decayModel.saveSettings( _doc, _this, "decay" );
	m_distModel.saveSettings( _doc, _this, "dist" );
	m_distEndModel.saveSettings( _doc, _this, "distend" );
	m_gainModel.saveSettings( _doc, _this, "gain" );
	m_envModel.saveSettings( _doc, _this, "env" );
	m_noiseModel.saveSettings( _doc, _this, "noise" );
	m_clickModel.saveSettings( _doc, _this, "click" );
	m_slopeModel.saveSettings( _doc, _this, "slope" );
	m_startNoteModel.saveSettings( _doc, _this, "startnote" );
	m_endNoteModel.saveSettings( _doc, _this, "endnote" );
	m_versionModel.saveSettings( _doc, _this, "version" );
}




void kickerInstrument::loadSettings( const QDomElement & _this )
{
	m_versionModel.loadSettings( _this, "version" );

	m_startFreqModel.loadSettings( _this, "startfreq" );
	m_endFreqModel.loadSettings( _this, "endfreq" );
	m_decayModel.loadSettings( _this, "decay" );
	m_distModel.loadSettings( _this, "dist" );
	if( _this.hasAttribute( "distend" ) )
	{
		m_distEndModel.loadSettings( _this, "distend" );
	}
	else
	{
		m_distEndModel.setValue( m_distModel.value() );
	}
	m_gainModel.loadSettings( _this, "gain" );
	m_envModel.loadSettings( _this, "env" );
	m_noiseModel.loadSettings( _this, "noise" );
	m_clickModel.loadSettings( _this, "click" );
	m_slopeModel.loadSettings( _this, "slope" );
	m_startNoteModel.loadSettings( _this, "startnote" );
	if( m_versionModel.value() < 1 )
	{
		m_startNoteModel.setValue( false );
	}
	m_endNoteModel.loadSettings( _this, "endnote" );

	// Try to maintain backwards compatibility
	if( !_this.hasAttribute( "version" ) )
	{

		m_decayModel.setValue( m_decayModel.value() * 1.33f );
		m_envModel.setValue( 1.0f );
		m_slopeModel.setValue( 1.0f );
		m_clickModel.setValue( 0.0f );
	}
	m_versionModel.setValue( KICKER_PRESET_VERSION );
}




QString kickerInstrument::nodeName() const
{
	return kicker_plugin_descriptor.name;
}



typedef DspEffectLibrary::Distortion DistFX;
typedef KickerOsc<DspEffectLibrary::MonoToStereoAdaptor<DistFX> > SweepOsc;


void kickerInstrument::playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer )
{
	const float decfr = m_decayModel.value() *
		engine::mixer()->processingSampleRate() / 1000.0f;
	const f_cnt_t tfp = _n->totalFramesPlayed();

	if ( tfp == 0 )
	{
		_n->m_pluginData = new SweepOsc(
					DistFX( m_distModel.value(),
							m_gainModel.value() ),
					m_startNoteModel.value() ? _n->frequency() : m_startFreqModel.value(),
					m_endNoteModel.value() ? _n->frequency() : m_endFreqModel.value(),
					m_noiseModel.value() * m_noiseModel.value(),
					m_clickModel.value() * 0.25f,
					m_slopeModel.value(),
					m_envModel.value(),
					m_distModel.value(),
					m_distEndModel.value(),
					decfr );
	}
	else if( tfp > decfr && !_n->isReleased() )
	{
		_n->noteOff();
	}

	const fpp_t frames = _n->framesLeftForCurrentPeriod();

	SweepOsc * so = static_cast<SweepOsc *>( _n->m_pluginData );
	so->update( _working_buffer, frames, engine::mixer()->processingSampleRate() );

	if( _n->isReleased() )
	{
		const float done = _n->releaseFramesDone();
		const float desired = desiredReleaseFrames();
		for( fpp_t f = 0; f < frames; ++f )
		{
			const float fac = ( done+f < desired ) ? ( 1.0f - ( ( done+f ) / desired ) ) : 0;
			_working_buffer[f][0] *= fac;
			_working_buffer[f][1] *= fac;
		}
	}

	instrumentTrack()->processAudioBuffer( _working_buffer, frames, _n );
}




void kickerInstrument::deleteNotePluginData( NotePlayHandle * _n )
{
	delete static_cast<SweepOsc *>( _n->m_pluginData );
}




PluginView * kickerInstrument::instantiateView( QWidget * _parent )
{
	return new kickerInstrumentView( this, _parent );
}




class kickerKnob : public knob
{
public:
	kickerKnob( QWidget * _parent ) :
			knob( knobStyled, _parent )
	{
		setFixedSize( 29, 29 );
		setObjectName( "smallKnob" );
	}
};


class kickerEnvKnob : public TempoSyncKnob
{
public:
	kickerEnvKnob( QWidget * _parent ) :
			TempoSyncKnob( knobStyled, _parent )
	{
		setFixedSize( 29, 29 );
		setObjectName( "smallKnob" );
	}
};


class kickerLargeKnob : public knob
{
public:
	kickerLargeKnob( QWidget * _parent ) :
			knob( knobStyled, _parent )
	{
		setFixedSize( 34, 34 );
		setObjectName( "largeKnob" );
	}
};




kickerInstrumentView::kickerInstrumentView( Instrument * _instrument,
							QWidget * _parent ) :
	InstrumentView( _instrument, _parent )
{
	const int ROW1 = 14;
	const int ROW2 = ROW1 + 56;
	const int ROW3 = ROW2 + 56;
	const int LED_ROW = 63;
	const int COL1 = 14;
	const int COL2 = COL1 + 56;
	const int COL3 = COL2 + 56;
	const int COL4 = COL3 + 41;
	const int COL5 = COL4 + 41;
	const int END_COL = COL1 + 48;
	
	m_startFreqKnob = new kickerLargeKnob( this );
	m_startFreqKnob->setHintText( tr( "Start frequency:" ) + " ", "Hz" );
	m_startFreqKnob->move( COL1, ROW1 );

	m_endFreqKnob = new kickerLargeKnob( this );
	m_endFreqKnob->setHintText( tr( "End frequency:" ) + " ", "Hz" );
	m_endFreqKnob->move( END_COL, ROW1 );

	m_slopeKnob = new kickerKnob( this );
	m_slopeKnob->setHintText( tr( "Frequency Slope:" ) + " ", "" );
	m_slopeKnob->move( COL3, ROW1 );

	m_gainKnob = new kickerKnob( this );
	m_gainKnob->setHintText( tr( "Gain:" ) + " ", "" );
	m_gainKnob->move( COL1, ROW3 );

	m_decayKnob = new kickerEnvKnob( this );
	m_decayKnob->setHintText( tr( "Envelope Length:" ) + " ", "ms" );
	m_decayKnob->move( COL2, ROW3 );

	m_envKnob = new kickerKnob( this );
	m_envKnob->setHintText( tr( "Envelope Slope:" ) + " ", "" );
	m_envKnob->move( COL3, ROW3 );

	m_clickKnob = new kickerKnob( this );
	m_clickKnob->setHintText( tr( "Click:" ) + " ", "" );
	m_clickKnob->move( COL5, ROW1 );

	m_noiseKnob = new kickerKnob( this );
	m_noiseKnob->setHintText( tr( "Noise:" ) + " ", "" );
	m_noiseKnob->move( COL5, ROW3 );

	m_distKnob = new kickerKnob( this );
	m_distKnob->setHintText( tr( "Distortion Start:" ) + " ", "" );
	m_distKnob->move( COL4, ROW2 );

	m_distEndKnob = new kickerKnob( this );
	m_distEndKnob->setHintText( tr( "Distortion End:" ) + " ", "" );
	m_distEndKnob->move( COL5, ROW2 );

	m_startNoteToggle = new ledCheckBox( "", this, "", ledCheckBox::Green );
	m_startNoteToggle->move( COL1 + 8, LED_ROW );

	m_endNoteToggle = new ledCheckBox( "", this, "", ledCheckBox::Green );
	m_endNoteToggle->move( END_COL + 8, LED_ROW );

	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
}




kickerInstrumentView::~kickerInstrumentView()
{
}




void kickerInstrumentView::modelChanged()
{
	kickerInstrument * k = castModel<kickerInstrument>();
	m_startFreqKnob->setModel( &k->m_startFreqModel );
	m_endFreqKnob->setModel( &k->m_endFreqModel );
	m_decayKnob->setModel( &k->m_decayModel );
	m_distKnob->setModel( &k->m_distModel );
	m_distEndKnob->setModel( &k->m_distEndModel );
	m_gainKnob->setModel( &k->m_gainModel );
	m_envKnob->setModel( &k->m_envModel );
	m_noiseKnob->setModel( &k->m_noiseModel );
	m_clickKnob->setModel( &k->m_clickModel );
	m_slopeKnob->setModel( &k->m_slopeModel );
	m_startNoteToggle->setModel( &k->m_startNoteModel );
	m_endNoteToggle->setModel( &k->m_endNoteModel );
}





extern "C"
{

// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model *, void * _data )
{
	return new kickerInstrument( static_cast<InstrumentTrack *>( _data ) );
}


}


#include "moc_kicker.cxx"

