/*
 * RenameDialog.cpp - implementation of dialog for renaming something
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


#include <QKeyEvent>
#include <QLineEdit>

#include "RenameDialog.h"



RenameDialog::RenameDialog( QString & _string ) :
	QDialog(),
	m_stringToEdit( _string ),
	m_originalString( _string )
{
	setWindowTitle( tr("Rename...") );
	setFixedHeight( 30 );
	m_stringLE = new QLineEdit( this );
	m_stringLE->setText( _string );
	m_stringLE->setGeometry ( 10, 5, 220, 20 );
	m_stringLE->selectAll();
	connect( m_stringLE, SIGNAL( textChanged( const QString & ) ), this,
				SLOT( textChanged( const QString & ) ) );
	connect( m_stringLE, SIGNAL( returnPressed() ), this,
							SLOT( accept() ) );
}




RenameDialog::~RenameDialog()
{
}




void RenameDialog::resizeEvent (QResizeEvent * event) {
	m_stringLE->setGeometry ( 10, 5, width() - 20, 20 );	
}




void RenameDialog::keyPressEvent( QKeyEvent * _ke )
{
	if( _ke->key() == Qt::Key_Escape )
	{
		m_stringLE->setText( m_originalString );
		accept();
	}
}




void RenameDialog::textChanged( const QString & _new_string )
{
	m_stringToEdit = _new_string;
}
