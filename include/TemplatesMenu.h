#ifndef TEMPLATESMENU_H
#define TEMPLATESMENU_H

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

private:
    int m_custom_templates_count;
};

#endif // TEMPLATESMENU_H
