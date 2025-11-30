/*
 * LedCheckBox.h - class LedCheckBox, an improved QCheckBox
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_GUI_LED_CHECKBOX_H
#define LMMS_GUI_LED_CHECKBOX_H

#include "AutomatableButton.h"




namespace lmms::gui
{

class LMMS_EXPORT LedCheckBox : public AutomatableButton
{
	Q_OBJECT
public:
	enum class LedColor
	{
		Yellow,
		Green,
		Red
	} ;

	LedCheckBox( const QString & _txt, QWidget * _parent,
				const QString & _name = QString(),
						LedColor _color = LedColor::Yellow,
						bool legacyMode = true);
	LedCheckBox( QWidget * _parent,
				const QString & _name = QString(),
						LedColor _color = LedColor::Yellow,
						bool legacyMode = true);

	inline const QString & text()
	{
		return( m_text );
	}

	void setText( const QString& s );

	Q_PROPERTY( QString text READ text WRITE setText )

protected:
	void paintEvent( QPaintEvent * _pe ) override;


private:
	QPixmap m_ledOnPixmap;
	QPixmap m_ledOffPixmap;

	QString m_text;

	bool m_legacyMode;
	
	void initUi( LedColor _color ); //!< to be called by ctors

	void onTextUpdated(); //!< to be called when you updated @a m_text
	void paintLegacy(QPaintEvent * p);
	void paintNonLegacy(QPaintEvent * p);

} ;


} // namespace lmms::gui

#endif // LMMS_GUI_LED_CHECKBOX_H
