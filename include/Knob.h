/*
 * Knob.h - powerful knob-widget
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
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

#ifndef LMMS_GUI_KNOB_H
#define LMMS_GUI_KNOB_H

#include <memory>
#include <QTextDocument>

#include "FloatModelEditorBase.h"


class QPixmap;

namespace lmms::gui
{


class SimpleTextFloat;

enum class KnobType
{
	Dark28, Bright26, Small17, Vintage32, Styled
} ;


void convertPixmapToGrayScale(QPixmap &pixMap);

class LMMS_EXPORT Knob : public FloatModelEditorBase
{
	Q_OBJECT
	Q_ENUMS( KnobType )

	Q_PROPERTY(float innerRadius READ innerRadius WRITE setInnerRadius)
	Q_PROPERTY(float outerRadius READ outerRadius WRITE setOuterRadius)

	Q_PROPERTY(float centerPointX READ centerPointX WRITE setCenterPointX)
	Q_PROPERTY(float centerPointY READ centerPointY WRITE setCenterPointY)

	Q_PROPERTY(float lineWidth READ lineWidth WRITE setLineWidth)

	// Unfortunately, the gradient syntax doesn't create our gradient
	// correctly so we need to do this:
	Q_PROPERTY(QColor outerColor READ outerColor WRITE setOuterColor)

	Q_PROPERTY(QColor lineActiveColor MEMBER m_lineActiveColor)
	Q_PROPERTY(QColor lineInactiveColor MEMBER m_lineInactiveColor)
	Q_PROPERTY(QColor arcActiveColor MEMBER m_arcActiveColor)
	Q_PROPERTY(QColor arcInactiveColor MEMBER m_arcInactiveColor)

	Q_PROPERTY(KnobType knobNum READ knobNum WRITE setknobNum)
	
	Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor)

	void initUi( const QString & _name ); //!< to be called by ctors
	void onKnobNumUpdated(); //!< to be called when you updated @a m_knobNum

public:
	Knob( KnobType _knob_num, QWidget * _parent = nullptr, const QString & _name = QString() );
	Knob( QWidget * _parent = nullptr, const QString & _name = QString() ); //!< default ctor
	Knob( const Knob& other ) = delete;

	void setLabel( const QString & txt );
	void setHtmlLabel( const QString &htmltxt );

	void setTotalAngle( float angle );

	// Begin styled knob accessors
	float innerRadius() const;
	void setInnerRadius( float r );

	float outerRadius() const;
	void setOuterRadius( float r );

	KnobType knobNum() const;
	void setknobNum( KnobType k );

	QPointF centerPoint() const;
	float centerPointX() const;
	void setCenterPointX( float c );
	float centerPointY() const;
	void setCenterPointY( float c );

	float lineWidth() const;
	void setLineWidth( float w );

	QColor outerColor() const;
	void setOuterColor( const QColor & c );
	
	QColor textColor() const;
	void setTextColor( const QColor & c );


protected:
	void paintEvent( QPaintEvent * _me ) override;

	void changeEvent(QEvent * ev) override;

private:
	QLineF calculateLine( const QPointF & _mid, float _radius,
						float _innerRadius = 1) const;

	void drawKnob( QPainter * _p );
	bool updateAngle();

	int angleFromValue( float value, float minValue, float maxValue, float totalAngle ) const
	{
		return static_cast<int>( ( value - 0.5 * ( minValue + maxValue ) ) / ( maxValue - minValue ) * m_totalAngle ) % 360;
	}

	QString m_label;
	bool m_isHtmlLabel;
	QTextDocument* m_tdRenderer;

	std::unique_ptr<QPixmap> m_knobPixmap;

	float m_totalAngle;
	int m_angle;
	QImage m_cache;

	// Styled knob stuff, could break out
	QPointF m_centerPoint;
	float m_innerRadius;
	float m_outerRadius;
	float m_lineWidth;
	QColor m_outerColor;

	QColor m_lineActiveColor;
	QColor m_lineInactiveColor;
	QColor m_arcActiveColor;
	QColor m_arcInactiveColor;
	
	QColor m_textColor;

	KnobType m_knobNum;
};

} // namespace lmms::gui

#endif // LMMS_GUI_KNOB_H
