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
#include "GroupBox.h"
#include "Knob.h"


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
	QHBoxLayout *trackToolBarLayout = new QHBoxLayout( trackToolBar );
	QLabel *trackLabel = new QLabel( tr("Track: ") );
	ComboBox *trackComboBox = new ComboBox();

	trackToolBarLayout->addWidget(trackLabel);
	trackToolBarLayout->addWidget(trackComboBox);
	trackToolBarLayout->setStretchFactor( trackLabel, 1 );
	trackToolBarLayout->setStretchFactor( trackComboBox, 2 );
	trackToolBar->setFixedHeight(40);

	// Knobs GroupBox - Here we have the MIDI CC controller knobs for the selected track
	GroupBox *knobsGroupBox = new GroupBox( tr("MIDI CC Knobs:") );

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
	Knob *k;
	for(int i = 0; i < 127; i++){
		k = new Knob( knobBright_26 );
		k->setLabel( QString("CC %1").arg(QString::number(i + 1)) );
		knobsAreaLayout->addWidget( k, i/3, i%3 );
	}

	// Adding everything to the main layout
	mainLayout->addWidget(trackToolBar);
	mainLayout->addWidget(knobsGroupBox);
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
