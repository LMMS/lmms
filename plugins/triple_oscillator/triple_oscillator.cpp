/*
 * triple_oscillator.cpp - powerful instrument with three oscillators
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */
 

#include "qt3support.h"

#ifdef QT4

#include <Qt/QtXml>
#include <QButtonGroup>
#include <QBitmap>
#include <QPainter>

#else

#include <qbuttongroup.h>
#include <qbitmap.h>
#include <qpainter.h>
#include <qdom.h>

#define setChecked setOn

#endif


#include "triple_oscillator.h"
#include "song_editor.h"
#include "channel_track.h"
#include "note_play_handle.h"
#include "knob.h"
#include "pixmap_button.h"
#include "buffer_allocator.h"
#include "debug.h"
#include "tooltip.h"

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
	0x0100,
	plugin::INSTRUMENT,
	PLUGIN_NAME::findEmbeddedData( "logo.png" )
} ;

}

 
tripleOscillator::tripleOscillator( channelTrack * _channel_track ) :
	instrument( _channel_track,
			tripleoscillator_plugin_descriptor.public_name ),
	m_modulationAlgo1( oscillator::MIX ),
	m_modulationAlgo2( oscillator::MIX )
{
#ifdef QT4
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
#else
	setErasePixmap( PLUGIN_NAME::getIconPixmap( "artwork" ) );
#endif

	m_fm1OscBtn = new pixmapButton( this );
	m_fm1OscBtn->move( 80, 50 );
	m_fm1OscBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "fm_active" ) );
	m_fm1OscBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"fm_inactive" ) );
	m_fm1OscBtn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap( "btn_mask" ).
						createHeuristicMask() ) );
	connect( m_fm1OscBtn, SIGNAL( toggled( bool ) ), this,
						SLOT( fm1BtnToggled( bool ) ) );
	toolTip::add( m_fm1OscBtn, tr( "use frequency modulation for "
					"modulating oscillator 2 with "
					"oscillator 1" ) );

	m_am1OscBtn = new pixmapButton( this );
	m_am1OscBtn->move( 120, 50 );
	m_am1OscBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "am_active" ) );
	m_am1OscBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"am_inactive" ) );
	m_am1OscBtn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap( "btn_mask" ).
						createHeuristicMask() ) );
	connect( m_am1OscBtn, SIGNAL( toggled( bool ) ), this,
						SLOT( am1BtnToggled( bool ) ) );
	toolTip::add( m_am1OscBtn, tr( "use amplitude modulation for "
					"modulating oscillator 2 with "
					"oscillator 1" ) );

	m_mix1OscBtn = new pixmapButton( this );
	m_mix1OscBtn->move( 160, 50 );
	m_mix1OscBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "mix_active" ) );
	m_mix1OscBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"mix_inactive" ) );
	m_mix1OscBtn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap( "btn_mask" ).
						createHeuristicMask() ) );
	connect( m_mix1OscBtn, SIGNAL( toggled( bool ) ), this,
					SLOT( mix1BtnToggled( bool ) ) );
	toolTip::add( m_mix1OscBtn, tr( "mix output of oscillator 1 & 2" ) );

	m_sync1OscBtn = new pixmapButton( this );
	m_sync1OscBtn->move( 200, 50 );
	m_sync1OscBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"sync_active" ) );
	m_sync1OscBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"sync_inactive" ) );
	m_sync1OscBtn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap( "btn_mask" ).
						createHeuristicMask() ) );
	connect( m_sync1OscBtn, SIGNAL( toggled( bool ) ), this, SLOT(
						sync1BtnToggled( bool ) ) );
	toolTip::add( m_sync1OscBtn, tr( "synchronize oscillator 1 with "
							"oscillator 2" ) );

	if( m_modulationAlgo1 == oscillator::FREQ_MODULATION )
	{
		m_fm1OscBtn->setChecked( TRUE );
	}
	else if( m_modulationAlgo1 == oscillator::AMP_MODULATION )
	{
		m_am1OscBtn->setChecked( TRUE );
	}
	else if( m_modulationAlgo1 == oscillator::MIX )
	{
		m_mix1OscBtn->setChecked( TRUE );
	}
	else if( m_modulationAlgo1 == oscillator::SYNC )
	{
		m_sync1OscBtn->setChecked( TRUE );
	}

	QButtonGroup * modulation_algo_group1 = new QButtonGroup( this );
	modulation_algo_group1->addButton( m_fm1OscBtn );
	modulation_algo_group1->addButton( m_am1OscBtn );
	modulation_algo_group1->addButton( m_mix1OscBtn );
	modulation_algo_group1->addButton( m_sync1OscBtn );
	modulation_algo_group1->setExclusive( TRUE );
#ifndef QT4
	modulation_algo_group1->hide();
#endif

	m_fm2OscBtn = new pixmapButton( this );
	m_fm2OscBtn->move( 80, 70 );
	m_fm2OscBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "fm_active" ) );
	m_fm2OscBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"fm_inactive" ) );
	m_fm2OscBtn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap( "btn_mask" ).
						createHeuristicMask() ) );
	connect( m_fm2OscBtn, SIGNAL( toggled( bool ) ), this, SLOT(
						fm2BtnToggled( bool ) ) );
	toolTip::add( m_fm2OscBtn, tr( "use frequency modulation for "
					"modulating oscillator 3 with "
					"oscillator 2" ) );

	m_am2OscBtn = new pixmapButton( this );
	m_am2OscBtn->move( 120, 70 );
	m_am2OscBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "am_active" ) );
	m_am2OscBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap("am_inactive" ) );
	m_am2OscBtn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap( "btn_mask" ).
						createHeuristicMask() ) );
	connect( m_am2OscBtn, SIGNAL( toggled( bool ) ), this,
						SLOT( am2BtnToggled( bool ) ) );
	toolTip::add( m_am2OscBtn, tr( "use amplitude modulation for "
					"modulating oscillator 3 with "
					"oscillator 2" ) );

	m_mix2OscBtn = new pixmapButton( this );
	m_mix2OscBtn->move( 160, 70 );
	m_mix2OscBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "mix_active" ) );
	m_mix2OscBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"mix_inactive" ) );
	m_mix2OscBtn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap( "btn_mask" ).
						createHeuristicMask() ) );
	connect( m_mix2OscBtn, SIGNAL( toggled( bool ) ), this,
					SLOT( mix2BtnToggled( bool ) ) );
	toolTip::add( m_mix2OscBtn, tr("mix output of oscillator 2 & 3" ) );

	m_sync2OscBtn = new pixmapButton( this );
	m_sync2OscBtn->move( 200, 70 );
	m_sync2OscBtn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"sync_active" ) );
	m_sync2OscBtn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"sync_inactive" ) );
	m_sync2OscBtn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap( "btn_mask" ).
						createHeuristicMask() ) );
	connect( m_sync2OscBtn, SIGNAL( toggled( bool ) ), this,
					SLOT( sync2BtnToggled( bool ) ) );
	toolTip::add( m_sync2OscBtn, tr( "synchronize oscillator 2 with "
							"oscillator 3" ) );

	if( m_modulationAlgo2 == oscillator::FREQ_MODULATION )
	{
		m_fm2OscBtn->setChecked( TRUE );
	}
	else if( m_modulationAlgo2 == oscillator::AMP_MODULATION )
	{
		m_am2OscBtn->setChecked( TRUE );
	}
	else if( m_modulationAlgo2 == oscillator::MIX )
	{
		m_mix2OscBtn->setChecked( TRUE );
	}
	else if( m_modulationAlgo2 == oscillator::SYNC )
	{
		m_sync2OscBtn->setChecked( TRUE );
	}

	QButtonGroup * modulation_algo_group2 = new QButtonGroup( this );
	modulation_algo_group2->addButton( m_fm2OscBtn );
	modulation_algo_group2->addButton( m_am2OscBtn );
	modulation_algo_group2->addButton( m_mix2OscBtn );
	modulation_algo_group2->addButton( m_sync2OscBtn );
	modulation_algo_group2->setExclusive( TRUE );
#ifndef QT4
	modulation_algo_group2->hide();
#endif


	for( int i = 0; i < NUM_OF_OSCILLATORS; ++i )
	{
		// reset current m_osc-structure
		m_osc[i].waveShape = oscillator::SIN_WAVE;
		
		// setup volume-knob
		m_osc[i].volKnob = new knob( knobSmall_17, this, tr(
						"Osc %1 volume" ).arg( i+1 ) );
		m_osc[i].volKnob->move( 6, 104+i*50 );
		m_osc[i].volKnob->setRange( MIN_VOLUME, MAX_VOLUME, 1.0f );
		m_osc[i].volKnob->setValue( DEFAULT_VOLUME, TRUE );
		m_osc[i].volKnob->setHintText( tr( "Osc %1 volume:" ).arg(
							i+1 ) + " ", "%" );
#ifdef QT4
		m_osc[i].volKnob->setWhatsThis(
#else
		QWhatsThis::add( m_osc[i].volKnob,
#endif
			tr( "With this knob you can set the volume of "
				"oscillator %1. When setting a value of 0 the "
				"oscillator is turned off. Otherwise you can "
				"hear the oscillator as loud as you set it "
				"here.").arg( i+1 ) );

		// setup panning-knob
		m_osc[i].panKnob = new knob( knobSmall_17, this,
					tr( "Osc %1 panning" ).arg( i + 1 ) );
		m_osc[i].panKnob->move( 33, 104+i*50 );
		m_osc[i].panKnob->setRange( PANNING_LEFT, PANNING_RIGHT, 1.0f );
		m_osc[i].panKnob->setValue( DEFAULT_PANNING, TRUE );
		m_osc[i].panKnob->setHintText( tr("Osc %1 panning:").arg( i+1 )
						+ " ", "" );
#ifdef QT4
		m_osc[i].panKnob->setWhatsThis(
#else
		QWhatsThis::add( m_osc[i].panKnob,
#endif
			tr( "With this knob you can set the panning of the "
				"oscillator %1. A value of -100 means 100% "
				"left and a value of 100 moves oscillator-"
				"output right.").arg( i+1 ) );

		// setup coarse-knob
		m_osc[i].coarseKnob = new knob( knobSmall_17, this,
				tr("Osc %1 coarse detuning").arg( i + 1 ) );
		m_osc[i].coarseKnob->move( 66, 104 + i * 50 );
		m_osc[i].coarseKnob->setRange( -2 * NOTES_PER_OCTAVE,
						2 * NOTES_PER_OCTAVE, 1.0f );
		m_osc[i].coarseKnob->setValue( 0.0f, TRUE );
		m_osc[i].coarseKnob->setHintText( tr( "Osc %1 coarse detuning:"
							).arg( i + 1 ) + " ",
						" " + tr( "semitones" ) );
#ifdef QT4
		m_osc[i].coarseKnob->setWhatsThis(
#else
		QWhatsThis::add( m_osc[i].coarseKnob,
#endif
			tr( "With this knob you can set the coarse detuning of "
				"oscillator %1. You can detune the oscillator "
				"12 semitones (1 octave) up and down. This is "
				"useful for creating sounds with a chord." ).
				arg( i + 1 ) );

		// setup knob for left fine-detuning
		m_osc[i].fineLKnob = new knob( knobSmall_17, this,
				tr( "Osc %1 fine detuning left" ).arg( i+1 ) );
		m_osc[i].fineLKnob->move( 90, 104 + i * 50 );
		m_osc[i].fineLKnob->setRange( -100.0f, 100.0f, 1.0f );
		m_osc[i].fineLKnob->setValue( 0.0f, TRUE );
		m_osc[i].fineLKnob->setHintText( tr( "Osc %1 fine detuning "
							"left:" ).arg( i + 1 )
							+ " ", " " +
							tr( "cents" ) );
#ifdef QT4
		m_osc[i].fineLKnob->setWhatsThis(
#else
		QWhatsThis::add( m_osc[i].fineLKnob,
#endif
			tr( "With this knob you can set the fine detuning of "
				"oscillator %1 for the left channel. The fine-"
				"detuning is ranged between -100 cents and "
				"+100 cents. This is useful for creating "
				"\"fat\" sounds." ).arg( i + 1 ) );

		// setup knob for right fine-detuning
		m_osc[i].fineRKnob = new knob( knobSmall_17, this,
						tr( "Osc %1 fine detuning right"
							).arg( i + 1 ) );
		m_osc[i].fineRKnob->move( 110, 104 + i * 50 );
		m_osc[i].fineRKnob->setRange( -100.0f, 100.0f, 1.0f );
		m_osc[i].fineRKnob->setValue( 0.0f, TRUE );
		m_osc[i].fineRKnob->setHintText( tr( "Osc %1 fine detuning "
							"right:").arg( i + 1 ) +
						" ", " " + tr( "cents" ) );
#ifdef QT4
		m_osc[i].fineRKnob->setWhatsThis(
#else
		QWhatsThis::add( m_osc[i].fineRKnob,
#endif
			tr( "With this knob you can set the fine detuning of "
				"oscillator %1 for the right channel. The "
				"fine-detuning is ranged between -100 cents "
				"and +100 cents. This is useful for creating "
				"\"fat\" sounds." ).arg( i+1 ) );

		// setup phase-offset-knob
		m_osc[i].phaseOffsetKnob = new knob( knobSmall_17, this,
							tr( "Osc %1 phase-"
							"offset" ).arg( i+1 ) );
		m_osc[i].phaseOffsetKnob->move( 142, 104 + i * 50 );
		m_osc[i].phaseOffsetKnob->setRange( 0.0f, 360.0f, 1.0f );
		m_osc[i].phaseOffsetKnob->setValue( 0.0f, TRUE );
		m_osc[i].phaseOffsetKnob->setHintText( tr( "Osc %1 phase-"
								"offset:" ).
								arg( i + 1 ) +
						" ", " " + tr( "degrees" ) );
#ifdef QT4
		m_osc[i].phaseOffsetKnob->setWhatsThis(
#else
		QWhatsThis::add( m_osc[i].phaseOffsetKnob,
#endif
			tr( "With this knob you can set the phase-offset of "
				"oscillator %1. That means you can move the "
				"point within an oscillation where the "
				"oscillator begins to oscillate. For example "
				"if you have a sine-wave and have a phase-"
				"offset of 180 degrees the wave will first go "
				"down. It's the same with a square-wave."
				).arg( i+1 ) );

		// setup stereo-phase-detuning-knob
		m_osc[i].stereoPhaseDetuningKnob = new knob( knobSmall_17, this,
						tr( "Osc %1 stereo phase-"
							"detuning" ).arg( i+1 )
							);
		m_osc[i].stereoPhaseDetuningKnob->move( 166, 104 + i * 50 );
		m_osc[i].stereoPhaseDetuningKnob->setRange( 0.0f, 360.0f,
									1.0f );
		m_osc[i].stereoPhaseDetuningKnob->setValue( 0.0f, TRUE );
		m_osc[i].stereoPhaseDetuningKnob->setHintText( tr("Osc %1 "
								"stereo phase-"
								"detuning:" ).
								arg( i + 1 ) +
								" ", " " +
							tr( "degrees" ) );
#ifdef QT4
		m_osc[i].stereoPhaseDetuningKnob->setWhatsThis(
#else
		QWhatsThis::add( m_osc[i].stereoPhaseDetuningKnob,
#endif
			tr( "With this knob you can set the stereo phase-"
				"detuning of oscillator %1. The stereo phase-"
				"detuning specifies the size of the difference "
				"between the phase-offset of left and right "
				"channel. This is very good for creating wide "
				"stereo-sounds." ).arg( i+1 ) );

		m_osc[i].sinWaveBtn = new pixmapButton( this );
		m_osc[i].sinWaveBtn->move( 188, 105 + i * 50 );
		m_osc[i].sinWaveBtn->setActiveGraphic( embed::getIconPixmap(
							"sin_wave_active" ) );
		m_osc[i].sinWaveBtn->setInactiveGraphic( embed::getIconPixmap(
							"sin_wave_inactive" ) );
		m_osc[i].sinWaveBtn->setChecked( TRUE );
		toolTip::add( m_osc[i].sinWaveBtn,
				tr( "Click here if you want a sine-wave for "
						"current oscillator." ) );

		m_osc[i].triangleWaveBtn = new pixmapButton( this );
		m_osc[i].triangleWaveBtn->move( 203, 105 + i * 50 );
		m_osc[i].triangleWaveBtn->setActiveGraphic(
			embed::getIconPixmap( "triangle_wave_active" ) );
		m_osc[i].triangleWaveBtn->setInactiveGraphic(
			embed::getIconPixmap( "triangle_wave_inactive" ) );
		toolTip::add( m_osc[i].triangleWaveBtn,
				tr( "Click here if you want a triangle-wave "
						"for current oscillator." ) );

		m_osc[i].sawWaveBtn = new pixmapButton( this );
		m_osc[i].sawWaveBtn->move( 218, 105 + i * 50 );
		m_osc[i].sawWaveBtn->setActiveGraphic( embed::getIconPixmap(
							"saw_wave_active" ) );
		m_osc[i].sawWaveBtn->setInactiveGraphic( embed::getIconPixmap(
							"saw_wave_inactive" ) );
		toolTip::add( m_osc[i].sawWaveBtn,
				tr( "Click here if you want a saw-wave for "
						"current oscillator." ) );

		m_osc[i].sqrWaveBtn = new pixmapButton( this );
		m_osc[i].sqrWaveBtn->move( 233, 105 + i * 50 );
		m_osc[i].sqrWaveBtn->setActiveGraphic( embed::getIconPixmap(
						"square_wave_active" ) );
		m_osc[i].sqrWaveBtn->setInactiveGraphic( embed::getIconPixmap(
						"square_wave_inactive" ) );
		toolTip::add( m_osc[i].sqrWaveBtn,
				tr( "Click here if you want a square-wave for "
						"current oscillator." ) );

		m_osc[i].moogSawWaveBtn = new pixmapButton( this );
		m_osc[i].moogSawWaveBtn->move( 188, 120+i*50 );
		m_osc[i].moogSawWaveBtn->setActiveGraphic(
			embed::getIconPixmap( "moog_saw_wave_active" ) );
		m_osc[i].moogSawWaveBtn->setInactiveGraphic(
			embed::getIconPixmap( "moog_saw_wave_inactive" ) );
		toolTip::add( m_osc[i].moogSawWaveBtn,
				tr( "Click here if you want a moog-saw-wave "
						"for current oscillator." ) );

		m_osc[i].expWaveBtn = new pixmapButton( this );
		m_osc[i].expWaveBtn->move( 203, 120+i*50 );
		m_osc[i].expWaveBtn->setActiveGraphic( embed::getIconPixmap(
							"exp_wave_active" ) );
		m_osc[i].expWaveBtn->setInactiveGraphic( embed::getIconPixmap(
							"exp_wave_inactive" ) );
		toolTip::add( m_osc[i].expWaveBtn,
				tr( "Click here if you want an exponential "
					"wave for current oscillator." ) );

		m_osc[i].whiteNoiseWaveBtn = new pixmapButton( this );
		m_osc[i].whiteNoiseWaveBtn->move( 218, 120+i*50 );
		m_osc[i].whiteNoiseWaveBtn->setActiveGraphic(
			embed::getIconPixmap( "white_noise_wave_active" ) );
		m_osc[i].whiteNoiseWaveBtn->setInactiveGraphic(
			embed::getIconPixmap( "white_noise_wave_inactive" ) );
		toolTip::add( m_osc[i].whiteNoiseWaveBtn,
				tr( "Click here if you want a white-noise for "
						"current oscillator." ) );

		m_osc[i].usrWaveBtn = new pixmapButton( this );
		m_osc[i].usrWaveBtn->move( 233, 120+i*50 );
		m_osc[i].usrWaveBtn->setActiveGraphic( embed::getIconPixmap(
							"usr_wave_active" ) );
		m_osc[i].usrWaveBtn->setInactiveGraphic( embed::getIconPixmap(
							"usr_wave_inactive" ) );
		toolTip::add( m_osc[i].usrWaveBtn,
				tr( "Click here if you want a user-defined "
				"wave-shape for current oscillator." ) );

		QButtonGroup * wave_btn_group = new QButtonGroup( this );
		wave_btn_group->addButton( m_osc[i].sinWaveBtn );
		wave_btn_group->addButton( m_osc[i].triangleWaveBtn );
		wave_btn_group->addButton( m_osc[i].sawWaveBtn );
		wave_btn_group->addButton( m_osc[i].sqrWaveBtn );
		wave_btn_group->addButton( m_osc[i].moogSawWaveBtn );
		wave_btn_group->addButton( m_osc[i].expWaveBtn );
		wave_btn_group->addButton( m_osc[i].whiteNoiseWaveBtn );
		wave_btn_group->addButton( m_osc[i].usrWaveBtn );
		wave_btn_group->setExclusive( TRUE );
#ifndef QT4
		wave_btn_group->hide();
#endif

		if( i == 0 )
		{		// Osc 1
			connect( m_osc[i].sinWaveBtn,
					SIGNAL( toggled( bool ) ), this,
					SLOT( osc01SinWaveCh( bool ) ) );
			connect( m_osc[i].triangleWaveBtn,
					SIGNAL( toggled( bool ) ), this,
					SLOT( osc01TriangleWaveCh( bool ) ) );
			connect( m_osc[i].sawWaveBtn,
					SIGNAL( toggled( bool ) ), this,
					SLOT( osc01SawWaveCh( bool ) ) );
			connect( m_osc[i].sqrWaveBtn,
					SIGNAL( toggled( bool ) ), this,
					SLOT( osc01SquareWaveCh( bool ) ) );
			connect( m_osc[i].moogSawWaveBtn,
					SIGNAL(toggled( bool ) ), this,
					SLOT( osc01MoogSawWaveCh( bool ) ) );
			connect( m_osc[i].expWaveBtn,
					SIGNAL( toggled( bool ) ), this,
					SLOT( osc01ExpWaveCh( bool ) ) );
			connect( m_osc[i].whiteNoiseWaveBtn,
					SIGNAL( toggled( bool ) ), this,
					SLOT( osc01WhiteNoiseCh( bool ) ) );
			connect( m_osc[i].usrWaveBtn,
					SIGNAL( toggled( bool ) ), this,
					SLOT( osc01UserDefWaveCh( bool ) ) );
			connect( m_osc[i].usrWaveBtn,
					SIGNAL( doubleClicked() ), this,
					SLOT( osc01UserDefWaveDblClick() ) );
		}
		else if( i == 1 )
		{	// Osc 2
			connect( m_osc[i].sinWaveBtn,
					SIGNAL( toggled( bool ) ), this,
					SLOT( osc02SinWaveCh( bool ) ) );
			connect( m_osc[i].triangleWaveBtn,
					SIGNAL( toggled( bool ) ), this,
					SLOT( osc02TriangleWaveCh( bool ) ) );
			connect( m_osc[i].sawWaveBtn,
					SIGNAL( toggled( bool ) ), this,
					SLOT( osc02SawWaveCh( bool ) ) );
			connect( m_osc[i].sqrWaveBtn,
					SIGNAL( toggled( bool ) ), this,
					SLOT( osc02SquareWaveCh( bool ) ) );
			connect( m_osc[i].moogSawWaveBtn,
					SIGNAL( toggled( bool ) ), this,
					SLOT( osc02MoogSawWaveCh( bool ) ) );
			connect( m_osc[i].expWaveBtn,
					SIGNAL( toggled( bool ) ), this,
					SLOT( osc02ExpWaveCh( bool ) ) );
			connect( m_osc[i].whiteNoiseWaveBtn,
					SIGNAL( toggled( bool ) ), this,
					SLOT( osc02WhiteNoiseCh( bool ) ) );
			connect( m_osc[i].usrWaveBtn,
					SIGNAL( toggled( bool ) ), this,
					SLOT( osc02UserDefWaveCh( bool ) ) );
			connect( m_osc[i].usrWaveBtn,
					SIGNAL( doubleClicked() ), this,
					SLOT( osc02UserDefWaveDblClick() ) );
		}
		else if( i == 2 )
		{	// Osc 3
			connect( m_osc[i].sinWaveBtn,
					SIGNAL( toggled( bool ) ), this,
					SLOT( osc03SinWaveCh( bool ) ) );
			connect( m_osc[i].triangleWaveBtn,
					SIGNAL( toggled( bool ) ), this,
					SLOT( osc03TriangleWaveCh( bool ) ) );
			connect( m_osc[i].sawWaveBtn,
					SIGNAL( toggled( bool ) ), this,
					SLOT( osc03SawWaveCh( bool ) ) );
			connect( m_osc[i].sqrWaveBtn,
					SIGNAL( toggled( bool ) ), this,
					SLOT( osc03SquareWaveCh( bool ) ) );
			connect( m_osc[i].moogSawWaveBtn,
					SIGNAL( toggled( bool ) ), this,
					SLOT( osc03MoogSawWaveCh( bool ) ) );
			connect( m_osc[i].expWaveBtn,
					SIGNAL( toggled( bool ) ), this,
					SLOT( osc03ExpWaveCh( bool ) ) );
			connect( m_osc[i].whiteNoiseWaveBtn,
					SIGNAL( toggled( bool ) ), this,
					SLOT( osc03WhiteNoiseCh( bool ) ) );
			connect( m_osc[i].usrWaveBtn,
					SIGNAL( toggled( bool ) ), this,
					SLOT( osc03UserDefWaveCh( bool ) ) );
			connect( m_osc[i].usrWaveBtn,
					SIGNAL( doubleClicked() ), this,
					SLOT( osc03UserDefWaveDblClick() ) );
		}
	}
}




tripleOscillator::~tripleOscillator()
{
}




void tripleOscillator::saveSettings( QDomDocument & _doc,
							QDomElement & _parent )
{
	QDomElement to_de = _doc.createElement( nodeName() );
	to_de.setAttribute( "modalgo1", QString::number( m_modulationAlgo1 ) );
	to_de.setAttribute( "modalgo2", QString::number( m_modulationAlgo2 ) );

	for( int i = 0; i < NUM_OF_OSCILLATORS; ++i )
	{
		QString is = QString::number( i );
		to_de.setAttribute( "vol" + is, QString::number(
						m_osc[i].volKnob->value() ) );
		to_de.setAttribute( "pan" + is, QString::number(
						m_osc[i].panKnob->value() ) );
		to_de.setAttribute( "coarse" + is, QString::number(
					m_osc[i].coarseKnob->value() ) );
		to_de.setAttribute( "finel" + is, QString::number(
						m_osc[i].fineLKnob->value() ) );
		to_de.setAttribute( "finer" + is, QString::number(
						m_osc[i].fineRKnob->value() ) );
		to_de.setAttribute( "phoffset" + is, QString::number(
					m_osc[i].phaseOffsetKnob->value() ) );
		to_de.setAttribute( "stphdetun" + is, QString::number(
				m_osc[i].stereoPhaseDetuningKnob->value() ) );
		to_de.setAttribute( "wavetype" + is, QString::number(
							m_osc[i].waveShape ) );
		to_de.setAttribute( "userwavefile" + is,
					m_osc[i].m_sampleBuffer.audioFile() );
	}

	_parent.appendChild( to_de );
}




void tripleOscillator::loadSettings( const QDomElement & _this )
{
	m_modulationAlgo1 = static_cast<oscillator::modulationAlgos>(
					_this.attribute( "modalgo1" ).toInt() );
	m_modulationAlgo2 = static_cast<oscillator::modulationAlgos>(
					_this.attribute( "modalgo2" ).toInt() );

	getModulationButton( m_modulationAlgo1, 1 )->setChecked( TRUE );
	getModulationButton( m_modulationAlgo2, 2 )->setChecked( TRUE );

	for( int i = 0; i < NUM_OF_OSCILLATORS; ++i )
	{
		QString is = QString::number( i );
		m_osc[i].volKnob->setValue( _this.attribute( "vol" + is ).
								toFloat() );
		m_osc[i].panKnob->setValue( _this.attribute( "pan" + is ).
								toFloat() );
		m_osc[i].coarseKnob->setValue( _this.attribute( "coarse" + is ).
								toFloat() );
		m_osc[i].fineLKnob->setValue( _this.attribute( "finel" + is ).
								toFloat() );
		m_osc[i].fineRKnob->setValue( _this.attribute( "finer" + is ).
								toFloat() );
		m_osc[i].phaseOffsetKnob->setValue( _this.attribute(
						"phoffset" + is ).toFloat() );
		m_osc[i].stereoPhaseDetuningKnob->setValue( _this.attribute(
						"stphdetun" + is ).toFloat() );
		m_osc[i].m_sampleBuffer.setAudioFile( _this.attribute(
							"userwavefile" + is ) );
		switch( _this.attribute( "wavetype" + is ).toInt() )
		{
			case oscillator::TRIANGLE_WAVE:
				m_osc[i].triangleWaveBtn->setChecked( TRUE );
				break;
			case oscillator::SAW_WAVE:
				m_osc[i].sawWaveBtn->setChecked( TRUE );
				break;
			case oscillator::SQUARE_WAVE:
				m_osc[i].sqrWaveBtn->setChecked( TRUE );
				break;
			case oscillator::MOOG_SAW_WAVE:
				m_osc[i].moogSawWaveBtn->setChecked( TRUE );
				break;
			case oscillator::EXP_WAVE:
				m_osc[i].expWaveBtn->setChecked( TRUE );
				break;
			case oscillator::WHITE_NOISE_WAVE:
				m_osc[i].whiteNoiseWaveBtn->setChecked( TRUE );
				break;
			case oscillator::USER_DEF_WAVE:
				toolTip::add( m_osc[i].usrWaveBtn,
					m_osc[i].m_sampleBuffer.audioFile() );
				m_osc[i].usrWaveBtn->setChecked( TRUE );
				break;
			case oscillator::SIN_WAVE:
			default:
				m_osc[i].sinWaveBtn->setChecked( TRUE );
				break;
		}
	}
}




QString tripleOscillator::nodeName( void ) const
{
	return( tripleoscillator_plugin_descriptor.name );
}




void tripleOscillator::playNote( notePlayHandle * _n )
{
	if( _n->totalFramesPlayed() == 0 )
	{
		float freq = getChannelTrack()->frequency( _n );

		oscillator * oscs_l[NUM_OF_OSCILLATORS];
		oscillator * oscs_r[NUM_OF_OSCILLATORS];

		for( Sint8 i = NUM_OF_OSCILLATORS-1; i >= 0; --i )
		{

			float osc_detuning_l = pow( 2.0, (
				(float)m_osc[i].coarseKnob->value() * 100.0f +
				(float)m_osc[i].fineLKnob->value() ) / 1200.0f);
			float osc_detuning_r = pow( 2.0, (
				(float)m_osc[i].coarseKnob->value() * 100.0f +
				(float)m_osc[i].fineRKnob->value() ) / 1200.0f);

			float vol_fac_l = ( m_osc[i].panKnob->value() +
						PANNING_RIGHT ) / 100.0f;
			float vol_fac_r = ( PANNING_RIGHT -
						m_osc[i].panKnob->value() ) /
									100.0f;

			if( vol_fac_l > 1.0f )
			{
				vol_fac_l = 1.0f;
			}
			if( vol_fac_r > 1.0f )
			{
				vol_fac_r = 1.0f;
			}

			vol_fac_l *= m_osc[i].volKnob->value() / 100.0f;
			vol_fac_r *= m_osc[i].volKnob->value() / 100.0f;

			// the third oscs needs no sub-oscs...
			if( i == 2 )
			{
				oscs_l[i] = oscillator::createOsc(
						m_osc[i].waveShape,
						oscillator::MIX,
						freq*osc_detuning_l,
						static_cast<int>(
					m_osc[i].phaseOffsetKnob->value() +
				m_osc[i].stereoPhaseDetuningKnob->value() ),
								vol_fac_l );
				oscs_r[i] = oscillator::createOsc(
						m_osc[i].waveShape,
						oscillator::MIX,
						freq*osc_detuning_r,
						static_cast<int>(
					m_osc[i].phaseOffsetKnob->value() ),
								vol_fac_r );
			}
			else
			{
				oscs_l[i] = oscillator::createOsc(
						m_osc[i].waveShape,
						getModulationAlgo( i + 1 ),
						freq*osc_detuning_l,
						static_cast<int>(
					m_osc[i].phaseOffsetKnob->value() +
				m_osc[i].stereoPhaseDetuningKnob->value() ),
						vol_fac_l, oscs_l[i + 1] );
				oscs_r[i] = oscillator::createOsc(
						m_osc[i].waveShape,
						getModulationAlgo( i + 1 ),
						freq*osc_detuning_r,
						static_cast<int>(
					m_osc[i].phaseOffsetKnob->value() ),
								vol_fac_r,
								oscs_r[i + 1] );
			}

			if( m_osc[i].waveShape == oscillator::USER_DEF_WAVE )
			{
				oscs_l[i]->setUserWave(
					m_osc[i].m_sampleBuffer.data(),
					m_osc[i].m_sampleBuffer.frames() );
				oscs_r[i]->setUserWave(
					m_osc[i].m_sampleBuffer.data(),
					m_osc[i].m_sampleBuffer.frames() );
			}

		}

		_n->m_pluginData = new oscPtr;
		static_cast<oscPtr *>( _n->m_pluginData )->oscLeft = oscs_l[0];
		static_cast< oscPtr *>( _n->m_pluginData )->oscRight =
								oscs_r[0];
	}

	oscillator * osc_l = static_cast<oscPtr *>( _n->m_pluginData )->oscLeft;
	oscillator * osc_r = static_cast<oscPtr *>( _n->m_pluginData
								)->oscRight;

	const Uint32 frames = mixer::inst()->framesPerAudioBuffer();
	sampleFrame * buf = bufferAllocator::alloc<sampleFrame>( frames );
	
	osc_l->update( buf, frames, 0 );
	osc_r->update( buf, frames, 1 );

	getChannelTrack()->processAudioBuffer( buf, frames, _n );

	bufferAllocator::free( buf );
}




void tripleOscillator::deleteNotePluginData( notePlayHandle * _n )
{
	if( _n->m_pluginData == NULL )
	{
		return;
	}
	delete static_cast<oscillator *>( static_cast<oscPtr *>(
						_n->m_pluginData )->oscLeft );
	delete static_cast<oscillator *>( static_cast<oscPtr *>(
						_n->m_pluginData )->oscRight );
	delete static_cast<oscPtr *>( _n->m_pluginData );
}




// now follows all the stupid UI-Code...

void tripleOscillator::setModulationAlgo(
		oscillator::modulationAlgos _new_modulation_algo, int _n )
{
	if( _n == 1 )
	{
		m_modulationAlgo1 = _new_modulation_algo;
	}
	else
	{
		m_modulationAlgo2 = _new_modulation_algo;
	}

	songEditor::inst()->setModified();
}




oscillator::modulationAlgos tripleOscillator::getModulationAlgo( int _n )
{
	if( _n == 1 )
	{
		return( m_modulationAlgo1 );
	}
	else
	{
		return( m_modulationAlgo2 );
	}
}




void tripleOscillator::doSinWaveBtn( oscillatorData * _osc )
{
	_osc->waveShape = oscillator::SIN_WAVE;
	songEditor::inst()->setModified();
}




void tripleOscillator::doTriangleWaveBtn( oscillatorData * _osc )
{
	_osc->waveShape = oscillator::TRIANGLE_WAVE;
	songEditor::inst()->setModified();
}




void tripleOscillator::doSawWaveBtn( oscillatorData * _osc )
{
	_osc->waveShape = oscillator::SAW_WAVE;
	songEditor::inst()->setModified();
}




void tripleOscillator::doSqrWaveBtn( oscillatorData * _osc )
{
	_osc->waveShape = oscillator::SQUARE_WAVE;
	songEditor::inst()->setModified();
}




void tripleOscillator::doMoogSawWaveBtn( oscillatorData * _osc )
{
	_osc->waveShape = oscillator::MOOG_SAW_WAVE;
	songEditor::inst()->setModified();
}




void tripleOscillator::doExpWaveBtn( oscillatorData * _osc )
{
	_osc->waveShape = oscillator::EXP_WAVE;
	songEditor::inst()->setModified();
}




void tripleOscillator::doWhiteNoiseWaveBtn( oscillatorData * _osc )
{
	_osc->waveShape = oscillator::WHITE_NOISE_WAVE;
	songEditor::inst()->setModified();
}




void tripleOscillator::doUsrWaveBtn( oscillatorData * _osc )
{
	_osc->waveShape = oscillator::USER_DEF_WAVE;
	songEditor::inst()->setModified();
}



// Slots for Osc 1
void tripleOscillator::osc01SinWaveCh( bool _on )
{
	if( _on ) doSinWaveBtn( &m_osc[0] );
}

void tripleOscillator::osc01TriangleWaveCh( bool _on )
{
	if( _on ) doTriangleWaveBtn( &m_osc[0] );
}

void tripleOscillator::osc01SawWaveCh( bool _on )
{
	if( _on ) doSawWaveBtn( &m_osc[0] );
}

void tripleOscillator::osc01SquareWaveCh( bool _on )
{
	if( _on ) doSqrWaveBtn( &m_osc[0] );
}

void tripleOscillator::osc01MoogSawWaveCh( bool _on )
{
	if( _on ) doMoogSawWaveBtn( &m_osc[0] );
}

void tripleOscillator::osc01ExpWaveCh( bool _on )
{
	if( _on ) doExpWaveBtn( &m_osc[0] );
}

void tripleOscillator::osc01WhiteNoiseCh( bool _on )
{
	if( _on ) doWhiteNoiseWaveBtn( &m_osc[0] );
}

void tripleOscillator::osc01UserDefWaveCh( bool _on )
{
	if( _on ) doUsrWaveBtn( &m_osc[0] );
}

void tripleOscillator::osc01UserDefWaveDblClick( void )
{
	QString af = m_osc[0].m_sampleBuffer.openAudioFile();
	if( af != "" )
	{
		m_osc[0].m_sampleBuffer.setAudioFile( af );
/*#ifndef QT4
		toolTip::remove( m_osc[0].usrWaveBtn );
#endif*/
		toolTip::add( m_osc[0].usrWaveBtn,
					m_osc[0].m_sampleBuffer.audioFile() );
	}
}



// Slots for Osc 2
void tripleOscillator::osc02SinWaveCh( bool _on )
{
	if( _on ) doSinWaveBtn( &m_osc[1] );
}

void tripleOscillator::osc02TriangleWaveCh( bool _on )
{
	if( _on ) doTriangleWaveBtn( &m_osc[1] );
}

void tripleOscillator::osc02SawWaveCh( bool _on )
{
	if( _on ) doSawWaveBtn( &m_osc[1] );
}

void tripleOscillator::osc02SquareWaveCh( bool _on )
{
	if( _on ) doSqrWaveBtn( &m_osc[1] );
}

void tripleOscillator::osc02MoogSawWaveCh( bool _on )
{
	if( _on ) doMoogSawWaveBtn( &m_osc[1] );
}

void tripleOscillator::osc02ExpWaveCh( bool _on )
{
	if( _on ) doExpWaveBtn( &m_osc[1] );
}

void tripleOscillator::osc02WhiteNoiseCh( bool _on )
{
	if( _on ) doWhiteNoiseWaveBtn( &m_osc[1] );
}

void tripleOscillator::osc02UserDefWaveCh( bool _on )
{
	if( _on ) doUsrWaveBtn( &m_osc[1] );
}

void tripleOscillator::osc02UserDefWaveDblClick( void )
{
	QString af = m_osc[1].m_sampleBuffer.openAudioFile();
	if( af != "" )
	{
		m_osc[1].m_sampleBuffer.setAudioFile( af );
/*#ifndef QT4
		toolTip::remove( m_osc[1].usrWaveBtn );
#endif*/
		toolTip::add( m_osc[1].usrWaveBtn,
					m_osc[1].m_sampleBuffer.audioFile() );
	}
}


// Slots for Osc 3
void tripleOscillator::osc03SinWaveCh( bool _on )
{
	if( _on ) doSinWaveBtn( &m_osc[2] );
}

void tripleOscillator::osc03TriangleWaveCh( bool _on )
{
	if( _on ) doTriangleWaveBtn( &m_osc[2] );
}

void tripleOscillator::osc03SawWaveCh( bool _on )
{
	if( _on ) doSawWaveBtn( &m_osc[2] );
}

void tripleOscillator::osc03SquareWaveCh( bool _on )
{
	if( _on ) doSqrWaveBtn( &m_osc[2] );
}

void tripleOscillator::osc03MoogSawWaveCh( bool _on )
{
	if( _on ) doMoogSawWaveBtn( &m_osc[2] );
}

void tripleOscillator::osc03ExpWaveCh( bool _on )
{
	if( _on ) doExpWaveBtn( &m_osc[2] );
}

void tripleOscillator::osc03WhiteNoiseCh( bool _on )
{
	if( _on ) doWhiteNoiseWaveBtn( &m_osc[2] );
}

void tripleOscillator::osc03UserDefWaveCh( bool _on )
{
	if( _on ) doUsrWaveBtn( &m_osc[2] );
}

void tripleOscillator::osc03UserDefWaveDblClick( void )
{
	QString af = m_osc[2].m_sampleBuffer.openAudioFile();
	if( af != "" )
	{
		m_osc[2].m_sampleBuffer.setAudioFile( af );
/*#ifndef QT4
		toolTip::remove( m_osc[2].usrWaveBtn );
#endif*/
		toolTip::add( m_osc[2].usrWaveBtn,
					m_osc[2].m_sampleBuffer.audioFile() );
	}
}




void tripleOscillator::fm1BtnToggled( bool _on )
{
	if( _on ) setModulationAlgo( oscillator::FREQ_MODULATION, 1 );
}



void tripleOscillator::am1BtnToggled( bool _on )
{
	if( _on ) setModulationAlgo( oscillator::AMP_MODULATION, 1 );
}



void tripleOscillator::mix1BtnToggled( bool _on )
{
	if( _on ) setModulationAlgo( oscillator::MIX, 1 );
}



void tripleOscillator::sync1BtnToggled( bool _on )
{
	if( _on ) setModulationAlgo( oscillator::SYNC, 1 );
}



void tripleOscillator::fm2BtnToggled( bool _on )
{
	if( _on ) setModulationAlgo( oscillator::FREQ_MODULATION, 2 );
}



void tripleOscillator::am2BtnToggled( bool _on )
{
	if( _on ) setModulationAlgo( oscillator::AMP_MODULATION, 2 );
}



void tripleOscillator::mix2BtnToggled( bool _on )
{
	if( _on ) setModulationAlgo( oscillator::MIX, 2 );
}



void tripleOscillator::sync2BtnToggled( bool _on )
{
	if( _on ) setModulationAlgo( oscillator::SYNC, 2 );
}




pixmapButton * tripleOscillator::getModulationButton(
			oscillator::modulationAlgos _modulation_algo, int _n )
{
	if( _n == 1 )
	{
		switch( _modulation_algo )
		{
			case oscillator::FREQ_MODULATION: return( m_fm1OscBtn );
			case oscillator::AMP_MODULATION: return( m_am1OscBtn );
			case oscillator::MIX: return( m_mix1OscBtn );
			case oscillator::SYNC: return( m_sync1OscBtn );
		}
	}
	else
	{
		switch( _modulation_algo )
		{
			case oscillator::FREQ_MODULATION: return( m_fm2OscBtn );
			case oscillator::AMP_MODULATION: return( m_am2OscBtn );
			case oscillator::MIX: return( m_mix2OscBtn );
			case oscillator::SYNC: return( m_sync2OscBtn );
		}
	}
#ifdef LMMS_DEBUG
	// there's something really not ok, if this case occurs, so let's exit
	assert( 1 != 1 );
#endif
	return( NULL );
}




extern "C"
{

// neccessary for getting instance out of shared lib
plugin * lmms_plugin_main( void * _data )
{
	return( new tripleOscillator(
				static_cast<channelTrack *>( _data ) ) );
}

}


#undef setChecked


#include "triple_oscillator.moc"

