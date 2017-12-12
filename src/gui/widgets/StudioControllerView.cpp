
#include <QApplication>
#include <QLayout>
#include <QPushButton>
#include <QComboBox>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QMdiArea>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>

#include <QDomElement>

#include "embed.h"
#include "Engine.h"
#include "MainWindow.h"
#include "Song.h"
#include "SongEditor.h"
#include "GuiApplication.h"
#include "StudioControllerView.h"
#include "ControllerConnection.h"
#include "MidiController.h"

StudioControllerView::StudioControllerView( ) :
	QWidget()
{
	setWindowIcon( embed::getIconPixmap( "note_double_whole" ) );
	setWindowTitle( tr( "Studio Controller" ) );

	m_layout = new QVBoxLayout();
	this->setLayout( m_layout );

	m_controllerLabel = new QLabel( tr( "Controller:" ), this );
	m_actionsLabel = new QLabel( tr( "Actions:" ), this );

	m_dropDown = new QComboBox(this);
	m_dropDown->setInsertPolicy(QComboBox::InsertAtTop);
	// Insert reverse order.
	m_dropDown->insertItem(0, "nanoKontrol" , QVariant::fromValue(1));
	m_dropDown->insertItem(0, tr("No controller") , QVariant::fromValue(0));
	
	m_dropDown->setCurrentIndex(0);

	m_layout->addWidget( m_controllerLabel );
	m_layout->addWidget( m_dropDown );
	m_layout->addWidget( m_actionsLabel );

	QMdiSubWindow * subWin = gui->mainWindow()->addWindowedWidget( this );

	// No maximize button.
	Qt::WindowFlags flags = subWin->windowFlags();
	flags &= ~Qt::WindowMaximizeButtonHint;
	subWin->setWindowFlags( flags );

	subWin->setFixedSize( 170, 340 );

	parentWidget()->setAttribute( Qt::WA_DeleteOnClose, false );
	parentWidget()->move( 1080, 90 );

	// LMMS MIDI configurable actions.
	m_homeButton = new AutomatableControlButton(this, "home");
	m_homeButton->setText(tr("Home"));
	m_layout->addWidget( m_homeButton );
	connect( m_homeButton->model(), SIGNAL( dataChanged() ), this, SLOT( doHome() ) );

	m_playButton = new AutomatableControlButton(this, "play");
	m_playButton->setText(tr("Play"));
	m_layout->addWidget( m_playButton );
	connect( m_playButton->model(), SIGNAL( dataChanged() ), this, SLOT( doPlay() ) );

	m_stopButton = new AutomatableControlButton(this, "stop");
	m_stopButton->setText(tr("Stop"));
	m_layout->addWidget( m_stopButton );
	connect( m_stopButton->model(), SIGNAL( dataChanged() ), this, SLOT( doStop() ) );

	m_recordButton = new AutomatableControlButton(this, "record");
	m_recordButton->setText(tr("Record"));
	m_layout->addWidget( m_recordButton );
	connect( m_recordButton->model(), SIGNAL( dataChanged() ), this, SLOT( doRecord() ) );

	m_nextButton = new AutomatableControlButton(this, "next");
	m_nextButton->setText(tr("Next"));
	m_layout->addWidget( m_nextButton );
	connect( m_nextButton->model(), SIGNAL( dataChanged() ), this, SLOT( doNext() ) );

	m_prevButton = new AutomatableControlButton(this, "prev");
	m_prevButton->setText(tr("Previous"));
	m_layout->addWidget( m_prevButton );
	connect( m_prevButton->model(), SIGNAL( dataChanged() ), this, SLOT( doPrev() ) );

	m_scrollLast = 0.0f;
	m_scrollButton = new AutomatableControlButton(this, "scroll");
	m_scrollButton->setText(tr("Scroll"));
	m_layout->addWidget( m_scrollButton );
	connect( m_scrollButton->model(), SIGNAL( dataChanged() ), this, SLOT( doScroll() ) );

	QPushButton* m_saveButton = new QPushButton(tr("Save"), this);
	m_layout->addWidget( m_saveButton );
	connect( m_saveButton, SIGNAL( clicked() ), this, SLOT( saveControllers() ) );
	
	// Controller actions.
	connect( m_dropDown, SIGNAL( activated(int) ), this, SLOT( controllerChanged(int) ) );

	update();
}

StudioControllerView::~StudioControllerView()
{
	delete m_dropDown;
}


void StudioControllerView::doHome()
{
	// Korg:  NOTE ON=0.496094, OFF=126.503906 
	// TODO: Confirm for other controllers
	bool click = m_homeButton->model()->value() < 1.0f;
	if (click) gui->songEditor()->home();
}


void StudioControllerView::doPlay()
{
	bool click = m_playButton->model()->value() < 1.0f;
	if (click) gui->songEditor()->play();
}


void StudioControllerView::doStop()
{
	// Receive 127 for note on (pressed) and 0 for released
	bool click = m_stopButton->model()->value() < 1.0f;
	if (click) gui->songEditor()->stop();
}


void StudioControllerView::doRecord()
{
	bool click = m_recordButton->model()->value() < 1.0f;
	if (click) gui->songEditor()->record();
}


void StudioControllerView::doNext()
{
	bool click = m_playButton->model()->value() < 1.0f;
	if (click) gui->songEditor()->next();
}


void StudioControllerView::doPrev()
{
	bool click = m_playButton->model()->value() < 1.0f;
	if (click) gui->songEditor()->prev();
}


void StudioControllerView::doScroll()
{
	// Korg: Value range 1-127, LMMS: 0.0-126.0.
	float pos = m_scrollButton->model()->value();
	if (pos > m_scrollLast ) gui->songEditor()->next();
	if (pos < m_scrollLast ) gui->songEditor()->prev();
	m_scrollLast = pos;
}




void StudioControllerView::controllerChanged(int index)
{
	// TODO: load a file of mappings.
}

QDomElement saveSettings(QDomDocument & doc, AutomatableControlButton* button)
{
	QDomElement elem = doc.createElement( button->text() );
	AutomatableModel* amp = dynamic_cast<AutomatableModel*>(button->model());
	amp->saveSettings(doc, elem, button->text() );
	return elem;
}

void StudioControllerView::saveControllers()
{
	QDomDocument doc = QDomDocument( "lmms-studio-controller" );
	QDomElement root = doc.createElement( "lmms-studio-controller" );
	root.setAttribute( "creator", "LMMS" );
	doc.appendChild(root);

	root.appendChild(saveSettings(doc, m_homeButton));
	root.appendChild(saveSettings(doc, m_playButton));
	root.appendChild(saveSettings(doc, m_stopButton));
	root.appendChild(saveSettings(doc, m_recordButton));
	root.appendChild(saveSettings(doc, m_nextButton));
	root.appendChild(saveSettings(doc, m_prevButton));
	root.appendChild(saveSettings(doc, m_scrollButton));
	root.appendChild(saveSettings(doc, m_saveButton));
	
	// FIXME: Make this platform independent.
	QFile outfile( "/var/tmp/lmms-studio-controller.xml" );
	if (! outfile.open(QIODevice::WriteOnly | QIODevice::Text) ) {
		QMessageBox::critical( NULL, "Could not open file", "Could not open file /var/tmp/lmms-studio-controller.xml" );
	}
	else {
		QTextStream ts( &outfile );
		ts << doc.toString();
		outfile.close();
	}
}


void StudioControllerView::loadControllers()
{
	QDomDocument doc = QDomDocument( "lmms-studio-controller" );
	QDomElement root = doc.createElement( "lmms-studio-controller" );
	
	AutomatableModel* am = dynamic_cast<AutomatableModel*>(m_homeButton->model());
	am->setControllerConnection(NULL);
}
