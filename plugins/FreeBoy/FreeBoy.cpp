/*
 * FreeBoy.cpp - GameBoy papu based instrument
 *
 * Copyright (c) 2008 Attila Herman <attila589/at/gmail.com>
 *                    Csaba Hruska <csaba.hruska/at/gmail.com>
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

#include "FreeBoy.h"

#include <cmath>
#include <QDomElement>

#include "GbApuWrapper.h"
#include "base64.h"
#include "InstrumentTrack.h"
#include "Knob.h"
#include "AudioEngine.h"
#include "NotePlayHandle.h"
#include "PixmapButton.h"
#include "Engine.h"
#include "Graph.h"

#include "embed.h"

#include "plugin_export.h"

namespace lmms
{


namespace
{
constexpr blip_time_t FRAME_LENGTH = 70224;
constexpr long CLOCK_RATE = 4194304;
}

extern "C"
{
Plugin::Descriptor PLUGIN_EXPORT freeboy_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"FreeBoy",
	QT_TRANSLATE_NOOP( "PluginBrowser", "Emulation of GameBoy (TM) APU" ),

	"Attila Herman <attila589/at/gmail.com>"
	"Csaba Hruska <csaba.hruska/at/gmail.com>",
	0x0100,
	Plugin::Type::Instrument,
	new PluginPixmapLoader( "logo" ),
	{},
	nullptr,
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

	m_graphModel( 0, 15, 32, this, false, 1 )
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




float FreeBoyInstrument::desiredReleaseTimeMs() const
{
	// Previous implementation was 1000 samples. At 44.1 kHz this is somewhat shy of 23. ms.
	return 23.f;
}



void FreeBoyInstrument::playNote(NotePlayHandle* nph, SampleFrame* workingBuffer)
{
	const f_cnt_t tfp = nph->totalFramesPlayed();
	const int samplerate = Engine::audioEngine()->outputSampleRate();
	const fpp_t frames = nph->framesLeftForCurrentPeriod();
	const f_cnt_t offset = nph->noteOffset();

	int data = 0;
	int freq = nph->frequency();

	if (!nph->m_pluginData)
	{
		auto papu = new GbApuWrapper{};
		papu->setSampleRate(samplerate, CLOCK_RATE);

		// Master sound circuitry power control
		papu->writeRegister(0xff26, 0x80);

		data = m_ch1VolumeModel.value();
		data = data << 1;
		data += m_ch1VolSweepDirModel.value();
		data = data << 3;
		data += m_ch1SweepStepLengthModel.value();
		papu->writeRegister(0xff12, data);

		data = m_ch2VolumeModel.value();
		data = data << 1;
		data += m_ch2VolSweepDirModel.value();
		data = data << 3;
		data += m_ch2SweepStepLengthModel.value();
		papu->writeRegister(0xff17, data);

		//channel 4 - noise
		data = m_ch4VolumeModel.value();
		data = data << 1;
		data += m_ch4VolSweepDirModel.value();
		data = data << 3;
		data += m_ch4SweepStepLengthModel.value();
		papu->writeRegister(0xff21, data);

		nph->m_pluginData = papu;
	}

	auto papu = static_cast<GbApuWrapper*>(nph->m_pluginData);

	papu->trebleEq(m_trebleModel.value());
	papu->bassFreq(m_bassModel.value());

	//channel 1 - square
	data = m_ch1SweepTimeModel.value();
	data = data << 1;
	data += m_ch1SweepDirModel.value();
	data = data << 3;
	data += m_ch1SweepRtShiftModel.value();
	papu->writeRegister(0xff10, data);

	data = m_ch1WavePatternDutyModel.value();
	data = data << 6;
	papu->writeRegister(0xff11, data);

	//channel 2 - square
	data = m_ch2WavePatternDutyModel.value();
	data = data << 6;
	papu->writeRegister(0xff16, data);

	//channel 3 - wave
	//data = m_ch3OnModel.value() ? 128 : 0;
	data = 128;
	papu->writeRegister(0xff1a, data);

	auto ch3voldata = std::array{0, 3, 2, 1};
	data = ch3voldata[(int)m_ch3VolumeModel.value()];
	data = data << 5;
	papu->writeRegister(0xff1c, data);

	//controls
	data = m_so1VolumeModel.value();
	data = data << 4;
	data += m_so2VolumeModel.value();
	papu->writeRegister(0xff24, data);

	data = m_ch4So2Model.value() ? 128 : 0;
	data += m_ch3So2Model.value() ? 64 : 0;
	data += m_ch2So2Model.value() ? 32 : 0;
	data += m_ch1So2Model.value() ? 16 : 0;
	data += m_ch4So1Model.value() ? 8 : 0;
	data += m_ch3So1Model.value() ? 4 : 0;
	data += m_ch2So1Model.value() ? 2 : 0;
	data += m_ch1So1Model.value() ? 1 : 0;
	papu->writeRegister(0xff25, data);

	const float* wpm = m_graphModel.samples();

	for (char i = 0; i < 16; ++i)
	{
		data = static_cast<int>(std::floor(wpm[i * 2])) << 4;
		data += static_cast<int>(std::floor(wpm[(i * 2) + 1]));
		papu->writeRegister(0xff30 + i, data);
	}

	if ((freq >= 65) && (freq <= 4000))
	{
		int initFlag = (tfp == 0) ? 128 : 0;
		// Hz = 4194304 / ((2048 - (11-bit-freq)) << 5)
		data = 2048 - ((4194304 / freq) >> 5);
		if (tfp == 0)
		{
			papu->writeRegister(0xff13, data & 0xff);
			papu->writeRegister(0xff14, (data >> 8) | initFlag);
		}
		papu->writeRegister(0xff18, data & 0xff);
		papu->writeRegister(0xff19, (data >> 8) | initFlag);
		papu->writeRegister(0xff1d, data & 0xff);
		papu->writeRegister(0xff1e, (data >> 8) | initFlag);
	}

	if (tfp == 0)
	{
		// Initialize noise channel...
		// PRNG Frequency = (1048576 Hz / (ratio + 1)) / 2 ^ (shiftclockfreq + 1)
		// When div_ratio = 0 the ratio should be 0.5. Since s = 0 is the only case where r = 0 gives
		// a unique frequency, we can start by guessing s = r = 0 here and then skip r = 0 in the loop.
		char clock_freq = 0;
		char div_ratio = 0;
		float closest_freq = 524288.0 / (0.5 * std::exp2(clock_freq + 1.0));
		// This nested for loop iterates over all possible combinations of clock frequency and dividing
		// ratio and chooses the combination whose resulting frequency is closest to the note frequency
		for (char s = 0; s < 16; ++s)
		{
			for (char r = 1; r < 8; ++r)
			{
				float f = 524288.0 / (r * std::exp2(s + 1.0));
				if (std::fabs(freq - closest_freq) > std::fabs(freq - f))
				{
					closest_freq = f;
					div_ratio = r;
					clock_freq = s;
				}
			}
		}

		data = clock_freq;
		data = data << 1;
		data += m_ch4ShiftRegWidthModel.value();
		data = data << 3;
		data += div_ratio;
		papu->writeRegister(0xff22, data);

		//channel 4 init
		papu->writeRegister(0xff23, 128);
	}

	constexpr auto bufSize = f_cnt_t{2048};
	auto framesLeft = frames;
	auto buf = std::array<blip_sample_t, bufSize * 2>{};
	while (framesLeft > 0)
	{
		int avail = papu->samplesAvail();
		if (avail <= 0)
		{
			papu->endFrame(FRAME_LENGTH);
			avail = papu->samplesAvail();
		}
		const auto dataLen = std::min({static_cast<f_cnt_t>(avail), framesLeft, bufSize});

		const auto count = static_cast<f_cnt_t>(papu->readSamples(buf.data(), dataLen * 2) / 2);

		for (auto frame = std::size_t{0}; frame < count; ++frame)
		{
			for (ch_cnt_t ch = 0; ch < DEFAULT_CHANNELS; ++ch)
			{
				sample_t s = static_cast<float>(buf[(frame * 2) + ch]) / 32768.0f;
				workingBuffer[frames - framesLeft + frame + offset][ch] = s;
			}
		}
		framesLeft -= count;
	}
}



void FreeBoyInstrument::deleteNotePluginData(NotePlayHandle* nph)
{
	delete static_cast<GbApuWrapper*>(nph->m_pluginData);
}




gui::PluginView * FreeBoyInstrument::instantiateView( QWidget * _parent )
{
	return( new gui::FreeBoyInstrumentView( this, _parent ) );
}


namespace gui
{


class FreeBoyKnob : public Knob
{
public:
	FreeBoyKnob( QWidget * _parent ) :
			Knob( KnobType::Styled, _parent )
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
	m_ch1SweepTimeKnob->setToolTip(tr("Sweep time"));

	m_ch1SweepRtShiftKnob = new FreeBoyKnob( this );
	m_ch1SweepRtShiftKnob->setHintText( tr( "Sweep rate shift amount:" )
										, "" );
	m_ch1SweepRtShiftKnob->move( 5 + 3*32, 106 );
	m_ch1SweepRtShiftKnob->setToolTip(tr("Sweep rate shift amount"));

	m_ch1WavePatternDutyKnob = new FreeBoyKnob( this );
	m_ch1WavePatternDutyKnob->setHintText( tr( "Wave pattern duty cycle:" )
									, "" );
	m_ch1WavePatternDutyKnob->move( 5 + 2*32, 106 );
	m_ch1WavePatternDutyKnob->setToolTip(tr("Wave pattern duty cycle"));

	m_ch1VolumeKnob = new FreeBoyKnob( this );
	m_ch1VolumeKnob->setHintText( tr( "Square channel 1 volume:" )
								, "" );
	m_ch1VolumeKnob->move( 5, 106 );
	m_ch1VolumeKnob->setToolTip(tr("Square channel 1 volume"));

	m_ch1SweepStepLengthKnob = new FreeBoyKnob( this );
	m_ch1SweepStepLengthKnob->setHintText( tr( "Length of each step in sweep:" )
									, "" );
	m_ch1SweepStepLengthKnob->move( 5 + 32, 106 );
	m_ch1SweepStepLengthKnob->setToolTip(tr("Length of each step in sweep"));



	m_ch2WavePatternDutyKnob = new FreeBoyKnob( this );
	m_ch2WavePatternDutyKnob->setHintText( tr( "Wave pattern duty cycle:" )
									, "" );
	m_ch2WavePatternDutyKnob->move( 5 + 2*32, 155 );
	m_ch2WavePatternDutyKnob->setToolTip(tr("Wave pattern duty cycle"));

	m_ch2VolumeKnob = new FreeBoyKnob( this );
	m_ch2VolumeKnob->setHintText( tr( "Square channel 2 volume:" )
							, "" );
	m_ch2VolumeKnob->move( 5, 155 );
	m_ch2VolumeKnob->setToolTip(tr("Square channel 2 volume"));

	m_ch2SweepStepLengthKnob = new FreeBoyKnob( this );
	m_ch2SweepStepLengthKnob->setHintText( tr( "Length of each step in sweep:" )
									, "" );
	m_ch2SweepStepLengthKnob->move( 5 + 32, 155 );
	m_ch2SweepStepLengthKnob->setToolTip(tr("Length of each step in sweep"));



	m_ch3VolumeKnob = new FreeBoyKnob( this );
	m_ch3VolumeKnob->setHintText( tr( "Wave pattern channel volume:" ), "" );
	m_ch3VolumeKnob->move( 5, 204 );
	m_ch3VolumeKnob->setToolTip(tr("Wave pattern channel volume"));



	m_ch4VolumeKnob = new FreeBoyKnob( this );
	m_ch4VolumeKnob->setHintText( tr( "Noise channel volume:" ), "" );
	m_ch4VolumeKnob->move( 144, 155 );
	m_ch4VolumeKnob->setToolTip(tr("Noise channel volume"));

	m_ch4SweepStepLengthKnob = new FreeBoyKnob( this );
	m_ch4SweepStepLengthKnob->setHintText( tr( "Length of each step in sweep:" )
									, "" );
	m_ch4SweepStepLengthKnob->move( 144 + 32, 155 );
	m_ch4SweepStepLengthKnob->setToolTip(tr("Length of each step in sweep"));



	m_so1VolumeKnob = new FreeBoyKnob( this );
	m_so1VolumeKnob->setHintText( tr( "SO1 volume (Right):" ), "" );
	m_so1VolumeKnob->move( 5, 58 );
	m_so1VolumeKnob->setToolTip(tr("SO1 volume (Right)"));

	m_so2VolumeKnob = new FreeBoyKnob( this );
	m_so2VolumeKnob->setHintText( tr( "SO2 volume (Left):" ), "" );
	m_so2VolumeKnob->move( 5 + 32, 58 );
	m_so2VolumeKnob->setToolTip(tr("SO2 volume (Left)"));

	m_trebleKnob = new FreeBoyKnob( this );
	m_trebleKnob->setHintText( tr( "Treble:" ), "" );
	m_trebleKnob->move( 5 + 2*32, 58 );
	m_trebleKnob->setToolTip(tr("Treble"));

	m_bassKnob = new FreeBoyKnob( this );
	m_bassKnob->setHintText( tr( "Bass:" ), "" );
	m_bassKnob->move( 5 + 3*32, 58 );
	m_bassKnob->setToolTip(tr("Bass"));

	m_ch1SweepDirButton = new PixmapButton( this, nullptr );
	m_ch1SweepDirButton->setCheckable( true );
	m_ch1SweepDirButton->move( 167, 108 );
	m_ch1SweepDirButton->setActiveGraphic(
							PLUGIN_NAME::getIconPixmap( "btn_down" ) );
	m_ch1SweepDirButton->setInactiveGraphic(
							PLUGIN_NAME::getIconPixmap( "btn_up" ) );
	m_ch1SweepDirButton->setToolTip(tr("Sweep direction"));

	m_ch1VolSweepDirButton = new PixmapButton( this, nullptr );
	m_ch1VolSweepDirButton->setCheckable( true );
	m_ch1VolSweepDirButton->move( 207, 108 );
	m_ch1VolSweepDirButton->setActiveGraphic(
								PLUGIN_NAME::getIconPixmap( "btn_up" ) );
	m_ch1VolSweepDirButton->setInactiveGraphic(
								PLUGIN_NAME::getIconPixmap( "btn_down" ) );
	m_ch1VolSweepDirButton->setToolTip(tr("Volume sweep direction"));



	m_ch2VolSweepDirButton = new PixmapButton( this,
										tr( "Volume sweep direction" ) );
	m_ch2VolSweepDirButton->setCheckable( true );
	m_ch2VolSweepDirButton->move( 102, 156 );
	m_ch2VolSweepDirButton->setActiveGraphic(
								PLUGIN_NAME::getIconPixmap( "btn_up" ) );
	m_ch2VolSweepDirButton->setInactiveGraphic(
								PLUGIN_NAME::getIconPixmap( "btn_down" ) );
	m_ch2VolSweepDirButton->setToolTip(tr("Volume sweep direction"));

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
	m_ch4VolSweepDirButton->setToolTip(tr("Volume sweep direction"));

	m_ch4ShiftRegWidthButton = new PixmapButton( this, nullptr );
	m_ch4ShiftRegWidthButton->setCheckable( true );
	m_ch4ShiftRegWidthButton->move( 207, 171 );
	m_ch4ShiftRegWidthButton->setActiveGraphic(
									PLUGIN_NAME::getIconPixmap( "btn_7" ) );
	m_ch4ShiftRegWidthButton->setInactiveGraphic(
									PLUGIN_NAME::getIconPixmap( "btn_15" ) );
	m_ch4ShiftRegWidthButton->setToolTip(tr("Shift register width"));




	m_ch1So1Button = new PixmapButton( this, nullptr );
	m_ch1So1Button->setCheckable( true );
	m_ch1So1Button->move( 208, 51 );
	m_ch1So1Button->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "btn_on" ) );
	m_ch1So1Button->setInactiveGraphic( PLUGIN_NAME::getIconPixmap("btn_off") );
	m_ch1So1Button->setToolTip(tr("Channel 1 to SO1 (Right)"));

	m_ch2So1Button = new PixmapButton( this, nullptr );
	m_ch2So1Button->setCheckable( true );
	m_ch2So1Button->move( 208, 51 + 12 );
	m_ch2So1Button->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "btn_on" ) );
	m_ch2So1Button->setInactiveGraphic( PLUGIN_NAME::getIconPixmap("btn_off") );
	m_ch2So1Button->setToolTip(tr("Channel 2 to SO1 (Right)"));

	m_ch3So1Button = new PixmapButton( this, nullptr );
	m_ch3So1Button->setCheckable( true );
	m_ch3So1Button->move( 208, 51 + 2*12 );
	m_ch3So1Button->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "btn_on" ) );
	m_ch3So1Button->setInactiveGraphic( PLUGIN_NAME::getIconPixmap("btn_off") );
	m_ch3So1Button->setToolTip(tr("Channel 3 to SO1 (Right)"));

	m_ch4So1Button = new PixmapButton( this, nullptr );
	m_ch4So1Button->setCheckable( true );
	m_ch4So1Button->setChecked( false );
	m_ch4So1Button->move( 208, 51 + 3*12 );
	m_ch4So1Button->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "btn_on" ) );
	m_ch4So1Button->setInactiveGraphic( PLUGIN_NAME::getIconPixmap("btn_off") );
	m_ch4So1Button->setToolTip(tr("Channel 4 to SO1 (Right)"));

	m_ch1So2Button = new PixmapButton( this, nullptr );
	m_ch1So2Button->setCheckable( true );
	m_ch1So2Button->move( 148, 51 );
	m_ch1So2Button->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "btn_on" ) );
	m_ch1So2Button->setInactiveGraphic( PLUGIN_NAME::getIconPixmap("btn_off") );
	m_ch1So2Button->setToolTip(tr("Channel 1 to SO2 (Left)"));

	m_ch2So2Button = new PixmapButton( this, nullptr );
	m_ch2So2Button->setCheckable( true );
	m_ch2So2Button->move( 148, 51 + 12 );
	m_ch2So2Button->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "btn_on" ) );
	m_ch2So2Button->setInactiveGraphic( PLUGIN_NAME::getIconPixmap("btn_off") );
	m_ch2So2Button->setToolTip(tr("Channel 2 to SO2 (Left)"));

	m_ch3So2Button = new PixmapButton( this, nullptr );
	m_ch3So2Button->setCheckable( true );
	m_ch3So2Button->move( 148, 51 + 2*12 );
	m_ch3So2Button->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "btn_on" ) );
	m_ch3So2Button->setInactiveGraphic( PLUGIN_NAME::getIconPixmap("btn_off") );
	m_ch3So2Button->setToolTip(tr("Channel 3 to SO2 (Left)"));

	m_ch4So2Button = new PixmapButton( this, nullptr );
	m_ch4So2Button->setCheckable( true );
	m_ch4So2Button->setChecked( false );
	m_ch4So2Button->move( 148, 51 + 3*12 );
	m_ch4So2Button->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "btn_on" ) );
	m_ch4So2Button->setInactiveGraphic( PLUGIN_NAME::getIconPixmap("btn_off") );
	m_ch4So2Button->setToolTip(tr("Channel 4 to SO2 (Left)"));


	m_graph = new Graph( this );
	m_graph->setGraphStyle( Graph::Style::Nearest );
	m_graph->setGraphColor( QColor(0x4E, 0x83, 0x2B) );
	m_graph->move( 37, 199 );
	m_graph->resize(208, 47);
	m_graph->setToolTip(tr("Wave pattern graph"));
}


void FreeBoyInstrumentView::modelChanged()
{
	auto p = castModel<FreeBoyInstrument>();

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


} // namespace gui

extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model *m, void * )
{
	return( new FreeBoyInstrument(
				static_cast<InstrumentTrack *>( m ) ) );
}


}


} // namespace lmms
