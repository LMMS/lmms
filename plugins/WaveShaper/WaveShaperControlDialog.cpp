/*
 * WaveShaperControlDialog.cpp - control dialog for WaveShaper effect
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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



#include "WaveShaperControlDialog.h"
#include "WaveShaperControls.h"
#include "embed.h"
#include "Graph.h"
#include "VectorGraph.h"
#include "Knob.h"
#include "PixmapButton.h"
#include "LedCheckBox.h"

namespace lmms::gui
{


WaveShaperControlDialog::WaveShaperControlDialog(
					WaveShaperControls * _controls ) :
	EffectControlDialog( _controls )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(),
				PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
	setFixedSize( 224, 274 );

	auto curGraph = new VectorGraphView(this, 204, 205, 10, 1024, false);
	curGraph->setModel(&_controls->m_vectorGraphModel);
	curGraph->setBackground(PLUGIN_NAME::getIconPixmap("wavegraph"));
	curGraph->applyDefaultColors();
	curGraph->move(10, 6);

	auto inputKnob = new Knob(KnobType::Bright26, this);
	inputKnob -> setVolumeKnob( true );
	inputKnob -> setVolumeRatio( 1.0 );
	inputKnob -> move( 26, 225 );
	inputKnob->setModel( &_controls->m_inputModel );
	inputKnob->setLabel( tr( "INPUT" ) );
	inputKnob->setHintText( tr( "Input gain:" ) , "" );

	auto outputKnob = new Knob(KnobType::Bright26, this);
	outputKnob -> setVolumeKnob( true );
	outputKnob -> setVolumeRatio( 1.0 );
	outputKnob -> move( 76, 225 );
	outputKnob->setModel( &_controls->m_outputModel );
	outputKnob->setLabel( tr( "OUTPUT" ) );
	outputKnob->setHintText( tr( "Output gain:" ), "" );

	auto resetButton = new PixmapButton(this, tr("Reset wavegraph"));
	resetButton -> move( 142, 225 );
	resetButton -> resize( 13, 46 );
	resetButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "reset_active" ) );
	resetButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "reset_inactive" ) );
	resetButton->setToolTip(tr("Reset wavegraph"));

	auto clipInputToggle = new LedCheckBox("Clip input", this, tr("Clip input"), LedCheckBox::LedColor::Green);
	clipInputToggle -> move( 131, 252 );
	clipInputToggle -> setModel( &_controls -> m_clipModel );
	clipInputToggle->setToolTip(tr("Clip input signal to 0 dB"));

	connect( resetButton, SIGNAL (clicked () ),
			_controls, SLOT ( resetClicked() ) );
}


} // namespace lmms::gui
