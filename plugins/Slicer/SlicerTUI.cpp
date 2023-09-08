#include "SlicerTUI.h"
#include "SlicerT.h"

#include <stdio.h>
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
SlicerTUI::SlicerTUI( SlicerT * _instrument,
					QWidget * _parent ) :
	InstrumentViewFixedSize( _instrument, _parent ),
	slicerTParent(_instrument),
	noteThresholdKnob(KnobType::Dark28, this),
	fadeOutKnob(KnobType::Dark28, this),
	bpmBox(3, "21pink", this),
	resetButton(embed::getIconPixmap("reload"), QString(), this),
	timeShiftButton(embed::getIconPixmap("max_length"), QString(), this),
	midiExportButton(embed::getIconPixmap("midi_tab"), QString(), this),
	wf(245, 125, _instrument, this)
{
	setAcceptDrops( true );

	wf.move(2, 5); 

	bpmBox.move(2, 150);
	bpmBox.setToolTip(tr("Original sample BPM"));
	bpmBox.setLabel(tr("BPM"));
	bpmBox.setModel(&slicerTParent->originalBPM);

	timeShiftButton.move(70, 150);
	timeShiftButton.setToolTip(tr("Timeshift sample"));
	connect(&timeShiftButton, SIGNAL( clicked() ), slicerTParent, SLOT( updateTimeShift() ));

	fadeOutKnob.move(200, 150);
	fadeOutKnob.setToolTip(tr("FadeOut for notes"));
	fadeOutKnob.setLabel(tr("FadeOut"));
	fadeOutKnob.setModel(&slicerTParent->fadeOutFrames);

	midiExportButton.move(150, 150);
	midiExportButton.setToolTip(tr("Copy midi pattern to clipboard"));
	connect(&midiExportButton, SIGNAL( clicked() ), this, SLOT( exportMidi() ));

	noteThresholdKnob.move(7, 200);
	noteThresholdKnob.setToolTip(tr("Threshold used for slicing"));
	noteThresholdKnob.setLabel(tr("Threshold"));
	noteThresholdKnob.setModel(&slicerTParent->noteThreshold);

	resetButton.move(70, 200);
	resetButton.setToolTip(tr("Reset Slices"));
	connect(&resetButton, SIGNAL( clicked() ), slicerTParent, SLOT( updateSlices() ));



}

// copied from piano roll
void SlicerTUI::exportMidi() {
	using namespace Clipboard;

	DataFile dataFile( DataFile::Type::ClipboardData );
	QDomElement note_list = dataFile.createElement( "note-list" );
	dataFile.content().appendChild( note_list );

	std::vector<Note> notes;
	slicerTParent->writeToMidi(&notes);
	if (notes.size() == 0) {
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

void SlicerTUI::mousePressEvent( QMouseEvent * _me ) {
	printf("clicked on TUI\n");
	// slicerTParent->findSlices();
	// slicerTParent->findBPM();
	// slicerTParent->timeShiftSample();
	update();
}

// all the drag stuff is copied from AudioFileProcessor
void SlicerTUI::dragEnterEvent( QDragEnterEvent * _dee )
{
		// For mimeType() and MimeType enum class
	using namespace Clipboard;

	if( _dee->mimeData()->hasFormat( mimeType( MimeType::StringPair ) ) )
	{
		QString txt = _dee->mimeData()->data(
						mimeType( MimeType::StringPair ) );
		if( txt.section( ':', 0, 0 ) == QString( "clip_%1" ).arg(
							static_cast<int>(Track::Type::Sample) ) )
		{
			_dee->acceptProposedAction();
		}
		else if( txt.section( ':', 0, 0 ) == "samplefile" )
		{
			_dee->acceptProposedAction();
		}
		else
		{
			_dee->ignore();
		}
	}
	else
	{
		_dee->ignore();
	}

}

void SlicerTUI::dropEvent( QDropEvent * _de ) {
	QString type = StringPairDrag::decodeKey( _de );
	QString value = StringPairDrag::decodeValue( _de );
	if( type == "samplefile" )
	{
		printf("type: samplefile\n");
		slicerTParent->updateFile( value );
		// castModel<AudioFileProcessor>()->setAudioFile( value );
		// _de->accept();
		// set wf wave file
		return;
	}
	else if( type == QString( "clip_%1" ).arg( static_cast<int>(Track::Type::Sample) ) )
	{
		printf("type: clip file\n");
		DataFile dataFile( value.toUtf8() );
		slicerTParent->updateFile( dataFile.content().firstChild().toElement().attribute( "src" ) );
		_de->accept();
		return;
	}

	_de->ignore();
}


}
}


