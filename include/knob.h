/*
 * knob.h - powerful knob-widget
 *
 * This file is based on the knob-widget of the Qwt Widget Library by
 * Josef Wilgen
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _KNOB_H
#define _KNOB_H

#include "qt3support.h"

#ifdef QT4

#include <QtGui/QWidget>
#include <QtCore/QPoint>

#else

#include <qwidget.h>
#include <qpoint.h>

#endif


#include "engine.h"
#include "automatable_object.h"


class QPixmap;
class textFloat;


enum knobTypes
{
	knobDark_28, knobBright_26, knobSmall_17, knobGreen_17
} ;



class knob : public QWidget, public automatableObject<float>
{
	Q_OBJECT
public:
	knob( int _knob_num, QWidget * _parent, const QString & _name,
					engine * _engine, track * _track );
	virtual ~knob();

    
	void setHintText( const QString & _txt_before,
						const QString & _txt_after );
	void setLabel( const QString & _txt );

	void setTotalAngle( float _angle );

	inline virtual void setInitValue( const float _val )
	{
		m_initValue = _val;
		autoObj::setInitValue( _val );
	}

	virtual void setValue( const float _x );

	virtual void setRange( const float _min, const float _max,
						const float _step = 0.0 );


public slots:
	void reset( void );
	void copyValue( void );
	void pasteValue( void );
	virtual void enterValue( void );
	void connectToMidiDevice( void );
	void displayHelp( void );


signals:
	void valueChanged( float value );
	void valueChanged( const QVariant & _data );
	void sliderPressed( void );
	void sliderReleased( void );
	void sliderMoved( float value );


protected:
	virtual void contextMenuEvent( QContextMenuEvent * _me );
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void mouseDoubleClickEvent( QMouseEvent * _me );
	virtual void paintEvent( QPaintEvent * _me );
	virtual void resizeEvent( QResizeEvent * _me );
	virtual void wheelEvent( QWheelEvent * _me );

	void drawKnob( QPainter * _p );
	void setPosition( const QPoint & _p );


// TODO: Need to figure out what is really used by tempoSyncKnob
// to get the private/protected attributes sorted out.  Right
// now, just make everything protected.
//private:
	void layoutKnob( bool _update = TRUE );
	float getValue( const QPoint & _p );
	void recalcAngle( void );
    
	void valueChange( void );
	void rangeChange( void );


	void buttonReleased( void );



	static float s_copiedValue;
	static textFloat * s_textFloat;


	float m_mouseOffset;
	QPoint m_origMousePos;
	bool m_buttonPressed;


	float m_pageSize;
	float m_angle;
	float m_totalAngle;


	QPixmap * m_knobPixmap;
	int m_knobNum;
	QString m_hintTextBeforeValue;
	QString m_hintTextAfterValue;
	QString m_label;


	float m_initValue;

} ;

#endif
