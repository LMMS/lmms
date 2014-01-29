/*
 * kicker.cpp - bassdrum-synthesizer
 *
 * Copyright (c) 2006-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtXml/QDomDocument>
#include <QtGui/QPainter>

#include "kicker.h"
#include "engine.h"
#include "InstrumentTrack.h"
#include "knob.h"
#include "NotePlayHandle.h"
#include "SweepOscillator.h"

#include "embed.cpp"


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT kicker_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Kicker",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"versatile kick- & bassdrum-synthesizer" ),
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
	m_decayModel( 120.0f, 5.0f, 1000.0f, 1.0f, this, tr( "Decay" ) ),
	m_distModel( 0.8f, 0.0f, 100.0f, 0.1f, this, tr( "Distortion" ) ),
	m_gainModel( 1.0f, 0.1f, 5.0f, 0.05f, this, tr( "Gain" ) )
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
	m_gainModel.saveSettings( _doc, _this, "gain" );
}




void kickerInstrument::loadSettings( const QDomElement & _this )
{
	m_startFreqModel.loadSettings( _this, "startfreq" );
	m_endFreqModel.loadSettings( _this, "endfreq" );
	m_decayModel.loadSettings( _this, "decay" );
	m_distModel.loadSettings( _this, "dist" );
	m_gainModel.loadSettings( _this, "gain" );
}




QString kickerInstrument::nodeName() const
{
	return kicker_plugin_descriptor.name;
}



//typedef effectLib::foldbackDistortion<> DistFX;
typedef effectLib::distortion DistFX;
typedef SweepOscillator<effectLib::monoToStereoAdaptor<DistFX> > SweepOsc;


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
							m_gainModel.value() ) );
	}
	else if( tfp > decfr && !_n->isReleased() )
	{
		_n->noteOff();
	}

	//const float freq = instrumentTrack()->frequency( _n ) / 2;
	const float fdiff = m_endFreqModel.value() - m_startFreqModel.value();
/*	const fpp_t frames = _n->isReleased() ?
		tMax( tMin<f_cnt_t>( desiredReleaseFrames() -
							_n->releaseFramesDone(),
			engine::mixer()->framesPerAudioBuffer() ), 0 )
		:
		engine::mixer()->framesPerAudioBuffer();*/
	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const float f1 = m_startFreqModel.value() + tfp * fdiff / decfr;
	const float f2 = m_startFreqModel.value() + (frames+tfp-1)*fdiff/decfr;


	SweepOsc * so = static_cast<SweepOsc *>( _n->m_pluginData );
	so->update( _working_buffer, frames, f1, f2,
				engine::mixer()->processingSampleRate() );

	if( _n->isReleased() )
	{
		const float rfd = _n->releaseFramesDone();
		const float drf = desiredReleaseFrames();
		for( fpp_t f = 0; f < frames; ++f )
		{
			const float fac = 1.0f - ( rfd+f ) / drf;
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
		setFixedSize( 37, 47 );
	}
};




kickerInstrumentView::kickerInstrumentView( Instrument * _instrument,
							QWidget * _parent ) :
	InstrumentView( _instrument, _parent )
{
	m_startFreqKnob = new kickerKnob( this );
	m_startFreqKnob->setHintText( tr( "Start frequency:" ) + " ", "Hz" );
	m_startFreqKnob->move( 12, 124 );

	m_endFreqKnob = new kickerKnob( this );
	m_endFreqKnob->setHintText( tr( "End frequency:" ) + " ", "Hz" );
	m_endFreqKnob->move( 59, 124 );

	m_decayKnob = new kickerKnob( this );
	m_decayKnob->setHintText( tr( "Decay:" ) + " ", "ms" );
	m_decayKnob->move( 107, 124 );

	m_distKnob = new kickerKnob( this );
	m_distKnob->setHintText( tr( "Distortion:" ) + " ", "" );
	m_distKnob->move( 155, 124 );

	m_gainKnob = new kickerKnob( this );
	m_gainKnob->setHintText( tr( "Gain:" ) + " ", "" );
	m_gainKnob->move( 203, 124 );

	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
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
	m_gainKnob->setModel( &k->m_gainModel );
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

