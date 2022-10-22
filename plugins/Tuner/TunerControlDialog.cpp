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
	setFixedSize(240, 240);
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

void TunerControlDialog::paintEvent(QPaintEvent* event) 
{
	auto painter = QPainter(this);
	painter.setPen(Qt::white);
	painter.drawArc(QRectF{20.0, 50.0, 200.0, 60.0}, 30 * 16, 120 * 16);

	painter.setBrush(Qt::white);
	painter.drawEllipse(QPoint{120, 50}, 8, 8);
}

void TunerControlDialog::frequencyCalculated(float frequency)
{	
	// A4 = referenceFrequency
	const float referenceFrequency = static_cast<TunerControls*>(m_effectControls)->m_referenceFreqModel.value();
	const int centsFromReference = std::round(1200.0f * std::log2f(frequency / referenceFrequency));
	
	const int octavesFromReference = std::round(centsFromReference / 1200.0f);
	const int octaveOfNote = 4 + octavesFromReference;
	
	int centsRemaining = (centsFromReference - (octavesFromReference * 1200));
	int semitonesFromReference = centsRemaining / 100;
	int centsOfNote = centsRemaining % 100;

	// If it is over 50ct, we can roll over to the nearest semitone
	if (centsOfNote >= 50 || centsOfNote <= -50) 
	{
		const auto centsOfNoteNormalized = centsOfNote / std::abs(centsOfNote);
		semitonesFromReference += centsOfNoteNormalized;
		centsOfNote = 100 - (centsOfNote * centsOfNoteNormalized);
	}

	if (semitonesFromReference < 0) { semitonesFromReference += 12; }
	std::string note = "";

	switch (static_cast<NoteName>(semitonesFromReference))
	{
	case NoteName::A:
		note = "A";
		break;
	case NoteName::ASharp:
		note = "A#";
		break;
	case NoteName::B:
		note = "B";
		break;
	case NoteName::C:
		note = "C";
		break;
	case NoteName::CSharp:
		note = "C#";
		break;
	case NoteName::D:
		note = "D";
		break;
	case NoteName::DSharp:
		note = "D#";
		break;
	case NoteName::E:
		note = "E";
		break;
	case NoteName::F:
		note = "F";
		break;
	case NoteName::FSharp:
		note = "F#";
		break;
	case NoteName::G:
		note = "G";
		break;
	case NoteName::GSharp:
		note = "G#";
		break;
	default:
		note = "";
	};

	m_noteLabel->setText(QString::fromStdString(note));
	if (octaveOfNote >= -1) { m_octaveLabel->setText(QString::number(octaveOfNote)); };
	m_centsLabel->setText((centsOfNote >= 0 ? "+" : "") + QString::number(centsOfNote) + "ct");

	if (centsOfNote >= 0 && centsOfNote <= 10) 
	{
		m_centsLabel->setStyleSheet("QLabel { color : green; }");
	}
	else if (centsOfNote > 10 && centsOfNote <= 30) 
	{
		m_centsLabel->setStyleSheet("QLabel { color : yellow; }");
	}
	else if (centsOfNote > 30) 
	{
		m_centsLabel->setStyleSheet("QLabel { color : red; }");
	}
}