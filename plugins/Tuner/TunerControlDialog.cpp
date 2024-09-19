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
#include <QSizePolicy>
#include <QVBoxLayout>
#include <cmath>
#include <iostream>

#include "LcdFloatSpinBox.h"
#include "LcdSpinBox.h"
#include "Tuner.h"
#include "TunerControls.h"

namespace lmms::gui {

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

	m_centsLabel = new QLabel();
	m_centsLabel->setFont(QFont{"Arial", 16, QFont::Bold});

	auto noteLayout = new QHBoxLayout();
	noteLayout->addStretch(1);
	noteLayout->addWidget(m_noteLabel, 0, Qt::AlignRight);
	noteLayout->addWidget(m_centsLabel, 1);

	auto referenceFreqSpinBox = new LcdSpinBox(3, this, tr("Reference"));
	referenceFreqSpinBox->setModel(&controls->m_referenceFreqModel);
	referenceFreqSpinBox->setLabel(tr("Reference"));

	auto layout = new QVBoxLayout(this);
	layout->setSpacing(1);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addLayout(noteLayout);
	layout->addWidget(referenceFreqSpinBox, 0, Qt::AlignRight);

	QObject::connect(controls->m_tuner, &Tuner::frequencyCalculated, this,
		[this](float frequency) { frequencyCalculated(frequency); });
}

auto TunerControlDialog::frequencyCalculated(float frequency) -> void
{
	if (frequency <= 0) { return; }
	const auto referenceFrequency = static_cast<TunerControls*>(m_effectControls)->m_referenceFreqModel.value();

	auto closestMidiNote = static_cast<int>(69 + 12 * std::log2(frequency / referenceFrequency));
	auto estimatedNote = static_cast<NoteName>(closestMidiNote % 12);

	auto closestMidiNoteFrequency = referenceFrequency * std::exp2((closestMidiNote - 69) / 12.0f);
	auto estimatedCentDifference = static_cast<int>(std::round(1200 * std::log2(frequency / closestMidiNoteFrequency)));

	// Roll over cents to get smaller cent values
	if (100 - std::abs(estimatedCentDifference) < std::abs(estimatedCentDifference))
	{
		auto increaseNote = estimatedCentDifference > 0;
		estimatedNote = static_cast<NoteName>((static_cast<int>(estimatedNote) + (increaseNote ? 1 : -1)) % 12);
		estimatedCentDifference = increaseNote ? estimatedCentDifference - 100 : 100 - estimatedCentDifference;
	}

	m_noteLabel->setText(noteToString(estimatedNote));
	m_centsLabel->setText(QString::number(estimatedCentDifference) + "ct");

	auto centDistance = std::abs(estimatedCentDifference);
	if (centDistance >= 0 && centDistance <= 10) { m_centsLabel->setStyleSheet("QLabel { color : green; }"); }
	else if (centDistance > 10 && centDistance <= 30) { m_centsLabel->setStyleSheet("QLabel { color : yellow; }"); }
	else if (centDistance > 30) { m_centsLabel->setStyleSheet("QLabel { color : red; }"); }
}

auto TunerControlDialog::noteToString(NoteName note) const -> const char*
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
} // namespace lmms::gui
