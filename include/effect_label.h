/*
 * effect_label.h - a label which is renamable by double-clicking it and
 *                  offers access to an effect rack
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/users.sourceforge.net>
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

#ifndef _EFFECT_LABEL_H
#define _EFFECT_LABEL_H

#include <QtGui/QWidget>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>

#include "journalling_object.h"


class effectTabWidget;
class sampleTrack;
class tabWidget;


class effectLabel: public QWidget, public journallingObject
{
	Q_OBJECT
public:
	effectLabel( const QString & _initial_name, QWidget * _parent,
							sampleTrack * _track );
	virtual ~effectLabel();

	QString text( void ) const;
	void FASTCALL setText( const QString & _text );
	
	virtual void FASTCALL saveSettings( QDomDocument & _doc, 
						QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );
	inline virtual QString nodeName( void ) const
	{
		return( "sample_track" );
	}
	
public slots:
	void showEffects( void );
	void closeEffects( void );
	void rename( void );
	
signals:
	void clicked( void );
	void nameChanged( void );
	void nameChanged( const QString & _new_name );
	void pixmapChanged( void );

protected:
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseDoubleClickEvent( QMouseEvent * _me );

private:
	sampleTrack * m_track;
	bool m_show;
	
	QLabel * m_label;
	QPushButton * m_effectBtn;
	tabWidget * m_tabWidget;
	effectTabWidget * m_effWidget;
};

#endif
