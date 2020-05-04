#ifndef TEMPLATESMENU_H
#define TEMPLATESMENU_H

#include <QDir>
#include <QMenu>

class TemplatesMenu : public QMenu
{
	Q_OBJECT
public:
	TemplatesMenu(QWidget *parent = nullptr);
	virtual ~TemplatesMenu() = default;

private slots:
	static void createNewProjectFromTemplate(QAction * _action);
	void fillTemplatesMenu();
	void addTemplatesFromDir( const QDir& dir );

};

#endif // TEMPLATESMENU_H
