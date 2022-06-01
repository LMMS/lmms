/*
 * TunerControls.cpp
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

#include "TunerControls.h"

#include <QDomElement>

#include "Tuner.h"

TunerControls::TunerControls(Tuner* tuner)
	: EffectControls(tuner)
	, m_tuner(tuner)
	, m_referenceFreqModel(440, 0, 999)
{
	QObject::connect(&m_referenceFreqModel, &LcdSpinBoxModel::dataChanged, tuner, &Tuner::syncReferenceFrequency);
}

void TunerControls::saveSettings(QDomDocument& domDocument, QDomElement& domElement)
{
	m_referenceFreqModel.saveSettings(domDocument, domElement, "reference");
}

void TunerControls::loadSettings(const QDomElement& domElement)
{
	m_referenceFreqModel.loadSettings(domElement, "reference");
}

QString TunerControls::nodeName() const
{
	return "TunerControls";
}

int TunerControls::controlCount()
{
	return 1;
}

EffectControlDialog* TunerControls::createView()
{
	m_tunerDialog = new TunerControlDialog(this);
	return m_tunerDialog;
}

void TunerControls::updateView(TunerNote note) 
{
	m_tunerDialog->m_noteLabel->setText(QString::fromStdString(note.fullNoteName()));
	m_tunerDialog->m_freqWidget->setValue(note.frequency());
	m_tunerDialog->m_centsWidget->setValue(note.cents());	
}