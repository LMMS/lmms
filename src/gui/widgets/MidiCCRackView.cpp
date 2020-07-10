#include <Qt> // For Qt::WindowFlags
#include <QWidget>
#include <QMdiSubWindow>

#include "MidiCCRackView.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "embed.h"


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

	subWin->setAttribute( Qt::WA_DeleteOnClose, false );
	subWin->move( 780, 50 );
	subWin->resize( 350, 200 );
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
