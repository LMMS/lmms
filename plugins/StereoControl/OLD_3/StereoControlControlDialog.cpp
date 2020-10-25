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
#include "LedCheckbox.h"
#include "ComboBox.h"
#include "gui_templates.h"


StereoControlControlDialog::StereoControlControlDialog( StereoControlControls* controls ) :
	EffectControlDialog(controls)
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
	setFixedSize( 359, 255 );

	Knob * volumeKnob = new Knob( knobBright_26, this);
	volumeKnob -> move( 18, 148 );
	volumeKnob -> setVolumeKnob( true );
	volumeKnob->setModel( &controls->m_volumeModel );
	volumeKnob->setHintText( tr( "Volume:" ) , "%" );

	Knob * stereoizerKnob = new Knob( knobBright_26, this);
	stereoizerKnob -> move( 168, 20 );
	stereoizerKnob->setModel( &controls->m_stereoizerModel );
	stereoizerKnob->setHintText( tr( "Stereoize:" ) , "%" );

	Knob * widthKnob = new Knob( knobBright_26, this);
	widthKnob -> move( 168, 69 );
	widthKnob->setModel( &controls->m_widthModel );
	widthKnob->setHintText( tr( "Width:" ) , "%" );

	Knob * monoBassFreqKnob = new Knob( knobBright_26, this);
	monoBassFreqKnob -> move( 277, 34 );
	monoBassFreqKnob->setModel( &controls->m_monoBassFreqModel );
	monoBassFreqKnob->setHintText( tr( "Mono Bass Frequency:" ) , " Hz" );

	Knob * stereoizerLPKnob = new Knob( knobSmall_17, this);
	stereoizerLPKnob -> move( 205, 32 );
	stereoizerLPKnob->setModel( &controls->m_stereoizerLPModel );
	stereoizerLPKnob->setHintText( tr( "Stereoizer Lowpass:" ) , " Hz" );

	Knob * stereoizerHPKnob = new Knob( knobSmall_17, this);
	stereoizerHPKnob -> move( 139, 32 );
	stereoizerHPKnob->setModel( &controls->m_stereoizerHPModel );
	stereoizerHPKnob->setHintText( tr( "Stereoizer Highpass:" ) , " Hz" );

	Knob * panSpectralKnob = new Knob( knobBright_26, this);
	panSpectralKnob -> move( 250, 200 );
	panSpectralKnob->setModel( &controls->m_panSpectralModel );
	panSpectralKnob->setLabel( tr( "Spectral" ) );
	panSpectralKnob->setHintText( tr( "Spectral Panning:" ) , "%" );

	Knob * panDelayKnob = new Knob( knobBright_26, this);
	panDelayKnob -> move( 150, 200 );
	panDelayKnob->setModel( &controls->m_panDelayModel );
	panDelayKnob->setLabel( tr( "Delay" ) );
	panDelayKnob->setHintText( tr( "Panning Delay:" ) , "%" );

	Knob * panDualLKnob = new Knob( knobBright_26, this);
	panDualLKnob -> move( 150, 230 );
	panDualLKnob->setModel( &controls->m_panDualLModel );
	panDualLKnob->setLabel( tr( "L" ) );
	panDualLKnob->setHintText( tr( "Left Pan:" ) , "%" );

	Knob * panDualRKnob = new Knob( knobBright_26, this);
	panDualRKnob -> move( 180, 230 );
	panDualRKnob->setModel( &controls->m_panDualRModel );
	panDualRKnob->setLabel( tr( "R" ) );
	panDualRKnob->setHintText( tr( "Right Pan:" ) , "%" );

	Knob * panKnob = new Knob( knobBright_26, this);
	panKnob -> move( 200, 200 );
	panKnob->setModel( &controls->m_panModel );
	panKnob->setLabel( tr( "PAN" ) );
	panKnob->setHintText( tr( "Pan:" ) , "%" );

	PixmapButton * gainButton = new PixmapButton(this, tr("Gain Panning"));
	gainButton->move(122, 136);
	gainButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("basic_sel"));
	gainButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("basic_unsel"));
	ToolTip::add(gainButton, tr("Change gain of each channel"));

	PixmapButton * stereoButton = new PixmapButton(this, tr("Stereo Panning"));
	stereoButton->move(197, 136);
	stereoButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("dual_sel"));
	stereoButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("dual_unsel"));
	ToolTip::add(stereoButton, tr("Pan one channel into the other"));

	PixmapButton * haasButton = new PixmapButton(this, tr("Haas Panning"));
	haasButton->move(273, 136);
	haasButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("binaural_sel"));
	haasButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("binaural_unsel"));
	ToolTip::add(haasButton, tr("Stereo panning with single-channel fractional delay"));

	automatableButtonGroup * panModeGroup = new automatableButtonGroup(this);
	panModeGroup->addButton(gainButton);
	panModeGroup->addButton(stereoButton);
	panModeGroup->addButton(haasButton);
	panModeGroup->setModel(&controls->m_panModeModel);

	PixmapButton * monoButton = new PixmapButton(this, tr("Mono"));
	monoButton->move(262, 88);
	monoButton->setModel(&controls->m_monoModel);
	monoButton->setCheckable(true);
	monoButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("mono_sel"));
	monoButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("mono_unsel"));
	ToolTip::add(monoButton, tr("Converts input to mono"));

	PixmapButton * dcButton = new PixmapButton(this, tr("DC Offset Removal"));
	dcButton->move(58, 161);
	dcButton->setModel(&controls->m_dcModel);
	dcButton->setCheckable(true);
	dcButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("dc_sel"));
	dcButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("dc_unsel"));
	ToolTip::add(dcButton, tr("Removes DC offset from the signal"));

	PixmapButton * muteButton = new PixmapButton(this, tr("Mute"));
	muteButton->move(15, 213);
	muteButton->setModel(&controls->m_muteModel);
	muteButton->setCheckable(true);
	muteButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("mute_sel"));
	muteButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("mute_unsel"));
	ToolTip::add(muteButton, tr("Mute audio"));

	LedCheckBox * monoBassButton = new LedCheckBox("", this, tr("Mono Bass"), LedCheckBox::Green);
	monoBassButton->move(260, 14);
	monoBassButton->setModel(&controls->m_monoBassModel);
	monoBassButton->setCheckable(true);
	ToolTip::add(monoBassButton, tr("Mono Bass"));

	PixmapButton * auditionButton = new PixmapButton(this, tr("Bass Mono Audition"));
	auditionButton->move(324, 28);
	auditionButton->setModel(&controls->m_auditionModel);
	auditionButton->setCheckable(true);
	auditionButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("audition_sel"));
	auditionButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("audition_unsel"));
	ToolTip::add(auditionButton, tr("Bass Mono Audition"));

	PixmapButton * invertLButton = new PixmapButton(this, tr("Invert Left Channel"));
	invertLButton->move(29, 89);
	invertLButton->setModel(&controls->m_invertLModel);
	invertLButton->setCheckable(true);
	invertLButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("invertL_sel"));
	invertLButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("invertL_unsel"));
	ToolTip::add(invertLButton, tr("Invert Left Channel"));

	PixmapButton * invertRButton = new PixmapButton(this, tr("Invert Right Channel"));
	invertRButton->move(59, 89);
	invertRButton->setModel(&controls->m_invertRModel);
	invertRButton->setCheckable(true);
	invertRButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("invertR_sel"));
	invertRButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("invertR_unsel"));
	ToolTip::add(invertRButton, tr("Invert Right Channel"));

	ComboBox * m_soloChannelBox = new ComboBox(this);
	m_soloChannelBox->setGeometry(13, 21, 82, 22);
	m_soloChannelBox->setFont(pointSize<8>(m_soloChannelBox->font()));
	m_soloChannelBox->move(12, 20);
	m_soloChannelBox->setModel(&controls->m_soloChannelModel);
}
