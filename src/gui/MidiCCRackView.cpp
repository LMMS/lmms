/*
 * MidiCCRackView.cpp - implementation of the MIDI CC rack widget
 *
 * Copyright (c) 2020 Ian Caio <iancaio_dev/at/hotmail.com>
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


#include "MidiCCRackView.h"

#include <QGridLayout>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

#include "embed.h"
#include "GroupBox.h"
#include "GuiApplication.h"
#include "InstrumentTrack.h"
#include "Knob.h"
#include "MainWindow.h"
#include "SubWindow.h"

namespace lmms::gui
{


MidiCCRackView::MidiCCRackView(InstrumentTrack * track) :
	QWidget(),
	m_track(track)
{
	setWindowIcon(embed::getIconPixmap("midi_cc_rack"));
	setWindowTitle(tr("MIDI CC Rack - %1").arg(m_track->name()));

	QMdiSubWindow * subWin = getGUI()->mainWindow()->addWindowedWidget(this);

	// Remove maximize button
	Qt::WindowFlags flags = subWin->windowFlags();
	flags &= ~Qt::WindowMaximizeButtonHint;
	subWin->setWindowFlags(flags);

	// Adjust window attributes, sizing and position
	subWin->setAttribute(Qt::WA_DeleteOnClose, false);
	subWin->resize(350, 300);
	subWin->setFixedWidth(350);
	subWin->setMinimumHeight(300);
	subWin->hide();

	// Main window layout
	auto mainLayout = new QVBoxLayout(this);

	// Knobs GroupBox - Here we have the MIDI CC controller knobs for the selected track
	m_midiCCGroupBox = new GroupBox(tr("MIDI CC Knobs:"));

	// Layout to keep scrollable area under the GroupBox header
	auto knobsGroupBoxLayout = new QVBoxLayout();
	knobsGroupBoxLayout->setContentsMargins(5, 16, 5, 5);

	m_midiCCGroupBox->setLayout(knobsGroupBoxLayout);

	// Scrollable area + widget + its layout that will have all the knobs
	auto knobsScrollArea = new QScrollArea();
	auto knobsArea = new QWidget();
	auto knobsAreaLayout = new QGridLayout();
	knobsAreaLayout->setVerticalSpacing(10);

	knobsArea->setLayout(knobsAreaLayout);
	knobsScrollArea->setWidget(knobsArea);
	knobsScrollArea->setWidgetResizable(true);

	knobsGroupBoxLayout->addWidget(knobsScrollArea);

	// Adds the controller knobs and sets their models
	for (int i = 0; i < MidiControllerCount; ++i)
	{
		auto knob = new Knob(KnobType::Bright26, tr("CC %1").arg(i), this);
		knob->setModel(m_track->m_midiCCModel[i].get());
		knobsAreaLayout->addWidget(knob, i/4, i%4, Qt::AlignHCenter);

		// TODO It seems that this is not really used/needed?
		m_controllerKnob[i] = knob;
	}

	// Set all the models
	// Set the LED button to enable/disable the track midi cc
	m_midiCCGroupBox->setModel(m_track->m_midiCCEnable.get());

	// Connection to update the name of the track on the label
	connect(m_track, SIGNAL(nameChanged()),
		this, SLOT(renameWindow()));

	// Adding everything to the main layout
	mainLayout->addWidget(m_midiCCGroupBox);
}

MidiCCRackView::~MidiCCRackView()
{
	if(parentWidget())
	{
		parentWidget()->hide();
		parentWidget()->deleteLater();
	}
}

void MidiCCRackView::renameWindow()
{
	setWindowTitle(tr("MIDI CC Rack - %1").arg(m_track->name()));
}

void MidiCCRackView::saveSettings(QDomDocument & doc, QDomElement & parent)
{
}

void MidiCCRackView::loadSettings(const QDomElement &)
{
}


} // namespace lmms::gui
