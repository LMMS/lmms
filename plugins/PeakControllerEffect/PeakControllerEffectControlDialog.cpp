/*
 * PeakControllerEffectControlDialog.cpp - control dialog for
 *                                             PeakControllerEffect
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
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
#include <QVBoxLayout>

#include "FontHelper.h"
#include "PeakControllerEffectControlDialog.h"
#include "PeakControllerEffectControls.h"
#include "ControllerRackView.h"
#include "GuiApplication.h"
#include "Knob.h"
#include "LedCheckBox.h"
#include "embed.h"

namespace lmms::gui
{

PeakControllerEffectControlDialog::PeakControllerEffectControlDialog(
				PeakControllerEffectControls * _controls ) :
	EffectControlDialog( _controls )
{
	setWindowIcon( embed::getIconPixmap( "controller" ) );
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush(backgroundRole(), QBrush(embed::getIconPixmap("peak_controller_artwork")));
	setPalette( pal );
	setFixedSize(240, 80);

	auto knobContainer = new QWidget(this);
	knobContainer->setContentsMargins(0, 0, 0, 0);
	knobContainer->setFixedSize(240, 50);

	auto ledContainer = new QWidget(this);
	ledContainer->setContentsMargins(0, 0, 0, 0);
	ledContainer->setFixedSize(240, 30);

	m_baseKnob = new Knob(KnobType::Bright26, tr("BASE"), SMALL_FONT_SIZE, knobContainer);
	m_baseKnob->setModel(&_controls->m_baseModel);
	m_baseKnob->setHintText(tr( "Base:" ), "");

	m_amountKnob = new Knob(KnobType::Bright26, tr("AMNT"), SMALL_FONT_SIZE, knobContainer);
	m_amountKnob->setModel(&_controls->m_amountModel);
	m_amountKnob->setHintText(tr("Modulation amount:" ), "");

	m_amountMultKnob = new Knob(KnobType::Bright26, tr("MULT"), SMALL_FONT_SIZE, knobContainer);
	m_amountMultKnob->setModel(&_controls->m_amountMultModel);
	m_amountMultKnob->setHintText(tr("Amount multiplicator:"), "");

	m_attackKnob = new Knob(KnobType::Bright26, tr("ATCK"), SMALL_FONT_SIZE, knobContainer);
	m_attackKnob->setModel(&_controls->m_attackModel);
	m_attackKnob->setHintText(tr("Attack:"), "");

	m_decayKnob = new Knob(KnobType::Bright26, tr("DCAY"), SMALL_FONT_SIZE, knobContainer);
	m_decayKnob->setModel(&_controls->m_decayModel);
	m_decayKnob->setHintText(tr("Release:"), "");

	m_thresholdKnob = new Knob(KnobType::Bright26, tr("TRSH"), SMALL_FONT_SIZE, knobContainer);
	m_thresholdKnob->setModel(&_controls->m_tresholdModel);
	m_thresholdKnob->setHintText(tr("Threshold:") , "");

	m_muteLed = new LedCheckBox(tr("Mute output"), ledContainer);
	m_muteLed->setModel(&_controls->m_muteModel);

	m_absLed = new LedCheckBox(tr("Absolute value"), ledContainer);
	m_absLed->setModel(&_controls->m_absModel);

	auto mainLayout = new QVBoxLayout();
	mainLayout->setSpacing(0);
	mainLayout->setContentsMargins(0, 0, 0, 10);

	auto knobLayout = new QHBoxLayout();
	knobLayout->setSizeConstraint(QLayout::SetMaximumSize);
	knobLayout->setContentsMargins(10, 0, 10, 0);

	auto ledLayout = new QHBoxLayout();
	ledLayout->setSizeConstraint(QLayout::SetMinimumSize);
	ledLayout->setContentsMargins(10, 0, 10, 0);

	knobLayout->addWidget(m_baseKnob);
	knobLayout->addWidget(m_amountKnob);
	knobLayout->addWidget(m_amountMultKnob);
	knobLayout->addWidget(m_attackKnob);
	knobLayout->addWidget(m_decayKnob);
	knobLayout->addWidget(m_thresholdKnob);

	ledLayout->addWidget(m_muteLed);
	ledLayout->addWidget(m_absLed);

	mainLayout->addWidget(knobContainer);
	mainLayout->addWidget(ledContainer);

	knobContainer->setLayout(knobLayout);
	ledContainer->setLayout(ledLayout);

	setLayout(mainLayout);
}


void PeakControllerEffectControlDialog::showDialog()
{
	getGUI()->getControllerRackView()->show();
	getGUI()->getControllerRackView()->subWin()->hide();
}


void PeakControllerEffectControlDialog::toggleView()
{
	auto* sw = getGUI()->getControllerRackView()->subWin();

	if (!sw->isVisible())
	{
		sw->show();
		sw->raise();
		m_effectControls->setViewVisible(true);
	}
	else
	{
		sw->hide();
		m_effectControls->setViewVisible(false);
	}
}

} // namespace lmms::gui