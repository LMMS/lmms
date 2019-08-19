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
	void createNewProjectFromTemplate( QAction * _idx );
	void fillTemplatesMenu();
	int addTemplatesFromDir( const QDir& dir );

private:
	int m_customTemplatesCount;
};

#endif // TEMPLATESMENU_H
