/*
 * SlicerTUI.cpp - controls the UI for slicerT
 *
 * Copyright (c) 2006-2008 Andreas Brandmaier <andy/at/brandmaier/dot/de>
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

#include "SlicerTUI.h"

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

SlicerTUI::SlicerTUI(SlicerT* instrument, QWidget* parent)
	: InstrumentViewFixedSize(instrument, parent)
	, m_slicerTParent(instrument)
	, m_noteThresholdKnob(this)
	, m_fadeOutKnob(this)
	, m_bpmBox(3, "21pink", this)
	, m_snapSetting(this, "Slice snap")
	, m_syncToggle("Sync", this, "SyncToggle", LedCheckBox::LedColor::Green)
	, m_resetButton(this, nullptr)
	, m_midiExportButton(this, nullptr)
	, m_wf(248, 128, instrument, this)
{
	// window settings
	setAcceptDrops(true);
	setAutoFillBackground(true);

	// render background
	QPalette pal;
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("bg"));
	setPalette(pal);

	// move editor and seeker
	m_wf.move(2, 6);

	// snap combo box
	m_snapSetting.setGeometry(190, 200, 55, ComboBox::DEFAULT_HEIGHT);
	m_snapSetting.setToolTip(tr("Set slice snapping for detection"));
	m_snapSetting.setModel(&m_slicerTParent->m_sliceSnap);

	// sync toggle
	m_syncToggle.move(135, 187);
	m_syncToggle.setToolTip(tr("Enable BPM sync"));
	m_syncToggle.setModel(&m_slicerTParent->m_enableSync);

	// bpm spin box
	m_bpmBox.move(135, 203);
	m_bpmBox.setToolTip(tr("Original sample BPM"));
	m_bpmBox.setLabel(tr("BPM"));
	m_bpmBox.setModel(&m_slicerTParent->m_originalBPM);

	// threshold knob
	m_noteThresholdKnob.move(14, 195);
	m_noteThresholdKnob.setToolTip(tr("Threshold used for slicing"));
	m_noteThresholdKnob.setLabel(tr("Threshold"));
	m_noteThresholdKnob.setModel(&m_slicerTParent->m_noteThreshold);

	// fadeout knob
	m_fadeOutKnob.move(80, 195);
	m_fadeOutKnob.setToolTip(tr("FadeOut for notes"));
	m_fadeOutKnob.setLabel(tr("FadeOut"));
	m_fadeOutKnob.setModel(&m_slicerTParent->m_fadeOutFrames);

	// midi copy button
	m_midiExportButton.move(205, 155);
	m_midiExportButton.setActiveGraphic(PLUGIN_NAME::getIconPixmap("copyMidi"));
	m_midiExportButton.setInactiveGraphic(PLUGIN_NAME::getIconPixmap("copyMidi"));
	m_midiExportButton.setToolTip(tr("Copy midi pattern to clipboard"));
	connect(&m_midiExportButton, SIGNAL(clicked()), this, SLOT(exportMidi()));

	// slcie reset button
	m_resetButton.move(25, 155);
	m_resetButton.setActiveGraphic(PLUGIN_NAME::getIconPixmap("resetSlices"));
	m_resetButton.setInactiveGraphic(PLUGIN_NAME::getIconPixmap("resetSlices"));
	m_resetButton.setToolTip(tr("Reset Slices"));
	connect(&m_resetButton, SIGNAL(clicked()), m_slicerTParent, SLOT(updateSlices()));
}

// copied from piano roll
void SlicerTUI::exportMidi()
{
	using namespace Clipboard;

	DataFile dataFile(DataFile::Type::ClipboardData);
	QDomElement note_list = dataFile.createElement("note-list");
	dataFile.content().appendChild(note_list);

	std::vector<Note> notes;
	m_slicerTParent->writeToMidi(&notes);
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
void SlicerTUI::dragEnterEvent(QDragEnterEvent* dee)
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

void SlicerTUI::dropEvent(QDropEvent* de)
{
	QString type = StringPairDrag::decodeKey(de);
	QString value = StringPairDrag::decodeValue(de);
	if (type == "samplefile")
	{
		m_slicerTParent->updateFile(value);
		// castModel<AudioFileProcessor>()->setAudioFile( value );
		// de->accept();
		// set m_wf wave file
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
void SlicerTUI::paintEvent(QPaintEvent* pe)
{
	QPainter brush(this);
	brush.setPen(QColor(255, 255, 255));
	brush.setFont(QFont(brush.font().family(), 7.5f, -1, false));
	brush.drawText(205, 170, 20, 20, Qt::AlignCenter, tr("Midi"));
	brush.drawText(22, 170, 25, 20, Qt::AlignCenter, tr("Reset"));
	brush.drawText(190, 217, 55, 20, Qt::AlignCenter, tr("Snap"));
}

} // namespace gui
} // namespace lmms