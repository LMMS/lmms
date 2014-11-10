/*
 * led_checkbox.h - class ledCheckBox, an improved QCheckBox
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _LED_CHECKBOX_H
#define _LED_CHECKBOX_H

#include "automatable_button.h"


class QPixmap;


class EXPORT ledCheckBox : public automatableButton
{
	Q_OBJECT
public:
	enum LedColors
	{
		Yellow,
		Green,
		Red,
		NumColors
	} ;

	ledCheckBox( const QString & _txt, QWidget * _parent,
				const QString & _name = QString::null,
						LedColors _color = Yellow );
	ledCheckBox( QWidget * _parent,
				const QString & _name = QString::null,
						LedColors _color = Yellow );

	virtual ~ledCheckBox();


	inline const QString & text()
	{
		return( m_text );
	}

	void setText( const QString& s );

	Q_PROPERTY( QString text READ text WRITE setText )

protected:
	virtual void paintEvent( QPaintEvent * _pe );


private:
	QPixmap * m_ledOnPixmap;
	QPixmap * m_ledOffPixmap;
	
	QString m_text;

	void initUi( LedColors _color ); //!< to be called by ctors
	void onTextUpdated(); //!< to be called when you updated @a m_text

} ;

#endif
