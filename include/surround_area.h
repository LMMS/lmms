/*
 * surround_area.h - class surroundArea which provides widget for setting
 *                   position of a channel + calculation of volume for each
 *                   speaker
 *
 * Copyright (c) 2004-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _SURROUND_AREA_H
#define _SURROUND_AREA_H

#include <QtGui/QWidget>

#include "automatable_model.h"
#include "mixer.h"


class QPixmap;
class knob;
class track;


const int SURROUND_AREA_SIZE = 1024;


class surroundAreaModel : public model
{
	Q_OBJECT
	mapPropertyFromModel(int,x,setX,m_posX);
	mapPropertyFromModel(int,y,setY,m_posY);
public:
	surroundAreaModel( ::model * _parent, track * _track = NULL,
					bool _default_constructed = FALSE );

	volumeVector getVolumeVector( float _v_scale ) const;

	void saveSettings( QDomDocument & _doc, QDomElement & _this,
					const QString & _name = "surpos" );
	void loadSettings( const QDomElement & _this,
					const QString & _name = "surpos" );

	inline void prepareJournalEntryFromOldVal( void )
	{
		m_posX.prepareJournalEntryFromOldVal();
		m_posY.prepareJournalEntryFromOldVal();
	}

	inline void addJournalEntryFromOldToCurVal( void )
	{
		m_posX.addJournalEntryFromOldToCurVal();
		m_posY.addJournalEntryFromOldToCurVal();
	}

	automationPattern * automationPatternX( void )
	{
		return( m_posX.getAutomationPattern() );
	}

	automationPattern * automationPatternY( void )
	{
		return( m_posY.getAutomationPattern() );
	}


private:
	intModel m_posX;
	intModel m_posY;

} ;



class surroundArea : public QWidget, public modelView
{
	Q_OBJECT
public:
	surroundArea( QWidget * _parent, const QString & _name );
	virtual ~surroundArea();


	surroundAreaModel * model( void )
	{
		return( castModel<surroundAreaModel>() );
	}

	const surroundAreaModel * model( void ) const
	{
		return( castModel<surroundAreaModel>() );
	}


protected:
	virtual void contextMenuEvent( QContextMenuEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );


private:
	static QPixmap * s_backgroundArtwork;

} ;


#endif

