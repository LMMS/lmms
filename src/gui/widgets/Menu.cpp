#include "Menu.h"

#include <QApplication>
#include <QKeyEvent>

void Menu::keyReleaseEvent(QKeyEvent *event)
{
  QKeyEvent e(*event);
  QApplication::sendEvent(parentWidget(),&e);
}
