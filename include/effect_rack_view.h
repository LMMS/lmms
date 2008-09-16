/*
 * effect_rack_view.h - view for effectChain-model
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn@netscape.net>
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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

#ifndef _EFFECT_RACK_VIEW_H
#define _EFFECT_RACK_VIEW_H

#include <QtGui/QWidget>

#include "effect_chain.h"
#include "types.h"

class QScrollArea;
class QVBoxLayout;

class effectView;
class groupBox;


class effectRackView : public QWidget, public modelView
{
	Q_OBJECT
public:
	effectRackView( effectChain * _model, QWidget * _parent = NULL );
	virtual ~effectRackView();


public slots:
	void clearViews( void );
	void moveUp( effectView * _view );
	void moveDown( effectView * _view );
	void deletePlugin( effectView * _view );


private slots:
	virtual void update( void );
	void addEffect( void );


private:
	virtual void modelChanged( void );

	inline effectChain * fxChain( void )
	{
		return( castModel<effectChain>() );
	}

	inline const effectChain * fxChain( void ) const
	{
		return( castModel<effectChain>() );
	}


	QVector<effectView *> m_effectViews;

	QVBoxLayout * m_mainLayout;
	groupBox * m_effectsGroupBox;
	QScrollArea * m_scrollArea;

	Uint32 m_lastY;

} ;

#endif
