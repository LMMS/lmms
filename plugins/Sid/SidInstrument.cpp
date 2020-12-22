/*
 * SidInstrument.cpp - ResID based software-synthesizer
 *
 * Copyright (c) 2008 Csaba Hruska <csaba.hruska/at/gmail.com>
 *                    Attila Herman <attila589/at/gmail.com>
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


#include <QPainter>
#include <QDomElement>

#include <cmath>
#include <cstdio>

#include "sid.h"

#include "SidInstrument.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "Knob.h"
#include "Mixer.h"
#include "NotePlayHandle.h"
#include "PixmapButton.h"
#include "ToolTip.h"

#include "embed.h"
#include "plugin_export.h"

#define C64_PAL_CYCLES_PER_SEC  985248

#define NUMSIDREGS 0x19
#define SIDWRITEDELAY 9 // lda $xxxx,x 4 cycles, sta $d400,x 5 cycles
#define SIDWAVEDELAY 4 // and $xxxx,x 4 cycles extra

unsigned char sidorder[] =
  {0x15,0x16,0x18,0x17,
   0x05,0x06,0x02,0x03,0x00,0x01,0x04,
   0x0c,0x0d,0x09,0x0a,0x07,0x08,0x0b,
   0x13,0x14,0x10,0x11,0x0e,0x0f,0x12};

static const char *attackTime[16] = { "2 ms", "8 ms", "16 ms", "24 ms",
									"38 ms", "56 ms", "68 ms", "80 ms",
									"100 ms", "250 ms", "500 ms", "800 ms",
									"1 s", "3 s", "5 s", "8 s" };
static const char *decRelTime[16] = { "6 ms", "24 ms", "48 ms", "72 ms",
									"114 ms", "168 ms", "204 ms", "240 ms",
									"300 ms", "750 ms", "1.5 s", "2.4 s",
									"3 s", "9 s", "15 s", "24 s" };
// release time time in ms
static const int relTime[16] = { 6, 24, 48, 72, 114, 168, 204, 240, 300, 750,
								1500, 2400, 3000, 9000, 15000, 24000 };


extern "C"
{
Plugin::Descriptor PLUGIN_EXPORT sid_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"SID",
	QT_TRANSLATE_NOOP( "PluginBrowser", "Emulation of the MOS6581 and MOS8580 "
					"SID.\nThis chip was used in the Commodore 64 computer." ),

	"Csaba Hruska <csaba.hruska/at/gmail.com>"
	"Attila Herman <attila589/at/gmail.com>",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
} ;

}

voiceObject::voiceObject( Model * _parent, int _idx ) :
	Model( _parent ),
	m_pulseWidthModel( 2048.0f, 0.0f, 4095.0f, 1.0f, this,
					tr( "Voice %1 pulse width" ).arg( _idx+1 ) ),
	m_attackModel( 8.0f, 0.0f, 15.0f, 1.0f, this,
					tr( "Voice %1 attack" ).arg( _idx+1 ) ),
	m_decayModel( 8.0f, 0.0f, 15.0f, 1.0f, this,
					tr( "Voice %1 decay" ).arg( _idx+1 ) ),
	m_sustainModel( 15.0f, 0.0f, 15.0f, 1.0f, this,
					tr( "Voice %1 sustain" ).arg( _idx+1 ) ),
	m_releaseModel( 8.0f, 0.0f, 15.0f, 1.0f, this,
					tr( "Voice %1 release" ).arg( _idx+1 ) ),
	m_coarseModel( 0.0f, -24.0, 24.0, 1.0f, this,
					tr( "Voice %1 coarse detuning" ).arg( _idx+1 ) ),
	m_waveFormModel( TriangleWave, 0, NumWaveShapes-1, this,
					tr( "Voice %1 wave shape" ).arg( _idx+1 ) ),

	m_syncModel( false, this, tr( "Voice %1 sync" ).arg( _idx+1 ) ),
	m_ringModModel( false, this, tr( "Voice %1 ring modulate" ).arg( _idx+1 ) ),
	m_filteredModel( false, this, tr( "Voice %1 filtered" ).arg( _idx+1 ) ),
	m_testModel( false, this, tr( "Voice %1 test" ).arg( _idx+1 ) )
{
}


voiceObject::~voiceObject()
{
}


SidInstrument::SidInstrument( InstrumentTrack * _instrument_track ) :
	Instrument( _instrument_track, &sid_plugin_descriptor ),
	// filter	
	m_filterFCModel( 1024.0f, 0.0f, 2047.0f, 1.0f, this, tr( "Cutoff frequency" ) ),
	m_filterResonanceModel( 8.0f, 0.0f, 15.0f, 1.0f, this, tr( "Resonance" ) ),
	m_filterModeModel( LowPass, 0, NumFilterTypes-1, this, tr( "Filter type" )),
	
	// misc
	m_voice3OffModel( false, this, tr( "Voice 3 off" ) ),
	m_volumeModel( 15.0f, 0.0f, 15.0f, 1.0f, this, tr( "Volume" ) ),
	m_chipModel( sidMOS8580, 0, NumChipModels-1, this, tr( "Chip model" ) )
{
	for( int i = 0; i < 3; ++i )
	{
		m_voice[i] = new voiceObject( this, i );
	}
}


SidInstrument::~SidInstrument()
{
}


void SidInstrument::saveSettings( QDomDocument & _doc,
							QDomElement & _this )
{
	// voices
	for( int i = 0; i < 3; ++i )
	{
		const QString is = QString::number( i );

		m_voice[i]->m_pulseWidthModel.saveSettings(
										_doc, _this, "pulsewidth" + is );
		m_voice[i]->m_attackModel.saveSettings(
										_doc, _this, "attack" + is );
		m_voice[i]->m_decayModel.saveSettings(
										_doc, _this, "decay" + is );
		m_voice[i]->m_sustainModel.saveSettings(
										_doc, _this, "sustain" + is );
		m_voice[i]->m_releaseModel.saveSettings(
										_doc, _this, "release" + is );
		m_voice[i]->m_coarseModel.saveSettings(
										_doc, _this, "coarse" + is );
		m_voice[i]->m_waveFormModel.saveSettings(
										_doc, _this,"waveform" + is );
		m_voice[i]->m_syncModel.saveSettings(
										_doc, _this, "sync" + is );
		m_voice[i]->m_ringModModel.saveSettings(
										_doc, _this, "ringmod" + is );
		m_voice[i]->m_filteredModel.saveSettings(
										_doc, _this,"filtered" + is );
		m_voice[i]->m_testModel.saveSettings(
										_doc, _this, "test" + is );
	}

	// filter	
	m_filterFCModel.saveSettings( _doc, _this, "filterFC" );
	m_filterResonanceModel.saveSettings( _doc, _this, "filterResonance" );
	m_filterModeModel.saveSettings( _doc, _this, "filterMode" );
	
	// misc
	m_voice3OffModel.saveSettings( _doc, _this, "voice3Off" );
	m_volumeModel.saveSettings( _doc, _this, "volume" );
	m_chipModel.saveSettings( _doc, _this, "chipModel" );
}




void SidInstrument::loadSettings( const QDomElement & _this )
{
	// voices
	for( int i = 0; i < 3; ++i )
	{
		const QString is = QString::number( i );

		m_voice[i]->m_pulseWidthModel.loadSettings( _this, "pulsewidth" + is );
		m_voice[i]->m_attackModel.loadSettings( _this, "attack" + is );
		m_voice[i]->m_decayModel.loadSettings( _this, "decay" + is );
		m_voice[i]->m_sustainModel.loadSettings( _this, "sustain" + is );
		m_voice[i]->m_releaseModel.loadSettings( _this, "release" + is );
		m_voice[i]->m_coarseModel.loadSettings( _this, "coarse" + is );
		m_voice[i]->m_waveFormModel.loadSettings( _this, "waveform" + is );
		m_voice[i]->m_syncModel.loadSettings( _this, "sync" + is );
		m_voice[i]->m_ringModModel.loadSettings( _this, "ringmod" + is );
		m_voice[i]->m_filteredModel.loadSettings( _this, "filtered" + is );
		m_voice[i]->m_testModel.loadSettings( _this, "test" + is );
	}
	
	// filter	
	m_filterFCModel.loadSettings( _this, "filterFC" );
	m_filterResonanceModel.loadSettings( _this, "filterResonance" );
	m_filterModeModel.loadSettings( _this, "filterMode" );
	
	// misc
	m_voice3OffModel.loadSettings( _this, "voice3Off" );
	m_volumeModel.loadSettings( _this, "volume" );
	m_chipModel.loadSettings( _this, "chipModel" );
}




QString SidInstrument::nodeName() const
{
	return( sid_plugin_descriptor.name );
}




f_cnt_t SidInstrument::desiredReleaseFrames() const
{
	const float samplerate = Engine::mixer()->processingSampleRate();
	int maxrel = 0;
	for( int i = 0 ; i < 3 ; ++i )
	{
		if( maxrel < m_voice[i]->m_releaseModel.value() )
			maxrel = (int)m_voice[i]->m_releaseModel.value();
	}

	return f_cnt_t( float(relTime[maxrel])*samplerate/1000.0 );
}




static int sid_fillbuffer(unsigned char* sidreg, SID *sid, int tdelta, short *ptr, int samples)
{
  int tdelta2;
  int result;
  int total = 0;
  int c;
//  customly added
  int residdelay = 0;

  int badline = rand() % NUMSIDREGS;

  for (c = 0; c < NUMSIDREGS; c++)
  {
    unsigned char o = sidorder[c];

  	// Extra delay for loading the waveform (and mt_chngate,x)
  	if ((o == 4) || (o == 11) || (o == 18))
  	{
  	  tdelta2 = SIDWAVEDELAY;
      result = sid->clock(tdelta2, ptr, samples);
      total += result;
      ptr += result;
      samples -= result;
      tdelta -= SIDWAVEDELAY;
    }

    // Possible random badline delay once per writing
    if ((badline == c) && (residdelay))
  	{
      tdelta2 = residdelay;
      result = sid->clock(tdelta2, ptr, samples);
      total += result;
      ptr += result;
      samples -= result;
      tdelta -= residdelay;
    }

    sid->write(o, sidreg[o]);

    tdelta2 = SIDWRITEDELAY;
    result = sid->clock(tdelta2, ptr, samples);
    total += result;
    ptr += result;
    samples -= result;
    tdelta -= SIDWRITEDELAY;
  }
  result = sid->clock(tdelta, ptr, samples);
  total += result;

  return total;
}




void SidInstrument::playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer )
{
	const f_cnt_t tfp = _n->totalFramesPlayed();

	const int clockrate = C64_PAL_CYCLES_PER_SEC;
	const int samplerate = Engine::mixer()->processingSampleRate();

	if ( tfp == 0 )
	{
		SID *sid = new SID();
		sid->set_sampling_parameters( clockrate, SAMPLE_FAST, samplerate );
		sid->set_chip_model( MOS8580 );
		sid->enable_filter( true );
		sid->reset();
		_n->m_pluginData = sid;
	}
	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();

	SID *sid = static_cast<SID *>( _n->m_pluginData );
	int delta_t = clockrate * frames / samplerate + 4;
	// avoid variable length array for msvc compat
	short* buf = reinterpret_cast<short*>(_working_buffer + offset);
	unsigned char sidreg[NUMSIDREGS];

	for (int c = 0; c < NUMSIDREGS; c++)
	{
		sidreg[c] = 0x00;
	}

	if( (ChipModel)m_chipModel.value() == sidMOS6581 )
	{
		sid->set_chip_model( MOS6581 );
	}
	else
	{
		sid->set_chip_model( MOS8580 );
	}

	// voices
	reg8 data8 = 0;
	reg8 data16 = 0;
	reg8 base = 0;
	float freq = 0.0;
	float note = 0.0;
	for( reg8 i = 0 ; i < 3 ; ++i )
	{
		base = i*7;
		// freq ( Fn = Fout / Fclk * 16777216 ) + coarse detuning
		freq = _n->frequency();
		note = 69.0 + 12.0 * log( freq / 440.0 ) / log( 2 );
		note += m_voice[i]->m_coarseModel.value();
		freq = 440.0 * pow( 2.0, (note-69.0)/12.0 );
		data16 = int( freq / float(clockrate) * 16777216.0 );

		sidreg[base+0] = data16&0x00FF;
		sidreg[base+1] = (data16>>8)&0x00FF;
		// pw
		data16 = (int)m_voice[i]->m_pulseWidthModel.value();
		
		sidreg[base+2] = data16&0x00FF;
		sidreg[base+3] = (data16>>8)&0x000F;
		// control: wave form, (test), ringmod, sync, gate
		data8 = _n->isReleased()?0:1;
		data8 += m_voice[i]->m_syncModel.value()?2:0;
		data8 += m_voice[i]->m_ringModModel.value()?4:0;
		data8 += m_voice[i]->m_testModel.value()?8:0;
		switch( m_voice[i]->m_waveFormModel.value() )
		{	
			default: break;
			case voiceObject::NoiseWave:	data8 += 128; break;
			case voiceObject::SquareWave:	data8 += 64; break;
			case voiceObject::SawWave:		data8 += 32; break;
			case voiceObject::TriangleWave:	data8 += 16; break;
		}
		sidreg[base+4] = data8&0x00FF;
		// ad
		data16 = (int)m_voice[i]->m_attackModel.value();

		data8 = (data16&0x0F)<<4;
		data16 = (int)m_voice[i]->m_decayModel.value();

		data8 += (data16&0x0F);
		sidreg[base+5] = data8&0x00FF;
		// sr
		data16 = (int)m_voice[i]->m_sustainModel.value();

		data8 = (data16&0x0F)<<4;
		data16 = (int)m_voice[i]->m_releaseModel.value();

		data8 += (data16&0x0F);
		sidreg[base+6] = data8&0x00FF;
	}
	// filtered

	// FC (FilterCutoff)
	data16 = (int)m_filterFCModel.value();
	sidreg[21] = data16&0x0007;
	sidreg[22] = (data16>>3)&0x00FF;
	
	// res, filt ex,3,2,1
	data16 = (int)m_filterResonanceModel.value();
	data8 = (data16&0x000F)<<4;
	data8 += m_voice[2]->m_filteredModel.value()?4:0;
	data8 += m_voice[1]->m_filteredModel.value()?2:0;
	data8 += m_voice[0]->m_filteredModel.value()?1:0;
	sidreg[23] = data8&0x00FF;

	// mode vol
	data16 = (int)m_volumeModel.value();
	data8 = data16&0x000F;
	data8 += m_voice3OffModel.value()?128:0;

	switch( m_filterModeModel.value() )
	{	
		default: break;
		case LowPass:	data8 += 16; break;
		case BandPass:	data8 += 32; break;
		case HighPass:	data8 += 64; break;
	}

	sidreg[24] = data8&0x00FF;
		
	int num = sid_fillbuffer(sidreg, sid,delta_t,buf, frames);
	if(num!=frames)
		printf("!!!Not enough samples\n");

	// loop backwards to avoid overwriting data in the short-to-float conversion
	for( fpp_t frame = frames - 1; frame >= 0; frame-- )
	{
		sample_t s = float(buf[frame])/32768.0;
		for( ch_cnt_t ch = 0; ch < DEFAULT_CHANNELS; ++ch )
		{
			_working_buffer[frame+offset][ch] = s;
		}
	}

	instrumentTrack()->processAudioBuffer( _working_buffer, frames + offset, _n );
}




void SidInstrument::deleteNotePluginData( NotePlayHandle * _n )
{
	delete static_cast<SID *>( _n->m_pluginData );
}




PluginView * SidInstrument::instantiateView( QWidget * _parent )
{
	return( new SidInstrumentView( this, _parent ) );
}




class sidKnob : public Knob
{
public:
	sidKnob( QWidget * _parent ) :
			Knob( knobStyled, _parent )
	{
		setFixedSize( 16, 16 );
		setCenterPointX( 7.5 );
		setCenterPointY( 7.5 );
		setInnerRadius( 2 );
		setOuterRadius( 8 );
		setTotalAngle( 270.0 );
		setLineWidth( 2 );
	}
};




SidInstrumentView::SidInstrumentView( Instrument * _instrument,
							QWidget * _parent ) :
	InstrumentViewFixedSize( _instrument, _parent )
{

	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );

	m_volKnob = new sidKnob( this );
	m_volKnob->setHintText( tr( "Volume:" ), "" );
	m_volKnob->move( 7, 64 );

	m_resKnob = new sidKnob( this );
	m_resKnob->setHintText( tr( "Resonance:" ), "" );
	m_resKnob->move( 7 + 28, 64 );

	m_cutKnob = new sidKnob( this );
	m_cutKnob->setHintText( tr( "Cutoff frequency:" ), " Hz" );
	m_cutKnob->move( 7 + 2*28, 64 );

	PixmapButton * hp_btn = new PixmapButton( this, NULL );
	hp_btn->move( 140, 77 );
	hp_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "hpred" ) );
	hp_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "hp" ) );
	ToolTip::add( hp_btn, tr( "High-pass filter ") );

	PixmapButton * bp_btn = new PixmapButton( this, NULL );
	bp_btn->move( 164, 77 );
	bp_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "bpred" ) );
	bp_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "bp" ) );
	ToolTip::add( bp_btn, tr( "Band-pass filter ") );

	PixmapButton * lp_btn = new PixmapButton( this, NULL );
	lp_btn->move( 185, 77 );
	lp_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "lpred" ) );
	lp_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "lp" ) );
	ToolTip::add( lp_btn, tr( "Low-pass filter ") );

	m_passBtnGrp = new automatableButtonGroup( this );
	m_passBtnGrp->addButton( hp_btn );
	m_passBtnGrp->addButton( bp_btn );
	m_passBtnGrp->addButton( lp_btn );

	m_offButton = new PixmapButton( this, NULL );
	m_offButton->setCheckable( true );
	m_offButton->move( 207, 77 );
	m_offButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "3offred" ) );
	m_offButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "3off" ) );
	ToolTip::add( m_offButton, tr( "Voice 3 off ") );

	PixmapButton * mos6581_btn = new PixmapButton( this, NULL );
	mos6581_btn->move( 170, 59 );
	mos6581_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "6581red" ) );
	mos6581_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "6581" ) );
	ToolTip::add( mos6581_btn, tr( "MOS6581 SID ") );

	PixmapButton * mos8580_btn = new PixmapButton( this, NULL );
	mos8580_btn->move( 207, 59 );
	mos8580_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "8580red" ) );
	mos8580_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "8580" ) );
	ToolTip::add( mos8580_btn, tr( "MOS8580 SID ") );

	m_sidTypeBtnGrp = new automatableButtonGroup( this );
	m_sidTypeBtnGrp->addButton( mos6581_btn );
	m_sidTypeBtnGrp->addButton( mos8580_btn );

	for( int i = 0; i < 3; i++ ) 
	{
		Knob *ak = new sidKnob( this );
		ak->setHintText( tr("Attack:"), "" );
		ak->move( 7, 114 + i*50 );

		Knob *dk = new sidKnob( this );
		dk->setHintText( tr("Decay:") , "" );
		dk->move( 7 + 28, 114 + i*50 );

		Knob *sk = new sidKnob( this );
		sk->setHintText( tr("Sustain:"), "" );
		sk->move( 7 + 2*28, 114 + i*50 );

		Knob *rk = new sidKnob( this );
		rk->setHintText( tr("Release:"), "" );
		rk->move( 7 + 3*28, 114 + i*50 );

		Knob *pwk = new sidKnob( this );
		pwk->setHintText( tr("Pulse Width:"), "" );
		pwk->move( 7 + 4*28, 114 + i*50 );

		Knob *crsk = new sidKnob( this );
		crsk->setHintText( tr("Coarse:"), " semitones" );
		crsk->move( 147, 114 + i*50 );

		PixmapButton * pulse_btn = new PixmapButton( this, NULL );
		pulse_btn->move( 187, 101 + i*50 );
		pulse_btn->setActiveGraphic(
			PLUGIN_NAME::getIconPixmap( "pulsered" ) );
		pulse_btn->setInactiveGraphic(
			PLUGIN_NAME::getIconPixmap( "pulse" ) );
		ToolTip::add( pulse_btn, tr( "Pulse wave" ) );

		PixmapButton * triangle_btn = new PixmapButton( this, NULL );
		triangle_btn->move( 168, 101 + i*50 );
		triangle_btn->setActiveGraphic(
			PLUGIN_NAME::getIconPixmap( "trianglered" ) );
		triangle_btn->setInactiveGraphic(
			PLUGIN_NAME::getIconPixmap( "triangle" ) );
		ToolTip::add( triangle_btn, tr( "Triangle wave" ) );

		PixmapButton * saw_btn = new PixmapButton( this, NULL );
		saw_btn->move( 207, 101 + i*50 );
		saw_btn->setActiveGraphic(
			PLUGIN_NAME::getIconPixmap( "sawred" ) );
		saw_btn->setInactiveGraphic(
			PLUGIN_NAME::getIconPixmap( "saw" ) );
		ToolTip::add( saw_btn, tr( "Saw wave" ) );

		PixmapButton * noise_btn = new PixmapButton( this, NULL );
		noise_btn->move( 226, 101 + i*50 );
		noise_btn->setActiveGraphic(
			PLUGIN_NAME::getIconPixmap( "noisered" ) );
		noise_btn->setInactiveGraphic(
			PLUGIN_NAME::getIconPixmap( "noise" ) );
		ToolTip::add( noise_btn, tr( "Noise" ) );

		automatableButtonGroup * wfbg =
			new automatableButtonGroup( this );

		wfbg->addButton( pulse_btn );
		wfbg->addButton( triangle_btn );
		wfbg->addButton( saw_btn );
		wfbg->addButton( noise_btn );

		PixmapButton * sync_btn = new PixmapButton( this, NULL );
		sync_btn->setCheckable( true );
		sync_btn->move( 207, 134 + i*50 );
		sync_btn->setActiveGraphic(
			PLUGIN_NAME::getIconPixmap( "syncred" ) );
		sync_btn->setInactiveGraphic(
			PLUGIN_NAME::getIconPixmap( "sync" ) );
		ToolTip::add( sync_btn, tr( "Sync" ) );

		PixmapButton * ringMod_btn = new PixmapButton( this, NULL );
		ringMod_btn->setCheckable( true );
		ringMod_btn->move( 170, 116 + i*50 );
		ringMod_btn->setActiveGraphic(
			PLUGIN_NAME::getIconPixmap( "ringred" ) );
		ringMod_btn->setInactiveGraphic(
			PLUGIN_NAME::getIconPixmap( "ring" ) );
		ToolTip::add( ringMod_btn, tr( "Ring modulation" ) );

		PixmapButton * filter_btn = new PixmapButton( this, NULL );
		filter_btn->setCheckable( true );
		filter_btn->move( 207, 116 + i*50 );
		filter_btn->setActiveGraphic(
			PLUGIN_NAME::getIconPixmap( "filterred" ) );
		filter_btn->setInactiveGraphic(
			PLUGIN_NAME::getIconPixmap( "filter" ) );
		ToolTip::add( filter_btn, tr( "Filtered" ) );

		PixmapButton * test_btn = new PixmapButton( this, NULL );
		test_btn->setCheckable( true );
		test_btn->move( 170, 134 + i*50 );
		test_btn->setActiveGraphic(
			PLUGIN_NAME::getIconPixmap( "testred" ) );
		test_btn->setInactiveGraphic(
			PLUGIN_NAME::getIconPixmap( "test" ) );
		ToolTip::add( test_btn, tr( "Test" ) );

		m_voiceKnobs[i] = voiceKnobs( ak, dk, sk, rk, pwk, crsk, wfbg,
								sync_btn, ringMod_btn, filter_btn, test_btn );
	}
}


SidInstrumentView::~SidInstrumentView()
{
}

void SidInstrumentView::updateKnobHint()
{
	SidInstrument * k = castModel<SidInstrument>();

	for( int i = 0; i < 3; ++i )
	{
		m_voiceKnobs[i].m_attKnob->setHintText( tr( "Attack:" ) + " ", " (" +
				QString::fromLatin1( attackTime[(int)k->m_voice[i]->
				m_attackModel.value()] ) + ")" );
		ToolTip::add( m_voiceKnobs[i].m_attKnob,
						attackTime[(int)k->m_voice[i]->m_attackModel.value()] );

		m_voiceKnobs[i].m_decKnob->setHintText( tr( "Decay:" ) + " ", " (" +
				QString::fromLatin1( decRelTime[(int)k->m_voice[i]->
				m_decayModel.value()] ) + ")" );
		ToolTip::add( m_voiceKnobs[i].m_decKnob,
						decRelTime[(int)k->m_voice[i]->m_decayModel.value()] );

		m_voiceKnobs[i].m_relKnob->setHintText( tr( "Release:" ) + " ", " (" +
				QString::fromLatin1( decRelTime[(int)k->m_voice[i]->
				m_releaseModel.value()] )  + ")" );
		ToolTip::add( m_voiceKnobs[i].m_relKnob,
						decRelTime[(int)k->m_voice[i]->m_releaseModel.value()]);
	
		m_voiceKnobs[i].m_pwKnob->setHintText( tr( "Pulse width:" )+ " ", " (" +
				QString::number(  (double)k->m_voice[i]->
				m_pulseWidthModel.value() / 40.95 ) + "%)" );
		ToolTip::add( m_voiceKnobs[i].m_pwKnob,
				QString::number( (double)k->m_voice[i]->
				m_pulseWidthModel.value() / 40.95 ) + "%" );
	}
	m_cutKnob->setHintText( tr( "Cutoff frequency:" ) + " ", " (" +
				QString::number ( (int) ( 9970.0 / 2047.0 *
				(double)k->m_filterFCModel.value() + 30.0 ) ) + " Hz)" );
	ToolTip::add( m_cutKnob, QString::number( (int) ( 9970.0 / 2047.0 *
					 (double)k->m_filterFCModel.value() + 30.0 ) ) + " Hz" );
}




void SidInstrumentView::updateKnobToolTip()
{
	SidInstrument * k = castModel<SidInstrument>();
	for( int i = 0; i < 3; ++i )
	{
		ToolTip::add( m_voiceKnobs[i].m_sustKnob,
				QString::number( (int)k->m_voice[i]->m_sustainModel.value() ) );
		ToolTip::add( m_voiceKnobs[i].m_crsKnob,
				QString::number( (int)k->m_voice[i]->m_coarseModel.value() ) +
				" semitones" );
	}
	ToolTip::add( m_volKnob,
					QString::number( (int)k->m_volumeModel.value() ) );
	ToolTip::add( m_resKnob,
					QString::number( (int)k->m_filterResonanceModel.value() ) );
}




void SidInstrumentView::modelChanged()
{
	SidInstrument * k = castModel<SidInstrument>();

	m_volKnob->setModel( &k->m_volumeModel );
	m_resKnob->setModel( &k->m_filterResonanceModel );
	m_cutKnob->setModel( &k->m_filterFCModel );
	m_passBtnGrp->setModel( &k->m_filterModeModel );
	m_offButton->setModel(  &k->m_voice3OffModel );
	m_sidTypeBtnGrp->setModel(  &k->m_chipModel );

	for( int i = 0; i < 3; ++i )
	{
		m_voiceKnobs[i].m_attKnob->setModel(
					&k->m_voice[i]->m_attackModel );
		m_voiceKnobs[i].m_decKnob->setModel(
					&k->m_voice[i]->m_decayModel );
		m_voiceKnobs[i].m_sustKnob->setModel(
					&k->m_voice[i]->m_sustainModel );
		m_voiceKnobs[i].m_relKnob->setModel(
					&k->m_voice[i]->m_releaseModel );
		m_voiceKnobs[i].m_pwKnob->setModel(
					&k->m_voice[i]->m_pulseWidthModel );
		m_voiceKnobs[i].m_crsKnob->setModel(
					&k->m_voice[i]->m_coarseModel );
		m_voiceKnobs[i].m_waveFormBtnGrp->setModel(
					&k->m_voice[i]->m_waveFormModel );
		m_voiceKnobs[i].m_syncButton->setModel(
					&k->m_voice[i]->m_syncModel );
		m_voiceKnobs[i].m_ringModButton->setModel(
					&k->m_voice[i]->m_ringModModel );
		m_voiceKnobs[i].m_filterButton->setModel(
					&k->m_voice[i]->m_filteredModel );
		m_voiceKnobs[i].m_testButton->setModel(
					&k->m_voice[i]->m_testModel );
	}

	for( int i = 0; i < 3; ++i )
	{
		connect( &k->m_voice[i]->m_attackModel, SIGNAL( dataChanged() ),
			this, SLOT( updateKnobHint() ) );
		connect( &k->m_voice[i]->m_decayModel, SIGNAL( dataChanged() ),
			this, SLOT( updateKnobHint() ) );
		connect( &k->m_voice[i]->m_releaseModel, SIGNAL( dataChanged() ),
			this, SLOT( updateKnobHint() ) );
		connect( &k->m_voice[i]->m_pulseWidthModel, SIGNAL( dataChanged() ),
			this, SLOT( updateKnobHint() ) );
		connect( &k->m_voice[i]->m_sustainModel, SIGNAL( dataChanged() ),
			this, SLOT( updateKnobToolTip() ) );
		connect( &k->m_voice[i]->m_coarseModel, SIGNAL( dataChanged() ),
			this, SLOT( updateKnobToolTip() ) );
	}
	
	connect( &k->m_volumeModel, SIGNAL( dataChanged() ),
		this, SLOT( updateKnobToolTip() ) );
	connect( &k->m_filterResonanceModel, SIGNAL( dataChanged() ),
		this, SLOT( updateKnobToolTip() ) );
	connect( &k->m_filterFCModel, SIGNAL( dataChanged() ),
		this, SLOT( updateKnobHint() ) );

	updateKnobHint();
	updateKnobToolTip();
}





extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model *m, void * )
{
	return( new SidInstrument( static_cast<InstrumentTrack *>( m ) ) );
}


}




