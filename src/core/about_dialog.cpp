#ifndef SINGLE_SOURCE_COMPILE

/*
 * about_dialog.cpp - implementation of about-dialog
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QResizeEvent>
#include <QtGui/QTabWidget>
#include <QtGui/QTextEdit>


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "about_dialog.h"
#include "embed.h"




aboutDialog::aboutDialog() :
	QDialog()
{
	setWindowTitle( tr( "About LMMS..." ) );

	m_iconLbl = new QLabel( this );
	m_iconLbl->setPixmap( embed::getIconPixmap( "icon" ) );
	m_iconLbl->setGeometry( 10, 10, 64, 64 );

	m_appNameLbl = new QLabel( tr( "Linux MultiMedia Studio %1"
						).arg( VERSION ), this );
	m_appNameLbl->setGeometry( 80, 30, 280, 20 );

	m_aboutTabs = new QTabWidget( this );

	QLabel * about_lbl = new QLabel( tr( "LMMS - A powerful "
						"synthesizer-studio\n\n"
						"Copyright (c) 2004-2008 "
						"LMMS-Developers\n\n"
						"http://lmms.sourceforge.net" )
					);
	about_lbl->setAlignment( Qt::AlignVCenter | Qt::AlignLeft );
	about_lbl->setIndent( 30 );


	QTextEdit * authors_lbl = new QTextEdit();
	authors_lbl->setPlainText( embed::getText( "AUTHORS" ) );
	authors_lbl->setReadOnly( TRUE );
	authors_lbl->setLineWrapMode( QTextEdit::NoWrap );

	QTextEdit * translation_lbl = new QTextEdit();
	translation_lbl->setPlainText( tr( "Current language not translated."
						"\n\nIf you're interested in "
						"translating LMMS in another "
						"language or want to improve "
						"existing translations, you're "
						"welcome to help us! Just "
						"contact the maintainer!" ) );
	translation_lbl->setReadOnly( TRUE );


	QTextEdit * copying_lbl = new QTextEdit();
	copying_lbl->setPlainText( embed::getText( "COPYING" ) );
	copying_lbl->setReadOnly( TRUE );
	copying_lbl->setLineWrapMode( QTextEdit::NoWrap );


	m_aboutTabs->addTab( about_lbl, tr( "About" ) );
	m_aboutTabs->addTab( authors_lbl, tr( "Authors" ) );
	m_aboutTabs->addTab( translation_lbl, tr( "Translation" ) );
	m_aboutTabs->addTab( copying_lbl, tr( "License" ) );

	m_okBtn = new QPushButton( tr( "Close" ), this );
	connect( m_okBtn, SIGNAL( clicked() ), this, SLOT( accept() ) );

	resize( 400, 390 );
}




aboutDialog::~aboutDialog()
{
}




void aboutDialog::keyPressEvent( QKeyEvent * _ke )
{
	if( _ke->key() == Qt::Key_Escape )
	{
		accept();
	}
}




void aboutDialog::resizeEvent( QResizeEvent * _re )
{
	m_aboutTabs->setGeometry( 10, 90, _re->size().width() - 20,
						_re->size().height() - 140 );
	m_okBtn->setGeometry( _re->size().width() - 110,
				_re->size().height() - 40, 100, 30 );
}




#include "about_dialog.moc"


#endif
