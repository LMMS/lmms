#include <Qt> // For Qt::WindowFlags
#include <QWidget>
#include <QMdiSubWindow>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>

#include "MidiCCRackView.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "embed.h"
#include "ComboBox.h"
#include "GroupBox.h"


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
	subWin->resize( 350, 200 );
	subWin->setFixedWidth( 350 );
	subWin->setMinimumHeight( 200 );

	// Main window layout
	QVBoxLayout *mainLayout = new QVBoxLayout( this );

	// Track Selector Layout - Here we select which track we are sending CC messages to
	// We use a widget to be able to make this layout have a fixed height
	QWidget *trackToolBar = new QWidget();
	QHBoxLayout *trackToolBarLayout = new QHBoxLayout(trackToolBar);
	QLabel *trackLabel = new QLabel( tr("Track: ") );
	ComboBox *trackComboBox = new ComboBox();

	trackToolBarLayout->addWidget(trackLabel);
	trackToolBarLayout->addWidget(trackComboBox);
	trackToolBarLayout->setStretchFactor( trackLabel, 1 );
	trackToolBarLayout->setStretchFactor( trackComboBox, 2 );
	trackToolBar->setFixedHeight(40);

	// Knobs Layout - Here we have the MIDI CC controller knobs for the selected track
	QVBoxLayout *knobsLayout = new QVBoxLayout();
	GroupBox *knobsGroupBox = new GroupBox( tr("MIDI CC Knobs:") );

	knobsLayout->addWidget(knobsGroupBox);

	// Adding both to the main layout
	mainLayout->addWidget(trackToolBar);
	mainLayout->addLayout(knobsLayout);
}

MidiCCRackView::~MidiCCRackView()
{
}

void MidiCCRackView::saveSettings( QDomDocument & _doc,
				QDomElement & _this )
{
}

void MidiCCRackView::loadSettings( const QDomElement & _this )
{
}
