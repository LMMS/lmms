#include "RecentProjectsMenu.h"
#include "embed.h"

RecentProjectsMenu::RecentProjectsMenu(QWidget *parent) :
	QMenu(tr( "&Recently Opened Projects" ), parent)
{
	setIcon(embed::getIconPixmap( "project_open_recent" ));
}
