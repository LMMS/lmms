/*
 * InstrumentMiscView.cpp - Miscellaneous instrument settings
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2019 Martin Pavelek <he29.HS/at/gmail.com>
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

#include "InstrumentMiscView.h"

#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>

#include "ComboBox.h"
#include "GroupBox.h"
#include "InstrumentTrack.h"
#include "Knob.h"


InstrumentMiscView::InstrumentMiscView(InstrumentTrack *it, QWidget *parent) :
	QWidget(parent)
{
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setMargin(5);

	// Master pitch toggle
	m_pitchGroupBox = new GroupBox(tr("MASTER PITCH"));
	m_pitchGroupBox->setModel(&it->m_useMasterPitchModel);
	layout->addWidget(m_pitchGroupBox);

	QHBoxLayout *masterPitchLayout = new QHBoxLayout(m_pitchGroupBox);
	masterPitchLayout->setContentsMargins(8, 18, 8, 8);

	QLabel *tlabel = new QLabel(tr("Enables the use of master pitch"));
	masterPitchLayout->addWidget(tlabel);


	// Microtuner settings
	int scaleEditWidth = 75;
	int mappingEditWidth = 75;
	int editHeight = 100;
	int buttonSize = 19;
	m_microtunerGroupBox = new GroupBox(tr("MICROTUNER"));
	m_microtunerGroupBox->setModel(it->m_tuner.getEnabledModel());
	layout->addWidget(m_microtunerGroupBox);

	// organize into 2 main columns
	QHBoxLayout *microtunerLayout = new QHBoxLayout(m_microtunerGroupBox);
	microtunerLayout->setContentsMargins(6, 18, 6, 8);

	// 1) general controls
	QVBoxLayout *generalLayout = new QVBoxLayout(m_microtunerGroupBox);
	microtunerLayout->addLayout(generalLayout);

	Knob *baseFreqKnob = new Knob(knobBright_26);
	baseFreqKnob->setModel(it->m_tuner.getBaseFreqModel());
	baseFreqKnob->setLabel(tr("BASE FREQ"));
	baseFreqKnob->setHintText(tr("Base note frequency:"), " " + tr("Hz"));
	baseFreqKnob->setToolTip(tr("Base note frequency"));
	generalLayout->addWidget(baseFreqKnob);

	generalLayout->addStretch();

	QPushButton *applyButton = new QPushButton(tr("Apply"), m_microtunerGroupBox);
	applyButton->setFixedWidth(50);
	generalLayout->addWidget(applyButton);

	// 2) edit column (scale and mapping)
	QGridLayout *editLayout = new QGridLayout(m_microtunerGroupBox);
	editLayout->setSpacing(2);
	microtunerLayout->addLayout(editLayout);

	QLineEdit *scaleNameEdit = new QLineEdit("12-TET", m_microtunerGroupBox);
	scaleNameEdit->setToolTip(tr("Scale description"));
	editLayout->addWidget(scaleNameEdit, 0, 0, 1, 4);

	// 2.1) scale sub-column
	// scale load / save buttons
	QLabel *scaleLabel = new QLabel(tr("Scale:"));
	editLayout->addWidget(scaleLabel, 1, 0, 1, 2, Qt::AlignBottom);
	QPushButton *scaleLoadButton = new QPushButton(tr("Load"), m_microtunerGroupBox);
	QPushButton *scaleSaveButton = new QPushButton(tr("Save"), m_microtunerGroupBox);
	scaleLoadButton->setFixedSize(scaleEditWidth / 2 - 1, buttonSize);
	scaleSaveButton->setFixedSize(scaleEditWidth / 2 - 1, buttonSize);
	editLayout->addWidget(scaleLoadButton, 2, 0, 1, 1);
	editLayout->addWidget(scaleSaveButton, 2, 1, 1, 1);

	QTextEdit *scaleTextEdit = new QTextEdit(m_microtunerGroupBox);
	scaleTextEdit->setFixedSize(scaleEditWidth, editHeight);
	scaleTextEdit->setAcceptRichText(false);
	scaleTextEdit->setPlainText("100.0\n200.0\n300.0\n400.0\n500.0\n600.0\n700.0\n800.0\n900.0\n1000.0\n1100.0\n1200.0");
	editLayout->addWidget(scaleTextEdit, 3, 0, 1, 2, Qt::AlignLeft | Qt::AlignBottom);

	// 2.2) mapping sub-column
	QVBoxLayout *mappingLayout = new QVBoxLayout(m_microtunerGroupBox);
	microtunerLayout->addLayout(mappingLayout);

	mappingLayout->addStretch();

	// mapping load / save buttons
	QLabel *mappingLabel = new QLabel(tr("Keymap:"));
	editLayout->addWidget(mappingLabel, 1, 2, 1, 2, Qt::AlignBottom);
	QPushButton *mappingLoadButton = new QPushButton(tr("Load"), m_microtunerGroupBox);
	QPushButton *mappingSaveButton = new QPushButton(tr("Save"), m_microtunerGroupBox);
	mappingLoadButton->setFixedSize(mappingEditWidth / 2 - 1, buttonSize);
	mappingSaveButton->setFixedSize(mappingEditWidth / 2 - 1, buttonSize);
	editLayout->addWidget(mappingLoadButton, 2, 2, 1, 1);
	editLayout->addWidget(mappingSaveButton, 2, 3, 1, 1);

	QTextEdit *mappingTextEdit = new QTextEdit(m_microtunerGroupBox);
	mappingTextEdit->setFixedSize(mappingEditWidth, editHeight);
	mappingTextEdit->setAcceptRichText(false);
	mappingTextEdit->setPlainText("1\n2\n4\n5\n6\n7\n8\n9\n10\n11");
	editLayout->addWidget(mappingTextEdit, 3, 2, 1, 2, Qt::AlignRight | Qt::AlignBottom);

	// fill remaining space
//	layout->addStretch();
}

InstrumentMiscView::~InstrumentMiscView()
{

}
