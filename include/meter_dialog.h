/*
 * meter_dialog.h - dialog for entering meter settings
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/yahoo.com>
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

#ifndef _METER_DIALOG_H
#define _METER_DIALOG_H

#include <QtGui/QWidget>

#include "lcd_spinbox.h"


class meterDialog : public QWidget
{
	Q_OBJECT
public:
	meterDialog( QWidget * _parent, track * _track );
	~meterDialog();

	void saveSettings( QDomDocument & _doc, QDomElement & _this,
						const QString & _name );
	void loadSettings( const QDomElement & _this,
						const QString & _name );

	inline int getNumerator( void )
	{
		return( m_numeratorModel->value() );
	}

	inline int getDenominator( void )
	{
		return( m_denominatorModel->value() );
	}


private:
	lcdSpinBox * m_numerator;
	lcdSpinBox * m_denominator;
	lcdSpinBoxModel * m_numeratorModel;
	lcdSpinBoxModel * m_denominatorModel;


signals:
	void numeratorChanged( void );
	void denominatorChanged( void );

} ;

#endif
