/*
 * FreeBoy.cpp - GameBoy papu based instrument
 *
 * Copyright (c) 2008 Attila Herman <attila589/at/gmail.com>
 *				Csaba Hruska <csaba.hruska/at/gmail.com>
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

#include <QPainter>
#include <QDomElement>
#include "FreeBoy.h"
#include "Gb_Apu_Buffer.h"
#include "Multi_Buffer.h"
#include "base64.h"
#include "InstrumentTrack.h"
#include "Knob.h"
#include "Mixer.h"
#include "NotePlayHandle.h"
#include "PixmapButton.h"
#include "ToolTip.h"
#include "Engine.h"
#include "Graph.h"

#include "embed.h"

#include "plugin_export.h"

const blip_time_t FRAME_LENGTH = 70224;
const long CLOCK_RATE = 4194304;

extern "C"
{
Plugin::Descriptor PLUGIN_EXPORT freeboy_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"FreeBoy",
	QT_TRANSLATE_NOOP( "PluginBrowser", "Emulation of GameBoy (TM) APU" ),

	"Attila Herman <attila589/at/gmail.com>"
	"Csaba Hruska <csaba.hruska/at/gmail.com>",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader( "logo" ),
	NULL
} ;

}


FreeBoyInstrument::FreeBoyInstrument( InstrumentTrack * _instrument_track ) :
	Instrument( _instrument_track, &freeboy_plugin_descriptor ),

	m_ch1SweepTimeModel( 4.0f, 0.0f, 7.0f, 1.0f, this, tr( "Sweep time" ) ),
	m_ch1SweepDirModel( false, this, tr( "Sweep direction" ) ),
	m_ch1SweepRtShiftModel( 4.0f, 0.0f, 7.0f, 1.0f, this,
						tr( "Sweep rate shift amount" ) ),
	m_ch1WavePatternDutyModel( 2.0f, 0.0f, 3.0f, 1.0f, this,
						tr( "Wave pattern duty cycle" ) ),
	m_ch1VolumeModel( 15.0f, 0.0f, 15.0f, 1.0f, this,
						tr( "Channel 1 volume" ) ),
	m_ch1VolSweepDirModel( false, this,
						tr( "Volume sweep direction" ) ),
	m_ch1SweepStepLengthModel( 0.0f, 0.0f, 7.0f, 1.0f, this,
						tr( "Length of each step in sweep" ) ),

	m_ch2WavePatternDutyModel( 2.0f, 0.0f, 3.0f, 1.0f, this,
						tr( "Wave pattern duty cycle" ) ),
	m_ch2VolumeModel( 15.0f, 0.0f, 15.0f, 1.0f, this,
						tr( "Channel 2 volume" ) ),
	m_ch2VolSweepDirModel( false, this,
						tr( "Volume sweep direction" ) ),
	m_ch2SweepStepLengthModel( 0.0f, 0.0f, 7.0f, 1.0f, this,
						tr( "Length of each step in sweep" ) ),

	//m_ch3OnModel( true, this, tr( "Channel 3 Master on/off" ) ),
	m_ch3VolumeModel( 3.0f, 0.0f, 3.0f, 1.0f, this,
						tr( "Channel 3 volume" ) ),

	m_ch4VolumeModel( 15.0f, 0.0f, 15.0f, 1.0f, this,
						tr( "Channel 4 volume" ) ),
	m_ch4VolSweepDirModel( false, this,
						tr( "Volume sweep direction" ) ),
	m_ch4SweepStepLengthModel( 0.0f, 0.0f, 7.0f, 1.0f, this,
						tr( "Length of each step in sweep" ) ),
	m_ch4ShiftRegWidthModel( false, this,
						tr( "Shift Register width" ) ),

	m_so1VolumeModel( 7.0f, 0.0f, 7.0f, 1.0f, this, tr( "Right output level") ),
	m_so2VolumeModel( 7.0f, 0.0f, 7.0f, 1.0f, this, tr( "Left output level" ) ),
	m_ch1So1Model( true, this, tr( "Channel 1 to SO2 (Left)" ) ),
	m_ch2So1Model( true, this, tr( "Channel 2 to SO2 (Left)" ) ),
	m_ch3So1Model( true, this, tr( "Channel 3 to SO2 (Left)" ) ),
	m_ch4So1Model( false, this, tr( "Channel 4 to SO2 (Left)" ) ),
	m_ch1So2Model( true, this, tr( "Channel 1 to SO1 (Right)" ) ),
	m_ch2So2Model( true, this, tr( "Channel 2 to SO1 (Right)" ) ),
	m_ch3So2Model( true, this, tr( "Channel 3 to SO1 (Right)" ) ),
	m_ch4So2Model( false, this, tr( "Channel 4 to SO1 (Right)" ) ),
	m_trebleModel( -20.0f, -100.0f, 200.0f, 1.0f, this, tr( "Treble" ) ),
	m_bassModel( 461.0f, -1.0f, 600.0f, 1.0f, this, tr( "Bass" ) ),

	m_graphModel( 0, 15, 32, this, false, 1 ),

	m_time(0)
{
}


FreeBoyInstrument::~FreeBoyInstrument()
{
}


void FreeBoyInstrument::saveSettings( QDomDocument & _doc,
							QDomElement & _this )
{
	m_ch1SweepTimeModel.saveSettings( _doc, _this, "st" );
	m_ch1SweepDirModel.saveSettings( _doc, _this, "sd" );
	m_ch1SweepRtShiftModel.saveSettings( _doc, _this, "srs" );
	m_ch1WavePatternDutyModel.saveSettings( _doc, _this, "ch1wpd" );
	m_ch1VolumeModel.saveSettings( _doc, _this, "ch1vol" );
	m_ch1VolSweepDirModel.saveSettings( _doc, _this, "ch1vsd" );
	m_ch1SweepStepLengthModel.saveSettings( _doc, _this, "ch1ssl" );

	m_ch2WavePatternDutyModel.saveSettings( _doc, _this, "ch2wpd" );
	m_ch2VolumeModel.saveSettings( _doc, _this, "ch2vol" );
	m_ch2VolSweepDirModel.saveSettings( _doc, _this, "ch2vsd" );
	m_ch2SweepStepLengthModel.saveSettings( _doc, _this, "ch2ssl" );

	//m_ch3OnModel.saveSettings( _doc, _this, "ch3on" );
	m_ch3VolumeModel.saveSettings( _doc, _this, "ch3vol" );

	m_ch4VolumeModel.saveSettings( _doc, _this, "ch4vol" );
	m_ch4VolSweepDirModel.saveSettings( _doc, _this, "ch4vsd" );
	m_ch4SweepStepLengthModel.saveSettings( _doc, _this, "ch4ssl" );
	m_ch4ShiftRegWidthModel.saveSettings( _doc, _this, "srw" );

	m_so1VolumeModel.saveSettings( _doc, _this, "so1vol" );
	m_so2VolumeModel.saveSettings( _doc, _this, "so2vol" );
	m_ch1So1Model.saveSettings( _doc, _this, "ch1so2" );
	m_ch2So1Model.saveSettings( _doc, _this, "ch2so2" );
	m_ch3So1Model.saveSettings( _doc, _this, "ch3so2" );
	m_ch4So1Model.saveSettings( _doc, _this, "ch4so2" );
	m_ch1So2Model.saveSettings( _doc, _this, "ch1so1" );
	m_ch2So2Model.saveSettings( _doc, _this, "ch2so1" );
	m_ch3So2Model.saveSettings( _doc, _this, "ch3so1" );
	m_ch4So2Model.saveSettings( _doc, _this, "ch4so1" );
	m_trebleModel.saveSettings( _doc, _this, "Treble" );
	m_bassModel.saveSettings( _doc, _this, "Bass" );

	QString sampleString;
	base64::encode( (const char *)m_graphModel.samples(),
		m_graphModel.length() * sizeof(float), sampleString );
	_this.setAttribute( "sampleShape", sampleString );
}

void FreeBoyInstrument::loadSettings( const QDomElement & _this )
{
	m_ch1SweepTimeModel.loadSettings( _this, "st" );
	m_ch1SweepDirModel.loadSettings( _this, "sd" );
	m_ch1SweepRtShiftModel.loadSettings( _this, "srs" );
	m_ch1WavePatternDutyModel.loadSettings( _this, "ch1wpd" );
	m_ch1VolumeModel.loadSettings( _this, "ch1vol" );
	m_ch1VolSweepDirModel.loadSettings( _this, "ch1vsd" );
	m_ch1SweepStepLengthModel.loadSettings( _this, "ch1ssl" );

	m_ch2WavePatternDutyModel.loadSettings( _this, "ch2wpd" );
	m_ch2VolumeModel.loadSettings( _this, "ch2vol" );
	m_ch2VolSweepDirModel.loadSettings( _this, "ch2vsd" );
	m_ch2SweepStepLengthModel.loadSettings( _this, "ch2ssl" );

	//m_ch3OnModel.loadSettings( _this, "ch3on" );
	m_ch3VolumeModel.loadSettings( _this, "ch3vol" );

	m_ch4VolumeModel.loadSettings( _this, "ch4vol" );
	m_ch4VolSweepDirModel.loadSettings( _this, "ch4vsd" );
	m_ch4SweepStepLengthModel.loadSettings( _this, "ch4ssl" );
	m_ch4ShiftRegWidthModel.loadSettings( _this, "srw" );

	m_so1VolumeModel.loadSettings( _this, "so1vol" );
	m_so2VolumeModel.loadSettings( _this, "so2vol" );
	m_ch1So1Model.loadSettings( _this, "ch1so2" );
	m_ch2So1Model.loadSettings( _this, "ch2so2" );
	m_ch3So1Model.loadSettings( _this, "ch3so2" );
	m_ch4So1Model.loadSettings( _this, "ch4so2" );
	m_ch1So2Model.loadSettings( _this, "ch1so1" );
	m_ch2So2Model.loadSettings( _this, "ch2so1" );
	m_ch3So2Model.loadSettings( _this, "ch3so1" );
	m_ch4So2Model.loadSettings( _this, "ch4so1" );
	m_trebleModel.loadSettings( _this, "Treble" );
	m_bassModel.loadSettings( _this, "Bass" );

	int size = 0;
	char * dst = 0;
	base64::decode( _this.attribute( "sampleShape"), &dst, &size );
	m_graphModel.setSamples( (float*) dst );
}

QString FreeBoyInstrument::nodeName() const
{
	return( freeboy_plugin_descriptor.name );
}




/*f_cnt_t FreeBoyInstrument::desiredReleaseFrames() const
{
	const float samplerate = Engine::mixer()->processingSampleRate();
	int maxrel = 0;
	for( int i = 0 ; i < 3 ; ++i )
	{
		if( maxrel < m_voice[i]->m_releaseModel.value() )
			maxrel = m_voice[i]->m_releaseModel.value();
	}

	return f_cnt_t( float(relTime[maxrel])*samplerate/1000.0 );
}*/

f_cnt_t FreeBoyInstrument::desiredReleaseFrames() const
{
	return f_cnt_t( 1000 );
}



void FreeBoyInstrument::playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer )
{
	const f_cnt_t tfp = _n->totalFramesPlayed();
	const int samplerate = Engine::mixer()->processingSampleRate();
	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();

	int data = 0;
	int freq = _n->frequency();

	if ( tfp == 0 )
	{
		Gb_Apu_Buffer *papu = new Gb_Apu_Buffer();
		papu->set_sample_rate( samplerate, CLOCK_RATE );

		// Master sound circuitry power control
		papu->write_register( fakeClock(),  0xff26, 0x80 );

		data = m_ch1VolumeModel.value();
		data = data<<1;
		data += m_ch1VolSweepDirModel.value();
		data = data<<3;
		data += m_ch1SweepStepLengthModel.value();
		papu->write_register( fakeClock(),  0xff12, data );

		data = m_ch2VolumeModel.value();
		data = data<<1;
		data += m_ch2VolSweepDirModel.value();
		data = data<<3;
		data += m_ch2SweepStepLengthModel.value();
		papu->write_register( fakeClock(),  0xff17, data );

		//channel 4 - noise
		data = m_ch4VolumeModel.value();
		data = data<<1;
		data += m_ch4VolSweepDirModel.value();
		data = data<<3;
		data += m_ch4SweepStepLengthModel.value();
		papu->write_register( fakeClock(),  0xff21, data );

		_n->m_pluginData = papu;
	}

	Gb_Apu_Buffer *papu = static_cast<Gb_Apu_Buffer *>( _n->m_pluginData );

	papu->treble_eq( m_trebleModel.value() );
	papu->bass_freq( m_bassModel.value() );

	//channel 1 - square
	data = m_ch1SweepTimeModel.value();
	data = data<<1;
	data += m_ch1SweepDirModel.value();
	data = data << 3;
	data += m_ch1SweepRtShiftModel.value();
	papu->write_register( fakeClock(),  0xff10, data );

	data = m_ch1WavePatternDutyModel.value();
	data = data<<6;
	papu->write_register( fakeClock(),  0xff11, data );


	//channel 2 - square
	data = m_ch2WavePatternDutyModel.value();
	data = data<<6;
	papu->write_register( fakeClock(),  0xff16, data );


	//channel 3 - wave
	//data = m_ch3OnModel.value()?128:0;
	data = 128;
	papu->write_register( fakeClock(),  0xff1a, data );

	int ch3voldata[4] = { 0, 3, 2, 1 };
	data = ch3voldata[(int)m_ch3VolumeModel.value()];
	data = data<<5;
	papu->write_register( fakeClock(),  0xff1c, data );


	//controls
	data = m_so1VolumeModel.value();
	data = data<<4;
	data += m_so2VolumeModel.value();
	papu->write_register( fakeClock(),  0xff24, data );

	data = m_ch4So2Model.value()?128:0;
	data += m_ch3So2Model.value()?64:0;
	data += m_ch2So2Model.value()?32:0;
	data += m_ch1So2Model.value()?16:0;
	data += m_ch4So1Model.value()?8:0;
	data += m_ch3So1Model.value()?4:0;
	data += m_ch2So1Model.value()?2:0;
	data += m_ch1So1Model.value()?1:0;
	papu->write_register( fakeClock(),  0xff25, data );

	const float * wpm = m_graphModel.samples();

	for( char i=0; i<16; i++ )
	{
		data = (int)floor(wpm[i*2]) << 4;
		data += (int)floor(wpm[i*2+1]);
		papu->write_register( fakeClock(),  0xff30 + i, data );
	}

	if( ( freq >= 65 ) && ( freq <=4000 ) )
	{
		int initflag = (tfp==0)?128:0;
		// Hz = 4194304 / ( ( 2048 - ( 11-bit-freq ) ) << 5 )
		data = 2048 - ( ( 4194304 / freq )>>5 );
		if( tfp==0 )
		{
			papu->write_register( fakeClock(),  0xff13, data & 0xff );
			papu->write_register( fakeClock(),  0xff14, (data>>8) | initflag );
		}
		papu->write_register( fakeClock(),  0xff18, data & 0xff );
		papu->write_register( fakeClock(),  0xff19, (data>>8) | initflag );
		papu->write_register( fakeClock(),  0xff1d, data & 0xff );
		papu->write_register( fakeClock(),  0xff1e, (data>>8) | initflag );
	}

	if( tfp == 0 )
	{
		//PRNG Frequency = (1048576 Hz / (ratio + 1)) / 2 ^ (shiftclockfreq + 1)
		char sopt=0;
		char ropt=1;
		float fopt = 524288.0 / ( ropt * pow( 2.0, sopt + 1.0 ) );
		float f;
		for ( char s=0; s<16; s++ )
		for ( char r=0; r<8; r++ ) {
			f = 524288.0 / ( r * pow( 2.0, s + 1.0 ) );
			if( fabs( freq-fopt ) > fabs( freq-f ) ) {
				fopt = f;
				ropt = r;
				sopt = s;
			}
		}
		data = sopt;
		data = data << 1;
		data += m_ch4ShiftRegWidthModel.value();
		data = data << 3;
		data += ropt;
		papu->write_register( fakeClock(),  0xff22, data );

		//channel 4 init
		papu->write_register( fakeClock(),  0xff23, 128 );
	}

	int const buf_size = 2048;
	int framesleft = frames;
	int datalen = 0;
	blip_sample_t buf [buf_size*2];
	while( framesleft > 0 )
	{
		int avail = papu->samples_avail();
		if( avail <= 0 )
		{
			m_time = 0;
			papu->end_frame(FRAME_LENGTH);
			avail = papu->samples_avail();
		}
		datalen = framesleft>avail?avail:framesleft;
		datalen = datalen>buf_size?buf_size:datalen;

		long count = papu->read_samples( buf, datalen*2)/2;

		for( fpp_t frame = 0; frame < count; ++frame )
		{
			for( ch_cnt_t ch = 0; ch < DEFAULT_CHANNELS; ++ch )
			{
				sample_t s = float(buf[frame*2+ch])/32768.0;
				_working_buffer[frames-framesleft+frame+offset][ch] = s;
			}
		}
		framesleft -= count;
	}
	instrumentTrack()->processAudioBuffer( _working_buffer, frames + offset, _n );
}



void FreeBoyInstrument::deleteNotePluginData( NotePlayHandle * _n )
{
	delete static_cast<Gb_Apu_Buffer *>( _n->m_pluginData );
}




PluginView * FreeBoyInstrument::instantiateView( QWidget * _parent )
{
	return( new FreeBoyInstrumentView( this, _parent ) );
}


class FreeBoyKnob : public Knob
{
public:
	FreeBoyKnob( QWidget * _parent ) :
			Knob( knobStyled, _parent )
	{
		setFixedSize( 30, 30 );
		setCenterPointX( 15.0 );
		setCenterPointY( 15.0 );
		setInnerRadius( 8 );
		setOuterRadius( 13 );
		setTotalAngle( 270.0 );
		setLineWidth( 1 );
		setOuterColor( QColor( 0xF1, 0xFF, 0x93 ) );
	}
};



FreeBoyInstrumentView::FreeBoyInstrumentView( Instrument * _instrument,
							QWidget * _parent ) :
	InstrumentViewFixedSize( _instrument, _parent )
{

	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );

	m_ch1SweepTimeKnob = new FreeBoyKnob( this );
	m_ch1SweepTimeKnob->setHintText( tr( "Sweep time:" ), "" );
	m_ch1SweepTimeKnob->move( 5 + 4*32, 106 );
	ToolTip::add( m_ch1SweepTimeKnob, tr( "Sweep time" ) );

	m_ch1SweepRtShiftKnob = new FreeBoyKnob( this );
	m_ch1SweepRtShiftKnob->setHintText( tr( "Sweep rate shift amount:" )
										, "" );
	m_ch1SweepRtShiftKnob->move( 5 + 3*32, 106 );
	ToolTip::add( m_ch1SweepRtShiftKnob, tr( "Sweep rate shift amount" ) );

	m_ch1WavePatternDutyKnob = new FreeBoyKnob( this );
	m_ch1WavePatternDutyKnob->setHintText( tr( "Wave pattern duty cycle:" )
									, "" );
	m_ch1WavePatternDutyKnob->move( 5 + 2*32, 106 );
	ToolTip::add( m_ch1WavePatternDutyKnob, tr( "Wave pattern duty cycle" ) );

	m_ch1VolumeKnob = new FreeBoyKnob( this );
	m_ch1VolumeKnob->setHintText( tr( "Square channel 1 volume:" )
								, "" );
	m_ch1VolumeKnob->move( 5, 106 );
	ToolTip::add( m_ch1VolumeKnob, tr( "Square channel 1 volume" ) );

	m_ch1SweepStepLengthKnob = new FreeBoyKnob( this );
	m_ch1SweepStepLengthKnob->setHintText( tr( "Length of each step in sweep:" )
									, "" );
	m_ch1SweepStepLengthKnob->move( 5 + 32, 106 );
	ToolTip::add( m_ch1SweepStepLengthKnob, tr( "Length of each step in sweep" ) );



	m_ch2WavePatternDutyKnob = new FreeBoyKnob( this );
	m_ch2WavePatternDutyKnob->setHintText( tr( "Wave pattern duty cycle:" )
									, "" );
	m_ch2WavePatternDutyKnob->move( 5 + 2*32, 155 );
	ToolTip::add( m_ch2WavePatternDutyKnob, tr( "Wave pattern duty cycle" ) );

	m_ch2VolumeKnob = new FreeBoyKnob( this );
	m_ch2VolumeKnob->setHintText( tr( "Square channel 2 volume:" )
							, "" );
	m_ch2VolumeKnob->move( 5, 155 );
	ToolTip::add( m_ch2VolumeKnob, tr( "Square channel 2 volume" ) );

	m_ch2SweepStepLengthKnob = new FreeBoyKnob( this );
	m_ch2SweepStepLengthKnob->setHintText( tr( "Length of each step in sweep:" )
									, "" );
	m_ch2SweepStepLengthKnob->move( 5 + 32, 155 );
	ToolTip::add( m_ch2SweepStepLengthKnob, tr( "Length of each step in sweep" ) );



	m_ch3VolumeKnob = new FreeBoyKnob( this );
	m_ch3VolumeKnob->setHintText( tr( "Wave pattern channel volume:" ), "" );
	m_ch3VolumeKnob->move( 5, 204 );
	ToolTip::add( m_ch3VolumeKnob, tr( "Wave pattern channel volume" ) );



	m_ch4VolumeKnob = new FreeBoyKnob( this );
	m_ch4VolumeKnob->setHintText( tr( "Noise channel volume:" ), "" );
	m_ch4VolumeKnob->move( 144, 155 );
	ToolTip::add( m_ch4VolumeKnob, tr( "Noise channel volume" ) );

	m_ch4SweepStepLengthKnob = new FreeBoyKnob( this );
	m_ch4SweepStepLengthKnob->setHintText( tr( "Length of each step in sweep:" )
									, "" );
	m_ch4SweepStepLengthKnob->move( 144 + 32, 155 );
	ToolTip::add( m_ch4SweepStepLengthKnob, tr( "Length of each step in sweep" ) );



	m_so1VolumeKnob = new FreeBoyKnob( this );
	m_so1VolumeKnob->setHintText( tr( "SO1 volume (Right):" ), "" );
	m_so1VolumeKnob->move( 5, 58 );
	ToolTip::add( m_so1VolumeKnob, tr( "SO1 volume (Right)" ) );

	m_so2VolumeKnob = new FreeBoyKnob( this );
	m_so2VolumeKnob->setHintText( tr( "SO2 volume (Left):" ), "" );
	m_so2VolumeKnob->move( 5 + 32, 58 );
	ToolTip::add( m_so2VolumeKnob, tr( "SO2 volume (Left)" ) );

	m_trebleKnob = new FreeBoyKnob( this );
	m_trebleKnob->setHintText( tr( "Treble:" ), "" );
	m_trebleKnob->move( 5 + 2*32, 58 );
	ToolTip::add( m_trebleKnob, tr( "Treble" ) );

	m_bassKnob = new FreeBoyKnob( this );
	m_bassKnob->setHintText( tr( "Bass:" ), "" );
	m_bassKnob->move( 5 + 3*32, 58 );
	ToolTip::add( m_bassKnob, tr( "Bass" ) );

	m_ch1SweepDirButton = new PixmapButton( this, NULL );
	m_ch1SweepDirButton->setCheckable( true );
	m_ch1SweepDirButton->move( 167, 108 );
	m_ch1SweepDirButton->setActiveGraphic(
							PLUGIN_NAME::getIconPixmap( "btn_down" ) );
	m_ch1SweepDirButton->setInactiveGraphic(
							PLUGIN_NAME::getIconPixmap( "btn_up" ) );
	ToolTip::add( m_ch1SweepDirButton, tr( "Sweep direction" ) );

	m_ch1VolSweepDirButton = new PixmapButton( this, NULL );
	m_ch1VolSweepDirButton->setCheckable( true );
	m_ch1VolSweepDirButton->move( 207, 108 );
	m_ch1VolSweepDirButton->setActiveGraphic(
								PLUGIN_NAME::getIconPixmap( "btn_up" ) );
	m_ch1VolSweepDirButton->setInactiveGraphic(
								PLUGIN_NAME::getIconPixmap( "btn_down" ) );
	ToolTip::add( m_ch1VolSweepDirButton, tr( "Volume sweep direction" ) );



	m_ch2VolSweepDirButton = new PixmapButton( this,
										tr( "Volume sweep direction" ) );
	m_ch2VolSweepDirButton->setCheckable( true );
	m_ch2VolSweepDirButton->move( 102, 156 );
	m_ch2VolSweepDirButton->setActiveGraphic(
								PLUGIN_NAME::getIconPixmap( "btn_up" ) );
	m_ch2VolSweepDirButton->setInactiveGraphic(
								PLUGIN_NAME::getIconPixmap( "btn_down" ) );
	ToolTip::add( m_ch2VolSweepDirButton, tr( "Volume sweep direction" ) );

	//m_ch3OnButton = new PixmapButton( this, NULL );
	//m_ch3OnButton->move( 176, 53 );

	m_ch4VolSweepDirButton = new PixmapButton( this,
										tr( "Volume sweep direction" ) );
	m_ch4VolSweepDirButton->setCheckable( true );
	m_ch4VolSweepDirButton->move( 207, 157 );
	m_ch4VolSweepDirButton->setActiveGraphic(
								PLUGIN_NAME::getIconPixmap( "btn_up" ) );
	m_ch4VolSweepDirButton->setInactiveGraphic(
								PLUGIN_NAME::getIconPixmap( "btn_down" ) );
	ToolTip::add( m_ch4VolSweepDirButton, tr( "Volume sweep direction" ) );

	m_ch4ShiftRegWidthButton = new PixmapButton( this, NULL );
	m_ch4ShiftRegWidthButton->setCheckable( true );
	m_ch4ShiftRegWidthButton->move( 207, 171 );
	m_ch4ShiftRegWidthButton->setActiveGraphic(
									PLUGIN_NAME::getIconPixmap( "btn_7" ) );
	m_ch4ShiftRegWidthButton->setInactiveGraphic(
									PLUGIN_NAME::getIconPixmap( "btn_15" ) );
	ToolTip::add( m_ch4ShiftRegWidthButton, tr( "Shift register width" ) );




	m_ch1So1Button = new PixmapButton( this, NULL );
	m_ch1So1Button->setCheckable( true );
	m_ch1So1Button->move( 208, 51 );
	m_ch1So1Button->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "btn_on" ) );
	m_ch1So1Button->setInactiveGraphic( PLUGIN_NAME::getIconPixmap("btn_off") );
	ToolTip::add( m_ch1So1Button, tr( "Channel 1 to SO1 (Right)" ) );

	m_ch2So1Button = new PixmapButton( this, NULL );
	m_ch2So1Button->setCheckable( true );
	m_ch2So1Button->move( 208, 51 + 12 );
	m_ch2So1Button->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "btn_on" ) );
	m_ch2So1Button->setInactiveGraphic( PLUGIN_NAME::getIconPixmap("btn_off") );
	ToolTip::add( m_ch2So1Button, tr( "Channel 2 to SO1 (Right)" ) );

	m_ch3So1Button = new PixmapButton( this, NULL );
	m_ch3So1Button->setCheckable( true );
	m_ch3So1Button->move( 208, 51 + 2*12 );
	m_ch3So1Button->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "btn_on" ) );
	m_ch3So1Button->setInactiveGraphic( PLUGIN_NAME::getIconPixmap("btn_off") );
	ToolTip::add( m_ch3So1Button, tr( "Channel 3 to SO1 (Right)" ) );

	m_ch4So1Button = new PixmapButton( this, NULL );
	m_ch4So1Button->setCheckable( true );
	m_ch4So1Button->setChecked( false );
	m_ch4So1Button->move( 208, 51 + 3*12 );
	m_ch4So1Button->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "btn_on" ) );
	m_ch4So1Button->setInactiveGraphic( PLUGIN_NAME::getIconPixmap("btn_off") );
	ToolTip::add( m_ch4So1Button, tr( "Channel 4 to SO1 (Right)" ) );

	m_ch1So2Button = new PixmapButton( this, NULL );
	m_ch1So2Button->setCheckable( true );
	m_ch1So2Button->move( 148, 51 );
	m_ch1So2Button->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "btn_on" ) );
	m_ch1So2Button->setInactiveGraphic( PLUGIN_NAME::getIconPixmap("btn_off") );
	ToolTip::add( m_ch1So2Button, tr( "Channel 1 to SO2 (Left)" ) );

	m_ch2So2Button = new PixmapButton( this, NULL );
	m_ch2So2Button->setCheckable( true );
	m_ch2So2Button->move( 148, 51 + 12 );
	m_ch2So2Button->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "btn_on" ) );
	m_ch2So2Button->setInactiveGraphic( PLUGIN_NAME::getIconPixmap("btn_off") );
	ToolTip::add( m_ch2So2Button, tr( "Channel 2 to SO2 (Left)" ) );

	m_ch3So2Button = new PixmapButton( this, NULL );
	m_ch3So2Button->setCheckable( true );
	m_ch3So2Button->move( 148, 51 + 2*12 );
	m_ch3So2Button->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "btn_on" ) );
	m_ch3So2Button->setInactiveGraphic( PLUGIN_NAME::getIconPixmap("btn_off") );
	ToolTip::add( m_ch3So2Button, tr( "Channel 3 to SO2 (Left)" ) );

	m_ch4So2Button = new PixmapButton( this, NULL );
	m_ch4So2Button->setCheckable( true );
	m_ch4So2Button->setChecked( false );
	m_ch4So2Button->move( 148, 51 + 3*12 );
	m_ch4So2Button->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "btn_on" ) );
	m_ch4So2Button->setInactiveGraphic( PLUGIN_NAME::getIconPixmap("btn_off") );
	ToolTip::add( m_ch4So2Button, tr( "Channel 4 to SO2 (Left)" ) );


	m_graph = new Graph( this );
	m_graph->setGraphStyle( Graph::NearestStyle );
	m_graph->setGraphColor( QColor(0x4E, 0x83, 0x2B) );
	m_graph->move( 37, 199 );
	m_graph->resize(208, 47);
	ToolTip::add( m_graph, tr( "Wave pattern graph" ) );
}


FreeBoyInstrumentView::~FreeBoyInstrumentView()
{
}


void FreeBoyInstrumentView::modelChanged()
{
	FreeBoyInstrument * p = castModel<FreeBoyInstrument>();

	m_ch1SweepTimeKnob->setModel( &p->m_ch1SweepTimeModel );
	m_ch1SweepDirButton->setModel( &p->m_ch1SweepDirModel );
	m_ch1SweepRtShiftKnob->setModel( &p->m_ch1SweepRtShiftModel );
	m_ch1WavePatternDutyKnob->setModel( &p->m_ch1WavePatternDutyModel );
	m_ch1VolumeKnob->setModel( &p->m_ch1VolumeModel );
	m_ch1VolSweepDirButton->setModel( &p->m_ch1VolSweepDirModel );
	m_ch1SweepStepLengthKnob->setModel( &p->m_ch1SweepStepLengthModel );

	m_ch2WavePatternDutyKnob->setModel( &p->m_ch2WavePatternDutyModel );
	m_ch2VolumeKnob->setModel( &p->m_ch2VolumeModel );
	m_ch2VolSweepDirButton->setModel( &p->m_ch2VolSweepDirModel );
	m_ch2SweepStepLengthKnob->setModel( &p->m_ch2SweepStepLengthModel );

	//m_ch3OnButton->setModel( &p->m_ch3OnModel );
	m_ch3VolumeKnob->setModel( &p->m_ch3VolumeModel );

	m_ch4VolumeKnob->setModel( &p->m_ch4VolumeModel );
	m_ch4VolSweepDirButton->setModel( &p->m_ch4VolSweepDirModel );
	m_ch4SweepStepLengthKnob->setModel( &p->m_ch4SweepStepLengthModel );
	m_ch4ShiftRegWidthButton->setModel( &p->m_ch4ShiftRegWidthModel );

	m_so1VolumeKnob->setModel( &p->m_so1VolumeModel );
	m_so2VolumeKnob->setModel( &p->m_so2VolumeModel );
	m_ch1So1Button->setModel( &p->m_ch1So1Model );
	m_ch2So1Button->setModel( &p->m_ch2So1Model );
	m_ch3So1Button->setModel( &p->m_ch3So1Model );
	m_ch4So1Button->setModel( &p->m_ch4So1Model );
	m_ch1So2Button->setModel( &p->m_ch1So2Model );
	m_ch2So2Button->setModel( &p->m_ch2So2Model );
	m_ch3So2Button->setModel( &p->m_ch3So2Model );
	m_ch4So2Button->setModel( &p->m_ch4So2Model );
	m_trebleKnob->setModel( &p->m_trebleModel );
	m_bassKnob->setModel( &p->m_bassModel );
	m_graph->setModel( &p->m_graphModel );
}

extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model *m, void * )
{
	return( new FreeBoyInstrument(
				static_cast<InstrumentTrack *>( m ) ) );
}


}


