/*
 * TrackRenameLineEdit.cpp - implementation of class TrackRenameLineEdit, which
 * 							 represents the text field that appears when one
 *							 double-clicks a track's label to rename it
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2017 Alexandre Almeida <http://m374lx.users.sourceforge.net/>
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


#include "TrackRenameLineEdit.h"

#include <QKeyEvent>



TrackRenameLineEdit::TrackRenameLineEdit( QWidget * parent ) :
	QLineEdit( parent )
{
}




void TrackRenameLineEdit::show()
{
	m_oldName = text();
	QLineEdit::show();
}




void TrackRenameLineEdit::keyPressEvent( QKeyEvent * ke )
{
	if( ke->key() == Qt::Key_Escape ) 
	{
		setText( m_oldName );
		hide();
	}
	
	QLineEdit::keyPressEvent( ke );
}
