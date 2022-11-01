/*
 * TunerControlsDialog.cpp
 *
 * Copyright (c) 2022 saker <sakertooth@gmail.com>
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

#include "TunerControlDialog.h"

#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>
#include <QSizePolicy>

#include "LcdFloatSpinBox.h"
#include "LcdSpinBox.h"
#include "Tuner.h"
#include "TunerControls.h"

#include <cmath>
#include <iostream>

TunerControlDialog::TunerControlDialog(TunerControls* controls)
	: EffectControlDialog(controls)
{
	setFixedSize(128, 128);
	setAutoFillBackground(true);

	auto pal = QPalette();
	pal.setColor(QPalette::Window, QColor{18, 19, 22});
	setPalette(pal);

	m_noteLabel = new QLabel();
	m_noteLabel->setFont(QFont{"Arial", 32, QFont::Bold});

	m_octaveLabel = new QLabel();
	m_octaveLabel->setFont(QFont{"Arial", 8, QFont::Bold});

	m_centsLabel = new QLabel();
	m_centsLabel->setFont(QFont{"Arial", 16, QFont::Bold});

	auto noteLayout = new QHBoxLayout();
	noteLayout->addStretch(1);
	noteLayout->addWidget(m_noteLabel, 0, Qt::AlignRight);
	noteLayout->addWidget(m_octaveLabel);
	noteLayout->addWidget(m_centsLabel, 1);

	auto referenceFreqSpinBox = new LcdSpinBox(3, this, tr("Reference"));
	referenceFreqSpinBox->setModel(&controls->m_referenceFreqModel);
	referenceFreqSpinBox->setLabel(tr("Reference"));
	
	auto layout = new QVBoxLayout(this);
	layout->setSpacing(1);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addLayout(noteLayout);
	layout->addWidget(referenceFreqSpinBox, 0, Qt::AlignRight);

	QObject::connect(controls->m_tuner, &Tuner::frequencyCalculated, this, [this](float frequency){ frequencyCalculated(frequency); });
}

void TunerControlDialog::frequencyCalculated(float frequency)
{	
	// A4 = referenceFrequency
	const float referenceFrequency = static_cast<TunerControls*>(m_effectControls)->m_referenceFreqModel.value();
	const int centsFromReference = std::round(1200.0f * std::log2f(frequency / referenceFrequency));
	
	const int octavesFromReference = std::round(centsFromReference / 1200.0f);
	const int octaveOfNote = 4 + octavesFromReference;
	
	int centsRemaining = centsFromReference - (octavesFromReference * 1200);
	int semitonesFromReference = std::round(centsRemaining / 100);
	int centsOfNote = centsRemaining - (semitonesFromReference * 100);

	if (semitonesFromReference < 0) { semitonesFromReference += 12; }
	auto note = noteToString(static_cast<NoteName>(semitonesFromReference));
	m_noteLabel->setText(QString::fromStdString(note));

	//Only give back the octave if it is in a useful range
	if (octaveOfNote >= -1 && octaveOfNote <= 8) { m_octaveLabel->setText(QString::number(octaveOfNote)); };
	
	m_centsLabel->setText((centsOfNote >= 0 ? "+" : "") + QString::number(centsOfNote) + "ct");
	auto centDistance = std::abs(centsOfNote);
	if (centDistance >= 0 && centDistance <= 10) 
	{
		m_centsLabel->setStyleSheet("QLabel { color : green; }");
	}
	else if (centDistance > 10 && centDistance <= 30) 
	{
		m_centsLabel->setStyleSheet("QLabel { color : yellow; }");
	}
	else if (centDistance > 30) 
	{
		m_centsLabel->setStyleSheet("QLabel { color : red; }");
	}
}

std::string TunerControlDialog::noteToString(NoteName note) 
{
	switch (note)
	{
	case NoteName::A:
		return "A";
	case NoteName::ASharp:
		return "A#";
	case NoteName::B:
		return "B";
	case NoteName::C:
		return "C";
	case NoteName::CSharp:
		return "C#";
	case NoteName::D:
		return "D";
	case NoteName::DSharp:
		return "D#";
	case NoteName::E:
		return "E";
	case NoteName::F:
		return "F";
	case NoteName::FSharp:
		return "F#";
	case NoteName::G:
		return "G";
	case NoteName::GSharp:
		return "G#";
	default:
		return "";
	};
} 