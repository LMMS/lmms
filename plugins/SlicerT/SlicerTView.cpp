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
	, m_noteThresholdKnob(this)
	, m_fadeOutKnob(this)
	, m_bpmBox(3, "21pink", this)
	, m_snapSetting(this, tr("Slice snap"))
	, m_syncToggle("Sync", this, tr("SyncToggle"), LedCheckBox::LedColor::Green)
	, m_resetButton(this, nullptr)
	, m_midiExportButton(this, nullptr)
	, m_wf(248, 128, instrument, this)
{
	// window settings
	setAcceptDrops(true);
	setAutoFillBackground(true);

	// render background
	QPalette pal;
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);

	// move editor and seeker
	m_wf.move(2, 6);

	// snap combo box
	m_snapSetting.setGeometry(185, 200, 55, ComboBox::DEFAULT_HEIGHT);
	m_snapSetting.setToolTip(tr("Set slice snapping for detection"));
	m_snapSetting.setModel(&m_slicerTParent->m_sliceSnap);

	// sync toggle
	m_syncToggle.move(135, 187);
	m_syncToggle.setToolTip(tr("Enable BPM sync"));
	m_syncToggle.setModel(&m_slicerTParent->m_enableSync);

	// bpm spin box
	m_bpmBox.move(130, 203);
	m_bpmBox.setToolTip(tr("Original sample BPM"));
	m_bpmBox.setLabel(tr("BPM"));
	m_bpmBox.setModel(&m_slicerTParent->m_originalBPM);

	// threshold knob
	m_noteThresholdKnob.move(9, 195);
	m_noteThresholdKnob.setToolTip(tr("Threshold used for slicing"));
	m_noteThresholdKnob.setLabel(tr("Threshold"));
	m_noteThresholdKnob.setModel(&m_slicerTParent->m_noteThreshold);

	// fadeout knob
	m_fadeOutKnob.move(75, 195);
	m_fadeOutKnob.setToolTip(tr("Fade Out for notes"));
	m_fadeOutKnob.setLabel(tr("Fade Out"));
	m_fadeOutKnob.setModel(&m_slicerTParent->m_fadeOutFrames);

	// midi copy button
	m_midiExportButton.move(215, 150);
	m_midiExportButton.setActiveGraphic(PLUGIN_NAME::getIconPixmap("copyMidi"));
	m_midiExportButton.setInactiveGraphic(PLUGIN_NAME::getIconPixmap("copyMidi"));
	m_midiExportButton.setToolTip(tr("Copy midi pattern to clipboard"));
	connect(&m_midiExportButton, SIGNAL(clicked()), this, SLOT(exportMidi()));

	// slice reset button
	m_resetButton.move(19, 150);
	m_resetButton.setActiveGraphic(PLUGIN_NAME::getIconPixmap("resetSlices"));
	m_resetButton.setInactiveGraphic(PLUGIN_NAME::getIconPixmap("resetSlices"));
	m_resetButton.setToolTip(tr("Reset Slices"));
	connect(&m_resetButton, SIGNAL(clicked()), m_slicerTParent, SLOT(updateSlices()));
}

// copied from piano roll
void SlicerTView::exportMidi()
{
	using namespace Clipboard;

	DataFile dataFile(DataFile::Type::ClipboardData);
	QDomElement note_list = dataFile.createElement("note-list");
	dataFile.content().appendChild(note_list);

	std::vector<Note> notes = m_slicerTParent->getMidi();
	if (notes.size() == 0) { return; }

	TimePos start_pos(notes.front().pos().getBar(), 0);
	for (Note note : notes)
	{
		Note clip_note(note);
		clip_note.setPos(clip_note.pos(start_pos));
		clip_note.saveState(dataFile, note_list);
	}

	copyString(dataFile.toString(), MimeType::Default);
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

// display button text
void SlicerTView::paintEvent(QPaintEvent* pe)
{
	QPainter brush(this);
	brush.setPen(QColor(255, 255, 255));
	brush.setFont(QFont(brush.font().family(), 7.5f, -1, false));
	brush.drawText(212, 165, 25, 20, Qt::AlignCenter, tr("Midi"));
	brush.drawText(14, 165, 30, 20, Qt::AlignCenter, tr("Reset"));
	brush.drawText(185, 217, 55, 20, Qt::AlignCenter, tr("Snap"));
}

} // namespace gui
} // namespace lmms