/*
 * LcdWidget.h - a widget for displaying numbers in LCD style
 *
 * Copyright (c) 2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef LCD_WIDGET_H
#define LCD_WIDGET_H

#include <QtCore/QMap>
#include <QWidget>

#include "lmms_export.h"

class LMMS_EXPORT LcdWidget : public QWidget
{
	Q_OBJECT
	
	// theming qproperties
	Q_PROPERTY( QColor textColor READ textColor WRITE setTextColor )
	Q_PROPERTY( QColor textShadowColor READ textShadowColor WRITE setTextShadowColor )
	
public:
	LcdWidget( QWidget* parent, const QString& name = QString() );
	LcdWidget( int numDigits, QWidget* parent, const QString& name = QString() );
	LcdWidget( int numDigits, const QString& style, QWidget* parent, const QString& name = QString() );

	virtual ~LcdWidget();

	void setValue( int value );
	void setLabel( const QString& label );

	void addTextForValue( int value, const QString& text )
	{
		m_textForValue[value] = text;
		update();
	}

	Q_PROPERTY( int numDigits READ numDigits WRITE setNumDigits )

	inline int numDigits() const { return m_numDigits; }
	inline void setNumDigits( int n ) { m_numDigits = n; updateSize(); }
	
	QColor textColor() const;
	void setTextColor( const QColor & c );
	
	QColor textShadowColor() const;
	void setTextShadowColor( const QColor & c );

public slots:
	virtual void setMarginWidth( int width );


protected:
	void paintEvent( QPaintEvent * pe ) override;

	virtual void updateSize();

	int cellHeight() const
	{
		return m_cellHeight;
	}


private:

	static const int charsPerPixmap = 12;

	QMap<int, QString> m_textForValue;

	QString m_display;

	QString m_label;
	QPixmap* m_lcdPixmap;

	QColor m_textColor;
	QColor m_textShadowColor;

	int m_cellWidth;
	int m_cellHeight;
	int m_numDigits;
	int m_marginWidth;

	void initUi( const QString& name, const QString &style = QString("19green") ); //!< to be called by ctors

} ;

#endif
