/*
 * tempo_sync_knob.h - adds bpm to ms conversion for knob class
 *
 * Copyright (c) 2005-2008 Danny McRae <khjklujn/at/yahoo.com>
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

#include <QtGui/QPixmap>

#include "knob.h"
#include "meter_model.h"


class QAction;
class meterDialog;



class tempoSyncKnobModel : public knobModel
{
	Q_OBJECT
public:
	enum tempoSyncMode
	{
		SyncNone,
		SyncDoubleWholeNote,
		SyncWholeNote,
		SyncHalfNote,
		SyncQuarterNote,
		SyncEighthNote,
		SyncSixteenthNote,
		SyncThirtysecondNote,
		SyncCustom
	} ;


	tempoSyncKnobModel( const float _val, const float _min,
				const float _max, const float _step,
				const float _scale, ::model * _parent,
				const QString & _display_name = QString::null );
	virtual ~tempoSyncKnobModel();

	virtual void saveSettings( QDomDocument & _doc,
						QDomElement & _this,
						const QString & _name );
	virtual void loadSettings( const QDomElement & _this,
						const QString & _name );

	tempoSyncMode getSyncMode( void );
	void setSyncMode( tempoSyncMode _new_mode );

	float getScale( void );
	void setScale( float _new_scale );

signals:
	void syncModeChanged( tempoSyncMode _new_mode );
	void scaleChanged( float _new_scale );


public slots:
	inline void disableSync( void )
	{
		setTempoSync( SyncNone );
	}
	void setTempoSync( int _note_type );
	void setTempoSync( QAction * _item );


protected slots:
	void calculateTempoSyncTime( bpm_t _bpm );
	void updateCustom( void );


private:
	tempoSyncMode m_tempoSyncMode;
	tempoSyncMode m_tempoLastSyncMode;
	float m_scale;

	meterModel m_custom;


	friend class tempoSyncKnob;

} ;



class tempoSyncKnob : public knob
{
	Q_OBJECT
public:
	tempoSyncKnob( int _knob_num, QWidget * _parent,
						const QString & _name = QString::null );
	virtual ~tempoSyncKnob();

	const QString & getSyncDescription( void );
	void setSyncDescription( const QString & _new_description );

	const QPixmap & getSyncIcon( void );
	void setSyncIcon( const QPixmap & _new_pix );

	tempoSyncKnobModel * model( void )
	{
		return( castModel<tempoSyncKnobModel>() );
	}

	virtual void modelChanged( void );


signals:
	void syncDescriptionChanged( const QString & _new_description );
	void syncIconChanged( void );


protected:
	virtual void contextMenuEvent( QContextMenuEvent * _me );


protected slots:
	void updateDescAndIcon( void );
	void showCustom( void );


private:
	QPixmap m_tempoSyncIcon;
	QString m_tempoSyncDescription;

	meterDialog * m_custom;

} ;



#endif
