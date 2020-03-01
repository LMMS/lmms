#ifndef RECENTPROJECTSMENU_H
#define RECENTPROJECTSMENU_H

#include <QMenu>

class RecentProjectsMenu : public QMenu
{
	Q_OBJECT
public:
	RecentProjectsMenu(QWidget *parent = nullptr);

private slots:
	void fillMenu();
	void openProject(QAction * _action );
};

#endif // RECENTPROJECTSMENU_H
