/*
 * live_tool.h - declaration of class liveTool, for live performance
 *
 * Copyright (c) 2006-2008 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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


#ifndef _LIVE_TOOL_H
#define _LIVE_TOOL_H


#include "tool.h"


class liveToolView : public toolView
{
public:
	liveToolView( tool * _tool );
	virtual ~liveToolView();

virtual bool eventFilter ( QObject * watched, QEvent * event );
protected:
	virtual void keyPressEvent( QKeyEvent * _ke );
	virtual void mousePressEvent( QMouseEvent * _me );

private:
	void toggleInstrument( int _n );

} ;




class liveTool : public tool
{
public:
	liveTool( model * _parent );
	virtual ~liveTool();

	virtual QString nodeName( void ) const;
	virtual pluginView * instantiateView( QWidget * )
	{
		return( new liveToolView( this ) );
	}

} ;




#endif
