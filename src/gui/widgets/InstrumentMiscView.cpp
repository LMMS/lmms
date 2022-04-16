/*
 * InstrumentMiscView.cpp - Miscellaneous instrument settings
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2020 Martin Pavelek <he29.HS/at/gmail.com>
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

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "ComboBox.h"
#include "GroupBox.h"
#include "gui_templates.h"
#include "InstrumentTrack.h"
#include "LedCheckbox.h"


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
	tlabel->setFont(pointSize<8>(tlabel->font()));
	masterPitchLayout->addWidget(tlabel);

	// Microtuner settings
	m_microtunerGroupBox = new GroupBox(tr("MICROTUNER"));
	m_microtunerGroupBox->setModel(it->m_microtuner.enabledModel());
	layout->addWidget(m_microtunerGroupBox);

	QVBoxLayout *microtunerLayout = new QVBoxLayout(m_microtunerGroupBox);
	microtunerLayout->setContentsMargins(8, 18, 8, 8);

	QLabel *scaleLabel = new QLabel(tr("Active scale:"));
	microtunerLayout->addWidget(scaleLabel);

	m_scaleCombo = new ComboBox();
	m_scaleCombo->setModel(it->m_microtuner.scaleModel());
	microtunerLayout->addWidget(m_scaleCombo);

	QLabel *keymapLabel = new QLabel(tr("Active keymap:"));
	microtunerLayout->addWidget(keymapLabel);

	m_keymapCombo = new ComboBox();
	m_keymapCombo->setModel(it->m_microtuner.keymapModel());
	microtunerLayout->addWidget(m_keymapCombo);

	m_rangeImportCheckbox = new LedCheckBox(tr("Import note ranges from keymap"), this);
	m_rangeImportCheckbox->setModel(it->m_microtuner.keyRangeImportModel());
	m_rangeImportCheckbox->setToolTip(tr("When enabled, the first, last and base notes of this instrument will be overwritten with values specified by the selected keymap."));
	m_rangeImportCheckbox->setCheckable(true);
	microtunerLayout->addWidget(m_rangeImportCheckbox);

	// Fill remaining space
	layout->addStretch();
}
