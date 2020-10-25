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
#include "gui_templates.h"
#include "ToolTip.h"


StereoControlControlDialog::StereoControlControlDialog(StereoControlControls* controls) :
	EffectControlDialog(controls),
	m_controls(controls)
{
	setAutoFillBackground(true);
	QPalette pal;
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);
	setFixedSize(335, 215);

	m_gainKnob = new Knob(knobBright_26, this);
	m_gainKnob->move(15, 126);
	m_gainKnob->setModel(&m_controls->m_gainModel);
	m_gainKnob->setHintText(tr("Gain:"), " db");
	ToolTip::add(m_gainKnob, tr("Gain"));

	m_stereoizerKnob = new Knob(knobBright_26, this);
	m_stereoizerKnob->move(155, 19);
	m_stereoizerKnob->setModel(&m_controls->m_stereoizerModel);
	m_stereoizerKnob->setHintText(tr("Stereoize:"), "%");
	ToolTip::add(m_stereoizerKnob, tr("Enhance width of any audio"));

	m_widthKnob = new Knob(knobBright_26, this);
	m_widthKnob->move(155, 68);
	m_widthKnob->setModel(&m_controls->m_widthModel);
	m_widthKnob->setHintText(tr("Width:"), "%");
	ToolTip::add(m_widthKnob, tr("Adjust volume of stereo audio"));

	m_monoBassFreqKnob = new Knob(knobBright_26, this);
	m_monoBassFreqKnob->move(256, 64);
	m_monoBassFreqKnob->setModel(&m_controls->m_monoBassFreqModel);
	m_monoBassFreqKnob->setHintText(tr("Mono Bass Frequency:"), " Hz");
	ToolTip::add(m_monoBassFreqKnob, tr("Remove sidebands from audio below this frequency"));

	m_stereoizerLPKnob = new Knob(knobSmall_17, this);
	m_stereoizerLPKnob->move(192, 31);
	m_stereoizerLPKnob->setModel(&m_controls->m_stereoizerLPModel);
	m_stereoizerLPKnob->setHintText(tr("Stereoizer Lowpass:"), " Hz");
	ToolTip::add(m_stereoizerLPKnob, tr("Lowpass delayed signal"));

	m_stereoizerHPKnob = new Knob(knobSmall_17, this);
	m_stereoizerHPKnob->move(128, 31);
	m_stereoizerHPKnob->setModel(&m_controls->m_stereoizerHPModel);
	m_stereoizerHPKnob->setHintText(tr("Stereoizer Highpass:"), " Hz");
	ToolTip::add(m_stereoizerHPKnob, tr("Highpass delayed signal"));

	m_panSpectralKnob = new Knob(knobSmall_17, this);
	m_panSpectralKnob->move(234, 171);
	m_panSpectralKnob->setModel(&m_controls->m_panSpectralModel);
	m_panSpectralKnob->setLabel(tr("Spectral"));
	m_panSpectralKnob->setHintText(tr("Spectral Panning:"), "%");
	ToolTip::add(m_panSpectralKnob, tr("Spectral adjustment amount"));

	m_panDelayKnob = new Knob(knobSmall_17, this);
	m_panDelayKnob->move(173, 171);
	m_panDelayKnob->setModel(&m_controls->m_panDelayModel);
	m_panDelayKnob->setLabel(tr("Delay"));
	m_panDelayKnob->setHintText(tr("Panning Delay:"), "%");
	ToolTip::add(m_panDelayKnob, tr("Single-channel delay length"));

	m_panDualLKnob = new Knob(knobBright_26, this);
	m_panDualLKnob->move(183, 162);
	m_panDualLKnob->setModel(&m_controls->m_panDualLModel);
	m_panDualLKnob->setLabel(tr("L"));
	m_panDualLKnob->setHintText(tr("Left Pan:"), "%");
	ToolTip::add(m_panDualLKnob, tr("Pan left channel"));

	m_panDualRKnob = new Knob(knobBright_26, this);
	m_panDualRKnob->move(225, 162);
	m_panDualRKnob->setModel(&m_controls->m_panDualRModel);
	m_panDualRKnob->setLabel(tr("R"));
	m_panDualRKnob->setHintText(tr("Right Pan:"), "%");
	ToolTip::add(m_panDualRKnob, tr("Pan right channel"));

	m_panKnob = new Knob(knobBright_26, this);
	m_panKnob->move(204, 161);
	m_panKnob->setModel(&m_controls->m_panModel);
	m_panKnob->setLabel(tr("PAN"));
	m_panKnob->setHintText(tr("Pan:"), "%");
	ToolTip::add(m_panKnob, tr("Pan"));

	m_gainButton = new PixmapButton(this, tr("Gain Panning"));
	m_gainButton->move(114, 135);
	m_gainButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("basic_sel"));
	m_gainButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("basic_unsel"));
	ToolTip::add(m_gainButton, tr("Change gain of each channel"));

	m_stereoButton = new PixmapButton(this, tr("Stereo Panning"));
	m_stereoButton->move(183, 135);
	m_stereoButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("dual_sel"));
	m_stereoButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("dual_unsel"));
	ToolTip::add(m_stereoButton, tr("Pan one channel into the other"));

	m_haasButton = new PixmapButton(this, tr("Haas Panning"));
	m_haasButton->move(252, 135);
	m_haasButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("binaural_sel"));
	m_haasButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("binaural_unsel"));
	ToolTip::add(m_haasButton, tr("Stereo panning with spectral adjustment and single-channel fractional delay"));

	m_panModeGroup = new automatableButtonGroup(this);
	m_panModeGroup->addButton(m_gainButton);
	m_panModeGroup->addButton(m_stereoButton);
	m_panModeGroup->addButton(m_haasButton);
	m_panModeGroup->setModel(&m_controls->m_panModeModel);

	m_monoButton = new PixmapButton(this, tr("Mono"));
	m_monoButton->move(238, 13);
	m_monoButton->setModel(&m_controls->m_monoModel);
	m_monoButton->setCheckable(true);
	m_monoButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("mono_sel"));
	m_monoButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("mono_unsel"));
	ToolTip::add(m_monoButton, tr("Converts input to mono"));

	m_dcButton = new PixmapButton(this, tr("DC Offset Removal"));
	m_dcButton->move(55, 146);
	m_dcButton->setModel(&m_controls->m_dcModel);
	m_dcButton->setCheckable(true);
	m_dcButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("dc_sel"));
	m_dcButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("dc_unsel"));
	ToolTip::add(m_dcButton, tr("Removes DC offset from the signal"));

	m_muteButton = new PixmapButton(this, tr("Mute"));
	m_muteButton->move(11, 181);
	m_muteButton->setModel(&m_controls->m_muteModel);
	m_muteButton->setCheckable(true);
	m_muteButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("mute_sel"));
	m_muteButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("mute_unsel"));
	ToolTip::add(m_muteButton, tr("Mute audio"));

	m_monoBassButton = new LedCheckBox("", this, tr("Mono Bass"), LedCheckBox::Green);
	m_monoBassButton->move(241, 41);
	m_monoBassButton->setModel(&m_controls->m_monoBassModel);
	m_monoBassButton->setCheckable(true);
	ToolTip::add(m_monoBassButton, tr("Mono Bass"));

	m_auditionButton = new PixmapButton(this, tr("Bass Mono audition"));
	m_auditionButton->move(305, 55);
	m_auditionButton->setModel(&m_controls->m_auditionModel);
	m_auditionButton->setCheckable(true);
	m_auditionButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("audition_sel"));
	m_auditionButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("audition_unsel"));
	ToolTip::add(m_auditionButton, tr("Bass Mono Audition"));

	m_invertLButton = new PixmapButton(this, tr("Invert Left Channel"));
	m_invertLButton->move(25, 77);
	m_invertLButton->setModel(&m_controls->m_invertLModel);
	m_invertLButton->setCheckable(true);
	m_invertLButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("invertL_sel"));
	m_invertLButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("invertL_unsel"));
	ToolTip::add(m_invertLButton, tr("Invert Left Channel"));

	m_invertRButton = new PixmapButton(this, tr("Invert Right Channel"));
	m_invertRButton->move(55, 77);
	m_invertRButton->setModel(&m_controls->m_invertRModel);
	m_invertRButton->setCheckable(true);
	m_invertRButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("invertR_sel"));
	m_invertRButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("invertR_unsel"));
	ToolTip::add(m_invertRButton, tr("Invert Right Channel"));

	m_soloChannelBox = new ComboBox(this);
	m_soloChannelBox->setGeometry(11, 13, 77, 22);
	m_soloChannelBox->setFont(pointSize<8>(m_soloChannelBox->font()));
	m_soloChannelBox->setModel(&m_controls->m_soloChannelModel);
	ToolTip::add(m_soloChannelBox, tr("Solo a single channel"));
	
	connect(&m_controls->m_panModeModel, SIGNAL(dataChanged()), this, SLOT(updateKnobVisibility()));
	emit updateKnobVisibility();
}


void StereoControlControlDialog::updateKnobVisibility()
{
	switch (m_controls->m_panModeModel.value())
	{
		case 0:
			m_panKnob->show();
			m_panDualLKnob->hide();
			m_panDualRKnob->hide();
			m_panSpectralKnob->hide();
			m_panDelayKnob->hide();
			break;
		case 1:
			m_panKnob->hide();
			m_panDualLKnob->show();
			m_panDualRKnob->show();
			m_panSpectralKnob->hide();
			m_panDelayKnob->hide();
			break;
		case 2:
			m_panKnob->show();
			m_panDualLKnob->hide();
			m_panDualRKnob->hide();
			m_panSpectralKnob->show();
			m_panDelayKnob->show();
			break;
	}
}
