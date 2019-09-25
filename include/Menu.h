
#ifndef MENU_H
#define MENU_H

#include <QMenu>
#include <QWidget>
#include <QString>

class Menu: public QMenu
{
public:
  Menu(const QString &title, QWidget *parent = nullptr):
    QMenu(title,parent) {};
  Menu(QWidget *parent = nullptr): QMenu(parent) {};

  void keyReleaseEvent(QKeyEvent *event) override;
};

#endif
