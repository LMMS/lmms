#include "SlicerTUI.h"
#include "SlicerT.h"
// #include <QDomElement>
#include <stdio.h>


#include "StringPairDrag.h"
#include "Clipboard.h"
#include "Track.h"
#include "DataFile.h"
// #include "AudioEngine.h"
// #include "base64.h"
// #include "Engine.h"
// #include "Graph.h"
// #include "InstrumentTrack.h"
// #include "Knob.h"
// #include "LedCheckBox.h"
// #include "NotePlayHandle.h"
// #include "PixmapButton.h"
// #include "Song.h"
// #include "interpolation.h"

// #include <QPainter>
#include <QFileInfo>
#include <QDropEvent>
#include "embed.h"

namespace lmms
{


namespace gui
{
SlicerTUI::SlicerTUI( SlicerT * _instrument,
					QWidget * _parent ) :
	InstrumentViewFixedSize( _instrument, _parent ),
	noteThresholdKnob(KnobType::Bright26, this),
	slicerTParent(_instrument),
	wf(245, 125, _instrument, this),
	bpmBox(3, "21pink", this),
	resetButton(embed::getIconPixmap("reload"), QString(), this)
	
{
	setAcceptDrops( true );

	wf.move(2, 5); 

	resetButton.move(100, 150);
	connect(&resetButton, SIGNAL( clicked() ), slicerTParent, SLOT( updateSlices() ));
	connect(&resetButton, SIGNAL( clicked() ), &wf, SLOT( updateUI() ));

	bpmBox.move(30, 150);
	bpmBox.setToolTip(tr("original sample BPM"));
	bpmBox.setLabel(tr("BPM"));
	bpmBox.setModel(&slicerTParent->originalBPM);

	noteThresholdKnob.move(30, 200);
	noteThresholdKnob.setModel(&slicerTParent->noteThreshold);

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
		wf.updateFile( value );
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


