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
	void fillTemplatesMenu();
	void addTemplatesFromDir(const QDir& templateDir);

};

#endif // TEMPLATESMENU_H
