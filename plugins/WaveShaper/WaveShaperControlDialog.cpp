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
#include "VectorGraphView.h"
#include "VectorGraphModel.h"
#include "Knob.h"
#include "PixmapButton.h"
#include "LedCheckBox.h"

namespace lmms::gui
{


WaveShaperControlDialog::WaveShaperControlDialog(
					WaveShaperControls * _controls ) :
	EffectControlDialog(_controls),
	m_vectorGraphWidget(nullptr)
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(),
				PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
	setFixedSize( 224, 274 );

	m_vectorGraphWidget = new VectorGraphView(this, 204, 205, 8, 30, false);
	m_vectorGraphWidget->setModel(&_controls->m_vectorGraphModel);
	m_vectorGraphWidget->setBackground(PLUGIN_NAME::getIconPixmap("wavegraph"));
	// this can cause problems with custom colors
	m_vectorGraphWidget->applyDefaultColors();
	// custom colors can be set this way (but this garph uses applyDefaultColros()):
	// example: m_vectorGraphWidget->setLineColor(QColor(210, 50, 50, 255), arrayLocation);
	m_vectorGraphWidget->move(10, 6);

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
	resetButton -> move(162, 225);
	resetButton -> resize( 13, 46 );
	resetButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "reset_active" ) );
	resetButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "reset_inactive" ) );
	resetButton->setToolTip(tr("Reset wavegraph"));

	auto simplifyButton = new PixmapButton(this, tr("Simplify graph displayed"));
	simplifyButton->move(112, 225);
	simplifyButton->resize(13, 46);
	simplifyButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("simplify_active"));
	simplifyButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("simplify_inactive"));
	simplifyButton->setToolTip(tr("Simplify the graph display for performance"));

	auto clipInputToggle = new LedCheckBox("Clip input", this, tr("Clip input"), LedCheckBox::LedColor::Green);
	clipInputToggle -> move( 131, 252 );
	clipInputToggle -> setModel( &_controls -> m_clipModel );
	clipInputToggle->setToolTip(tr("Clip input signal to 0 dB"));

	connect( resetButton, SIGNAL (clicked () ),
			_controls, SLOT ( resetClicked() ) );
	connect(simplifyButton, SIGNAL(clicked()),
			this, SLOT(simplifyClicked()));
}

void WaveShaperControlDialog::simplifyClicked()
{
	if (m_vectorGraphWidget != nullptr)
	{
		m_vectorGraphWidget->setIsSimplified(!m_vectorGraphWidget->getIsSimplified());
		m_vectorGraphWidget->model()->updateGraphModel(true);
	}
}

} // namespace lmms::gui
