/*
 * ToolButtonList.h - horizontal list of tightly packed toolbar buttons
 *
 * Copyright (c) 2019 Lathigos <lathigos/at/tutanota.com>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#ifndef TOOL_BUTTON_LIST_H
#define TOOL_BUTTON_LIST_H

#include <QWidget>
#include <QColor>
#include <QResizeEvent>

class QFrame;
class QHBoxLayout;
class QToolButton;

class ToolButtonList : public QWidget
{
	Q_OBJECT
public:
	ToolButtonList( const QString & customName, QWidget * _parent=nullptr);

	inline ToolButtonList(QWidget * _parent) :
		QWidget(_parent)
	{ }
	
	QToolButton * addToolButton( const QPixmap & _pixmap, 
					const QString & _tooltip,
					QObject * _receiver=nullptr,
					const char * _slot=nullptr );
	
	void addSeparator();

	virtual ~ToolButtonList() = default;

protected:
	void resizeEvent( QResizeEvent * event ) override;

signals:
	void resized( QResizeEvent * event );

private:
	QHBoxLayout * m_layout;
	
	bool startNewRegion = true;
	QToolButton * previousButton = nullptr;
	QFrame * m_currentFrame = nullptr;
	QHBoxLayout * m_currentFrameLayout = nullptr;
} ;

#endif

