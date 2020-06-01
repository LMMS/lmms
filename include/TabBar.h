/*
 * TabBar.h - class tabBar
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef TAB_BAR_H
#define TAB_BAR_H

#include <QtCore/QMap>
#include <QLayout>
#include <QWidget>

#include "lmms_export.h"


class TabButton;


class LMMS_EXPORT TabBar : public QWidget
{
	Q_OBJECT
public:
	TabBar( QWidget * _parent,
			QBoxLayout::Direction _dir = QBoxLayout::LeftToRight );
	virtual ~TabBar() = default;

	TabButton * addTab( QWidget * _w, const QString & _text,
					int _id, bool _add_stretch = false,
					bool _text_is_tooltip = false );
	void removeTab( int _id );

	inline void setExclusive( bool _on )
	{
		m_exclusive = _on;
	}

	int activeTab();


public slots:
	void setActiveTab( int _id );


protected:
	bool tabState( int _id );
	void setTabState( int _id, bool _checked );
	bool allHidden();


protected slots:
	void hideAll( int _exception = -1 );
	void tabClicked( int _id );


private:
	QMap<int, QPair<TabButton *, QWidget *> > m_tabs;
	QBoxLayout * m_layout;
	bool m_exclusive;


signals:
	void allWidgetsHidden();
	void widgetShown();

} ;


#endif
