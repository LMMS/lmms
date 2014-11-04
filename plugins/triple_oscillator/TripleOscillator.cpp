/*
 * TripleOscillator.cpp - powerful instrument with three oscillators
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include <QtGui/QBitmap>
#include <QtGui/QPainter>

#include "TripleOscillator.h"
#include "automatable_button.h"
#include "debug.h"
#include "engine.h"
#include "InstrumentTrack.h"
#include "knob.h"
#include "NotePlayHandle.h"
#include "pixmap_button.h"
#include "SampleBuffer.h"
#include "tooltip.h"

#include "embed.cpp"


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT tripleoscillator_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"TripleOscillator",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"Three powerful oscillators you can modulate "
				"in several ways" ),
	"Tobias Doerffel <tobydox/at/users.sf.net>",
	0x0110,
	Plugin::Instrument,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
} ;

}



OscillatorObject::OscillatorObject( Model * _parent, int _idx ) :
	Model( _parent ),
	m_volumeModel( DefaultVolume / NUM_OF_OSCILLATORS, MinVolume,
			MaxVolume, 1.0f, this, tr( "Osc %1 volume" ).arg( _idx+1 ) ),
	m_panModel( DefaultPanning, PanningLeft, PanningRight, 1.0f, this,
			tr( "Osc %1 panning" ).arg( _idx+1 ) ),
	m_coarseModel( -_idx*KeysPerOctave,
			-2 * KeysPerOctave, 2 * KeysPerOctave, 1.0f, this,
			tr( "Osc %1 coarse detuning" ).arg( _idx+1 ) ),
	m_fineLeftModel( 0.0f, -100.0f, 100.0f, 1.0f, this,
			tr( "Osc %1 fine detuning left" ).arg( _idx+1 ) ),
	m_fineRightModel( 0.0f, -100.0f, 100.0f, 1.0f, this,
			tr( "Osc %1 fine detuning right" ).arg( _idx + 1 ) ),
	m_phaseOffsetModel( 0.0f, 0.0f, 360.0f, 1.0f, this,
			tr( "Osc %1 phase-offset" ).arg( _idx+1 ) ), 
	m_stereoPhaseDetuningModel( 0.0f, 0.0f, 360.0f, 1.0f, this,
			tr( "Osc %1 stereo phase-detuning" ).arg( _idx+1 ) ),
	m_waveShapeModel( Oscillator::SineWave, 0, 
			Oscillator::NumWaveShapes-1, this,
			tr( "Osc %1 wave shape" ).arg( _idx+1 ) ),
	m_modulationAlgoModel( Oscillator::SignalMix, 0,
				Oscillator::NumModulationAlgos-1, this,
				tr( "Modulation type %1" ).arg( _idx+1 ) ),

	m_sampleBuffer( new SampleBuffer ),
	m_volumeLeft( 0.0f ),
	m_volumeRight( 0.0f ),
	m_detuningLeft( 0.0f ),
	m_detuningRight( 0.0f ),
	m_phaseOffsetLeft( 0.0f ),
	m_phaseOffsetRight( 0.0f )
{
	// Connect knobs with Oscillators' inputs
	connect( &m_volumeModel, SIGNAL( dataChanged() ),
					this, SLOT( updateVolume() ) );
	connect( &m_panModel, SIGNAL( dataChanged() ),
					this, SLOT( updateVolume() ) );
	updateVolume();

	connect( &m_coarseModel, SIGNAL( dataChanged() ),
				this, SLOT( updateDetuningLeft() ) );
	connect( &m_coarseModel, SIGNAL( dataChanged() ),
				this, SLOT( updateDetuningRight() ) );
	connect( &m_fineLeftModel, SIGNAL( dataChanged() ),
				this, SLOT( updateDetuningLeft() ) );
	connect( &m_fineRightModel, SIGNAL( dataChanged() ),
				this, SLOT( updateDetuningRight() ) );
	updateDetuningLeft();
	updateDetuningRight();

	connect( &m_phaseOffsetModel, SIGNAL( dataChanged() ),
			this, SLOT( updatePhaseOffsetLeft() ) );
	connect( &m_phaseOffsetModel, SIGNAL( dataChanged() ),
			this, SLOT( updatePhaseOffsetRight() ) );
	connect( &m_stereoPhaseDetuningModel, SIGNAL( dataChanged() ),
			this, SLOT( updatePhaseOffsetLeft() ) );
	updatePhaseOffsetLeft();
	updatePhaseOffsetRight();

}




OscillatorObject::~OscillatorObject()
{
	sharedObject::unref( m_sampleBuffer );
}




void OscillatorObject::oscUserDefWaveDblClick()
{
	QString af = m_sampleBuffer->openAndSetWaveformFile();
	if( af != "" )
	{
		// TODO:
		//toolTip::add( m_usrWaveBtn, m_sampleBuffer->audioFile() );
	}
}




void OscillatorObject::updateVolume()
{
	if( m_panModel.value() >= 0.0f )
	{
		const float panningFactorLeft = 1.0f - m_panModel.value()
							/ (float)PanningRight;
		m_volumeLeft = panningFactorLeft * m_volumeModel.value() /
									100.0f;
		m_volumeRight = m_volumeModel.value() / 100.0f;
	}
	else
	{
		m_volumeLeft = m_volumeModel.value() / 100.0f;
		const float panningFactorRight = 1.0f + m_panModel.value()
							/ (float)PanningRight;
		m_volumeRight = panningFactorRight * m_volumeModel.value() /
									100.0f;
	}
}




void OscillatorObject::updateDetuningLeft()
{
	m_detuningLeft = powf( 2.0f, ( (float)m_coarseModel.value() * 100.0f
				+ (float)m_fineLeftModel.value() ) / 1200.0f )
				/ engine::mixer()->processingSampleRate();
}




void OscillatorObject::updateDetuningRight()
{
	m_detuningRight = powf( 2.0f, ( (float)m_coarseModel.value() * 100.0f
				+ (float)m_fineRightModel.value() ) / 1200.0f )
				/ engine::mixer()->processingSampleRate();
}




void OscillatorObject::updatePhaseOffsetLeft()
{
	m_phaseOffsetLeft = ( m_phaseOffsetModel.value() +
				m_stereoPhaseDetuningModel.value() ) / 360.0f;
}




void OscillatorObject::updatePhaseOffsetRight()
{
	m_phaseOffsetRight = m_phaseOffsetModel.value() / 360.0f;
}


 

TripleOscillator::TripleOscillator( InstrumentTrack * _instrument_track ) :
	Instrument( _instrument_track, &tripleoscillator_plugin_descriptor )
{
	for( int i = 0; i < NUM_OF_OSCILLATORS; ++i )
	{
		m_osc[i] = new OscillatorObject( this, i );

	}

	connect( engine::mixer(), SIGNAL( sampleRateChanged() ),
			this, SLOT( updateAllDetuning() ) );
}




TripleOscillator::~TripleOscillator()
{
}




void TripleOscillator::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	for( int i = 0; i < NUM_OF_OSCILLATORS; ++i )
	{
		QString is = QString::number( i );
		m_osc[i]->m_volumeModel.saveSettings( _doc, _this, "vol" + is );
		m_osc[i]->m_panModel.saveSettings( _doc, _this, "pan" + is );
		m_osc[i]->m_coarseModel.saveSettings( _doc, _this, "coarse"
									+ is );
		m_osc[i]->m_fineLeftModel.saveSettings( _doc, _this, "finel" +
									is );
		m_osc[i]->m_fineRightModel.saveSettings( _doc, _this, "finer" +
									is );
		m_osc[i]->m_phaseOffsetModel.saveSettings( _doc, _this,
							"phoffset" + is );
		m_osc[i]->m_stereoPhaseDetuningModel.saveSettings( _doc, _this,
							"stphdetun" + is );
		m_osc[i]->m_waveShapeModel.saveSettings( _doc, _this,
							"wavetype" + is );
		m_osc[i]->m_modulationAlgoModel.saveSettings( _doc, _this,
					"modalgo" + QString::number( i+1 ) );
		_this.setAttribute( "userwavefile" + is,
					m_osc[i]->m_sampleBuffer->audioFile() );
	}
}




void TripleOscillator::loadSettings( const QDomElement & _this )
{
	for( int i = 0; i < NUM_OF_OSCILLATORS; ++i )
	{
		const QString is = QString::number( i );
		m_osc[i]->m_volumeModel.loadSettings( _this, "vol" + is );
		m_osc[i]->m_panModel.loadSettings( _this, "pan" + is );
		m_osc[i]->m_coarseModel.loadSettings( _this, "coarse" + is );
		m_osc[i]->m_fineLeftModel.loadSettings( _this, "finel" + is );
		m_osc[i]->m_fineRightModel.loadSettings( _this, "finer" + is );
		m_osc[i]->m_phaseOffsetModel.loadSettings( _this,
							"phoffset" + is );
		m_osc[i]->m_stereoPhaseDetuningModel.loadSettings( _this,
							"stphdetun" + is );
		m_osc[i]->m_waveShapeModel.loadSettings( _this, "wavetype" +
									is );
		m_osc[i]->m_modulationAlgoModel.loadSettings( _this,
					"modalgo" + QString::number( i+1 ) );
		m_osc[i]->m_sampleBuffer->setAudioFile( _this.attribute(
							"userwavefile" + is ) );
	}
}




QString TripleOscillator::nodeName() const
{
	return( tripleoscillator_plugin_descriptor.name );
}




void TripleOscillator::playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer )
{
	if( _n->totalFramesPlayed() == 0 || _n->m_pluginData == NULL )
	{
		Oscillator * oscs_l[NUM_OF_OSCILLATORS];
		Oscillator * oscs_r[NUM_OF_OSCILLATORS];

		for( int i = NUM_OF_OSCILLATORS - 1; i >= 0; --i )
		{

			// the last oscs needs no sub-oscs...
			if( i == NUM_OF_OSCILLATORS - 1 )
			{
				oscs_l[i] = new Oscillator(
						&m_osc[i]->m_waveShapeModel,
						&m_osc[i]->m_modulationAlgoModel,
						_n->frequency(),
						m_osc[i]->m_detuningLeft,
						m_osc[i]->m_phaseOffsetLeft,
						m_osc[i]->m_volumeLeft );
				oscs_r[i] = new Oscillator(
						&m_osc[i]->m_waveShapeModel,
						&m_osc[i]->m_modulationAlgoModel,
						_n->frequency(),
						m_osc[i]->m_detuningRight,
						m_osc[i]->m_phaseOffsetRight,
						m_osc[i]->m_volumeRight );
			}
			else
			{
				oscs_l[i] = new Oscillator(
						&m_osc[i]->m_waveShapeModel,
						&m_osc[i]->m_modulationAlgoModel,
						_n->frequency(),
						m_osc[i]->m_detuningLeft,
						m_osc[i]->m_phaseOffsetLeft,
						m_osc[i]->m_volumeLeft,
						oscs_l[i + 1] );
				oscs_r[i] = new Oscillator(
						&m_osc[i]->m_waveShapeModel,
						&m_osc[i]->m_modulationAlgoModel,
						_n->frequency(),
						m_osc[i]->m_detuningRight,
						m_osc[i]->m_phaseOffsetRight,
						m_osc[i]->m_volumeRight,
						oscs_r[i + 1] );
			}

			oscs_l[i]->setUserWave( m_osc[i]->m_sampleBuffer );
			oscs_r[i]->setUserWave( m_osc[i]->m_sampleBuffer );

		}

		_n->m_pluginData = new oscPtr;
		static_cast<oscPtr *>( _n->m_pluginData )->oscLeft = oscs_l[0];
		static_cast< oscPtr *>( _n->m_pluginData )->oscRight =
								oscs_r[0];
	}

	Oscillator * osc_l = static_cast<oscPtr *>( _n->m_pluginData )->oscLeft;
	Oscillator * osc_r = static_cast<oscPtr *>( _n->m_pluginData )->oscRight;

	const fpp_t frames = _n->framesLeftForCurrentPeriod();

	osc_l->update( _working_buffer, frames, 0 );
	osc_r->update( _working_buffer, frames, 1 );

	applyRelease( _working_buffer, _n );

	instrumentTrack()->processAudioBuffer( _working_buffer, frames, _n );
}




void TripleOscillator::deleteNotePluginData( NotePlayHandle * _n )
{
	delete static_cast<Oscillator *>( static_cast<oscPtr *>(
						_n->m_pluginData )->oscLeft );
	delete static_cast<Oscillator *>( static_cast<oscPtr *>(
						_n->m_pluginData )->oscRight );
	delete static_cast<oscPtr *>( _n->m_pluginData );
}




PluginView * TripleOscillator::instantiateView( QWidget * _parent )
{
	return new TripleOscillatorView( this, _parent );
}




void TripleOscillator::updateAllDetuning()
{
	for( int i = 0; i < NUM_OF_OSCILLATORS; ++i )
	{
		m_osc[i]->updateDetuningLeft();
		m_osc[i]->updateDetuningRight();
	}
}




class TripleOscKnob : public knob
{
public:
	TripleOscKnob( QWidget * _parent ) :
			knob( knobStyled, _parent )
	{
		setFixedSize( 28, 35 );
	}
};

// 82, 109


TripleOscillatorView::TripleOscillatorView( Instrument * _instrument,
							QWidget * _parent ) :
	InstrumentView( _instrument, _parent )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(),
				PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );

	const int mod_x = 66;
	const int mod1_y = 58;
	const int mod2_y = 75;
	const int osc_y = 109;
	const int osc_h = 52;

	// TODO: clean rewrite using layouts and all that...
	pixmapButton * pm_osc1_btn = new pixmapButton( this, NULL );
	pm_osc1_btn->move( mod_x, mod1_y );
	pm_osc1_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"pm_active" ) );
	pm_osc1_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"pm_inactive" ) );
	toolTip::add( pm_osc1_btn, tr( "Use phase modulation for "
					"modulating oscillator 2 with "
					"oscillator 1" ) );

	pixmapButton * am_osc1_btn = new pixmapButton( this, NULL );
	am_osc1_btn->move( mod_x + 35, mod1_y );
	am_osc1_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"am_active" ) );
	am_osc1_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"am_inactive" ) );
	toolTip::add( am_osc1_btn, tr( "Use amplitude modulation for "
					"modulating oscillator 2 with "
					"oscillator 1" ) );

	pixmapButton * mix_osc1_btn = new pixmapButton( this, NULL );
	mix_osc1_btn->move( mod_x + 70, mod1_y );
	mix_osc1_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"mix_active" ) );
	mix_osc1_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"mix_inactive" ) );
	toolTip::add( mix_osc1_btn, tr( "Mix output of oscillator 1 & 2" ) );

	pixmapButton * sync_osc1_btn = new pixmapButton( this, NULL );
	sync_osc1_btn->move( mod_x + 105, mod1_y );
	sync_osc1_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"sync_active" ) );
	sync_osc1_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"sync_inactive" ) );
	toolTip::add( sync_osc1_btn, tr( "Synchronize oscillator 1 with "
							"oscillator 2" ) );

	pixmapButton * fm_osc1_btn = new pixmapButton( this, NULL );
	fm_osc1_btn->move( mod_x + 140, mod1_y );
	fm_osc1_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"fm_active" ) );
	fm_osc1_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"fm_inactive" ) );
	toolTip::add( fm_osc1_btn, tr( "Use frequency modulation for "
					"modulating oscillator 2 with "
					"oscillator 1" ) );

	m_mod1BtnGrp = new automatableButtonGroup( this );
	m_mod1BtnGrp->addButton( pm_osc1_btn );
	m_mod1BtnGrp->addButton( am_osc1_btn );
	m_mod1BtnGrp->addButton( mix_osc1_btn );
	m_mod1BtnGrp->addButton( sync_osc1_btn );
	m_mod1BtnGrp->addButton( fm_osc1_btn );



	pixmapButton * pm_osc2_btn = new pixmapButton( this, NULL );
	pm_osc2_btn->move( mod_x, mod2_y );
	pm_osc2_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"pm_active" ) );
	pm_osc2_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"pm_inactive" ) );
	toolTip::add( pm_osc2_btn, tr( "Use phase modulation for "
					"modulating oscillator 3 with "
					"oscillator 2" ) );

	pixmapButton * am_osc2_btn = new pixmapButton( this, NULL );
	am_osc2_btn->move( mod_x + 35, mod2_y );
	am_osc2_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"am_active" ) );
	am_osc2_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"am_inactive" ) );
	toolTip::add( am_osc2_btn, tr( "Use amplitude modulation for "
					"modulating oscillator 3 with "
					"oscillator 2" ) );

	pixmapButton * mix_osc2_btn = new pixmapButton( this, NULL );
	mix_osc2_btn->move( mod_x + 70, mod2_y );
	mix_osc2_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"mix_active" ) );
	mix_osc2_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"mix_inactive" ) );
	toolTip::add( mix_osc2_btn, tr("Mix output of oscillator 2 & 3" ) );

	pixmapButton * sync_osc2_btn = new pixmapButton( this, NULL );
	sync_osc2_btn->move( mod_x + 105, mod2_y );
	sync_osc2_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"sync_active" ) );
	sync_osc2_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"sync_inactive" ) );
	toolTip::add( sync_osc2_btn, tr( "Synchronize oscillator 2 with "
							"oscillator 3" ) );

	pixmapButton * fm_osc2_btn = new pixmapButton( this, NULL );
	fm_osc2_btn->move( mod_x + 140, mod2_y );
	fm_osc2_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"fm_active" ) );
	fm_osc2_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"fm_inactive" ) );
	toolTip::add( fm_osc2_btn, tr( "Use frequency modulation for "
					"modulating oscillator 3 with "
					"oscillator 2" ) );

	m_mod2BtnGrp = new automatableButtonGroup( this );

	m_mod2BtnGrp->addButton( pm_osc2_btn );
	m_mod2BtnGrp->addButton( am_osc2_btn );
	m_mod2BtnGrp->addButton( mix_osc2_btn );
	m_mod2BtnGrp->addButton( sync_osc2_btn );
	m_mod2BtnGrp->addButton( fm_osc2_btn );


	for( int i = 0; i < NUM_OF_OSCILLATORS; ++i )
	{
		int knob_y = osc_y + i * osc_h;

		// setup volume-knob
		knob * vk = new knob( knobStyled, this );
		vk->setVolumeKnob( true );
		vk->setFixedSize( 28, 35 );
		vk->move( 6, knob_y );
		vk->setHintText( tr( "Osc %1 volume:" ).arg(
							i+1 ) + " ", "%" );
		vk->setWhatsThis(
			tr( "With this knob you can set the volume of "
				"oscillator %1. When setting a value of 0 the "
				"oscillator is turned off. Otherwise you can "
				"hear the oscillator as loud as you set it "
				"here.").arg( i+1 ) );

		// setup panning-knob
		knob * pk = new TripleOscKnob( this );
		pk->move( 35, knob_y );
		pk->setHintText( tr("Osc %1 panning:").arg( i + 1 ) + " ", "" );
		pk->setWhatsThis(
			tr( "With this knob you can set the panning of the "
				"oscillator %1. A value of -100 means 100% "
				"left and a value of 100 moves oscillator-"
				"output right.").arg( i+1 ) );

		// setup coarse-knob
		knob * ck = new TripleOscKnob( this );
		ck->move( 82, knob_y );
		ck->setHintText( tr( "Osc %1 coarse detuning:" ).arg( i + 1 ) +
						" ", " " + tr( "semitones" ) );
		ck->setWhatsThis(
			tr( "With this knob you can set the coarse detuning of "
				"oscillator %1. You can detune the oscillator "
				"12 semitones (1 octave) up and down. This is "
				"useful for creating sounds with a chord." ).
				arg( i + 1 ) );

		
		// setup knob for left fine-detuning
		knob * flk = new TripleOscKnob( this );
		flk->move( 111, knob_y );
		flk->setHintText( tr( "Osc %1 fine detuning left:" ).
						arg( i + 1 ) + " ",
							" " + tr( "cents" ) );
		flk->setWhatsThis(
			tr( "With this knob you can set the fine detuning of "
				"oscillator %1 for the left channel. The fine-"
				"detuning is ranged between -100 cents and "
				"+100 cents. This is useful for creating "
				"\"fat\" sounds." ).arg( i + 1 ) );

		// setup knob for right fine-detuning
		knob * frk = new TripleOscKnob( this );
		frk->move( 140, knob_y );
		frk->setHintText( tr( "Osc %1 fine detuning right:" ).
						arg( i + 1 ) + " ",
							" " + tr( "cents" ) );
		frk->setWhatsThis(
			tr( "With this knob you can set the fine detuning of "
				"oscillator %1 for the right channel. The "
				"fine-detuning is ranged between -100 cents "
				"and +100 cents. This is useful for creating "
				"\"fat\" sounds." ).arg( i+1 ) );


		// setup phase-offset-knob
		knob * pok = new TripleOscKnob( this );
		pok->move( 188, knob_y );
		pok->setHintText( tr( "Osc %1 phase-offset:" ).
						arg( i + 1 ) + " ",
							" " + tr( "degrees" ) );
		pok->setWhatsThis(
			tr( "With this knob you can set the phase-offset of "
				"oscillator %1. That means you can move the "
				"point within an oscillation where the "
				"oscillator begins to oscillate. For example "
				"if you have a sine-wave and have a phase-"
				"offset of 180 degrees the wave will first go "
				"down. It's the same with a square-wave."
				).arg( i+1 ) );

		// setup stereo-phase-detuning-knob
		knob * spdk = new TripleOscKnob( this );
		spdk->move( 217, knob_y );
		spdk->setHintText( tr("Osc %1 stereo phase-detuning:" ).
					arg( i + 1 ) + " ",
							" " + tr( "degrees" ) );
		spdk->setWhatsThis(
			tr( "With this knob you can set the stereo phase-"
				"detuning of oscillator %1. The stereo phase-"
				"detuning specifies the size of the difference "
				"between the phase-offset of left and right "
				"channel. This is very good for creating wide "
				"stereo sounds." ).arg( i+1 ) );

		int btn_y = 96 + i * osc_h;

		pixmapButton * sin_wave_btn = new pixmapButton( this, NULL );
		sin_wave_btn->move( 128, btn_y );
		sin_wave_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"sin_shape_active" ) );
		sin_wave_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"sin_shape_inactive" ) );
		toolTip::add( sin_wave_btn,
				tr( "Use a sine-wave for "
						"current oscillator." ) );

		pixmapButton * triangle_wave_btn =
						new pixmapButton( this, NULL );
		triangle_wave_btn->move( 143, btn_y );
		triangle_wave_btn->setActiveGraphic(
			PLUGIN_NAME::getIconPixmap( "triangle_shape_active" ) );
		triangle_wave_btn->setInactiveGraphic(
			PLUGIN_NAME::getIconPixmap( "triangle_shape_inactive" ) );
		toolTip::add( triangle_wave_btn,
				tr( "Use a triangle-wave "
						"for current oscillator." ) );

		pixmapButton * saw_wave_btn = new pixmapButton( this, NULL );
		saw_wave_btn->move( 158, btn_y );
		saw_wave_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"saw_shape_active" ) );
		saw_wave_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"saw_shape_inactive" ) );
		toolTip::add( saw_wave_btn,
				tr( "Use a saw-wave for "
						"current oscillator." ) );

		pixmapButton * sqr_wave_btn = new pixmapButton( this, NULL );
		sqr_wave_btn->move( 173, btn_y );
		sqr_wave_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
						"square_shape_active" ) );
		sqr_wave_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
						"square_shape_inactive" ) );
		toolTip::add( sqr_wave_btn,
				tr( "Use a square-wave for "
						"current oscillator." ) );

		pixmapButton * moog_saw_wave_btn =
						new pixmapButton( this, NULL );
		moog_saw_wave_btn->move( 188, btn_y );
		moog_saw_wave_btn->setActiveGraphic(
			PLUGIN_NAME::getIconPixmap( "moog_saw_shape_active" ) );
		moog_saw_wave_btn->setInactiveGraphic(
			PLUGIN_NAME::getIconPixmap( "moog_saw_shape_inactive" ) );
		toolTip::add( moog_saw_wave_btn,
				tr( "Use a moog-like saw-wave "
						"for current oscillator." ) );

		pixmapButton * exp_wave_btn = new pixmapButton( this, NULL );
		exp_wave_btn->move( 203, btn_y );
		exp_wave_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"exp_shape_active" ) );
		exp_wave_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"exp_shape_inactive" ) );
		toolTip::add( exp_wave_btn,
				tr( "Use an exponential "
					"wave for current oscillator." ) );

		pixmapButton * white_noise_btn = new pixmapButton( this, NULL );
		white_noise_btn->move( 218, btn_y );
		white_noise_btn->setActiveGraphic(
			PLUGIN_NAME::getIconPixmap( "white_noise_shape_active" ) );
		white_noise_btn->setInactiveGraphic(
			PLUGIN_NAME::getIconPixmap( "white_noise_shape_inactive" ) );
		toolTip::add( white_noise_btn,
				tr( "Use white-noise for "
						"current oscillator." ) );

		pixmapButton * uwb = new pixmapButton( this, NULL );
		uwb->move( 233, btn_y );
		uwb->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"usr_shape_active" ) );
		uwb->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"usr_shape_inactive" ) );
		toolTip::add( uwb, tr( "Use a user-defined "
				"waveform for current oscillator." ) );

		automatableButtonGroup * wsbg =
			new automatableButtonGroup( this );

		wsbg->addButton( sin_wave_btn );
		wsbg->addButton( triangle_wave_btn );
		wsbg->addButton( saw_wave_btn );
		wsbg->addButton( sqr_wave_btn );
		wsbg->addButton( moog_saw_wave_btn );
		wsbg->addButton( exp_wave_btn );
		wsbg->addButton( white_noise_btn );
		wsbg->addButton( uwb );

		m_oscKnobs[i] = OscillatorKnobs( vk, pk, ck, flk, frk, pok,
							spdk, uwb, wsbg );
	}
}




TripleOscillatorView::~TripleOscillatorView()
{
}




void TripleOscillatorView::modelChanged()
{
	TripleOscillator * t = castModel<TripleOscillator>();
	m_mod1BtnGrp->setModel( &t->m_osc[0]->m_modulationAlgoModel );
	m_mod2BtnGrp->setModel( &t->m_osc[1]->m_modulationAlgoModel );

	for( int i = 0; i < NUM_OF_OSCILLATORS; ++i )
	{
		m_oscKnobs[i].m_volKnob->setModel(
					&t->m_osc[i]->m_volumeModel );
		m_oscKnobs[i].m_panKnob->setModel(
					&t->m_osc[i]->m_panModel );
		m_oscKnobs[i].m_coarseKnob->setModel(
					&t->m_osc[i]->m_coarseModel );
		m_oscKnobs[i].m_fineLeftKnob->setModel(
					&t->m_osc[i]->m_fineLeftModel );
		m_oscKnobs[i].m_fineRightKnob->setModel(
					&t->m_osc[i]->m_fineRightModel );
		m_oscKnobs[i].m_phaseOffsetKnob->setModel(
					&t->m_osc[i]->m_phaseOffsetModel );
		m_oscKnobs[i].m_stereoPhaseDetuningKnob->setModel(
				&t->m_osc[i]->m_stereoPhaseDetuningModel );
		m_oscKnobs[i].m_waveShapeBtnGrp->setModel(
					&t->m_osc[i]->m_waveShapeModel );
		connect( m_oscKnobs[i].m_userWaveButton,
						SIGNAL( doubleClicked() ),
				t->m_osc[i], SLOT( oscUserDefWaveDblClick() ) );
	}
}




extern "C"
{

// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model *, void * _data )
{
	return new TripleOscillator( static_cast<InstrumentTrack *>( _data ) );
}

}


#include "moc_TripleOscillator.cxx"

