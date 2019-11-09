/*
 * IntegerDisplayWidget.h - widget displaying numbers in modernized LCD style
 *
 * Copyright (c) 2019 Lathigos <lathigos/at/tutanota.com>
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
#include <QLabel>
#include <QVector>

#include "lmms_export.h"

class QHBoxLayout;
class QLabel;

class LMMS_EXPORT IntegerDisplayWidget : public QLabel
{
	Q_OBJECT
public:
	IntegerDisplayWidget( QWidget* parent, const QString& name = QString() );
	IntegerDisplayWidget( int numDigits,
					QWidget* parent,
					const QString& name = QString() );

	virtual ~IntegerDisplayWidget();

	void setValue( int value );
	inline int value() { return m_value; }

	void addTextForValue( int value, const QString& text )
	{
		m_textForValue[value] = text;
		update();
	}
	
	Q_PROPERTY( bool zeroesVisible READ zeroesVisible WRITE setZeroesVisible )
	inline bool zeroesVisible() const { return m_zeroesVisible; }
	inline void setZeroesVisible( bool b )
	{
		m_zeroesVisible = b;
		setValue( m_value );
		update();
	}
	
	Q_PROPERTY( bool forceSign READ forceSign WRITE setForceSign )
	inline bool forceSign() const { return m_forceSign; }
	inline void setForceSign( bool b )
	{
		m_forceSign = b;
		setValue( m_value );
		update();
	}

private:
	QMap<int, QString> m_textForValue;

	int m_numDigits;
	bool m_zeroesVisible = true;
	bool m_forceSign = false;
	int m_value = 0;

	void initUi( const QString& name ); //!< to be called by ctors

} ;

#endif
