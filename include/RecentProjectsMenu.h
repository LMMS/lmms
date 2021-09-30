#ifndef RECENTPROJECTSMENU_H
#define RECENTPROJECTSMENU_H

#include <QMenu>

namespace lmms::gui
{


class RecentProjectsMenu : public QMenu
{
	Q_OBJECT
public:
	RecentProjectsMenu(QWidget *parent = nullptr);

private slots:
	void fillMenu();
	void openProject(QAction * _action );
};


} // namespace lmms::gui

#endif // RECENTPROJECTSMENU_H
