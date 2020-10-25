/*
 * StereoControlControlDialog.cpp
 *
 * Copyright (c) 2020 Lost Robot <r94231@gmail.com>
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

#include <QLayout>

#include "StereoControlControlDialog.h"
#include "StereoControlControls.h"
#include "embed.h"
#include "ToolTip.h"
#include "Knob.h"
#include "PixmapButton.h"



StereoControlControlDialog::StereoControlControlDialog( StereoControlControls* controls ) :
	EffectControlDialog( controls )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
	setFixedSize( 350, 255 );

	Knob * volumeKnob = new Knob( knobBright_26, this);
	volumeKnob -> move( 40, 40 );
	volumeKnob -> setVolumeKnob( true );
	volumeKnob->setModel( &controls->m_volumeModel );
	volumeKnob->setLabel( tr( "VOL" ) );
	volumeKnob->setHintText( tr( "Volume:" ) , "%" );

	Knob * stereoizerKnob = new Knob( knobBright_26, this);
	stereoizerKnob -> move( 169, 19 );
	stereoizerKnob->setModel( &controls->m_stereoizerModel );
	stereoizerKnob->setHintText( tr( "Stereoize:" ) , "%" );

	Knob * widthKnob = new Knob( knobBright_26, this);
	widthKnob -> move( 169, 68 );
	widthKnob->setModel( &controls->m_widthModel );
	widthKnob->setHintText( tr( "Width:" ) , "%" );

	Knob * monoBassFreqKnob = new Knob( knobBright_26, this);
	monoBassFreqKnob -> move( 278, 33 );
	monoBassFreqKnob->setModel( &controls->m_monoBassFreqModel );
	monoBassFreqKnob->setHintText( tr( "Mono Bass Frequency:" ) , " Hz" );

	Knob * stereoizerLPKnob = new Knob( knobBright_26, this);
	stereoizerLPKnob -> move( 206, 31 );
	stereoizerLPKnob->setModel( &controls->m_stereoizerLPModel );
	stereoizerLPKnob->setHintText( tr( "Stereoizer Lowpass:" ) , " Hz" );

	Knob * stereoizerHPKnob = new Knob( knobBright_26, this);
	stereoizerHPKnob -> move( 140, 31 );
	stereoizerHPKnob->setModel( &controls->m_stereoizerHPModel );
	stereoizerHPKnob->setHintText( tr( "Stereoizer Highpass:" ) , " Hz" );

	Knob * panSpectralKnob = new Knob( knobBright_26, this);
	panSpectralKnob -> move( 200, 250 );
	panSpectralKnob->setModel( &controls->m_panSpectralModel );
	panSpectralKnob->setLabel( tr( "Spectral" ) );
	panSpectralKnob->setHintText( tr( "Spectral Panning:" ) , "%" );

	Knob * panWidthKnob = new Knob( knobBright_26, this);
	panWidthKnob -> move( 200, 150 );
	panWidthKnob->setModel( &controls->m_panWidthModel );
	panWidthKnob->setLabel( tr( "Pan Width" ) );
	panWidthKnob->setHintText( tr( "Pan Width:" ) , "%" );

	Knob * panKnob = new Knob( knobBright_26, this);
	panKnob -> move( 200, 200 );
	panKnob->setModel( &controls->m_panModel );
	panKnob->setLabel( tr( "PAN" ) );
	panKnob->setHintText( tr( "Pan:" ) , "%" );

	PixmapButton * gainButton = new PixmapButton(this, tr("Gain Panning"));
	gainButton->move(123, 135);
	gainButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("basic_sel"));
	gainButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("basic_unsel"));
	ToolTip::add(gainButton, tr("Change gain of each channel"));

	PixmapButton * stereoButton = new PixmapButton(this, tr("Stereo Panning"));
	stereoButton->move(198, 135);
	stereoButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("stereo_sel"));
	stereoButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("stereo_unsel"));
	ToolTip::add(stereoButton, tr("Pan one channel into the other"));

	PixmapButton * haasButton = new PixmapButton(this, tr("Haas Panning"));
	haasButton->move(273, 135);
	haasButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("binaural_sel"));
	haasButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("binaural_unsel"));
	ToolTip::add(haasButton, tr("Stereo panning with single-channel fractional delay"));

	automatableButtonGroup * panModeGroup = new automatableButtonGroup(this);
	panModeGroup->addButton(gainButton);
	panModeGroup->addButton(stereoButton);
	panModeGroup->addButton(haasButton);
	panModeGroup->setModel(&controls->m_panModeModel);

	PixmapButton * monoButton = new PixmapButton(this, tr("Mono"));
	monoButton->move(263, 87);
	monoButton->setModel(&controls->m_monoModel);
	monoButton->setCheckable(true);
	monoButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("mono_sel"));
	monoButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("mono_unsel"));
	ToolTip::add(monoButton, tr("Converts input to mono"));

	PixmapButton * dcButton = new PixmapButton(this, tr("DC Offset Removal"));
	dcButton->move(59, 168);
	dcButton->setModel(&controls->m_dcModel);
	dcButton->setCheckable(true);
	dcButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("dc_sel"));
	dcButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("dc_unsel"));
	ToolTip::add(dcButton, tr("Removes DC offset from the signal"));

	PixmapButton * muteButton = new PixmapButton(this, tr("Mute"));
	muteButton->move(16, 212);
	muteButton->setModel(&controls->m_muteModel);
	muteButton->setCheckable(true);
	muteButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("mute_sel"));
	muteButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("mute_unsel"));
	ToolTip::add(muteButton, tr("Mute audio"));

	PixmapButton * monoBassButton = new PixmapButton(this, tr("Mono Bass"));
	monoBassButton->move(278, 3);
	monoBassButton->setModel(&controls->m_monoBassModel);
	monoBassButton->setCheckable(true);
	monoBassButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("monoBass_sel"));
	monoBassButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("monoBass_unsel"));
	ToolTip::add(monoBassButton, tr("Mono Bass"));

	PixmapButton * auditionButton = new PixmapButton(this, tr("Bass Mono Audition"));
	auditionButton->move(325, 27);
	auditionButton->setModel(&controls->m_auditionModel);
	auditionButton->setCheckable(true);
	auditionButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("audition_sel"));
	auditionButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("audition_unsel"));
	ToolTip::add(auditionButton, tr("Bass Mono Audition"));

	PixmapButton * invertLButton = new PixmapButton(this, tr("Invert Left Channel"));
	invertLButton->move(30, 79);
	invertLButton->setModel(&controls->m_invertLModel);
	invertLButton->setCheckable(true);
	invertLButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("invertL_sel"));
	invertLButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("invertL_unsel"));
	ToolTip::add(invertLButton, tr("Invert Left Channel"));

	PixmapButton * invertRButton = new PixmapButton(this, tr("Invert Right Channel"));
	invertRButton->move(60, 79);
	invertRButton->setModel(&controls->m_invertRModel);
	invertRButton->setCheckable(true);
	invertRButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("invertR_sel"));
	invertRButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("invertR_unsel"));
	ToolTip::add(invertRButton, tr("Invert Right Channel"));

	PixmapButton * soloStereoButton = new PixmapButton(this, tr("Use both channels"));
	soloStereoButton->move(200, 200);
	soloStereoButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("soloStereo_sel"));
	soloStereoButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("soloStereo_unsel"));
	ToolTip::add(soloStereoButton, tr("Use both channels"));

	PixmapButton * soloLButton = new PixmapButton(this, tr("Solo left channel"));
	soloLButton->move(200, 217);
	soloLButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("soloL_sel"));
	soloLButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("soloL_unsel"));
	ToolTip::add(soloLButton, tr("Solo left channel"));

	PixmapButton * soloRButton = new PixmapButton(this, tr("Solo right channel"));
	soloRButton->move(200, 234);
	soloRButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("soloR_sel"));
	soloRButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("soloR_unsel"));
	ToolTip::add(soloRButton, tr("Solo right channel"));

	automatableButtonGroup * soloChannelGroup = new automatableButtonGroup(this);
	soloChannelGroup->addButton(soloStereoButton);
	soloChannelGroup->addButton(soloLButton);
	soloChannelGroup->addButton(soloRButton);
	soloChannelGroup->setModel(&controls->m_soloChannelModel);
}
