
#include <QApplication>
#include <QLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QMdiArea>

#include <QtXml/QDomElement>

#include "embed.h"
#include "Engine.h"
#include "MainWindow.h"
#include "Song.h"
#include "GuiApplication.h"

#include "Groove.h"
#include "GrooveView.h"
#include "HydrogenSwing.h"
#include "HalfSwing.h"
#include "GrooveExperiments.h"
#include "MidiSwing.h"

//GrooveView::GrooveView(QWidget *parent) :
//	QWidget(parent)
GrooveView::GrooveView( ) :
	QWidget()
{
	setMinimumWidth( 250 );
	setMinimumHeight( 210 );
	setMaximumWidth( 250 );
	resize( 250, 220 );

	setWindowIcon( embed::getIconPixmap( "note_double_whole" ) );
	setWindowTitle( tr( "Groove" ) );

	m_dropDown = new QComboBox(this);
	// insert reverse order
	m_dropDown->insertItem(0, "Half Swing" , QVariant::fromValue(5) );
	m_dropDown->insertItem(0, "Experiment" , QVariant::fromValue(4) );
	m_dropDown->insertItem(0, "Midi Swing" , QVariant::fromValue(3) );
	m_dropDown->insertItem(0, "Hydrogen Swing" , QVariant::fromValue(2) );
	m_dropDown->insertItem(0, "Straight" , QVariant::fromValue(1) );
	m_dropDown->setCurrentIndex(0);

	m_layout = new QVBoxLayout();
	m_layout->addWidget( m_dropDown );
	m_layout->addWidget( new QLabel("Select groove") );
	this->setLayout( m_layout );

	QMdiSubWindow * subWin = gui->mainWindow()->addWindowedWidget( this );

	// No maximize button
	Qt::WindowFlags flags = subWin->windowFlags();
	flags &= ~Qt::WindowMaximizeButtonHint;
	subWin->setWindowFlags( flags );

	parentWidget()->setAttribute( Qt::WA_DeleteOnClose, false );
	parentWidget()->move( 880, 490 );

	connect( m_dropDown, SIGNAL( activated(int) ),
			this, SLOT( grooveChanged(int) ) );

	connect( Engine::getSong(), SIGNAL( dataChanged() ),
			this, SLOT( update() ) );

	connect( Engine::getSong(), SIGNAL( projectLoaded() ),
			this, SLOT( update() ) );

	update();
}

GrooveView::~GrooveView()
{
	delete m_dropDown;
}

void GrooveView::update()
{
	Groove * groove = Engine::getSong()->globalGroove();
	if (groove->nodeName() == "none")
	{
		m_dropDown->setCurrentIndex(0);
	}
	if (groove->nodeName() == "hydrogen")
	{
		m_dropDown->setCurrentIndex(1);
	}
	if (groove->nodeName() == "midi")
	{
		m_dropDown->setCurrentIndex(2);
	}
	if (groove->nodeName() == "experiment")
	{
		m_dropDown->setCurrentIndex(3);
	}
	if (groove->nodeName() == "half")
	{
		m_dropDown->setCurrentIndex(3);
	}
	setView(groove);
}

void GrooveView::clear()
{
	
	QLayoutItem * li = m_layout->takeAt(1);
	delete li->widget();
	delete li;
	
	m_dropDown->setCurrentIndex(0);
	m_layout->addWidget( new QLabel("Select groove") );
	
}

void GrooveView::grooveChanged(int index)
{
	Groove * groove = NULL;

	int selectedIdx = m_dropDown->currentIndex();
	switch (selectedIdx) {
		case 0 : 
		{
			groove = new Groove();
			break;
		}
		case 1 :
		{
			groove = new HydrogenSwing();
			break;
		}
		case 2 :
		{
			groove = new MidiSwing();
			break;
		}
		case 3 :
		{
			groove = new GrooveExperiments();
			break;
		}
		case 4 :
		{
			groove = new HalfSwing();
			break;
		}
	}
	
	Song * song = Engine::getSong();
	song->setGlobalGroove(groove); // TODO this can fail
	setView(groove);
	
}

void GrooveView::setView(Groove * groove)
{
	
	QWidget * view = groove->instantiateView(this);
	QLayoutItem * li = m_layout->takeAt(1);
	delete li->widget();
	delete li;
	
	m_layout->addWidget( view );
}

