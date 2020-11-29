#include "RecentProjectsMenu.h"

#include <QFileInfo>

#include "ConfigManager.h"
#include "Engine.h"
#include "Song.h"

#include "embed.h"
#include "GuiApplication.h"
#include "MainWindow.h"

RecentProjectsMenu::RecentProjectsMenu(QWidget *parent) :
	QMenu(tr( "&Recently Opened Projects" ), parent)
{
	setIcon(embed::getIconPixmap( "project_open_recent" ));

	connect( this, SIGNAL( aboutToShow() ),
			 this, SLOT(fillMenu() ) );
	connect( this, SIGNAL( triggered( QAction * ) ),
			 this, SLOT(openProject(QAction * ) ) );
}




void RecentProjectsMenu::fillMenu()
{
	clear();
	QStringList rup = ConfigManager::inst()->recentlyOpenedProjects();

	auto projectFileIcon = embed::getIconPixmap( "project_file" );

	//	The file history goes 50 deep but we only show the 15
	//	most recent ones that we can open and omit .mpt files.
	int shownInMenu = 0;
	for(QString& fileName : rup)
	{
		QFileInfo recentFile(fileName);
		if (!recentFile.exists() ||
			fileName == ConfigManager::inst()->recoveryFile() )
		{
			continue;
		}

		if( recentFile.suffix().toLower() == "mpt" )
		{
			continue;
		}

		addAction(projectFileIcon, fileName.replace("&", "&&") );
#ifdef LMMS_BUILD_APPLE
		actions().last()->setIconVisibleInMenu(false); // QTBUG-44565 workaround
		actions().last()->setIconVisibleInMenu(true);
#endif

		shownInMenu++;
		if( shownInMenu >= 15 )
		{
			break;
		}
	}
}




void RecentProjectsMenu::openProject(QAction * _action )
{
	auto mainWindow = gui->mainWindow();
	if (mainWindow->mayChangeProject(true))
	{
		const QString f = _action->text().replace("&&", "&");
		mainWindow->setCursor( Qt::WaitCursor );
		Engine::getSong()->loadProject( f );
		mainWindow->setCursor( Qt::ArrowCursor );
	}
}
