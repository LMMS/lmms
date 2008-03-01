/*
 * triple_oscillator.cpp - powerful instrument with three oscillators
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 

#include <Qt/QtXml>
#include <QtGui/QBitmap>
#include <QtGui/QPainter>

#include "triple_oscillator.h"
#include "automatable_button.h"
#include "debug.h"
#include "engine.h"
#include "instrument_track.h"
#include "knob.h"
#include "note_play_handle.h"
#include "pixmap_button.h"
#include "sample_buffer.h"
#include "song_editor.h"
#include "tooltip.h"
#include "volume_knob.h"
#include "automatable_model_templates.h"


#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"


extern "C"
{

plugin::descriptor tripleoscillator_plugin_descriptor =
{
	STRINGIFY_PLUGIN_NAME( PLUGIN_NAME ),
	"TripleOscillator",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"three powerful oscillators you can modulate "
				"in several ways" ),
	"Tobias Doerffel <tobydox/at/users.sf.net>",
	0x0110,
	plugin::Instrument,
	new QPixmap( PLUGIN_NAME::getIconPixmap( "logo" ) ),
	NULL
} ;

}



oscillatorObject::oscillatorObject( model * _parent, track * _track ) :
	model( _parent ),
	m_volumeModel( DEFAULT_VOLUME / NUM_OF_OSCILLATORS,
					MIN_VOLUME, MAX_VOLUME, 1.0f, this ),
	m_panModel( DEFAULT_PANNING, PANNING_LEFT, PANNING_RIGHT, 1.0f, this ),
	m_coarseModel( 0, -2 * NOTES_PER_OCTAVE, 2 * NOTES_PER_OCTAVE,
								1.0f, this ),
	m_fineLeftModel( 0.0f, -100.0f, 100.0f, 1.0f, this ),
	m_fineRightModel( 0.0f, -100.0f, 100.0f, 1.0f, this ),
	m_phaseOffsetModel( 0.0f, 0.0f, 360.0f, 1.0f, this ),
	m_stereoPhaseDetuningModel( 0.0f, 0.0f, 360.0f, 1.0f, this ),
	m_waveShapeModel( oscillator::SineWave, 0, oscillator::NumWaveShapes-1,
								1, this ),
	m_modulationAlgoModel( oscillator::SignalMix, 0,
				oscillator::NumModulationAlgos-1, 1, this ),
	m_sampleBuffer( new sampleBuffer ),
	m_volumeLeft( 0.0f ),
	m_volumeRight( 0.0f ),
	m_detuningLeft( 0.0f ),
	m_detuningRight( 0.0f ),
	m_phaseOffsetLeft( 0.0f ),
	m_phaseOffsetRight( 0.0f )
{
	m_volumeModel.setTrack( _track );
	m_panModel.setTrack( _track );
	m_coarseModel.setTrack( _track );
	m_fineLeftModel.setTrack( _track );
	m_fineRightModel.setTrack( _track );
	m_phaseOffsetModel.setTrack( _track );
	m_stereoPhaseDetuningModel.setTrack( _track );
	m_waveShapeModel.setTrack( _track );
	m_modulationAlgoModel.setTrack( _track );

	// Connect knobs with oscillators' inputs
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




oscillatorObject::~oscillatorObject()
{
	sharedObject::unref( m_sampleBuffer );
}




void oscillatorObject::oscUserDefWaveDblClick( void )
{
	QString af = m_sampleBuffer->openAudioFile();
	if( af != "" )
	{
		m_sampleBuffer->setAudioFile( af );
		// TODO:
		//toolTip::add( m_usrWaveBtn, m_sampleBuffer->audioFile() );
	}
}




void oscillatorObject::updateVolume( void )
{
	if( m_panModel.value() >= 0.0f )
	{
		const float panningFactorLeft = 1.0f - m_panModel.value()
							/ (float)PANNING_RIGHT;
		m_volumeLeft = panningFactorLeft * m_volumeModel.value() /
									100.0f;
		m_volumeRight = m_volumeModel.value() / 100.0f;
	}
	else
	{
		m_volumeLeft = m_volumeModel.value() / 100.0f;
		const float panningFactorRight = 1.0f + m_panModel.value()
							/ (float)PANNING_RIGHT;
		m_volumeRight = panningFactorRight * m_volumeModel.value() /
									100.0f;
	}
}




void oscillatorObject::updateDetuningLeft( void )
{
	m_detuningLeft = powf( 2.0f, ( (float)m_coarseModel.value() * 100.0f
				+ (float)m_fineLeftModel.value() ) / 1200.0f )
					/ engine::getMixer()->sampleRate();
}




void oscillatorObject::updateDetuningRight( void )
{
	m_detuningRight = powf( 2.0f, ( (float)m_coarseModel.value() * 100.0f
				+ (float)m_fineRightModel.value() ) / 1200.0f )
					/ engine::getMixer()->sampleRate();
}




void oscillatorObject::updatePhaseOffsetLeft( void )
{
	m_phaseOffsetLeft = ( m_phaseOffsetModel.value() +
				m_stereoPhaseDetuningModel.value() ) / 360.0f;
}




void oscillatorObject::updatePhaseOffsetRight( void )
{
	m_phaseOffsetRight = m_phaseOffsetModel.value() / 360.0f;
}







 
tripleOscillator::tripleOscillator( instrumentTrack * _instrument_track ) :
	instrument( _instrument_track, &tripleoscillator_plugin_descriptor )
{
	for( int i = 0; i < NUM_OF_OSCILLATORS; ++i )
	{
		m_osc[i] = new oscillatorObject( this, _instrument_track );

	}

	connect( engine::getMixer(), SIGNAL( sampleRateChanged() ),
			this, SLOT( updateAllDetuning() ) );
}




tripleOscillator::~tripleOscillator()
{
}




void tripleOscillator::saveSettings( QDomDocument & _doc, QDomElement & _this )
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




void tripleOscillator::loadSettings( const QDomElement & _this )
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




void tripleOscillator::setParameter( const QString & _param,
							const QString & _value )
{
	if( _param == "samplefile" )
	{
		for( int i = 0; i < NUM_OF_OSCILLATORS; ++i )
		{
			m_osc[i]->m_sampleBuffer->setAudioFile( _value );
		}
	}
}




QString tripleOscillator::nodeName( void ) const
{
	return( tripleoscillator_plugin_descriptor.name );
}




void tripleOscillator::playNote( notePlayHandle * _n, bool )
{
	if( _n->totalFramesPlayed() == 0 || _n->m_pluginData == NULL )
	{
		oscillator * oscs_l[NUM_OF_OSCILLATORS];
		oscillator * oscs_r[NUM_OF_OSCILLATORS];

		for( Sint8 i = NUM_OF_OSCILLATORS - 1; i >= 0; --i )
		{

			// the last oscs needs no sub-oscs...
			if( i == NUM_OF_OSCILLATORS - 1 )
			{
				oscs_l[i] = new oscillator(
						m_osc[i]->m_waveShapeModel,
						m_osc[i]->m_modulationAlgoModel,
						_n->frequency(),
						m_osc[i]->m_detuningLeft,
						m_osc[i]->m_phaseOffsetLeft,
						m_osc[i]->m_volumeLeft );
				oscs_r[i] = new oscillator(
						m_osc[i]->m_waveShapeModel,
						m_osc[i]->m_modulationAlgoModel,
						_n->frequency(),
						m_osc[i]->m_detuningRight,
						m_osc[i]->m_phaseOffsetRight,
						m_osc[i]->m_volumeRight );
			}
			else
			{
				oscs_l[i] = new oscillator(
						m_osc[i]->m_waveShapeModel,
						m_osc[i]->m_modulationAlgoModel,
						_n->frequency(),
						m_osc[i]->m_detuningLeft,
						m_osc[i]->m_phaseOffsetLeft,
						m_osc[i]->m_volumeLeft,
						oscs_l[i + 1] );
				oscs_r[i] = new oscillator(
						m_osc[i]->m_waveShapeModel,
						m_osc[i]->m_modulationAlgoModel,
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

	oscillator * osc_l = static_cast<oscPtr *>( _n->m_pluginData )->oscLeft;
	oscillator * osc_r = static_cast<oscPtr *>( _n->m_pluginData
								)->oscRight;

	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	sampleFrame * buf = new sampleFrame[frames];

	osc_l->update( buf, frames, 0 );
	osc_r->update( buf, frames, 1 );

	applyRelease( buf, _n );

	getInstrumentTrack()->processAudioBuffer( buf, frames, _n );

	delete[] buf;
}




void tripleOscillator::deleteNotePluginData( notePlayHandle * _n )
{
	delete static_cast<oscillator *>( static_cast<oscPtr *>(
						_n->m_pluginData )->oscLeft );
	delete static_cast<oscillator *>( static_cast<oscPtr *>(
						_n->m_pluginData )->oscRight );
	delete static_cast<oscPtr *>( _n->m_pluginData );
}




pluginView * tripleOscillator::instantiateView( QWidget * _parent )
{
	return( new tripleOscillatorView( this, _parent ) );
}




void tripleOscillator::updateAllDetuning( void )
{
	for( int i = 0; i < NUM_OF_OSCILLATORS; ++i )
	{
		m_osc[i]->updateDetuningLeft();
		m_osc[i]->updateDetuningRight();
	}
}





tripleOscillatorView::tripleOscillatorView( instrument * _instrument,
							QWidget * _parent ) :
	instrumentView( _instrument, _parent )
{
	setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( backgroundRole(),
				PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );

	// TODO: clean rewrite using layouts and all that...
	pixmapButton * pm_osc1_btn = new pixmapButton( this, NULL );
	pm_osc1_btn->move( 46, 50 );
	pm_osc1_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"pm_active" ) );
	pm_osc1_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"pm_inactive" ) );
	pm_osc1_btn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap( "btn_mask" ).
						createHeuristicMask() ) );
	toolTip::add( pm_osc1_btn, tr( "use phase modulation for "
					"modulating oscillator 2 with "
					"oscillator 1" ) );

	pixmapButton * am_osc1_btn = new pixmapButton( this, NULL );
	am_osc1_btn->move( 86, 50 );
	am_osc1_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"am_active" ) );
	am_osc1_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"am_inactive" ) );
	am_osc1_btn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap( "btn_mask" ).
						createHeuristicMask() ) );
	toolTip::add( am_osc1_btn, tr( "use amplitude modulation for "
					"modulating oscillator 2 with "
					"oscillator 1" ) );

	pixmapButton * mix_osc1_btn = new pixmapButton( this, NULL );
	mix_osc1_btn->move( 126, 50 );
	mix_osc1_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"mix_active" ) );
	mix_osc1_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"mix_inactive" ) );
	mix_osc1_btn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap(
					"btn_mask" ).createHeuristicMask() ) );
	toolTip::add( mix_osc1_btn, tr( "mix output of oscillator 1 & 2" ) );

	pixmapButton * sync_osc1_btn = new pixmapButton( this, NULL );
	sync_osc1_btn->move( 166, 50 );
	sync_osc1_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"sync_active" ) );
	sync_osc1_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"sync_inactive" ) );
	sync_osc1_btn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap(
					"btn_mask" ).createHeuristicMask() ) );
	toolTip::add( sync_osc1_btn, tr( "synchronize oscillator 1 with "
							"oscillator 2" ) );

	pixmapButton * fm_osc1_btn = new pixmapButton( this, NULL );
	fm_osc1_btn->move( 206, 50 );
	fm_osc1_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"fm_active" ) );
	fm_osc1_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"fm_inactive" ) );
	fm_osc1_btn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap( "btn_mask" ).
						createHeuristicMask() ) );
	toolTip::add( fm_osc1_btn, tr( "use frequency modulation for "
					"modulating oscillator 2 with "
					"oscillator 1" ) );

	m_mod1BtnGrp = new automatableButtonGroup( this,
						tr( "Modulation type 1" ) );
	m_mod1BtnGrp->addButton( pm_osc1_btn );
	m_mod1BtnGrp->addButton( am_osc1_btn );
	m_mod1BtnGrp->addButton( mix_osc1_btn );
	m_mod1BtnGrp->addButton( sync_osc1_btn );
	m_mod1BtnGrp->addButton( fm_osc1_btn );



	pixmapButton * pm_osc2_btn = new pixmapButton( this, NULL );
	pm_osc2_btn->move( 46, 68 );
	pm_osc2_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"pm_active" ) );
	pm_osc2_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"pm_inactive" ) );
	pm_osc2_btn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap( "btn_mask" ).
						createHeuristicMask() ) );
	toolTip::add( pm_osc2_btn, tr( "use phase modulation for "
					"modulating oscillator 3 with "
					"oscillator 2" ) );

	pixmapButton * am_osc2_btn = new pixmapButton( this, NULL );
	am_osc2_btn->move( 86, 68 );
	am_osc2_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"am_active" ) );
	am_osc2_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"am_inactive" ) );
	am_osc2_btn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap( "btn_mask" ).
						createHeuristicMask() ) );
	toolTip::add( am_osc2_btn, tr( "use amplitude modulation for "
					"modulating oscillator 3 with "
					"oscillator 2" ) );

	pixmapButton * mix_osc2_btn = new pixmapButton( this, NULL );
	mix_osc2_btn->move( 126, 68 );
	mix_osc2_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"mix_active" ) );
	mix_osc2_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"mix_inactive" ) );
	mix_osc2_btn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap(
					"btn_mask" ).createHeuristicMask() ) );
	toolTip::add( mix_osc2_btn, tr("mix output of oscillator 2 & 3" ) );

	pixmapButton * sync_osc2_btn = new pixmapButton( this, NULL );
	sync_osc2_btn->move( 166, 68 );
	sync_osc2_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"sync_active" ) );
	sync_osc2_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"sync_inactive" ) );
	sync_osc2_btn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap(
					"btn_mask" ).createHeuristicMask() ) );
	toolTip::add( sync_osc2_btn, tr( "synchronize oscillator 2 with "
							"oscillator 3" ) );

	pixmapButton * fm_osc2_btn = new pixmapButton( this, NULL );
	fm_osc2_btn->move( 206, 68 );
	fm_osc2_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"fm_active" ) );
	fm_osc2_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"fm_inactive" ) );
	fm_osc2_btn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap( "btn_mask" ).
						createHeuristicMask() ) );
	toolTip::add( fm_osc2_btn, tr( "use frequency modulation for "
					"modulating oscillator 3 with "
					"oscillator 2" ) );

	m_mod2BtnGrp = new automatableButtonGroup( this,
						tr( "Modulation type 2" ) );
	m_mod2BtnGrp->addButton( pm_osc2_btn );
	m_mod2BtnGrp->addButton( am_osc2_btn );
	m_mod2BtnGrp->addButton( mix_osc2_btn );
	m_mod2BtnGrp->addButton( sync_osc2_btn );
	m_mod2BtnGrp->addButton( fm_osc2_btn );


	for( int i = 0; i < NUM_OF_OSCILLATORS; ++i )
	{
		// setup volume-knob
		volumeKnob * vk = new volumeKnob( knobSmall_17, this, tr(
						"Osc %1 volume" ).arg( i+1 ) );
		vk->move( 6, 104 + i * 50 );
		vk->setHintText( tr( "Osc %1 volume:" ).arg(
							i+1 ) + " ", "%" );
		vk->setWhatsThis(
			tr( "With this knob you can set the volume of "
				"oscillator %1. When setting a value of 0 the "
				"oscillator is turned off. Otherwise you can "
				"hear the oscillator as loud as you set it "
				"here.").arg( i+1 ) );

		// setup panning-knob
		knob * pk = new knob( knobSmall_17, this,
					tr( "Osc %1 panning" ).arg( i + 1 ) );
		pk->move( 33, 104 + i * 50 );
		pk->setHintText( tr("Osc %1 panning:").arg( i + 1 ) + " ", "" );
		pk->setWhatsThis(
			tr( "With this knob you can set the panning of the "
				"oscillator %1. A value of -100 means 100% "
				"left and a value of 100 moves oscillator-"
				"output right.").arg( i+1 ) );

		// setup coarse-knob
		knob * ck = new knob( knobSmall_17, this,
				tr( "Osc %1 coarse detuning" ).arg( i + 1 ) );
		ck->move( 66, 104 + i * 50 );
		ck->setHintText( tr( "Osc %1 coarse detuning:" ).arg( i + 1 ) +
						" ", " " + tr( "semitones" ) );
		ck->setWhatsThis(
			tr( "With this knob you can set the coarse detuning of "
				"oscillator %1. You can detune the oscillator "
				"12 semitones (1 octave) up and down. This is "
				"useful for creating sounds with a chord." ).
				arg( i + 1 ) );

		// setup knob for left fine-detuning
		knob * flk = new knob( knobSmall_17, this,
				tr( "Osc %1 fine detuning left" ).arg( i+1 ) );
		flk->move( 90, 104 + i * 50 );
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
		knob * frk = new knob( knobSmall_17, this,
			tr( "Osc %1 fine detuning right" ).arg( i + 1 ) );
		frk->move( 110, 104 + i * 50 );
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
		knob * pok = new knob( knobSmall_17, this,
				tr( "Osc %1 phase-offset" ).arg( i+1 ) );
		pok->move( 142, 104 + i * 50 );
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
		knob * spdk = new knob( knobSmall_17, this,
			tr( "Osc %1 stereo phase-detuning" ).arg( i+1 ) );
		spdk->move( 166, 104 + i * 50 );
		spdk->setHintText( tr("Osc %1 stereo phase-detuning:" ).
					arg( i + 1 ) + " ",
							" " + tr( "degrees" ) );
		spdk->setWhatsThis(
			tr( "With this knob you can set the stereo phase-"
				"detuning of oscillator %1. The stereo phase-"
				"detuning specifies the size of the difference "
				"between the phase-offset of left and right "
				"channel. This is very good for creating wide "
				"stereo-sounds." ).arg( i+1 ) );


		pixmapButton * sin_wave_btn = new pixmapButton( this, NULL );
		sin_wave_btn->move( 188, 105 + i * 50 );
		sin_wave_btn->setActiveGraphic( embed::getIconPixmap(
							"sin_wave_active" ) );
		sin_wave_btn->setInactiveGraphic( embed::getIconPixmap(
							"sin_wave_inactive" ) );
		toolTip::add( sin_wave_btn,
				tr( "Click here if you want a sine-wave for "
						"current oscillator." ) );

		pixmapButton * triangle_wave_btn =
						new pixmapButton( this, NULL );
		triangle_wave_btn->move( 203, 105 + i * 50 );
		triangle_wave_btn->setActiveGraphic(
			embed::getIconPixmap( "triangle_wave_active" ) );
		triangle_wave_btn->setInactiveGraphic(
			embed::getIconPixmap( "triangle_wave_inactive" ) );
		toolTip::add( triangle_wave_btn,
				tr( "Click here if you want a triangle-wave "
						"for current oscillator." ) );

		pixmapButton * saw_wave_btn = new pixmapButton( this, NULL );
		saw_wave_btn->move( 218, 105 + i * 50 );
		saw_wave_btn->setActiveGraphic( embed::getIconPixmap(
							"saw_wave_active" ) );
		saw_wave_btn->setInactiveGraphic( embed::getIconPixmap(
							"saw_wave_inactive" ) );
		toolTip::add( saw_wave_btn,
				tr( "Click here if you want a saw-wave for "
						"current oscillator." ) );

		pixmapButton * sqr_wave_btn = new pixmapButton( this, NULL );
		sqr_wave_btn->move( 233, 105 + i * 50 );
		sqr_wave_btn->setActiveGraphic( embed::getIconPixmap(
						"square_wave_active" ) );
		sqr_wave_btn->setInactiveGraphic( embed::getIconPixmap(
						"square_wave_inactive" ) );
		toolTip::add( sqr_wave_btn,
				tr( "Click here if you want a square-wave for "
						"current oscillator." ) );

		pixmapButton * moog_saw_wave_btn =
						new pixmapButton( this, NULL );
		moog_saw_wave_btn->move( 188, 120+i*50 );
		moog_saw_wave_btn->setActiveGraphic(
			embed::getIconPixmap( "moog_saw_wave_active" ) );
		moog_saw_wave_btn->setInactiveGraphic(
			embed::getIconPixmap( "moog_saw_wave_inactive" ) );
		toolTip::add( moog_saw_wave_btn,
				tr( "Click here if you want a moog-saw-wave "
						"for current oscillator." ) );

		pixmapButton * exp_wave_btn = new pixmapButton( this, NULL );
		exp_wave_btn->move( 203, 120+i*50 );
		exp_wave_btn->setActiveGraphic( embed::getIconPixmap(
							"exp_wave_active" ) );
		exp_wave_btn->setInactiveGraphic( embed::getIconPixmap(
							"exp_wave_inactive" ) );
		toolTip::add( exp_wave_btn,
				tr( "Click here if you want an exponential "
					"wave for current oscillator." ) );

		pixmapButton * white_noise_btn = new pixmapButton( this, NULL );
		white_noise_btn->move( 218, 120+i*50 );
		white_noise_btn->setActiveGraphic(
			embed::getIconPixmap( "white_noise_wave_active" ) );
		white_noise_btn->setInactiveGraphic(
			embed::getIconPixmap( "white_noise_wave_inactive" ) );
		toolTip::add( white_noise_btn,
				tr( "Click here if you want a white-noise for "
						"current oscillator." ) );

		pixmapButton * uwb = new pixmapButton( this, NULL );
		uwb->move( 233, 120+i*50 );
		uwb->setActiveGraphic( embed::getIconPixmap(
							"usr_wave_active" ) );
		uwb->setInactiveGraphic( embed::getIconPixmap(
							"usr_wave_inactive" ) );
		toolTip::add( uwb, tr( "Click here if you want a user-defined "
				"wave-shape for current oscillator." ) );

		automatableButtonGroup * wsbg =
			new automatableButtonGroup( this,
				tr( "Osc %1 wave shape" ).arg( i + 1 ) );
		wsbg->addButton( sin_wave_btn );
		wsbg->addButton( triangle_wave_btn );
		wsbg->addButton( saw_wave_btn );
		wsbg->addButton( sqr_wave_btn );
		wsbg->addButton( moog_saw_wave_btn );
		wsbg->addButton( exp_wave_btn );
		wsbg->addButton( white_noise_btn );
		wsbg->addButton( uwb );

/*		connect( m_osc[i].m_usrWaveBtn, SIGNAL( doubleClicked() ),
				&m_osc[i], SLOT( oscUserDefWaveDblClick() ) );*/

		m_oscKnobs[i] = oscillatorKnobs( vk, pk, ck, flk, frk, pok,
							spdk, uwb, wsbg );

	}
}




tripleOscillatorView::~tripleOscillatorView()
{
}




void tripleOscillatorView::modelChanged( void )
{
	tripleOscillator * t = castModel<tripleOscillator>();
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

// neccessary for getting instance out of shared lib
plugin * lmms_plugin_main( model *, void * _data )
{
	return( new tripleOscillator(
				static_cast<instrumentTrack *>( _data ) ) );
}

}


#include "triple_oscillator.moc"

