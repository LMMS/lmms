
#include <QApplication>
#include <QLayout>
#include <QPushButton>
#include <QComboBox>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QMdiArea>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>

#include <QtXml/QDomElement>

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
	setMinimumWidth( 250 );
	setMinimumHeight( 210 );
	setMaximumWidth( 250 );
	resize( 250, 220 );

	setWindowIcon( embed::getIconPixmap( "note_double_whole" ) );
	setWindowTitle( tr( "Studio Controller" ) );

	m_dropDown = new QComboBox(this);
	m_dropDown->setInsertPolicy(QComboBox::InsertAtTop);
	// insert reverse order
	m_dropDown->insertItem(0, "No Controller" , QVariant::fromValue(0) );
	
	m_dropDown->insertItem(0, "nanoKontrol" , QVariant::fromValue(1) );
	
	m_dropDown->setCurrentIndex(0);

	m_layout = new QVBoxLayout();
	m_layout->addWidget( m_dropDown );
	this->setLayout( m_layout );

	QMdiSubWindow * subWin = gui->mainWindow()->addWindowedWidget( this );

	// No maximize button
	Qt::WindowFlags flags = subWin->windowFlags();
	flags &= ~Qt::WindowMaximizeButtonHint;
	subWin->setWindowFlags( flags );

	parentWidget()->setAttribute( Qt::WA_DeleteOnClose, false );
	parentWidget()->move( 90, 90 );

	// LMMS midi configurable actions
	m_homeButton = new AutomatableControlButton(this, "home");
	m_homeButton->setText("home");
	m_layout->addWidget( m_homeButton );
	connect( m_homeButton->model(), SIGNAL( dataChanged() ), this, SLOT( doHome() ) );
	
	m_stopButton = new AutomatableControlButton(this, "stop");
	m_stopButton->setText("stop");
	m_layout->addWidget( m_stopButton );
	connect( m_stopButton->model(), SIGNAL( dataChanged() ), this, SLOT( doStop() ) );
	
	m_playButton = new AutomatableControlButton(this, "play");
	m_playButton->setText("play");
	m_layout->addWidget( m_playButton );
	connect( m_playButton->model(), SIGNAL( dataChanged() ), this, SLOT( doPlay() ) );
	
	m_recordButton = new AutomatableControlButton(this, "record");
	m_recordButton->setText("record");
	m_layout->addWidget( m_recordButton );
	connect( m_recordButton->model(), SIGNAL( dataChanged() ), this, SLOT( doRecord() ) );
	
	m_scrollLast = 0.0f;
	m_scrollButton = new AutomatableControlButton(this, "scroll");
	m_scrollButton->setText("scroll");
	m_layout->addWidget( m_scrollButton );
	connect( m_scrollButton->model(), SIGNAL( dataChanged() ), this, SLOT( doScroll() ) );

	m_nextButton = new AutomatableControlButton(this, "next");
	m_nextButton->setText("next");
	m_layout->addWidget( m_nextButton );
	connect( m_nextButton->model(), SIGNAL( dataChanged() ), this, SLOT( doNext() ) );

	m_prevButton = new AutomatableControlButton(this, "prev");
	m_prevButton->setText("prev");
	m_layout->addWidget( m_prevButton );
	connect( m_prevButton->model(), SIGNAL( dataChanged() ), this, SLOT( doPrev() ) );
	
	QPushButton* m_saveButton = new QPushButton("save", this);
	m_layout->addWidget( m_saveButton );
	connect( m_saveButton, SIGNAL( clicked() ), this, SLOT( saveControllers() ) );
	
	// controller actions
	connect( m_dropDown, SIGNAL( activated(int) ), this, SLOT( controllerChanged(int) ) );

	update();
}

StudioControllerView::~StudioControllerView()
{
	delete m_dropDown;
}


void StudioControllerView::doHome()
{

	// Korg
	//Note ON we get  0.496094
	//Note OFF we get 126.503906
	// TODO presumably other controllers don't do control change like this?
	bool click = m_homeButton->model()->value() < 1.0f;
	
	if (click) gui->songEditor()->home();

}


void StudioControllerView::doStop()
{

	// Korg
	//Note ON we get  0.496094
	//Note OFF we get 126.503906
	// TODO presumably other controllers don't do control change like this?
	bool click = m_stopButton->model()->value() < 1.0f;
	
	// receive 127 for note on (pressed) and 0 for released
	if (click) gui->songEditor()->stop();
}


void StudioControllerView::doPlay()
{

	// Korg
	//Note ON we get  0.496094
	//Note OFF we get 126.503906
	// TODO presumably other controllers don't do control change like this?
	bool click = m_playButton->model()->value() < 1.0f;
	
	if (click) gui->songEditor()->play();

}


void StudioControllerView::doNext()
{

	// Korg
	//Note ON we get  0.496094
	//Note OFF we get 126.503906
	// TODO presumably other controllers don't do control change like this?
	bool click = m_playButton->model()->value() < 1.0f;
	
	if (click) gui->songEditor()->next();

}


void StudioControllerView::doPrev()
{

	// Korg
	//Note ON we get  0.496094
	//Note OFF we get 126.503906
	// TODO presumably other controllers don't do control change like this?
	bool click = m_playButton->model()->value() < 1.0f;
	
	if (click) gui->songEditor()->prev();

}


void StudioControllerView::doRecord()
{

	// Korg
	//Note ON we get  0.496094
	//Note OFF we get 126.503906
	// TODO presumably other controllers don't do control change like this?
	bool click = m_recordButton->model()->value() < 1.0f;
	
	if (click) gui->songEditor()->record();

}


void StudioControllerView::doScroll()
{

	// Korg
	// get a value from 1-127 tha LMMS retgurns as 0-126.0

	float pos = m_scrollButton->model()->value();
	
	if (pos > m_scrollLast ) gui->songEditor()->next();
	if (pos < m_scrollLast ) gui->songEditor()->prev();
	
	m_scrollLast = pos;
	
}




void StudioControllerView::controllerChanged(int index)
{
	// TODO load a file of mappings
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
	qWarning("saving...");
	QDomDocument doc = QDomDocument( "lmms-studio-controller" );
	QDomElement root = doc.createElement( "lmms-studio-controller" );
	root.setAttribute( "creator", "LMMS" );
	doc.appendChild(root);

	root.appendChild(saveSettings(doc, m_homeButton));
	root.appendChild(saveSettings(doc, m_nextButton));
	root.appendChild(saveSettings(doc, m_playButton));
	root.appendChild(saveSettings(doc, m_prevButton));
	root.appendChild(saveSettings(doc, m_recordButton));
	root.appendChild(saveSettings(doc, m_saveButton));
	root.appendChild(saveSettings(doc, m_scrollButton));
	root.appendChild(saveSettings(doc, m_stopButton));
	
	QFile outfile( "/var/tmp/lmms-studio-controller.xml" );
	if (! outfile.open(QIODevice::WriteOnly | QIODevice::Text) ) {
		QMessageBox::critical( NULL, "Could not open file", "Could not open file /var/tmp/lmms-studio-controller.xml" );
	}
	else {
		QTextStream ts( &outfile );
		ts << doc.toString();
		outfile.close();
	}
	qWarning("saved");
}



void StudioControllerView::loadControllers()
{
	QDomDocument doc = QDomDocument( "lmms-studio-controller" );
	QDomElement root = doc.createElement( "lmms-studio-controller" );
	
	AutomatableModel* am = dynamic_cast<AutomatableModel*>(m_homeButton->model());
	am->setControllerConnection(NULL);
}
