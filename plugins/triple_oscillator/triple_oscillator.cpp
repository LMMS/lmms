/*
 * triple_oscillator.cpp - powerful instrument with three oscillators
 *
 * Copyright (c) 2004-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 

#include "qt3support.h"

#ifdef QT4

#include <Qt/QtXml>
#include <QtGui/QBitmap>
#include <QtGui/QPainter>

#else

#include <qbitmap.h>
#include <qpainter.h>
#include <qdom.h>
#include <qwhatsthis.h>

#define setChecked setOn

#endif


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
	0x0100,
	plugin::Instrument,
	new QPixmap( PLUGIN_NAME::getIconPixmap( "logo" ) ),
	NULL
} ;

}

 
tripleOscillator::tripleOscillator( instrumentTrack * _channel_track ) :
	instrument( _channel_track, &tripleoscillator_plugin_descriptor )
{
#ifdef QT4
	setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( backgroundRole(),
				PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
#else
	setErasePixmap( PLUGIN_NAME::getIconPixmap( "artwork" ) );
#endif

	pixmapButton * pm_osc1_btn = new pixmapButton( this, NULL, NULL );
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

	pixmapButton * am_osc1_btn = new pixmapButton( this, NULL, NULL );
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

	pixmapButton * mix_osc1_btn = new pixmapButton( this, NULL, NULL );
	mix_osc1_btn->move( 126, 50 );
	mix_osc1_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"mix_active" ) );
	mix_osc1_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"mix_inactive" ) );
	mix_osc1_btn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap(
					"btn_mask" ).createHeuristicMask() ) );
	toolTip::add( mix_osc1_btn, tr( "mix output of oscillator 1 & 2" ) );

	pixmapButton * sync_osc1_btn = new pixmapButton( this, NULL, NULL );
	sync_osc1_btn->move( 166, 50 );
	sync_osc1_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"sync_active" ) );
	sync_osc1_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"sync_inactive" ) );
	sync_osc1_btn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap(
					"btn_mask" ).createHeuristicMask() ) );
	toolTip::add( sync_osc1_btn, tr( "synchronize oscillator 1 with "
							"oscillator 2" ) );

	pixmapButton * fm_osc1_btn = new pixmapButton( this, NULL, NULL );
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
						tr( "Modulation type 1" ),
						_channel_track );
	m_mod1BtnGrp->addButton( pm_osc1_btn );
	m_mod1BtnGrp->addButton( am_osc1_btn );
	m_mod1BtnGrp->addButton( mix_osc1_btn );
	m_mod1BtnGrp->addButton( sync_osc1_btn );
	m_mod1BtnGrp->addButton( fm_osc1_btn );
	m_mod1BtnGrp->setInitValue( m_osc[0].m_modulationAlgo );

	connect( m_mod1BtnGrp, SIGNAL( valueChanged( int ) ),
					&m_osc[0], SLOT( modCh( int ) ) );


	pixmapButton * pm_osc2_btn = new pixmapButton( this, NULL, NULL );
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

	pixmapButton * am_osc2_btn = new pixmapButton( this, NULL, NULL );
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

	pixmapButton * mix_osc2_btn = new pixmapButton( this, NULL, NULL );
	mix_osc2_btn->move( 126, 68 );
	mix_osc2_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"mix_active" ) );
	mix_osc2_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"mix_inactive" ) );
	mix_osc2_btn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap(
					"btn_mask" ).createHeuristicMask() ) );
	toolTip::add( mix_osc2_btn, tr("mix output of oscillator 2 & 3" ) );

	pixmapButton * sync_osc2_btn = new pixmapButton( this, NULL, NULL );
	sync_osc2_btn->move( 166, 68 );
	sync_osc2_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"sync_active" ) );
	sync_osc2_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"sync_inactive" ) );
	sync_osc2_btn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap(
					"btn_mask" ).createHeuristicMask() ) );
	toolTip::add( sync_osc2_btn, tr( "synchronize oscillator 2 with "
							"oscillator 3" ) );

	pixmapButton * fm_osc2_btn = new pixmapButton( this, NULL, NULL );
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
						tr( "Modulation type 2" ),
						_channel_track );
	m_mod2BtnGrp->addButton( pm_osc2_btn );
	m_mod2BtnGrp->addButton( am_osc2_btn );
	m_mod2BtnGrp->addButton( mix_osc2_btn );
	m_mod2BtnGrp->addButton( sync_osc2_btn );
	m_mod2BtnGrp->addButton( fm_osc2_btn );
	m_mod2BtnGrp->setInitValue( m_osc[1].m_modulationAlgo );

	connect( m_mod2BtnGrp, SIGNAL( valueChanged( int ) ),
					&m_osc[1], SLOT( modCh( int ) ) );


	for( int i = 0; i < NUM_OF_OSCILLATORS; ++i )
	{
		// setup volume-knob
		m_osc[i].m_volKnob = new volumeKnob( knobSmall_17, this, tr(
				"Osc %1 volume" ).arg( i+1 ), _channel_track );
		m_osc[i].m_volKnob->move( 6, 104 + i * 50 );
		m_osc[i].m_volKnob->setRange( MIN_VOLUME, MAX_VOLUME, 1.0f );
		m_osc[i].m_volKnob->setInitValue( DEFAULT_VOLUME
							/ NUM_OF_OSCILLATORS );
		m_osc[i].m_volKnob->setHintText( tr( "Osc %1 volume:" ).arg(
							i+1 ) + " ", "%" );
#ifdef QT4
		m_osc[i].m_volKnob->setWhatsThis(
#else
		QWhatsThis::add( m_osc[i].m_volKnob,
#endif
			tr( "With this knob you can set the volume of "
				"oscillator %1. When setting a value of 0 the "
				"oscillator is turned off. Otherwise you can "
				"hear the oscillator as loud as you set it "
				"here.").arg( i+1 ) );

		// setup panning-knob
		m_osc[i].m_panKnob = new knob( knobSmall_17, this,
				tr( "Osc %1 panning" ).arg( i + 1 ),
							_channel_track );
		m_osc[i].m_panKnob->move( 33, 104 + i * 50 );
		m_osc[i].m_panKnob->setRange( PANNING_LEFT, PANNING_RIGHT,
									1.0f );
		m_osc[i].m_panKnob->setInitValue( DEFAULT_PANNING );
		m_osc[i].m_panKnob->setHintText( tr("Osc %1 panning:").arg(
									i + 1 )
						+ " ", "" );
#ifdef QT4
		m_osc[i].m_panKnob->setWhatsThis(
#else
		QWhatsThis::add( m_osc[i].m_panKnob,
#endif
			tr( "With this knob you can set the panning of the "
				"oscillator %1. A value of -100 means 100% "
				"left and a value of 100 moves oscillator-"
				"output right.").arg( i+1 ) );

		// setup coarse-knob
		m_osc[i].m_coarseKnob = new knob( knobSmall_17, this,
				tr( "Osc %1 coarse detuning" ).arg( i + 1 ),
							_channel_track );
		m_osc[i].m_coarseKnob->move( 66, 104 + i * 50 );
		m_osc[i].m_coarseKnob->setRange( -2 * NOTES_PER_OCTAVE,
						2 * NOTES_PER_OCTAVE, 1.0f );
		m_osc[i].m_coarseKnob->setInitValue( 0.0f );
		m_osc[i].m_coarseKnob->setHintText(
			tr( "Osc %1 coarse detuning:" ).arg( i + 1 ) + " ",
						" " + tr( "semitones" ) );
#ifdef QT4
		m_osc[i].m_coarseKnob->setWhatsThis(
#else
		QWhatsThis::add( m_osc[i].m_coarseKnob,
#endif
			tr( "With this knob you can set the coarse detuning of "
				"oscillator %1. You can detune the oscillator "
				"12 semitones (1 octave) up and down. This is "
				"useful for creating sounds with a chord." ).
				arg( i + 1 ) );

		// setup knob for left fine-detuning
		m_osc[i].m_fineLKnob = new knob( knobSmall_17, this,
				tr( "Osc %1 fine detuning left" ).arg( i+1 ),
							_channel_track );
		m_osc[i].m_fineLKnob->move( 90, 104 + i * 50 );
		m_osc[i].m_fineLKnob->setRange( -100.0f, 100.0f, 1.0f );
		m_osc[i].m_fineLKnob->setInitValue( 0.0f );
		m_osc[i].m_fineLKnob->setHintText( tr( "Osc %1 fine detuning "
							"left:" ).arg( i + 1 )
							+ " ", " " +
							tr( "cents" ) );
#ifdef QT4
		m_osc[i].m_fineLKnob->setWhatsThis(
#else
		QWhatsThis::add( m_osc[i].m_fineLKnob,
#endif
			tr( "With this knob you can set the fine detuning of "
				"oscillator %1 for the left channel. The fine-"
				"detuning is ranged between -100 cents and "
				"+100 cents. This is useful for creating "
				"\"fat\" sounds." ).arg( i + 1 ) );

		// setup knob for right fine-detuning
		m_osc[i].m_fineRKnob = new knob( knobSmall_17, this,
						tr( "Osc %1 fine detuning right"
							).arg( i + 1 ),
							_channel_track );
		m_osc[i].m_fineRKnob->move( 110, 104 + i * 50 );
		m_osc[i].m_fineRKnob->setRange( -100.0f, 100.0f, 1.0f );
		m_osc[i].m_fineRKnob->setInitValue( 0.0f );
		m_osc[i].m_fineRKnob->setHintText( tr( "Osc %1 fine detuning "
							"right:").arg( i + 1 ) +
						" ", " " + tr( "cents" ) );
#ifdef QT4
		m_osc[i].m_fineRKnob->setWhatsThis(
#else
		QWhatsThis::add( m_osc[i].m_fineRKnob,
#endif
			tr( "With this knob you can set the fine detuning of "
				"oscillator %1 for the right channel. The "
				"fine-detuning is ranged between -100 cents "
				"and +100 cents. This is useful for creating "
				"\"fat\" sounds." ).arg( i+1 ) );

		// setup phase-offset-knob
		m_osc[i].m_phaseOffsetKnob = new knob( knobSmall_17, this,
							tr( "Osc %1 phase-"
							"offset" ).arg( i+1 ),
							_channel_track );
		m_osc[i].m_phaseOffsetKnob->move( 142, 104 + i * 50 );
		m_osc[i].m_phaseOffsetKnob->setRange( 0.0f, 360.0f, 1.0f );
		m_osc[i].m_phaseOffsetKnob->setInitValue( 0.0f );
		m_osc[i].m_phaseOffsetKnob->setHintText( tr( "Osc %1 phase-"
								"offset:" ).
								arg( i + 1 ) +
						" ", " " + tr( "degrees" ) );
#ifdef QT4
		m_osc[i].m_phaseOffsetKnob->setWhatsThis(
#else
		QWhatsThis::add( m_osc[i].m_phaseOffsetKnob,
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
		m_osc[i].m_stereoPhaseDetuningKnob = new knob( knobSmall_17,
						this, tr( "Osc %1 stereo phase-"
							"detuning" ).arg( i+1 ),
							_channel_track );
		m_osc[i].m_stereoPhaseDetuningKnob->move( 166, 104 + i * 50 );
		m_osc[i].m_stereoPhaseDetuningKnob->setRange( 0.0f, 360.0f,
									1.0f );
		m_osc[i].m_stereoPhaseDetuningKnob->setInitValue( 0.0f );
		m_osc[i].m_stereoPhaseDetuningKnob->setHintText( tr("Osc %1 "
								"stereo phase-"
								"detuning:" ).
								arg( i + 1 ) +
								" ", " " +
							tr( "degrees" ) );
#ifdef QT4
		m_osc[i].m_stereoPhaseDetuningKnob->setWhatsThis(
#else
		QWhatsThis::add( m_osc[i].m_stereoPhaseDetuningKnob,
#endif
			tr( "With this knob you can set the stereo phase-"
				"detuning of oscillator %1. The stereo phase-"
				"detuning specifies the size of the difference "
				"between the phase-offset of left and right "
				"channel. This is very good for creating wide "
				"stereo-sounds." ).arg( i+1 ) );

		// Connect knobs with oscillators' inputs
		connect( m_osc[i].m_volKnob, SIGNAL( valueChanged() ),
					&m_osc[i], SLOT( updateVolume() ) );
		connect( m_osc[i].m_panKnob, SIGNAL( valueChanged() ),
					&m_osc[i], SLOT( updateVolume() ) );
		m_osc[i].updateVolume();

		connect( m_osc[i].m_coarseKnob, SIGNAL( valueChanged() ),
				&m_osc[i], SLOT( updateDetuningLeft() ) );
		connect( m_osc[i].m_coarseKnob, SIGNAL( valueChanged() ),
				&m_osc[i], SLOT( updateDetuningRight() ) );
		connect( m_osc[i].m_fineLKnob, SIGNAL( valueChanged() ),
				&m_osc[i], SLOT( updateDetuningLeft() ) );
		connect( m_osc[i].m_fineRKnob, SIGNAL( valueChanged() ),
				&m_osc[i], SLOT( updateDetuningRight() ) );
		m_osc[i].updateDetuningLeft();
		m_osc[i].updateDetuningRight();

		connect( m_osc[i].m_phaseOffsetKnob, SIGNAL( valueChanged() ),
				&m_osc[i], SLOT( updatePhaseOffsetLeft() ) );
		connect( m_osc[i].m_phaseOffsetKnob, SIGNAL( valueChanged() ),
				&m_osc[i], SLOT( updatePhaseOffsetRight() ) );
		connect( m_osc[i].m_stereoPhaseDetuningKnob,
						SIGNAL( valueChanged() ),
				&m_osc[i], SLOT( updatePhaseOffsetLeft() ) );
		m_osc[i].updatePhaseOffsetLeft();
		m_osc[i].updatePhaseOffsetRight();

		pixmapButton * sin_wave_btn = new pixmapButton( this, NULL,
									NULL );
		sin_wave_btn->move( 188, 105 + i * 50 );
		sin_wave_btn->setActiveGraphic( embed::getIconPixmap(
							"sin_wave_active" ) );
		sin_wave_btn->setInactiveGraphic( embed::getIconPixmap(
							"sin_wave_inactive" ) );
		sin_wave_btn->setChecked( TRUE );
		toolTip::add( sin_wave_btn,
				tr( "Click here if you want a sine-wave for "
						"current oscillator." ) );

		pixmapButton * triangle_wave_btn = new pixmapButton( this, NULL,
									NULL );
		triangle_wave_btn->move( 203, 105 + i * 50 );
		triangle_wave_btn->setActiveGraphic(
			embed::getIconPixmap( "triangle_wave_active" ) );
		triangle_wave_btn->setInactiveGraphic(
			embed::getIconPixmap( "triangle_wave_inactive" ) );
		toolTip::add( triangle_wave_btn,
				tr( "Click here if you want a triangle-wave "
						"for current oscillator." ) );

		pixmapButton * saw_wave_btn = new pixmapButton( this, NULL,
									NULL );
		saw_wave_btn->move( 218, 105 + i * 50 );
		saw_wave_btn->setActiveGraphic( embed::getIconPixmap(
							"saw_wave_active" ) );
		saw_wave_btn->setInactiveGraphic( embed::getIconPixmap(
							"saw_wave_inactive" ) );
		toolTip::add( saw_wave_btn,
				tr( "Click here if you want a saw-wave for "
						"current oscillator." ) );

		pixmapButton * sqr_wave_btn = new pixmapButton( this, NULL,
									NULL );
		sqr_wave_btn->move( 233, 105 + i * 50 );
		sqr_wave_btn->setActiveGraphic( embed::getIconPixmap(
						"square_wave_active" ) );
		sqr_wave_btn->setInactiveGraphic( embed::getIconPixmap(
						"square_wave_inactive" ) );
		toolTip::add( sqr_wave_btn,
				tr( "Click here if you want a square-wave for "
						"current oscillator." ) );

		pixmapButton * moog_saw_wave_btn = new pixmapButton( this, NULL,
									NULL );
		moog_saw_wave_btn->move( 188, 120+i*50 );
		moog_saw_wave_btn->setActiveGraphic(
			embed::getIconPixmap( "moog_saw_wave_active" ) );
		moog_saw_wave_btn->setInactiveGraphic(
			embed::getIconPixmap( "moog_saw_wave_inactive" ) );
		toolTip::add( moog_saw_wave_btn,
				tr( "Click here if you want a moog-saw-wave "
						"for current oscillator." ) );

		pixmapButton * exp_wave_btn = new pixmapButton( this, NULL,
									NULL );
		exp_wave_btn->move( 203, 120+i*50 );
		exp_wave_btn->setActiveGraphic( embed::getIconPixmap(
							"exp_wave_active" ) );
		exp_wave_btn->setInactiveGraphic( embed::getIconPixmap(
							"exp_wave_inactive" ) );
		toolTip::add( exp_wave_btn,
				tr( "Click here if you want an exponential "
					"wave for current oscillator." ) );

		pixmapButton * white_noise_btn = new pixmapButton( this, NULL,
									NULL );
		white_noise_btn->move( 218, 120+i*50 );
		white_noise_btn->setActiveGraphic(
			embed::getIconPixmap( "white_noise_wave_active" ) );
		white_noise_btn->setInactiveGraphic(
			embed::getIconPixmap( "white_noise_wave_inactive" ) );
		toolTip::add( white_noise_btn,
				tr( "Click here if you want a white-noise for "
						"current oscillator." ) );

		m_osc[i].m_usrWaveBtn = new pixmapButton( this, NULL, NULL );
		m_osc[i].m_usrWaveBtn->move( 233, 120+i*50 );
		m_osc[i].m_usrWaveBtn->setActiveGraphic( embed::getIconPixmap(
							"usr_wave_active" ) );
		m_osc[i].m_usrWaveBtn->setInactiveGraphic( embed::getIconPixmap(
							"usr_wave_inactive" ) );
		toolTip::add( m_osc[i].m_usrWaveBtn,
				tr( "Click here if you want a user-defined "
				"wave-shape for current oscillator." ) );

		m_osc[i].m_waveBtnGrp = new automatableButtonGroup( this,
					tr( "Osc %1 wave shape" ).arg( i + 1 ),
					_channel_track );
		m_osc[i].m_waveBtnGrp->addButton( sin_wave_btn );
		m_osc[i].m_waveBtnGrp->addButton( triangle_wave_btn );
		m_osc[i].m_waveBtnGrp->addButton( saw_wave_btn );
		m_osc[i].m_waveBtnGrp->addButton( sqr_wave_btn );
		m_osc[i].m_waveBtnGrp->addButton( moog_saw_wave_btn );
		m_osc[i].m_waveBtnGrp->addButton( exp_wave_btn );
		m_osc[i].m_waveBtnGrp->addButton( white_noise_btn );
		m_osc[i].m_waveBtnGrp->addButton( m_osc[i].m_usrWaveBtn );

		connect( m_osc[i].m_waveBtnGrp, SIGNAL( valueChanged( int ) ),
					&m_osc[i], SLOT( oscWaveCh( int ) ) );
		connect( m_osc[i].m_usrWaveBtn, SIGNAL( doubleClicked() ),
				&m_osc[i], SLOT( oscUserDefWaveDblClick() ) );
	}

	connect( engine::getMixer(), SIGNAL( sampleRateChanged() ),
			this, SLOT( updateAllDetuning() ) );
}




tripleOscillator::~tripleOscillator()
{
}




void tripleOscillator::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	m_mod1BtnGrp->saveSettings( _doc, _this, "modalgo1" );
	m_mod2BtnGrp->saveSettings( _doc, _this, "modalgo2" );

	for( int i = 0; i < NUM_OF_OSCILLATORS; ++i )
	{
		QString is = QString::number( i );
		m_osc[i].m_volKnob->saveSettings( _doc, _this, "vol" + is );
		m_osc[i].m_panKnob->saveSettings( _doc, _this, "pan" + is );
		m_osc[i].m_coarseKnob->saveSettings( _doc, _this, "coarse"
									+ is );
		m_osc[i].m_fineLKnob->saveSettings( _doc, _this, "finel" + is );
		m_osc[i].m_fineRKnob->saveSettings( _doc, _this, "finer" + is );
		m_osc[i].m_phaseOffsetKnob->saveSettings( _doc, _this,
							"phoffset" + is );
		m_osc[i].m_stereoPhaseDetuningKnob->saveSettings( _doc, _this,
							"stphdetun" + is );
		m_osc[i].m_waveBtnGrp->saveSettings( _doc, _this,
							"wavetype" + is );
		_this.setAttribute( "userwavefile" + is,
					m_osc[i].m_sampleBuffer->audioFile() );
	}
}




void tripleOscillator::loadSettings( const QDomElement & _this )
{
	m_mod1BtnGrp->loadSettings( _this, "modalgo1" );
	m_mod2BtnGrp->loadSettings( _this, "modalgo2" );

	for( int i = 0; i < NUM_OF_OSCILLATORS; ++i )
	{
		QString is = QString::number( i );
		m_osc[i].m_volKnob->loadSettings( _this, "vol" + is );
		m_osc[i].m_panKnob->loadSettings( _this, "pan" + is );
		m_osc[i].m_coarseKnob->loadSettings( _this, "coarse" + is );
		m_osc[i].m_fineLKnob->loadSettings( _this, "finel" + is );
		m_osc[i].m_fineRKnob->loadSettings( _this, "finer" + is );
		m_osc[i].m_phaseOffsetKnob->loadSettings( _this,
							"phoffset" + is );
		m_osc[i].m_stereoPhaseDetuningKnob->loadSettings( _this,
							"stphdetun" + is );
		m_osc[i].m_sampleBuffer->setAudioFile( _this.attribute(
							"userwavefile" + is ) );
		m_osc[i].m_waveBtnGrp->loadSettings( _this, "wavetype" + is );
	}
}




void tripleOscillator::setParameter( const QString & _param,
							const QString & _value )
{
	if( _param == "samplefile" )
	{
		for( int i = 0; i < NUM_OF_OSCILLATORS; ++i )
		{
			m_osc[i].m_sampleBuffer->setAudioFile( _value );
		}
	}
}




QString tripleOscillator::nodeName( void ) const
{
	return( tripleoscillator_plugin_descriptor.name );
}




void tripleOscillator::playNote( notePlayHandle * _n, bool )
{
	if( _n->totalFramesPlayed() == 0 )
	{
		oscillator * oscs_l[NUM_OF_OSCILLATORS];
		oscillator * oscs_r[NUM_OF_OSCILLATORS];

		for( Sint8 i = NUM_OF_OSCILLATORS - 1; i >= 0; --i )
		{

			// the last oscs needs no sub-oscs...
			if( i == NUM_OF_OSCILLATORS - 1 )
			{
				oscs_l[i] = new oscillator(
						m_osc[i].m_waveShape,
						m_osc[i].m_modulationAlgo,
						_n->frequency(),
						m_osc[i].m_detuningLeft,
						m_osc[i].m_phaseOffsetLeft,
						m_osc[i].m_volumeLeft );
				oscs_r[i] = new oscillator(
						m_osc[i].m_waveShape,
						m_osc[i].m_modulationAlgo,
						_n->frequency(),
						m_osc[i].m_detuningRight,
						m_osc[i].m_phaseOffsetRight,
						m_osc[i].m_volumeRight );
			}
			else
			{
				oscs_l[i] = new oscillator(
						m_osc[i].m_waveShape,
						m_osc[i].m_modulationAlgo,
						_n->frequency(),
						m_osc[i].m_detuningLeft,
						m_osc[i].m_phaseOffsetLeft,
						m_osc[i].m_volumeLeft,
						oscs_l[i + 1] );
				oscs_r[i] = new oscillator(
						m_osc[i].m_waveShape,
						m_osc[i].m_modulationAlgo,
						_n->frequency(),
						m_osc[i].m_detuningRight,
						m_osc[i].m_phaseOffsetRight,
						m_osc[i].m_volumeRight,
						oscs_r[i + 1] );
			}

			oscs_l[i]->setUserWave( m_osc[i].m_sampleBuffer );
			oscs_r[i]->setUserWave( m_osc[i].m_sampleBuffer );

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




void tripleOscillator::updateAllDetuning( void )
{
	for( int i = 0; i < NUM_OF_OSCILLATORS; ++i )
	{
		m_osc[i].updateDetuningLeft();
		m_osc[i].updateDetuningRight();
	}
}








oscillatorObject::oscillatorObject( void ) :
	m_waveShape( oscillator::SIN_WAVE ),
	m_sampleBuffer( new sampleBuffer ),
	m_modulationAlgo( oscillator::MIX )
{
}




oscillatorObject::~oscillatorObject()
{
	sharedObject::unref( m_sampleBuffer );
}




void oscillatorObject::oscWaveCh( int _n )
{
	m_waveShape = static_cast<oscillator::waveShapes>( _n );
}




void oscillatorObject::oscUserDefWaveDblClick( void )
{
	QString af = m_sampleBuffer->openAudioFile();
	if( af != "" )
	{
		m_sampleBuffer->setAudioFile( af );
		toolTip::add( m_usrWaveBtn, m_sampleBuffer->audioFile() );
	}
}




void oscillatorObject::modCh( int _n )
{
	m_modulationAlgo = static_cast<oscillator::modulationAlgos>( _n );
}




void oscillatorObject::updateVolume( void )
{
	if( m_panKnob->value() >= 0.0f )
	{
		float panningFactorLeft = 1.0f - m_panKnob->value()
							/ (float)PANNING_RIGHT;
		m_volumeLeft = panningFactorLeft * m_volKnob->value() / 100.0f;
		m_volumeRight = m_volKnob->value() / 100.0f;
	}
	else
	{
		m_volumeLeft = m_volKnob->value() / 100.0f;
		float panningFactorRight = 1.0f + m_panKnob->value()
							/ (float)PANNING_RIGHT;
		m_volumeRight = panningFactorRight * m_volKnob->value()
								/ 100.0f;
	}
}




void oscillatorObject::updateDetuningLeft( void )
{
	m_detuningLeft = powf( 2.0f, ( (float)m_coarseKnob->value() * 100.0f
				+ (float)m_fineLKnob->value() ) / 1200.0f )
					/ engine::getMixer()->sampleRate();
}




void oscillatorObject::updateDetuningRight( void )
{
	m_detuningRight = powf( 2.0f, ( (float)m_coarseKnob->value() * 100.0f
				+ (float)m_fineRKnob->value() ) / 1200.0f )
					/ engine::getMixer()->sampleRate();
}




void oscillatorObject::updatePhaseOffsetLeft( void )
{
	m_phaseOffsetLeft = ( m_phaseOffsetKnob->value() +
				m_stereoPhaseDetuningKnob->value() ) / 360.0f;
}




void oscillatorObject::updatePhaseOffsetRight( void )
{
	m_phaseOffsetRight = m_phaseOffsetKnob->value() / 360.0f;
}




extern "C"
{

// neccessary for getting instance out of shared lib
plugin * lmms_plugin_main( void * _data )
{
	return( new tripleOscillator(
				static_cast<instrumentTrack *>( _data ) ) );
}

}


#undef setChecked


#include "triple_oscillator.moc"

