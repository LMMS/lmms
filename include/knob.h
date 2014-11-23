/*
 * knob.h - powerful knob-widget
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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


#ifndef KNOB_H
#define KNOB_H

#include <QtGui/QWidget>
#include <QtCore/QPoint>

#include "AutomatableModelView.h"
#include "templates.h"


class QPixmap;
class textFloat;

enum knobTypes
{
	knobDark_28, knobBright_26, knobSmall_17, knobGreen_17, knobVintage_32, knobStyled
} ;



class EXPORT knob : public QWidget, public FloatModelView
{
	Q_OBJECT
	Q_ENUMS( knobTypes )

	Q_PROPERTY(float innerRadius READ innerRadius WRITE setInnerRadius)
	Q_PROPERTY(float outerRadius READ outerRadius WRITE setOuterRadius)

	Q_PROPERTY(float centerPointX READ centerPointX WRITE setCenterPointX)
	Q_PROPERTY(float centerPointY READ centerPointY WRITE setCenterPointY)

	Q_PROPERTY(float lineWidth READ lineWidth WRITE setLineWidth)

	// Unfortunately, the gradient syntax doesn't create our gradient
	// correctly so we need to do this:
	Q_PROPERTY(QColor outerColor READ outerColor WRITE setOuterColor)
	Q_PROPERTY(QColor lineColor READ lineColor WRITE setlineColor)
	Q_PROPERTY(QColor arcColor READ arcColor WRITE setarcColor)
	mapPropertyFromModel(bool,isVolumeKnob,setVolumeKnob,m_volumeKnob);
	mapPropertyFromModel(float,volumeRatio,setVolumeRatio,m_volumeRatio);

	Q_PROPERTY(knobTypes knobNum READ knobNum WRITE setknobNum)

	void initUi( const QString & _name ); //!< to be called by ctors
	void onKnobNumUpdated(); //!< to be called when you updated @a m_knobNum

public:
	knob( knobTypes _knob_num, QWidget * _parent = NULL, const QString & _name = QString() );
	knob( QWidget * _parent = NULL, const QString & _name = QString() ); //!< default ctor
	virtual ~knob();

	// TODO: remove
	inline void setHintText( const QString & _txt_before,
						const QString & _txt_after )
	{
		setDescription( _txt_before );
		setUnit( _txt_after );
	}
	void setLabel( const QString & _txt );

	void setTotalAngle( float _angle );

	// Begin styled knob accessors
	float innerRadius() const;
	void setInnerRadius( float _r );

	float outerRadius() const;
	void setOuterRadius( float _r );

	knobTypes knobNum() const;
	void setknobNum( knobTypes _k );

	QPointF centerPoint() const;
	float centerPointX() const;
	void setCenterPointX( float _c );
	float centerPointY() const;
	void setCenterPointY( float _c );

	float lineWidth() const;
	void setLineWidth( float _w );

	QColor outerColor() const;
	void setOuterColor( const QColor & _c );
	QColor lineColor() const;
	void setlineColor( const QColor & _c );
	QColor arcColor() const;
	void setarcColor( const QColor & _c );


signals:
	void sliderPressed();
	void sliderReleased();
	void sliderMoved( float value );


protected:
	virtual void contextMenuEvent( QContextMenuEvent * _me );
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );
	virtual void focusOutEvent( QFocusEvent * _fe );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void mouseDoubleClickEvent( QMouseEvent * _me );
	virtual void paintEvent( QPaintEvent * _me );
	virtual void wheelEvent( QWheelEvent * _me );

	virtual float getValue( const QPoint & _p );

private slots:
	virtual void enterValue();
	void displayHelp();
	void friendlyUpdate();
	void toggleScale();

private:
	QString displayValue() const;

	virtual void doConnections();

	QLineF calculateLine( const QPointF & _mid, float _radius,
						float _innerRadius = 1) const;

	void drawKnob( QPainter * _p );
	void setPosition( const QPoint & _p );
	bool updateAngle();

	int angleFromValue( float value, float minValue, float maxValue, float totalAngle ) const
	{
		return static_cast<int>( ( value - 0.5 * ( minValue + maxValue ) ) / ( maxValue - minValue ) * m_totalAngle ) % 360;
	}

	inline float pageSize() const
	{
		return ( model()->maxValue() - model()->minValue() ) / 100.0f;
	}


	static textFloat * s_textFloat;

	QString m_label;

	QPixmap * m_knobPixmap;
	BoolModel m_volumeKnob;
	FloatModel m_volumeRatio;

	QPoint m_mouseOffset;
	QPoint m_origMousePos;
	float m_leftOver;
	bool m_buttonPressed;

	float m_totalAngle;
	int m_angle;
	QImage m_cache;

	// Styled knob stuff, could break out
	QPointF m_centerPoint;
	float m_innerRadius;
	float m_outerRadius;
	float m_lineWidth;
	QColor m_outerColor;
	QColor m_lineColor; //!< unused yet
	QColor m_arcColor; //!< unused yet

	knobTypes m_knobNum;

} ;

#endif
