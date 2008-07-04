/*
 * effect_label.h - a label which is renamable by double-clicking it and
 *                  offers access to an effect rack
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/users.sourceforge.net>
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

#ifndef _EFFECT_LABEL_H
#define _EFFECT_LABEL_H

#include <QtGui/QWidget>


class QLabel;
class QPushButton;

class effectRackView;
class sampleTrack;


class effectLabel: public QWidget
{
	Q_OBJECT
public:
	effectLabel( QWidget * _parent, sampleTrack * _track );
	virtual ~effectLabel();

	QString text( void ) const;
	void setText( const QString & _text );


public slots:
	void showEffects( void );
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

	QLabel * m_label;
	QPushButton * m_effectBtn;
	QWidget * m_effWindow;
	effectRackView * m_effectRack;

} ;

#endif
