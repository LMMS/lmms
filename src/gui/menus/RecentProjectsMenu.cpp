#include "RecentProjectsMenu.h"
#include "embed.h"
#include "ConfigManager.h"
#include <QFileInfo>
#include "Song.h"
#include "Engine.h"
#include "MainWindow.h"
#include "GuiApplication.h"

RecentProjectsMenu::RecentProjectsMenu(QWidget *parent) :
	QMenu(tr( "&Recently Opened Projects" ), parent)
{
	setIcon(embed::getIconPixmap( "project_open_recent" ));

	connect( this, SIGNAL( aboutToShow() ),
			 this, SLOT( updateRecentlyOpenedProjectsMenu() ) );
	connect( this, SIGNAL( triggered( QAction * ) ),
			 this, SLOT( openRecentlyOpenedProject( QAction * ) ) );
}




void RecentProjectsMenu::updateRecentlyOpenedProjectsMenu()
{
	clear();
	QStringList rup = ConfigManager::inst()->recentlyOpenedProjects();

	//	The file history goes 50 deep but we only show the 15
	//	most recent ones that we can open and omit .mpt files.
	int shownInMenu = 0;
	for( QStringList::iterator it = rup.begin(); it != rup.end(); ++it )
	{
		QFileInfo recentFile( *it );
		if ( recentFile.exists() &&
			 *it != ConfigManager::inst()->recoveryFile() )
		{
			if( recentFile.suffix().toLower() == "mpt" )
			{
				continue;
			}

			addAction(
				embed::getIconPixmap( "project_file" ), it->replace("&", "&&") );
#ifdef LMMS_BUILD_APPLE
			actions().last()->setIconVisibleInMenu(false); // QTBUG-44565 workaround
			actions().last()->setIconVisibleInMenu(true);
#endif
			shownInMenu++;
			if( shownInMenu >= 15 )
			{
				return;
			}
		}
	}
}




void RecentProjectsMenu::openRecentlyOpenedProject( QAction * _action )
{
	if ( gui->mainWindow()->mayChangeProject(true) )
	{
		const QString f = _action->text().replace("&&", "&");
		setCursor( Qt::WaitCursor );
		Engine::getSong()->loadProject( f );
		setCursor( Qt::ArrowCursor );
	}
}
