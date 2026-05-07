/*
 * SfzSamplerView.cpp - GUI for SfzSampler
 *
 * Copyright (c) 2026 Keratin
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

#include "SfzSamplerView.h"

#include <QDropEvent>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QDebug>

#include "Clipboard.h"
#include "ComboBox.h"
#include "DataFile.h"
#include "InstrumentView.h"
#include "Knob.h"
#include "LcdSpinBox.h"
#include "PixmapButton.h"
#include "SfzSampler.h"
#include "StringPairDrag.h"
#include "Track.h"
#include "embed.h"
#include "ConfigManager.h"
#include "FileDialog.h"
#include "PathUtil.h"
#include "embed.h"
#include "MidiEvent.h"
#include "InstrumentTrack.h"

namespace lmms {

namespace gui {

SfzSamplerView::SfzSamplerView(SfzSampler* instrument, QWidget* parent)
	: InstrumentView(instrument, parent)
	, m_instrument(instrument)
	, m_statusLabel(new QLabel(this))
	, m_generalInfoLabel(new QLabel(this))
	, m_switchKeysLabel(new QLabel(this))
	, m_infoLabelsWidget(new QWidget(this))
	, m_controlsWidget(new QWidget(this))
	, m_knobLayout(new QGridLayout(m_controlsWidget))
{
	setAcceptDrops(true);
	setAutoFillBackground(true);

	setMaximumSize(QSize(10000, 10000));
	setMinimumSize(QSize(500, 250));

	auto layout1 = new QVBoxLayout(this);

	auto openFileButton = new QPushButton(embed::getIconPixmap("folder"), tr("Open SFZ File"), this);
	connect(openFileButton, &PixmapButton::clicked, this, &SfzSamplerView::openFile);
	layout1->addWidget(openFileButton);

	layout1->addWidget(m_statusLabel);

	layout1->addWidget(m_infoLabelsWidget);

	auto layout2 = new QHBoxLayout(m_infoLabelsWidget);
	layout2->setContentsMargins(0, 0, 0, 0);
	layout2->addWidget(m_generalInfoLabel);
	layout2->addWidget(m_switchKeysLabel);

	layout1->addWidget(m_controlsWidget);


	// Whenever a new SFZ file is loaded, set the default CC values
	connect(m_instrument, &SfzSampler::fileLoaded, [this](){ onFileLoaded(); }); // this lambda is so bad, but it doesn't work as a slot for some reason

	connect(m_instrument, &SfzSampler::statusInfo, [this](const QString& statusText){ updateStatusInfo(statusText); });

	onFileLoaded();

	update();
}


void SfzSamplerView::onFileLoaded()
{
	// Remove any old knobs
	delete m_controlsWidget;
	m_controlsWidget = new QWidget(this);
	m_knobLayout = new QGridLayout(m_controlsWidget);
	layout()->addWidget(m_controlsWidget);

	// Initialize new knobs
	int activeControlCount = 0;
	for (int i = 0; i < NumMidiCCs; ++i)
	{
		// Only add a knob if the control is actually used
		if (m_instrument->m_controlsConfig.m_activeMidiCCs.at(i))
		{
			const QString& controlLabel = m_instrument->m_controlsConfig.m_label_cc.at(i).value_or(tr("CC %1").arg(i));
			auto ccKnob = new Knob(KnobType::Bright26, controlLabel, m_controlsWidget);
			ccKnob->setModel(m_instrument->m_parentTrack->midiCCModel(i));
			m_knobLayout->addWidget(ccKnob, activeControlCount / 8, activeControlCount % 8);
			activeControlCount++;
		}
	}
	// If we are using midi CC's make sure to enable them in the instrument track
	if (activeControlCount > 0)
	{
		m_instrument->m_parentTrack->midiCCEnableModel()->setValue(true, true);
	}

	// Update the switch key list
	QStringList switchKeyLabels;
	for (const auto& [key, info] : m_instrument->m_controlsConfig.m_switchKeyInfo)
	{
		QString label = QString("- %1 %2 [range: %3 - %4]")
			.arg(keyNumToString(key))
			.arg(info.sw_label)
			.arg(keyNumToString(info.sw_lokey))
			.arg(keyNumToString(info.sw_hikey));
		if (info.sw_default != std::nullopt)
		{
			label += QString(" default: %1").arg(keyNumToString(info.sw_default.value()));
		}
		switchKeyLabels.push_back(label);
	}
	if (switchKeyLabels.size() > 0)
	{
		m_switchKeysLabel->setText(tr("Switch Keys:\n") + switchKeyLabels.join("\n"));
	}
	else
	{
		m_switchKeysLabel->setText("");
	}

	// Update general info
	m_generalInfoLabel->setText(
		QString("File: %1\nRegions: %2\nSamples: %3")
			.arg(QFileInfo(m_instrument->m_sfzFilePath).fileName())
			.arg(m_instrument->m_regionManager->allRegions().size())
			.arg(m_instrument->m_samplePool->sampleCount())
	);
}

void SfzSamplerView::updateStatusInfo(const QString& statusText)
{
	m_statusLabel->setText(statusText);
	update(); // For some reason the gui doesn't always update if the user isn't interacting with it or panning the workspace view
}


void SfzSamplerView::openFile()
{
	auto openFileDialog = FileDialog(nullptr, QObject::tr("Open SFZ File"));
	auto dir = ConfigManager::inst()->userSamplesDir();
	openFileDialog.setDirectory(dir);
	if (openFileDialog.exec() == QDialog::Accepted)
	{
		if (openFileDialog.selectedFiles().isEmpty()) { return; }
		m_instrument->loadFile(openFileDialog.selectedFiles()[0]);
	}
}


void SfzSamplerView::dragEnterEvent(QDragEnterEvent* dee)
{
	QString value = StringPairDrag::decodeValue(dee);
	if (value.endsWith(".sfz")) 
	{
		dee->accept();
		return;
	}
	dee->ignore();
}

void SfzSamplerView::dropEvent(QDropEvent* de)
{
	QString value = StringPairDrag::decodeValue(de);
	if (value.endsWith(".sfz")) 
	{
		de->accept();
		m_instrument->loadFile(value);
		return;
	}
	de->ignore();
}

void SfzSamplerView::resizeEvent(QResizeEvent* re)
{
}

void SfzSamplerView::paintEvent(QPaintEvent* pe)
{
	//QPainter brush(this);
	//brush.fillRect(rect(), QColor(19, 19, 20));
}

} // namespace gui
} // namespace lmms
