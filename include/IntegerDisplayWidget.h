/*
 * IntegerDisplayWidget.h - widget displaying numbers in modernized LCD style
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


#ifndef INTEGER_DISPLAY_WIDGET_H
#define INTEGER_DISPLAY_WIDGET_H

#include <QtCore/QMap>
#include <QWidget>
#include <QVector>

#include "lmms_export.h"

class QHBoxLayout;
class QLabel;

class LMMS_EXPORT IntegerDisplayWidget : public QWidget
{
	Q_OBJECT
public:
	IntegerDisplayWidget( QWidget* parent, const QString& name = QString() );
	IntegerDisplayWidget( int numDigits, QWidget* parent, const QString& name = QString() );
	IntegerDisplayWidget( int numDigits, const QString& style, QWidget* parent, const QString& name = QString() );

	virtual ~IntegerDisplayWidget();

	void setValue( int value );
	//void setLabel( const QString& label );

	void addTextForValue( int value, const QString& text )
	{
		m_textForValue[value] = text;
		update();
	}

	Q_PROPERTY( int numDigits READ numDigits WRITE setNumDigits )

	inline int numDigits() const { return m_numDigits; }
	inline void setNumDigits( int n ) { m_numDigits = n; update(); }


private:
	QMap<int, QString> m_textForValue;

	QString m_display;
	
	QVector<QLabel *> m_digitsList;

	int m_numDigits;

	void initUi( const QString& name, const QString &style = QString("19green") ); //!< to be called by ctors

} ;

#endif
