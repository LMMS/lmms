/*
 * effect_control_dialog.h - base-class for effect-dialogs for displaying and
 *                           editing control port values
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _EFFECT_CONTROL_DIALOG_H
#define _EFFECT_CONTROL_DIALOG_H

#ifdef QT4

#include <QtGui/QWidget>

#else

#include <qwidget.h>

#endif

#include "qt3support.h"

#include "journalling_object.h"
#include "effect.h"


class track;


class effectControlDialog : public QWidget, public journallingObject
{
	Q_OBJECT
	
public:
	effectControlDialog( QWidget * _parent, effect * _eff );
	virtual ~effectControlDialog();

	virtual ch_cnt_t getControlCount( void ) = 0;

signals:
	void closed();


protected:
	virtual void closeEvent( QCloseEvent * _ce );
	template<class T>
	T * getEffect( void )
	{
		return( dynamic_cast<T *>( m_effect ) );
	}


private:
	effect * m_effect;

} ;

#endif
