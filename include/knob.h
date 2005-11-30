/*
 * knob.h - powerful knob-widget
 *
 * This file is based on the knob-widget of the Qwt Widget Library by
 * Josef Wilgen
 *
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QWidget>
#include <QPoint>

#else

#include <qwidget.h>
#include <qpoint.h>

#endif


class QPixmap;
class textFloat;


enum knobTypes
{
	knobDark_28, knobBright_26, knobSmall_17
} ;



class knob : public QWidget
{
	Q_OBJECT
public:
	knob( int _knob_num, QWidget * _parent, const QString & _name );
	virtual ~knob();

    
	void setHintText( const QString & _txt_before,
						const QString & _txt_after );
	void setLabel( const QString & _txt );

	void setTotalAngle( float _angle );

	void setRange( float _vmin, float _vmax, float _vstep = 0.0,
							int _pagesize = 1 );

	inline float value( void ) const
	{
		return( m_value );
	}

	void setStep( float );

	inline float maxValue( void ) const
	{
		return( m_maxValue );
	}
	inline float minValue( void ) const
	{
		return( m_minValue );
	}

	inline void incPages( int _n_pages )
	{
		setNewValue( m_value + float( _n_pages ) * m_pageSize, 1 );
	}



public slots:
	void setValue( float _val, bool _is_init_value = FALSE );
	void fitValue( float _val );
	void incValue( int _steps );
	void reset( void );
	void copyValue( void );
	void pasteValue( void );
	void enterValue( void );
	void connectToMidiDevice( void );
	void displayHelp( void );


signals:
	void valueChanged( float value );
	void sliderPressed( void );
	void sliderReleased( void );
	void sliderMoved( float value );
    

protected:
	virtual void paintEvent( QPaintEvent * _me );
	virtual void resizeEvent( QResizeEvent * _me );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void mouseDoubleClickEvent( QMouseEvent * _me );
	virtual void wheelEvent( QWheelEvent * _me );
	virtual void contextMenuEvent( QContextMenuEvent * _me );

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

	void setNewValue( float _x, bool _align = FALSE );


	static float s_copiedValue;
	static textFloat * s_textFloat;


	float m_mouseOffset;
	QPoint m_origMousePos;
	bool m_buttonPressed;


	float m_angle;
	float m_totalAngle;

	QPixmap * m_knobPixmap;
	int m_knobNum;
	QString m_hintTextBeforeValue;
	QString m_hintTextAfterValue;
	QString m_label;


	float m_minValue;
	float m_maxValue;
	float m_step;
	float m_pageSize;
	float m_value;
	float m_exactValue;
	float m_exactPrevValue;
	float m_prevValue;
	float m_initValue;

} ;

#endif
