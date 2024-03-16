/*
 * AmplifierControlDialog.cpp - control dialog for amplifier effect
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "FFTFilterControlDialog.h"
#include "FFTFilterControls.h"
#include "embed.h"

#include "AutomatableButton.h"
#include "embed.h"
#include "Knob.h"
#include "LedCheckBox.h"
#include "PixmapButton.h"
#include "LcdSpinBox.h"
#include "PointGraph.h"

namespace lmms::gui
{

FFTFilterControlDialog::FFTFilterControlDialog(FFTFilterControls* controls) :
	EffectControlDialog(controls)
{
	setAutoFillBackground(true);
	QPalette pal;
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);
	setFixedSize(600, 700);
	/*
	auto makeKnob = [this](int x, int y, const QString& label, const QString& hintText, const QString& unit, FloatModel* model, bool isVolume)
	{
        Knob* newKnob = new Knob(KnobType::Bright26, this);
        newKnob->move(x, y);
        newKnob->setModel(model);
        newKnob->setLabel(label);
        newKnob->setHintText(hintText, unit);
        newKnob->setVolumeKnob(isVolume);
        return newKnob;
    };*/
	/*
	m_volumeModel.loadSettings(parent, "volume");
	m_freqControlModel.loadSettings(parent, "freqcontrol");
	m_effectControlModel.loadSettings(parent, "effectcontrol");
	m_bufferModel.loadSettings(parent, "buffer");
	m_displayFFTModel.loadSettings(parent, "display");
	*/
	
	auto volumeKnob = new Knob(KnobType::Bright26, this);
	volumeKnob->setModel(&controls->m_volumeModel);
	volumeKnob->setLabel(tr("VOLUME"));
	//->setCheckable(true);
	volumeKnob->setHintText(tr("Input gain:"),"");
	//->setVolumeKnob(true);
	//->setVolumeRatio(1.0);
	volumeKnob->move(10, 10);

	auto freqControlKnob = new Knob(KnobType::Bright26, this);
	freqControlKnob->setModel(&controls->m_freqControlModel);
	freqControlKnob->setLabel(tr("FREQ_A_TODO"));
	freqControlKnob->setHintText(tr("Input gain:"),"");
	freqControlKnob->move(10, 100);

	auto effectControlKnob = new Knob(KnobType::Bright26, this);
	effectControlKnob->setModel(&controls->m_effectControlModel);
	effectControlKnob->setLabel(tr("EFFECT_A_TODO"));
	effectControlKnob->setHintText(tr("Input gain:"),"");
	effectControlKnob->move(10, 150);

	auto bufferInput = new LcdSpinBox(2, this, "BUFFER_A_TODO");
	bufferInput->setModel(&controls->m_bufferModel);
	//bufferInput->setLabel(tr("INPUT"));
	//bufferInput->setHintText(tr("Input gain:"),"");
	bufferInput->move(10, 200);

	auto displayFFTKnob = new LedCheckBox("DISPLAY_A_TODO", this, tr("DISPLAY_A_TODO"), LedCheckBox::LedColor::Green);
	displayFFTKnob->setModel(&controls->m_displayFFTModel);
	//displayFFTKnob->setLabel(tr("DISPLAY_A_TODO"));
	displayFFTKnob->setCheckable(true);
	displayFFTKnob->move(300, 10);

	auto resetButton = new PixmapButton(this, tr("RESET_A_TODO"));
	resetButton->setCheckable(true);
	resetButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("smooth_active"));
	resetButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("smooth_inactive"));
	resetButton->setToolTip(tr("RESET_A_TODO"));
	resetButton->resize(13, 48);
	resetButton->move(300, 100);

	auto curGraph = new PointGraphView(this, 300, 200, 10, 1024);
	curGraph->setModel(&controls->m_graphModel);
	curGraph->move(100, 240);
/*
	auto swapInputs = new LedCheckBox("Swap inputs", this, tr("Swap inputs"), LedCheckBox::LedColor::Green);
	swapInputs->move( 20, 275 );
	swapInputs->setModel( & controls->m_swapInputs );
	swapInputs->setToolTip(tr("Swap left and right input channels for reflections"));
	*/
	/*
	auto  = new Knob(KnobType::Bright26, this);
	->setModel(&_controls->m_volumeModel);
	->setLabel(tr("INPUT"));
	->setCheckable(true);
	->setHintText(tr("Input gain:"),"");
	->setVolumeKnob(true);
	->setVolumeRatio(1.0);
	->move(26, 223);
	*/
}

} // namespace lmms::gui
