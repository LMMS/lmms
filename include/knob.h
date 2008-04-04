/*
 * knob.h - powerful knob-widget
 *
 * This file is based on the knob-widget of the Qwt Widget Library by
 * Josef Wilgen
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


#ifndef _KNOB_H
#define _KNOB_H

#include <QtGui/QWidget>
#include <QtCore/QPoint>

#include "automatable_model.h"
#include "templates.h"


class QPixmap;
class textFloat;


enum knobTypes
{
	knobDark_28, knobBright_26, knobSmall_17, knobGreen_17
} ;



class knob : public QWidget, public floatModelView
{
	Q_OBJECT
public:
	knob( int _knob_num, QWidget * _parent, const QString & _name );
	virtual ~knob();


	void setHintText( const QString & _txt_before,
						const QString & _txt_after );
	void setLabel( const QString & _txt );

	void setTotalAngle( float _angle );


public slots:
	void reset( void );
	void copyValue( void );
	void pasteValue( void );
	virtual void enterValue( void );
	void connectToMidiDevice( void );
	void connectToController( void );
	void displayHelp( void );


signals:
//	void valueChanged( float value );
//	void valueChanged( void );
	void sliderPressed( void );
	void sliderReleased( void );
	void sliderMoved( float value );


protected:
	static float s_copiedValue;
	static textFloat * s_textFloat;

	float m_mouseOffset;
	QPoint m_origMousePos;
	bool m_buttonPressed;

	QPixmap * m_knobPixmap;
	QString m_hintTextBeforeValue;
	QString m_hintTextAfterValue;

	virtual void contextMenuEvent( QContextMenuEvent * _me );
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void mouseDoubleClickEvent( QMouseEvent * _me );
	virtual void paintEvent( QPaintEvent * _me );
	virtual void wheelEvent( QWheelEvent * _me );

	void drawKnob( QPainter * _p );
	void setPosition( const QPoint & _p );

	float getValue( const QPoint & _p );


private:
	inline float pageSize( void ) const
	{
		return( tMax<float>( ( model()->maxValue() -
					model()->minValue() ) / 100.0f,
							model()->step() ) );
	}

	void valueChange( void );
	void buttonReleased( void );

	float m_totalAngle;

	int m_knobNum;
	QString m_label;

} ;


typedef knob::autoModel knobModel;

#endif
