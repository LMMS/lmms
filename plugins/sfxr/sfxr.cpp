/*
 * sfxr.cpp - port of sfxr to LMMS
 * The original readme file of sfxr can be found in readme.txt in this directory.
 *
 * Copyright (c) 2014 Wong Cho Ching
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
 */

#include <stdlib.h>

#define rnd(n) (rand()%(n+1))

#define PI 3.14159265f

float frnd(float range)
{
	return (float)rnd(10000)/10000*range;
}


#include <QtXml/QDomElement>

#include "sfxr.h"
#include "engine.h"
#include "graph.h"
#include "InstrumentTrack.h"
#include "knob.h"
#include "led_checkbox.h"
#include "note_play_handle.h"
#include "pixmap_button.h"
#include "song_editor.h"
#include "templates.h"
#include "tooltip.h"
#include "song.h"

#include "embed.cpp"

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT sfxr_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"sfxr",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"LMMS port of sfxr" ),
	"Wong Cho Ching",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
} ;

}


SfxrSynth::SfxrSynth( float * _shape, int _length, notePlayHandle * _nph, bool _interpolation,
				float _factor, const sample_rate_t _sample_rate ) :
	sample_index( 0 ),
	sample_realindex( 0 ),
	nph( _nph ),
	sample_length( _length ),
	sample_rate( _sample_rate ),
	interpolation( _interpolation)
{
	sample_shape = new float[sample_length];
	for (int i=0; i < _length; ++i)
	{
		sample_shape[i] = _shape[i] * _factor;
	}
}


SfxrSynth::~SfxrSynth()
{
	delete[] sample_shape;
}


sample_t SfxrSynth::nextStringSample()
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
		float frac = sample_realindex - static_cast<int>(sample_realindex);

		sample = sample_shape[a]*(1-frac) + sample_shape[b]*(frac);

	} else {
		// No interpolation
		sample_index = static_cast<int>(sample_realindex);
		sample = sample_shape[sample_index];
	}

	// progress in shape
	sample_realindex += sample_step;

	return sample;
}




sfxrInstrument::sfxrInstrument( InstrumentTrack * _instrument_track ) :
	Instrument( _instrument_track, &sfxr_plugin_descriptor ),
	m_attModel(0.0f, this),
	m_holdModel(0.3f, this),
	m_susModel(0.0f, this),
	m_decModel(0.4f, this),

	m_startFreqModel(0.3f, this),
	m_minFreqModel(0.0f, this),
	m_slideModel(0.0f, this),
	m_dSlideModel(0.0f, this),
	m_vibDepthModel(0.0f, this),
	m_vibSpeedModel(0.0f, this),

	m_changeAmtModel(0.0f, this),
	m_changeSpeedModel(0.0f, this),

	m_sqrDutyModel(0.0f, this),
	m_sqrSweepModel(0.0f, this),

	m_repeatSpeedModel(0.0f, this),

	m_phaserOffsetModel(0.0f, this),
	m_phaserSweepModel(0.0f, this),

	m_lpFilCutModel(1.0f, this),
	m_lpFilCutSweepModel(0.0f, this),
	m_lpFilResoModel(0.0f, this),
	m_hpFilCutModel(0.0f, this),
	m_hpFilCutSweepModel(0.0f, this),
	m_waveFormModel( SQR_WAVE, 0, WAVES_NUM-1, this, tr( "Wave Form" ) )
{
	//TODO
	/*
	connect( &m_sampleLength, SIGNAL( dataChanged( ) ),
			this, SLOT( lengthChanged( ) ) );

	connect( &m_graph, SIGNAL( samplesChanged( int, int ) ),
			this, SLOT( samplesChanged( int, int ) ) );
	*/
}




sfxrInstrument::~sfxrInstrument()
{
}




void sfxrInstrument::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "version", "1" );
	m_attModel.saveSettings( _doc, _this, "att" );
	m_holdModel.saveSettings( _doc, _this, "hold" );
	m_susModel.saveSettings( _doc, _this, "sus" );
	m_decModel.saveSettings( _doc, _this, "dec" );

	m_startFreqModel.saveSettings( _doc, _this, "startFreq" );
	m_minFreqModel.saveSettings( _doc, _this, "minFreq" );
	m_slideModel.saveSettings( _doc, _this, "slide" );
	m_dSlideModel.saveSettings( _doc, _this, "dSlide" );
	m_vibDepthModel.saveSettings( _doc, _this, "vibDepth" );
	m_vibSpeedModel.saveSettings( _doc, _this, "vibSpeed" );

	m_changeAmtModel.saveSettings( _doc, _this, "changeAmt" );
	m_changeSpeedModel.saveSettings( _doc, _this, "changeSpeed" );

	m_sqrDutyModel.saveSettings( _doc, _this, "sqrDuty" );
	m_sqrSweepModel.saveSettings( _doc, _this, "sqrSweep" );

	m_repeatSpeedModel.saveSettings( _doc, _this, "repeatSpeed" );

	m_phaserOffsetModel.saveSettings( _doc, _this, "phaserOffset" );
	m_phaserSweepModel.saveSettings( _doc, _this, "phaserSweep" );

	m_lpFilCutModel.saveSettings( _doc, _this, "lpFilCut" );
	m_lpFilCutSweepModel.saveSettings( _doc, _this, "lpFilCutSweep" );
	m_lpFilResoModel.saveSettings( _doc, _this, "lpFilReso" );
	m_hpFilCutModel.saveSettings( _doc, _this, "hpFilCut" );
	m_hpFilCutSweepModel.saveSettings( _doc, _this, "hpFilCutSweep" );

	m_waveFormModel.saveSettings( _doc, _this, "waveForm" );
}




void sfxrInstrument::loadSettings( const QDomElement & _this )
{

	m_attModel.loadSettings(_this, "att" );
	m_holdModel.loadSettings( _this, "hold" );
	m_susModel.loadSettings( _this, "sus" );
	m_decModel.loadSettings( _this, "dec" );

	m_startFreqModel.loadSettings( _this, "startFreq" );
	m_minFreqModel.loadSettings( _this, "minFreq" );
	m_slideModel.loadSettings( _this, "slide" );
	m_dSlideModel.loadSettings( _this, "dSlide" );
	m_vibDepthModel.loadSettings( _this, "vibDepth" );
	m_vibSpeedModel.loadSettings( _this, "vibSpeed" );

	m_changeAmtModel.loadSettings( _this, "changeAmt" );
	m_changeSpeedModel.loadSettings( _this, "changeSpeed" );

	m_sqrDutyModel.loadSettings( _this, "sqrDuty" );
	m_sqrSweepModel.loadSettings( _this, "sqrSweep" );

	m_repeatSpeedModel.loadSettings( _this, "repeatSpeed" );

	m_phaserOffsetModel.loadSettings( _this, "phaserOffset" );
	m_phaserSweepModel.loadSettings( _this, "phaserSweep" );

	m_lpFilCutModel.loadSettings( _this, "lpFilCut" );
	m_lpFilCutSweepModel.loadSettings( _this, "lpFilCutSweep" );
	m_lpFilResoModel.loadSettings( _this, "lpFilReso" );
	m_hpFilCutModel.loadSettings( _this, "hpFilCut" );
	m_hpFilCutSweepModel.loadSettings( _this, "hpFilCutSweep" );

	m_waveFormModel.loadSettings( _this, "waveForm" );

}




void sfxrInstrument::samplesChanged( int _begin, int _end )
{
	//TODO
	//normalize();
	//engine::getSongEditor()->setModified();
}




QString sfxrInstrument::nodeName() const
{
	return( sfxr_plugin_descriptor.name );
}




f_cnt_t sfxrInstrument::desiredReleaseFrames() const
{
	//TODO: check whether this disables
	return 0;
}



void sfxrInstrument::playNote( notePlayHandle * _n,
						sampleFrame * _working_buffer )
{
	//TODO
	/*if ( _n->totalFramesPlayed() == 0 || _n->m_pluginData == NULL )
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

		_n->m_pluginData = new SfxrSynth(
					const_cast<float*>( m_graph.samples() ),
					m_graph.length(),
					_n,
					m_interpolation.value(), factor,
				engine::mixer()->processingSampleRate() );
	}

	const fpp_t frames = _n->framesLeftForCurrentPeriod();

	SfxrSynth * ps = static_cast<SfxrSynth *>( _n->m_pluginData );
	for( fpp_t frame = 0; frame < frames; ++frame )
	{
		const sample_t cur = ps->nextStringSample();
		for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
		{
			_working_buffer[frame][chnl] = cur;
		}
	}

	applyRelease( _working_buffer, _n );

	instrumentTrack()->processAudioBuffer( _working_buffer, frames, _n );*/
}




void sfxrInstrument::deleteNotePluginData( notePlayHandle * _n )
{
	delete static_cast<SfxrSynth *>( _n->m_pluginData );
}




PluginView * sfxrInstrument::instantiateView( QWidget * _parent )
{
	return( new sfxrInstrumentView( this, _parent ) );
}




void sfxrInstrument::resetModels()
{
	m_attModel.reset();
	m_holdModel.reset();
	m_susModel.reset();
	m_decModel.reset();

	m_startFreqModel.reset();
	m_minFreqModel.reset();
	m_slideModel.reset();
	m_dSlideModel.reset();
	m_vibDepthModel.reset();
	m_vibSpeedModel.reset();

	m_changeAmtModel.reset();
	m_changeSpeedModel.reset();

	m_sqrDutyModel.reset();
	m_sqrSweepModel.reset();

	m_repeatSpeedModel.reset();

	m_phaserOffsetModel.reset();
	m_phaserSweepModel.reset();

	m_lpFilCutModel.reset();
	m_lpFilCutSweepModel.reset();
	m_lpFilResoModel.reset();
	m_hpFilCutModel.reset();
	m_hpFilCutSweepModel.reset();

	m_waveFormModel.reset();
}




class sfxrKnob : public knob
{
public:
	sfxrKnob( QWidget * _parent ) :
			knob( knobStyled, _parent )
	{
		setFixedSize( 20, 20 );
		setCenterPointX( 10.0 );
		setCenterPointY( 10.0 );
		setTotalAngle( 270.0 );
		setLineWidth( 1 );
	}
};




#define createKnob(_knob, _x, _y, _name)\
	_knob = new sfxrKnob( this ); \
	_knob->setHintText( tr( _name ":" ), "" ); \
	_knob->move( _x, _y ); \
	toolTip::add( _knob, tr( _name ) );




#define createButton(_button, _x, _y, _name, _resName)\
	_button = new pixmapButton( this, tr( "Sine wave" ) );\
	_button->move( _x, _y );\
	_button->setActiveGraphic( embed::getIconPixmap( _resName "_active" ) );\
	_button->setInactiveGraphic( embed::getIconPixmap( _resName "_inactive" ) );\
	toolTip::add( _button, tr( _name ) );




#define createButtonLocalGraphic(_button, _x, _y, _name, _resName)\
	_button = new pixmapButton( this, tr( "Sine wave" ) );\
	_button->move( _x, _y );\
	_button->setActiveGraphic( PLUGIN_NAME::getIconPixmap( _resName "_active" ) );\
	_button->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( _resName "_inactive" ) );\
	toolTip::add( _button, tr( _name ) );




sfxrInstrumentView::sfxrInstrumentView( Instrument * _instrument,
					QWidget * _parent ) :
	InstrumentView( _instrument, _parent )
{
	srand(time(NULL));
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );

	createKnob(m_attKnob, 	KNOBS_BASE_X+KNOB_BLOCK_SIZE_X*0, KNOBS_BASE_Y+KNOB_BLOCK_SIZE_Y*0, "Attack Time");
	createKnob(m_holdKnob, 	KNOBS_BASE_X+KNOB_BLOCK_SIZE_X*1, KNOBS_BASE_Y+KNOB_BLOCK_SIZE_Y*0, "Sustain Time");
	createKnob(m_susKnob, 	KNOBS_BASE_X+KNOB_BLOCK_SIZE_X*2, KNOBS_BASE_Y+KNOB_BLOCK_SIZE_Y*0, "Sustain Punch");
	createKnob(m_decKnob, 	KNOBS_BASE_X+KNOB_BLOCK_SIZE_X*3, KNOBS_BASE_Y+KNOB_BLOCK_SIZE_Y*0, "Decay Time");

	createKnob(m_startFreqKnob,	KNOBS_BASE_X+KNOB_BLOCK_SIZE_X*0, KNOBS_BASE_Y+KNOB_BLOCK_SIZE_Y*1, "Start Frequency");
	createKnob(m_minFreqKnob, 	KNOBS_BASE_X+KNOB_BLOCK_SIZE_X*1, KNOBS_BASE_Y+KNOB_BLOCK_SIZE_Y*1, "Min Frequency");
	createKnob(m_slideKnob, 	KNOBS_BASE_X+KNOB_BLOCK_SIZE_X*2, KNOBS_BASE_Y+KNOB_BLOCK_SIZE_Y*1, "Slide");
	createKnob(m_dSlideKnob, 	KNOBS_BASE_X+KNOB_BLOCK_SIZE_X*3, KNOBS_BASE_Y+KNOB_BLOCK_SIZE_Y*1, "Delta Slide");
	createKnob(m_vibDepthKnob, 	KNOBS_BASE_X+KNOB_BLOCK_SIZE_X*4, KNOBS_BASE_Y+KNOB_BLOCK_SIZE_Y*1, "Vibrato Depth");
	createKnob(m_vibSpeedKnob, 	KNOBS_BASE_X+KNOB_BLOCK_SIZE_X*5, KNOBS_BASE_Y+KNOB_BLOCK_SIZE_Y*1, "Vibrato Speed");

	createKnob(m_changeAmtKnob, 	KNOBS_BASE_X+KNOB_BLOCK_SIZE_X*0, KNOBS_BASE_Y+KNOB_BLOCK_SIZE_Y*2, "Change Amount");
	createKnob(m_changeSpeedKnob, 	KNOBS_BASE_X+KNOB_BLOCK_SIZE_X*1, KNOBS_BASE_Y+KNOB_BLOCK_SIZE_Y*2, "Change Speed");

	createKnob(m_sqrDutyKnob, 	KNOBS_BASE_X+KNOB_BLOCK_SIZE_X*3, KNOBS_BASE_Y+KNOB_BLOCK_SIZE_Y*2, "Squre Duty(Square wave only)");
	createKnob(m_sqrSpeedKnob, 	KNOBS_BASE_X+KNOB_BLOCK_SIZE_X*4, KNOBS_BASE_Y+KNOB_BLOCK_SIZE_Y*2, "Squre Sweep(Square wave only)");

	createKnob(m_repeatSpeedKnob, 	KNOBS_BASE_X+KNOB_BLOCK_SIZE_X*0, KNOBS_BASE_Y+KNOB_BLOCK_SIZE_Y*3, "Repeat Speed");

	createKnob(m_phaserOffsetKnob, 	KNOBS_BASE_X+KNOB_BLOCK_SIZE_X*2, KNOBS_BASE_Y+KNOB_BLOCK_SIZE_Y*3, "Phaser Offset");
	createKnob(m_phaserSweepKnob, 	KNOBS_BASE_X+KNOB_BLOCK_SIZE_X*3, KNOBS_BASE_Y+KNOB_BLOCK_SIZE_Y*3, "Phaser Sweep");

	createKnob(m_lpFilCutKnob,		KNOBS_BASE_X+KNOB_BLOCK_SIZE_X*0, KNOBS_BASE_Y+KNOB_BLOCK_SIZE_Y*4, "LP Filter Cutoff");
	createKnob(m_lpFilCutSweepKnob, KNOBS_BASE_X+KNOB_BLOCK_SIZE_X*1, KNOBS_BASE_Y+KNOB_BLOCK_SIZE_Y*4, "LP Filter Cutoff Sweep");
	createKnob(m_lpFilResoKnob, 	KNOBS_BASE_X+KNOB_BLOCK_SIZE_X*2, KNOBS_BASE_Y+KNOB_BLOCK_SIZE_Y*4, "LP Filter Resonance");
	createKnob(m_hpFilCutKnob,		KNOBS_BASE_X+KNOB_BLOCK_SIZE_X*3, KNOBS_BASE_Y+KNOB_BLOCK_SIZE_Y*4, "HP Filter Cutoff");
	createKnob(m_hpFilCutSweepKnob, KNOBS_BASE_X+KNOB_BLOCK_SIZE_X*4, KNOBS_BASE_Y+KNOB_BLOCK_SIZE_Y*4, "HP Filter Cutoff Sweep");

	createButton(m_sqrWaveBtn,		KNOBS_BASE_X+WAVEFORM_BUTTON_WIDTH*0, WAVEFORM_BASE_Y, "Square Wave", "square_wave");
	createButton(m_sawWaveBtn,		KNOBS_BASE_X+WAVEFORM_BUTTON_WIDTH*1, WAVEFORM_BASE_Y, "Saw Wave", "saw_wave");
	createButton(m_sinWaveBtn,		KNOBS_BASE_X+WAVEFORM_BUTTON_WIDTH*2, WAVEFORM_BASE_Y, "Sine Wave", "sin_wave");
	createButton(m_noiseWaveBtn,	KNOBS_BASE_X+WAVEFORM_BUTTON_WIDTH*3, WAVEFORM_BASE_Y, "Noise", "white_noise_wave");

	m_waveBtnGroup = new automatableButtonGroup( this );
	m_waveBtnGroup->addButton(m_sqrWaveBtn);
	m_waveBtnGroup->addButton(m_sawWaveBtn);
	m_waveBtnGroup->addButton(m_sinWaveBtn);
	m_waveBtnGroup->addButton(m_noiseWaveBtn);
	connect( m_waveBtnGroup, SIGNAL ( dataChanged() ),
			 this, SLOT ( waveFormChanged() ) );


	createButtonLocalGraphic(m_pickupBtn,		GENERATOR_BASE_X+GENERATOR_BUTTON_WIDTH*0, GENERATOR_BASE_Y, "Generate pick up/coin sfx", "pickup");
	createButtonLocalGraphic(m_laserBtn,		GENERATOR_BASE_X+GENERATOR_BUTTON_WIDTH*1, GENERATOR_BASE_Y, "Generate laser/shoot sfx", "laser");
	createButtonLocalGraphic(m_explosionBtn,	GENERATOR_BASE_X+GENERATOR_BUTTON_WIDTH*2, GENERATOR_BASE_Y, "Generate explosion sfx", "explosion");
	createButtonLocalGraphic(m_powerupBtn,		GENERATOR_BASE_X+GENERATOR_BUTTON_WIDTH*3, GENERATOR_BASE_Y, "Generate power up sfx", "powerup");
	createButtonLocalGraphic(m_hitBtn,			GENERATOR_BASE_X+GENERATOR_BUTTON_WIDTH*4, GENERATOR_BASE_Y, "Generate hit/hurt sfx", "hit");
	createButtonLocalGraphic(m_jumpBtn,			GENERATOR_BASE_X+GENERATOR_BUTTON_WIDTH*5, GENERATOR_BASE_Y, "Generate jump sfx", "jump");
	createButtonLocalGraphic(m_blipBtn,			GENERATOR_BASE_X+GENERATOR_BUTTON_WIDTH*6, GENERATOR_BASE_Y, "Generate blip/select sfx", "blip");
	connect( m_pickupBtn, SIGNAL ( clicked() ), this, SLOT ( genPickup() ) );
	connect( m_laserBtn, SIGNAL ( clicked() ), this, SLOT ( genLaser() ) );
	connect( m_explosionBtn, SIGNAL ( clicked() ), this, SLOT ( genExplosion() ) );
	connect( m_powerupBtn, SIGNAL ( clicked() ), this, SLOT ( genPowerup() ) );
	connect( m_hitBtn, SIGNAL ( clicked() ), this, SLOT ( genHit() ) );
	connect( m_jumpBtn, SIGNAL ( clicked() ), this, SLOT ( genJump() ) );
	connect( m_blipBtn, SIGNAL ( clicked() ), this, SLOT ( genBlip() ) );


	createButtonLocalGraphic(m_randomizeBtn,	RAND_BUTTON_X, RAND_BUTTON_Y, "Generate random sfx", "randomize");
	createButtonLocalGraphic(m_mutateBtn,		MUTA_BUTTON_X, MUTA_BUTTON_Y, "Mutate sfx", "mutate");
	connect( m_randomizeBtn, SIGNAL ( clicked() ), this, SLOT ( randomize() ) );
	connect( m_mutateBtn, SIGNAL ( clicked() ), this, SLOT ( mutate() ) );

	//TODO: for each generator button:
	/*connect( m_pickupBtn, SIGNAL ( dataChanged() ),
			 this, SLOT ( pickupClicked() ) );*/


}




void sfxrInstrumentView::modelChanged()
{
	sfxrInstrument * s = castModel<sfxrInstrument>();

	m_attKnob->setModel( &s->m_attModel );
	m_holdKnob->setModel( &s->m_holdModel );
	m_susKnob->setModel( &s->m_susModel );
	m_decKnob->setModel( &s->m_decModel );

	m_startFreqKnob->setModel( &s->m_startFreqModel );
	m_minFreqKnob->setModel( &s->m_minFreqModel );
	m_slideKnob->setModel( &s->m_slideModel );
	m_dSlideKnob->setModel( &s->m_dSlideModel );
	m_vibDepthKnob->setModel( &s->m_vibDepthModel );
	m_vibSpeedKnob->setModel( &s->m_vibSpeedModel );

	m_changeAmtKnob->setModel( &s->m_changeAmtModel );
	m_changeSpeedKnob->setModel( &s->m_changeSpeedModel );

	m_sqrDutyKnob->setModel( &s->m_sqrDutyModel );
	m_sqrSpeedKnob->setModel( &s->m_sqrSweepModel );

	m_repeatSpeedKnob->setModel( &s->m_repeatSpeedModel );

	m_phaserOffsetKnob->setModel( &s->m_phaserOffsetModel );
	m_phaserSweepKnob->setModel( &s->m_phaserSweepModel );

	m_lpFilCutKnob->setModel( &s->m_lpFilCutModel );
	m_lpFilCutSweepKnob->setModel( &s->m_lpFilCutSweepModel );
	m_lpFilResoKnob->setModel( &s->m_lpFilResoModel );
	m_hpFilCutKnob->setModel( &s->m_hpFilCutModel );
	m_hpFilCutSweepKnob->setModel( &s->m_hpFilCutSweepModel );

	m_waveBtnGroup->setModel( &s->m_waveFormModel );
}




void sfxrInstrumentView::waveFormChanged()
{
	//TODO: do we even need this slot?
	/*
	m_graph->model()->setWaveToNoise();
	engine::getSong()->setModified();
	*/
}




void sfxrInstrumentView::genPickup()
{
	sfxrInstrument * s = castModel<sfxrInstrument>();
	s->resetModels();
	s->m_startFreqModel.setValue(	0.4f+frnd(0.5f)	);
	s->m_attModel.setValue(			0.0f			);
	s->m_holdModel.setValue(			frnd(0.1f)		);
	s->m_decModel.setValue(			0.1f+frnd(0.4f)	);
	s->m_susModel.setValue(			0.3f+frnd(0.3f)	);

	if(rnd(1))
	{
		s->m_changeAmtModel.setValue( 0.5f+frnd(0.2f) );
		s->m_changeSpeedModel.setValue( 0.2f+frnd(0.4f) );
	}
}




void sfxrInstrumentView::genLaser()
{
	sfxrInstrument * s = castModel<sfxrInstrument>();
	s->resetModels();

	s->m_waveFormModel.setValue( rnd(2) );
	if(s->m_waveFormModel.value()==2 && rnd(1))
		s->m_waveFormModel.setValue(rnd(1));

	s->m_startFreqModel.setValue(	0.5f+frnd(0.5f)	);
	s->m_minFreqModel.setValue(	s->m_startFreqModel.value()-0.2f-frnd(0.6f)	);

	if(s->m_minFreqModel.value()<0.2f)
	{
		s->m_minFreqModel.setValue(0.2f);
	}

	s->m_slideModel.setValue( -0.15f-frnd(0.2f) );

	if(rnd(2)==0)
	{
		s->m_startFreqModel.setValue( 0.3f+frnd(0.6f) );
		s->m_minFreqModel.setValue( frnd(0.1f) );
		s->m_slideModel.setValue(  -0.35f-frnd(0.3f) );
	}

	if(rnd(1))
	{
		s->m_sqrDutyModel.setValue( frnd(0.5f) );
		s->m_sqrSweepModel.setValue( 0.2f );
	}
	else
	{
		s->m_sqrDutyModel.setValue( 0.4f+frnd(0.5f) );
		s->m_sqrSweepModel.setValue( -frnd(0.7f) );
	}

	s->m_attModel.setValue( 0.0f );
	s->m_holdModel.setValue( 0.1f+frnd(0.2f) );
	s->m_decModel.setValue( frnd(0.4f) );

	if(rnd(1))
	{
		s->m_susModel.setValue( frnd(0.3f) );
	}

	if(rnd(2)==0)
	{
		s->m_phaserOffsetModel.setValue( frnd(0.2f) );
		s->m_phaserSweepModel.setValue( -frnd(0.2f) );
	}

	if(rnd(1))
		s->m_hpFilCutModel.setValue( frnd(0.3f) );
}




void sfxrInstrumentView::genExplosion()
{
	sfxrInstrument * s = castModel<sfxrInstrument>();
	s->resetModels();

	s->m_waveFormModel.setValue( 3 );

	if(rnd(1))
	{
		s->m_startFreqModel.setValue( 0.1f+frnd(0.4f) );
		s->m_slideModel.setValue( -0.1f+frnd(0.4f) );
	}
	else
	{
		s->m_startFreqModel.setValue( 0.2f+frnd(0.7f) );
		s->m_slideModel.setValue( -0.2f-frnd(0.2f) );
	}
	s->m_startFreqModel.setValue( s->m_startFreqModel.value()*s->m_startFreqModel.value() );

	if(rnd(4)==0)
	{
		s->m_slideModel.setValue( 0.0f );
	}

	if(rnd(2)==0)
	{
		s->m_repeatSpeedModel.setValue( 0.3f+frnd(0.5f) );
	}

	s->m_attModel.setValue( 0.0f );
	s->m_holdModel.setValue( 0.1f+frnd(0.3f) );
	s->m_decModel.setValue( 0.5f );
	if(rnd(1)==0)
	{
		s->m_phaserOffsetModel.setValue( -0.3f+frnd(0.9f) );
		s->m_phaserSweepModel.setValue( -frnd(0.3f) );
	}
	s->m_susModel.setValue( 0.2f+frnd(0.6f) );

	if(rnd(1))
	{
		s->m_vibDepthModel.setValue( frnd(0.7f) );
		s->m_vibSpeedModel.setValue( frnd(0.6f) );
	}
	if(rnd(2)==0)
	{
		s->m_changeSpeedModel.setValue( 0.6f+frnd(0.3f) );
		s->m_changeAmtModel.setValue( 0.8f-frnd(1.6f) );
	}

}




void sfxrInstrumentView::genPowerup()
{
	sfxrInstrument * s = castModel<sfxrInstrument>();
	s->resetModels();

	if(rnd(1))
		s->m_waveFormModel.setValue( 1 );
	else
		s->m_sqrDutyModel.setValue( frnd(0.6f) );
	if(rnd(1))
	{
		s->m_startFreqModel.setValue( 0.2f+frnd(0.3f) );
		s->m_slideModel.setValue( 0.1f+frnd(0.4f) );
		s->m_repeatSpeedModel.setValue( 0.4f+frnd(0.4f) );
	}
	else
	{
		s->m_startFreqModel.setValue( 0.2f+frnd(0.3f) );
		s->m_slideModel.setValue( 0.05f+frnd(0.2f) );
		if(rnd(1))
		{
			s->m_vibDepthModel.setValue( frnd(0.7f) );
			s->m_vibSpeedModel.setValue( frnd(0.6f) );
		}
	}

	s->m_attModel.setValue( 0.0f );
	s->m_holdModel.setValue( frnd(0.4f) );
	s->m_decModel.setValue( 0.1f+frnd(0.4f) );
}




void sfxrInstrumentView::genHit()
{
	sfxrInstrument * s = castModel<sfxrInstrument>();
	s->resetModels();

	s->m_waveFormModel.setValue( rnd(2) );
	if(s->m_waveFormModel.value()==2)
		s->m_waveFormModel.setValue( 3 );
	if(s->m_waveFormModel.value()==0)
		s->m_sqrDutyModel.setValue( frnd(0.6f) );

	s->m_startFreqModel.setValue( 0.2f+frnd(0.6f) );
	s->m_slideModel.setValue( -0.3f-frnd(0.4f) );

	s->m_attModel.setValue( 0.0f );
	s->m_holdModel.setValue( frnd(0.1f) );
	s->m_decModel.setValue( 0.1f+frnd(0.2f) );
	if(rnd(1))
	{
		s->m_hpFilCutModel.setValue( frnd(0.3f) );
	}
}




void sfxrInstrumentView::genJump()
{
	sfxrInstrument * s = castModel<sfxrInstrument>();
	s->resetModels();

	s->m_waveFormModel.setValue( 0 );
	s->m_sqrDutyModel.setValue( frnd(0.6f) );

	s->m_startFreqModel.setValue( 0.3f+frnd(0.3f) );
	s->m_slideModel.setValue( 0.1f+frnd(0.2f) );

	s->m_attModel.setValue( 0.0f );
	s->m_holdModel.setValue( 0.1f+frnd(0.3f) );
	s->m_decModel.setValue( 0.1f+frnd(0.2f) );

	if(rnd(1))
	{
		s->m_hpFilCutModel.setValue( frnd(0.3f) );
	}

	if(rnd(1))
	{

		s->m_lpFilCutModel.setValue( 1.0f-frnd(0.6f) );
	}
}




void sfxrInstrumentView::genBlip()
{
	sfxrInstrument * s = castModel<sfxrInstrument>();
	s->resetModels();

	s->m_waveFormModel.setValue( rnd(1) );
	if( s->m_waveFormModel.value()==0 )
	{
		s->m_sqrDutyModel.setValue( frnd(0.6f) );
	}

	s->m_startFreqModel.setValue( 0.2f+frnd(0.4f) );

	s->m_attModel.setValue( 0.0f );
	s->m_holdModel.setValue( 0.1f+frnd(0.1f) );
	s->m_decModel.setValue( frnd(0.2f) );
	s->m_hpFilCutModel.setValue( 0.1f );
}




void sfxrInstrumentView::randomize()
{
	sfxrInstrument * s = castModel<sfxrInstrument>();

	s->m_startFreqModel.setValue( pow(frnd(2.0f)-1.0f, 2.0f) );
	if(rnd(1))
	{
		s->m_startFreqModel.setValue( pow(frnd(2.0f)-1.0f, 3.0f)+0.5f );
	}
	s->m_minFreqModel.setValue( 0.0f );
	s->m_slideModel.setValue( pow(frnd(2.0f)-1.0f, 5.0f) );
	if( s->m_startFreqModel.value()>0.7f && s->m_slideModel.value()>0.2f )
	{
		s->m_slideModel.setValue( -s->m_slideModel.value() );
	}
	if( s->m_startFreqModel.value()<0.2f && s->m_slideModel.value()<-0.05f )
	{
		s->m_slideModel.setValue( -s->m_slideModel.value() );
	}
	s->m_dSlideModel.setValue( pow(frnd(2.0f)-1.0f, 3.0f) );

	s->m_sqrDutyModel.setValue( frnd(2.0f)-1.0f );
	s->m_sqrSweepModel.setValue( pow(frnd(2.0f)-1.0f, 3.0f) );

	s->m_vibDepthModel.setValue( pow(frnd(2.0f)-1.0f, 3.0f) );
	s->m_vibSpeedModel.setValue( frnd(2.0f)-1.0f );
	//s->m_vibDelayModel.setValue( frnd(2.0f)-1.0f );

	s->m_attModel.setValue( pow(frnd(2.0f)-1.0f, 3.0f) );
	s->m_holdModel.setValue( pow(frnd(2.0f)-1.0f, 2.0f) );
	s->m_decModel.setValue( frnd(2.0f)-1.0f );
	s->m_susModel.setValue( pow(frnd(0.8f), 2.0f) );
	if(s->m_attModel.value()+s->m_holdModel.value()+s->m_decModel.value()<0.2f)
	{
		s->m_holdModel.setValue( s->m_holdModel.value()+0.2f+frnd(0.3f) );
		s->m_decModel.setValue( s->m_decModel.value()+0.2f+frnd(0.3f) );
	}

	s->m_lpFilResoModel.setValue( frnd(2.0f)-1.0f );
	s->m_lpFilCutModel.setValue( 1.0f-pow(frnd(1.0f), 3.0f) );
	s->m_lpFilCutSweepModel.setValue( pow(frnd(2.0f)-1.0f, 3.0f) );
	if(s->m_lpFilCutModel.value()<0.1f && s->m_lpFilCutSweepModel.value()<-0.05f)
	{
		s->m_lpFilCutSweepModel.setValue( -s->m_lpFilCutSweepModel.value() );
	}
	s->m_hpFilCutModel.setValue( pow(frnd(1.0f), 5.0f) );
	s->m_hpFilCutSweepModel.setValue( pow(frnd(2.0f)-1.0f, 5.0f) );

	s->m_phaserOffsetModel.setValue( pow(frnd(2.0f)-1.0f, 3.0f) );
	s->m_phaserSweepModel.setValue( pow(frnd(2.0f)-1.0f, 3.0f) );

	s->m_repeatSpeedModel.setValue( frnd(2.0f)-1.0f );

	s->m_changeSpeedModel.setValue( frnd(2.0f)-1.0f );
	s->m_changeAmtModel.setValue( frnd(2.0f)-1.0f );

}




void sfxrInstrumentView::mutate()
{
	sfxrInstrument * s = castModel<sfxrInstrument>();

	if(rnd(1)) s->m_startFreqModel.setValue( s->m_startFreqModel.value()+frnd(0.1f)-0.05f );
	//		if(rnd(1)) s->m_minFreqModel.setValue( s->m_minFreqModel.value()+frnd(0.1f)-0.05f );
	if(rnd(1)) s->m_slideModel.setValue( s->m_slideModel.value()+frnd(0.1f)-0.05f );
	if(rnd(1)) s->m_dSlideModel.setValue( s->m_dSlideModel.value()+frnd(0.1f)-0.05f );

	if(rnd(1)) s->m_sqrDutyModel.setValue( s->m_sqrDutyModel.value()+frnd(0.1f)-0.05f );
	if(rnd(1)) s->m_sqrSweepModel.setValue( s->m_sqrSweepModel.value()+frnd(0.1f)-0.05f );

	if(rnd(1)) s->m_vibDepthModel.setValue( s->m_vibDepthModel.value()+frnd(0.1f)-0.05f );
	if(rnd(1)) s->m_vibSpeedModel.setValue( s->m_vibSpeedModel.value()+frnd(0.1f)-0.05f );
	//		if(rnd(1)) s->m_vibDelayModel.setValue( s->m_vibDelayModel.value()+frnd(0.1f)-0.05f );

	if(rnd(1)) s->m_attModel.setValue( s->m_attModel.value()+frnd(0.1f)-0.05f );
	if(rnd(1)) s->m_holdModel.setValue( s->m_holdModel.value()+frnd(0.1f)-0.05f );
	if(rnd(1)) s->m_decModel.setValue( s->m_decModel.value()+frnd(0.1f)-0.05f );
	if(rnd(1)) s->m_susModel.setValue( s->m_susModel.value()+frnd(0.1f)-0.05f );

	if(rnd(1)) s->m_lpFilResoModel.setValue( s->m_lpFilResoModel.value()+frnd(0.1f)-0.05f );
	if(rnd(1)) s->m_lpFilCutModel.setValue( s->m_lpFilCutModel.value()+frnd(0.1f)-0.05f );
	if(rnd(1)) s->m_lpFilCutSweepModel.setValue( s->m_lpFilCutSweepModel.value()+frnd(0.1f)-0.05f );
	if(rnd(1)) s->m_hpFilCutModel.setValue( s->m_hpFilCutModel.value()+frnd(0.1f)-0.05f );
	if(rnd(1)) s->m_hpFilCutSweepModel.setValue( s->m_hpFilCutSweepModel.value()+frnd(0.1f)-0.05f );

	if(rnd(1)) s->m_phaserOffsetModel.setValue( s->m_phaserOffsetModel.value()+frnd(0.1f)-0.05f );
	if(rnd(1)) s->m_phaserSweepModel.setValue( s->m_phaserSweepModel.value()+frnd(0.1f)-0.05f );

	if(rnd(1)) s->m_repeatSpeedModel.setValue( s->m_repeatSpeedModel.value()+frnd(0.1f)-0.05f );

	if(rnd(1)) s->m_changeSpeedModel.setValue( s->m_changeSpeedModel.value()+frnd(0.1f)-0.05f );
	if(rnd(1)) s->m_changeAmtModel.setValue( s->m_changeAmtModel.value()+frnd(0.1f)-0.05f );

}




extern "C"
{

// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model *, void * _data )
{
	return( new sfxrInstrument( static_cast<InstrumentTrack *>( _data ) ) );
}


}



#include "moc_sfxr.cxx"
