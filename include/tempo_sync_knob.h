/*
 * tempo_sync_knob.h - adds bpm to ms conversion for knob class
 *
 * Copyright (c) 2005-2007 Danny McRae <khjklujn/at/yahoo.com>
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


#ifndef _TEMPO_SYNC_KNOB_H
#define _TEMPO_SYNC_KNOB_H

#ifndef QT3

#include <QtGui/QPixmap>

#else

#include <qpixmap.h>

#endif

#include "knob.h"


class QAction;
class meterDialog;


class tempoSyncKnob : public knob
{
	Q_OBJECT
public:
	enum tempoSyncMode
	{
		NO_SYNC,
		DOUBLE_WHOLE_NOTE,
		WHOLE_NOTE,
		HALF_NOTE,
		QUARTER_NOTE,
		EIGHTH_NOTE,
		SIXTEENTH_NOTE,
		THIRTYSECOND_NOTE,
		CUSTOM
	} ;


	tempoSyncKnob( int _knob_num, QWidget * _parent, const QString & _name,
					track * _track, float _scale = 1.0f );
	virtual ~tempoSyncKnob();

	virtual void FASTCALL saveSettings( QDomDocument & _doc,
						QDomElement & _this,
						const QString & _name );
	virtual void FASTCALL loadSettings( const QDomElement & _this,
						const QString & _name );

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
	void setTempoSync( QAction * _item );


protected:
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void contextMenuEvent( QContextMenuEvent * _me );
	virtual void wheelEvent( QWheelEvent * _me );


protected slots:
	void calculateTempoSyncTime( bpm_t _bpm );
	void updateCustom( int );
	void showCustom( void );

private:
	tempoSyncMode m_tempoSyncMode;
	float m_scale;
	QPixmap m_tempoSyncIcon;
	QString m_tempoSyncDescription;
	
	tempoSyncMode m_tempoLastSyncMode;
	QPointer<meterDialog> m_custom;

} ;


#endif
