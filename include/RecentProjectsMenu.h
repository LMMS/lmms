#ifndef RECENTPROJECTSMENU_H
#define RECENTPROJECTSMENU_H

#include <QMenu>

class RecentProjectsMenu : public QMenu
{
	Q_OBJECT
public:
	RecentProjectsMenu(QWidget *parent = nullptr);

private slots:
	void updateRecentlyOpenedProjectsMenu();
};

#endif // RECENTPROJECTSMENU_H
