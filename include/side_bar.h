/*
 * side_bar.h - code for side-bar in LMMS
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _SIDE_BAR_H
#define _SIDE_BAR_H

#include "qt3support.h"

#ifdef QT4

#include <QMap>

#else

#include <qmap.h>

#endif


#include "kmultitabbar.h"
#include "side_bar_widget.h"


class sideBar : public KMultiTabBar
{
	Q_OBJECT
public:
	sideBar( Qt::Orientation _o, QWidget * _parent ) :
		KMultiTabBar( _o, _parent )
	{
	}
	virtual ~sideBar()
	{
	}

	inline int appendTab( sideBarWidget * _sbw, int _id )
	{
		int ret = KMultiTabBar::appendTab( _sbw->icon(), _id,
								_sbw->title() );
		m_widgets[_id] = _sbw;
		_sbw->hide();
		_sbw->setMinimumWidth( 200 );
		connect( tab( _id ), SIGNAL( clicked( int ) ), this,
						SLOT( tabClicked( int ) ) );
		return( ret );
	}


private slots:
	inline void tabClicked( int _id )
	{
		// disable all other tabbar-buttons 
		QMap<int, QWidget *>::Iterator it;
		for( it = m_widgets.begin(); it != m_widgets.end(); ++it )
		{
			if( it.key() != _id/* && isTabRaised(it.key()) == TRUE*/ )
			{
				setTab( it.key(), FALSE );
			}
			if( m_widgets[it.key()] != NULL )
			{
				m_widgets[it.key()]->hide();
			}
		}
		if( m_widgets[_id] != NULL )
		{
			if( isTabRaised( _id ) )
			{
				m_widgets[_id]->show();
			}
			else
			{
				m_widgets[_id]->hide();
			}
		}
	}


private:
	QMap<int, QWidget *> m_widgets;

} ;


#endif
