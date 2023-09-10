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
#include "SlicerT.h"

#include <QFileInfo>
#include <QDropEvent>

#include "StringPairDrag.h"
#include "Clipboard.h"
#include "Track.h"
#include "DataFile.h"

#include "Engine.h"
#include "Song.h"
#include "InstrumentTrack.h"

#include "embed.h"

namespace lmms
{


namespace gui
{

SlicerTUI::SlicerTUI( SlicerT * instrument,
					QWidget * parent ) :
	InstrumentViewFixedSize( instrument, parent ),
	m_slicerTParent(instrument),
	m_noteThresholdKnob(KnobType::Dark28, this),
	m_fadeOutKnob(KnobType::Dark28, this),
	m_bpmBox(3, "21pink", this),
	m_resetButton(embed::getIconPixmap("reload"), QString(), this),
	m_midiExportButton(embed::getIconPixmap("midi_tab"), QString(), this),
	m_wf(245, 125, instrument, this)
{
	setAcceptDrops( true );

	m_wf.move(2, 5);

	m_bpmBox.move(2, 150);
	m_bpmBox.setToolTip(tr("Original sample BPM"));
	m_bpmBox.setLabel(tr("BPM"));
	m_bpmBox.setModel(&m_slicerTParent->m_originalBPM);

	m_fadeOutKnob.move(200, 150);
	m_fadeOutKnob.setToolTip(tr("FadeOut for notes"));
	m_fadeOutKnob.setLabel(tr("FadeOut"));
	m_fadeOutKnob.setModel(&m_slicerTParent->m_fadeOutFrames);

	m_midiExportButton.move(150, 200);
	m_midiExportButton.setToolTip(tr("Copy midi pattern to clipboard"));
	connect(&m_midiExportButton, SIGNAL( clicked() ), this, SLOT( exportMidi() ));

	m_noteThresholdKnob.move(7, 200);
	m_noteThresholdKnob.setToolTip(tr("Threshold used for slicing"));
	m_noteThresholdKnob.setLabel(tr("Threshold"));
	m_noteThresholdKnob.setModel(&m_slicerTParent->m_noteThreshold);

	m_resetButton.move(70, 200);
	m_resetButton.setToolTip(tr("Reset Slices"));
	connect(&m_resetButton, SIGNAL( clicked() ), m_slicerTParent, SLOT( updateSlices() ));
}

// copied from piano roll
void SlicerTUI::exportMidi() {
	using namespace Clipboard;

	DataFile dataFile( DataFile::Type::ClipboardData );
	QDomElement note_list = dataFile.createElement( "note-list" );
	dataFile.content().appendChild( note_list );

	std::vector<Note> notes;
	m_slicerTParent->writeToMidi(&notes);
	if (notes.size() == 0)
	{
		return;
	}

	TimePos start_pos( notes.front().pos().getBar(), 0 );
	for( Note note : notes )
	{
		Note clip_note( note );
		clip_note.setPos( clip_note.pos( start_pos ) );
		clip_note.saveState( dataFile, note_list );
	}

	copyString( dataFile.toString(), MimeType::Default );
}

// all the drag stuff is copied from AudioFileProcessor
void SlicerTUI::dragEnterEvent( QDragEnterEvent * dee )
{
		// For mimeType() and MimeType enum class
	using namespace Clipboard;

	if( dee->mimeData()->hasFormat( mimeType( MimeType::StringPair ) ) )
	{
		QString txt = dee->mimeData()->data(
						mimeType( MimeType::StringPair ) );
		if( txt.section( ':', 0, 0 ) == QString( "clip_%1" ).arg(
							static_cast<int>(Track::Type::Sample) ) )
		{
			dee->acceptProposedAction();
		}
		else if( txt.section( ':', 0, 0 ) == "samplefile" )
		{
			dee->acceptProposedAction();
		}
		else
		{
			dee->ignore();
		}
	}
	else
	{
		dee->ignore();
	}
}

void SlicerTUI::dropEvent( QDropEvent * de ) {
	QString type = StringPairDrag::decodeKey( de );
	QString value = StringPairDrag::decodeValue( de );
	if( type == "samplefile" )
	{
		m_slicerTParent->updateFile( value );
		// castModel<AudioFileProcessor>()->setAudioFile( value );
		// de->accept();
		// set m_wf wave file
		return;
	}
	else if( type == QString( "clip_%1" ).arg( static_cast<int>(Track::Type::Sample) ) )
	{
		DataFile dataFile( value.toUtf8() );
		m_slicerTParent->updateFile( dataFile.content().firstChild().toElement().attribute( "src" ) );
		de->accept();
		return;
	}

	de->ignore();
}
} // namespace gui
} // namespace lmms