#include <Qt> // For Qt::WindowFlags
#include <QWidget>
#include <QMdiSubWindow>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>

#include "MidiCCRackView.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "embed.h"
#include "ComboBox.h"
#include "ComboBoxModel.h"
#include "GroupBox.h"
#include "Knob.h"
#include "Track.h"
#include "InstrumentTrack.h"
#include "TrackContainer.h"
#include "BBTrackContainer.h"
#include "Engine.h"
#include "Song.h"
#include "SongEditor.h"
#include "BBEditor.h"


MidiCCRackView::MidiCCRackView() :
	QWidget()
{
	setWindowIcon( embed::getIconPixmap( "midi_cc_rack" ) );
	setWindowTitle( tr("Midi CC Rack") );

	QMdiSubWindow * subWin = gui->mainWindow()->addWindowedWidget( this );

	// Remove maximize button
	Qt::WindowFlags flags = subWin->windowFlags();
	flags &= ~Qt::WindowMaximizeButtonHint;
	subWin->setWindowFlags( flags );

	// Adjust window attributes, sizing and position
	subWin->setAttribute( Qt::WA_DeleteOnClose, false );
	subWin->move( 780, 50 );
	subWin->resize( 350, 300 );
	subWin->setFixedWidth( 350 );
	subWin->setMinimumHeight( 300 );

	// Main window layout
	QVBoxLayout *mainLayout = new QVBoxLayout( this );

	// Track Selector Layout - Here we select which track we are sending CC messages to
	// We use a widget to be able to make this layout have a fixed height
	QWidget *trackToolBar = new QWidget();
	QHBoxLayout *trackToolBarLayout = new QHBoxLayout( trackToolBar );
	QLabel *trackLabel = new QLabel( tr("Track: ") );
	m_trackComboBox = new ComboBox();
	m_trackComboBoxModel = new ComboBoxModel();
	m_trackComboBox->setModel(m_trackComboBoxModel);

	trackToolBarLayout->addWidget(trackLabel);
	trackToolBarLayout->addWidget(m_trackComboBox);
	trackToolBarLayout->setStretchFactor( trackLabel, 1 );
	trackToolBarLayout->setStretchFactor( m_trackComboBox, 2 );
	trackToolBar->setFixedHeight(40);

	// Knobs GroupBox - Here we have the MIDI CC controller knobs for the selected track
	GroupBox *knobsGroupBox = new GroupBox( tr("MIDI CC Knobs:") );

	m_midiCCLed = knobsGroupBox->ledButton();

	// Layout to keep scrollable area under the GroupBox header
	QVBoxLayout *knobsGroupBoxLayout = new QVBoxLayout();
	knobsGroupBoxLayout->setContentsMargins( 5, 16, 5, 5 );

	knobsGroupBox->setLayout(knobsGroupBoxLayout);

	// Scrollable area + widget + its layout that will have all the knobs
	QScrollArea *knobsScrollArea = new QScrollArea();
	QWidget *knobsArea = new QWidget();
	QGridLayout *knobsAreaLayout = new QGridLayout();

	knobsArea->setLayout( knobsAreaLayout );
	knobsScrollArea->setWidget( knobsArea );
	knobsScrollArea->setWidgetResizable( true );

	knobsGroupBoxLayout->addWidget(knobsScrollArea);

	// Adds the controller knobs
	for(int i = 0; i < MIDI_CC_MAX_CONTROLLERS; i++){
		m_controllerKnob[i] = new Knob( knobBright_26 );
		m_controllerKnob[i]->setLabel( QString("CC %1").arg(QString::number(i)) );
		knobsAreaLayout->addWidget( m_controllerKnob[i], i/3, i%3 );
	}

	// Connections are made to make sure the track ComboBox is updated when tracks are
	// added, removed or renamed
	// On the song editor:
	connect( Engine::getSong() , SIGNAL( trackAdded(Track *) ),
		this, SLOT( updateTracksComboBox() ) );
	connect( Engine::getSong() , SIGNAL( trackRemoved() ),
		this, SLOT( updateTracksComboBox() ) );
	connect( Engine::getSong() , SIGNAL( trackRenamed() ),
		this, SLOT( updateTracksComboBox() ) );
	// On the BB editor:
	connect( Engine::getBBTrackContainer() , SIGNAL( trackAdded(Track *) ),
		this, SLOT( updateTracksComboBox() ) );
	connect( Engine::getBBTrackContainer() , SIGNAL( trackRemoved() ),
		this, SLOT( updateTracksComboBox() ) );
	connect( Engine::getBBTrackContainer() , SIGNAL( trackRenamed() ),
		this, SLOT( updateTracksComboBox() ) );
	// Also when tracks are moved on the song editor and BB editor
	connect( gui->songEditor()->m_editor, SIGNAL( movedTrackView() ),
		this, SLOT( updateTracksComboBox() ) );
	connect( gui->getBBEditor()->trackContainerView(), SIGNAL( movedTrackView() ),
		this, SLOT( updateTracksComboBox() ) );

	// Connection to update the knobs when the ComboBox selects another track
	connect( m_trackComboBoxModel, SIGNAL( dataChanged() ),
		this, SLOT( updateKnobsModels() ));

	// Adding everything to the main layout
	mainLayout->addWidget(trackToolBar);
	mainLayout->addWidget(knobsGroupBox);
}

MidiCCRackView::~MidiCCRackView()
{
}

void MidiCCRackView::updateTracksComboBox()
{
	// Reset the combo box model to fill it with instrument tracks from the song/BB editors
	m_trackComboBoxModel->clear();

	// Reset our list with pointers to the tracks
	m_tracks.clear();

	TrackContainer::TrackList songEditorTracks;
	songEditorTracks = Engine::getSong()->tracks();
	int songEditorID = 1;

	TrackContainer::TrackList bbEditorTracks;
	bbEditorTracks = Engine::getBBTrackContainer()->tracks();
	int bbEditorID = 1;

	for(Track *t: songEditorTracks)
	{
		if( t->type() == Track::InstrumentTrack )
		{
			m_trackComboBoxModel->addItem("SongEditor: " + QString::number(songEditorID) + ". " + t->name());
			m_tracks += t;
			++songEditorID;
		}
	}
	for(Track *t: bbEditorTracks)
	{
		if( t->type() == Track::InstrumentTrack )
		{
			m_trackComboBoxModel->addItem("BBEditor: " + QString::number(bbEditorID) + ". " + t->name());
			m_tracks += t;
			++bbEditorID;
		}
	}

	updateKnobsModels();
}

void MidiCCRackView::updateKnobsModels()
{
	if( m_tracks.size() > 0 )
	{
		InstrumentTrack *selectedTrack = dynamic_cast<InstrumentTrack *>( m_tracks[ m_trackComboBoxModel->value() ] );

		// TODO: I need to figure out why this line is necessary. Without it I get a segfault because at
		// LMMS's startup sometimes m_tracks will hold tracks that have type() == Tracks::InstrumentTracks
		// but casting the to InstrumentTrack * returns a nullptr. Meaning maybe the constructor wasn't
		// executed yet.
		if( selectedTrack )
		{
			// Set the LED button to enable/disable the track midi cc
			m_midiCCLed->setModel( selectedTrack->m_midiCCEnable );

			// Set the model for each Knob
			for( int i = 0; i < MIDI_CC_MAX_CONTROLLERS; ++i ){
				m_controllerKnob[i]->setModel( selectedTrack->m_midiCCModel[i] );
			}
		}
	}
}

void MidiCCRackView::saveSettings( QDomDocument & _doc,
				QDomElement & _this )
{
}

void MidiCCRackView::loadSettings( const QDomElement & _this )
{
}
