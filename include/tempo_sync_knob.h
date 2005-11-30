/*
 * tempo_sync_knob.h - adds bpm to ms conversion for knob class
 *
 * Copyright (c) 2005 Danny McRae <khjklujn@yahoo.com>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _TEMPO_SYNC_KNOB_H
#define _TEMPO_SYNC_KNOB_H

#ifdef QT4

#include <QPixmap.h>

#else

#include <qpixmap.h>

#endif

#include "knob.h"


enum tempoSyncMode
{
	NO_SYNC,
	DOUBLE_WHOLE_NOTE,
	WHOLE_NOTE,
	HALF_NOTE,
	QUARTER_NOTE,
	EIGHTH_NOTE,
	SIXTEENTH_NOTE,
	THIRTYSECOND_NOTE
} ;

	
class tempoSyncKnob : public knob
{
	Q_OBJECT
public:
	tempoSyncKnob( int _knob_num, QWidget * _parent, const QString & _name, 
			float _scale = 1.0f );
	virtual ~tempoSyncKnob();

	tempoSyncMode getSyncMode( void );
	void setSyncMode( tempoSyncMode _new_mode );

	float getScale( void );
	void setScale( float _new_scale );

	const QString & getSyncDescription( void );
	void setSyncDescription( const QString & _new_description );

	const QPixmap & getSyncIcon( void );
	void setSyncIcon( const QPixmap & _new_pix );


signals:
	void syncModeChanged( tempoSyncMode _new_mode );
	void scaleChanged( float _new_scale );
	void syncDescriptionChanged( const QString & _new_description );
	void syncIconChanged( void );


public slots:
	void setTempoSync( int _note_type );


protected:
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void contextMenuEvent( QContextMenuEvent * _me );
	virtual void wheelEvent( QWheelEvent * _me );


protected slots:
	void calculateTempoSyncTime( int _bpm );


private:
	tempoSyncMode m_tempoSyncMode;
	float m_scale;
	QPixmap m_tempoSyncIcon;
	QString m_tempoSyncDescription;
	
	tempoSyncMode m_tempoLastSyncMode;

} ;


#endif
