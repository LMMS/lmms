/*
 * SlicerTView.cpp - controls the UI for slicerT
 *
 * Copyright (c) 2023 Daniel Kauss Serna <daniel.kauss.serna@gmail.com>
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

#include "SlicerTView.h"

#include <QDropEvent>
#include <QFileInfo>

#include "Clipboard.h"
#include "DataFile.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "SampleLoader.h"
#include "SlicerT.h"
#include "Song.h"
#include "StringPairDrag.h"
#include "Track.h"
#include "embed.h"

namespace lmms {

namespace gui {

SlicerTView::SlicerTView(SlicerT* instrument, QWidget* parent)
	: InstrumentViewFixedSize(instrument, parent)
	, m_slicerTParent(instrument)
{
	// window settings
	setAcceptDrops(true);
	setAutoFillBackground(true);

	// render background
	QPalette pal;
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);

	m_wf = new SlicerTWaveform(248, 128, instrument, this);
	m_wf->move(2, 6);

	m_snapSetting = new ComboBox(this, tr("Slice snap"));
	m_snapSetting->setGeometry(185, 200, 55, ComboBox::DEFAULT_HEIGHT);
	m_snapSetting->setToolTip(tr("Set slice snapping for detection"));
	m_snapSetting->setModel(&m_slicerTParent->m_sliceSnap);

	m_syncToggle = new LedCheckBox("Sync", this, tr("SyncToggle"), LedCheckBox::LedColor::Green);
	m_syncToggle->move(135, 187);
	m_syncToggle->setToolTip(tr("Enable BPM sync"));
	m_syncToggle->setModel(&m_slicerTParent->m_enableSync);

	m_bpmBox = new LcdSpinBox(3, "19purple", this);
	m_bpmBox->move(130, 201);
	m_bpmBox->setToolTip(tr("Original sample BPM"));
	m_bpmBox->setModel(&m_slicerTParent->m_originalBPM);

	m_noteThresholdKnob = createStyledKnob();
	m_noteThresholdKnob->move(10, 197);
	m_noteThresholdKnob->setToolTip(tr("Threshold used for slicing"));
	m_noteThresholdKnob->setModel(&m_slicerTParent->m_noteThreshold);

	m_fadeOutKnob = createStyledKnob();
	m_fadeOutKnob->move(64, 197);
	m_fadeOutKnob->setToolTip(tr("Fade Out per note in milliseconds"));
	m_fadeOutKnob->setModel(&m_slicerTParent->m_fadeOutFrames);

	m_midiExportButton = new QPushButton(this);
	m_midiExportButton->move(199, 150);
	m_midiExportButton->setIcon(PLUGIN_NAME::getIconPixmap("copy_midi"));
	m_midiExportButton->setToolTip(tr("Copy midi pattern to clipboard"));
	connect(m_midiExportButton, &PixmapButton::clicked, this, &SlicerTView::exportMidi);

	m_resetButton = new QPushButton(this);
	m_resetButton->move(18, 150);
	m_resetButton->setIcon(PLUGIN_NAME::getIconPixmap("reset_slices"));
	m_resetButton->setToolTip(tr("Reset Slices"));
	connect(m_resetButton, &PixmapButton::clicked, m_slicerTParent, &SlicerT::updateSlices);
}

Knob* SlicerTView::createStyledKnob()
{
	Knob* newKnob = new Knob(KnobType::Styled, this);
	newKnob->setFixedSize(50, 40);
	newKnob->setCenterPointX(24.0);
	newKnob->setCenterPointY(15.0);
	return newKnob;
}

// copied from piano roll
void SlicerTView::exportMidi()
{
	using namespace Clipboard;
	if (m_slicerTParent->m_originalSample.sampleSize() <= 1) { return; }

	DataFile dataFile(DataFile::Type::ClipboardData);
	QDomElement noteList = dataFile.createElement("note-list");
	dataFile.content().appendChild(noteList);

	auto notes = m_slicerTParent->getMidi();
	if (notes.empty()) { return; }

	TimePos startPos(notes.front().pos().getBar(), 0);
	for (Note& note : notes)
	{
		note.setPos(note.pos(startPos));
		note.saveState(dataFile, noteList);
	}

	copyString(dataFile.toString(), MimeType::Default);
}

void SlicerTView::openFiles()
{
	const auto audioFile = SampleLoader::openAudioFile();
	if (audioFile.isEmpty()) { return; }
	m_slicerTParent->updateFile(audioFile);
}

// all the drag stuff is copied from AudioFileProcessor
void SlicerTView::dragEnterEvent(QDragEnterEvent* dee)
{
	// For mimeType() and MimeType enum class
	using namespace Clipboard;

	if (dee->mimeData()->hasFormat(mimeType(MimeType::StringPair)))
	{
		QString txt = dee->mimeData()->data(mimeType(MimeType::StringPair));
		if (txt.section(':', 0, 0) == QString("clip_%1").arg(static_cast<int>(Track::Type::Sample)))
		{
			dee->acceptProposedAction();
		}
		else if (txt.section(':', 0, 0) == "samplefile") { dee->acceptProposedAction(); }
		else { dee->ignore(); }
	}
	else { dee->ignore(); }
}

void SlicerTView::dropEvent(QDropEvent* de)
{
	QString type = StringPairDrag::decodeKey(de);
	QString value = StringPairDrag::decodeValue(de);
	if (type == "samplefile")
	{
		// set m_wf wave file
		m_slicerTParent->updateFile(value);
		return;
	}
	else if (type == QString("clip_%1").arg(static_cast<int>(Track::Type::Sample)))
	{
		DataFile dataFile(value.toUtf8());
		m_slicerTParent->updateFile(dataFile.content().firstChild().toElement().attribute("src"));
		de->accept();
		return;
	}

	de->ignore();
}

void SlicerTView::paintEvent(QPaintEvent* pe)
{
	QPainter brush(this);
	brush.setPen(QColor(255, 255, 255));
	brush.setFont(QFont(brush.font().family(), 7, -1, false));

	brush.drawText(8, s_topTextY, s_textBoxWidth, s_textBoxHeight, Qt::AlignCenter, tr("Reset"));
	brush.drawText(188, s_topTextY, s_textBoxWidth, s_textBoxHeight, Qt::AlignCenter, tr("Midi"));

	brush.drawText(8, s_bottomTextY, s_textBoxWidth, s_textBoxHeight, Qt::AlignCenter, tr("Threshold"));
	brush.drawText(63, s_bottomTextY, s_textBoxWidth, s_textBoxHeight, Qt::AlignCenter, tr("Fade Out"));
	brush.drawText(127, s_bottomTextY, s_textBoxWidth, s_textBoxHeight, Qt::AlignCenter, tr("BPM"));
	brush.drawText(188, s_bottomTextY, s_textBoxWidth, s_textBoxHeight, Qt::AlignCenter, tr("Snap"));
}

} // namespace gui
} // namespace lmms
