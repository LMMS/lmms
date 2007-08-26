#ifndef SINGLE_SOURCE_COMPILE

/*
 * project_notes.cpp - implementation of project-notes-editor
 *
 * Copyright (c) 2005-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <Qt/QtXml>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QColorDialog>
#include <QtGui/QComboBox>
#include <QtGui/QFontDatabase>
#include <QtGui/QLineEdit>
#include <QtGui/QTextCursor>
#include <QtGui/QTextEdit>
#include <QtGui/QToolBar>

#include "project_notes.h"
#include "embed.h"
#include "engine.h"
#include "main_window.h"
#include "song_editor.h"



projectNotes::projectNotes( void ) :
	QMainWindow( engine::getMainWindow()->workspace() )
{
	engine::getMainWindow()->workspace()->addWindow( this );

	m_edit = new QTextEdit( this );
	m_edit->setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setColor( m_edit->backgroundRole(), QColor( 64, 64, 64 ) );
	m_edit->setPalette( pal );

	clear();

	connect( m_edit, SIGNAL( currentFontChanged( const QFont & ) ),
			this, SLOT( fontChanged( const QFont & ) ) );
	connect( m_edit, SIGNAL( currentColorChanged( const QColor & ) ),
			this, SLOT( colorChanged( const QColor & ) ) );
	connect( m_edit, SIGNAL( currentAlignmentChanged( int ) ),
			this, SLOT( alignmentChanged( int ) ) );
	connect( m_edit, SIGNAL( textChanged() ),
			engine::getSongEditor(), SLOT( setModified() ) );

	setupActions();

	setCentralWidget( m_edit );
	setWindowTitle( tr( "Project notes" ) );
	setWindowIcon( embed::getIconPixmap( "project_notes" ) );

	resize( 300, 200 );

	QWidget * w = ( parentWidget() != NULL ) ? parentWidget() : this;
	if( engine::getMainWindow()->workspace() != NULL )
	{
		w->move( 700, 10 );
	}
	else
	{
		w->move( 800, 10 );
	}

	//show();

}




projectNotes::~projectNotes()
{
}




void projectNotes::clear( void )
{
	m_edit->setHtml( tr( "Put down your project notes here." ) );
	m_edit->selectAll();
	m_edit->setTextColor( QColor( 224, 224, 224 ) );
	m_edit->setTextCursor( QTextCursor() );
}




void projectNotes::setText( const QString & _text )
{
	m_edit->setHtml( _text );
}




void projectNotes::setupActions()
{
	QToolBar * tb = new QToolBar( tr( "Edit Actions" ), this );
	QAction * a;

	a = new QAction( embed::getIconPixmap( "edit_undo" ), tr( "&Undo" ),
									tb );
	a->setShortcut( tr( "Ctrl+Z" ) );
	connect( a, SIGNAL( activated() ), m_edit, SLOT( undo() ) );

	a = new QAction( embed::getIconPixmap( "edit_redo" ), tr( "&Redo" ),
									tb );
	a->setShortcut( tr( "Ctrl+Y" ) );
	connect( a, SIGNAL( activated() ), m_edit, SLOT( redo() ) );

	a = new QAction( embed::getIconPixmap( "edit_copy" ), tr( "&Copy" ),
									tb );
	a->setShortcut( tr( "Ctrl+C" ) );
	connect( a, SIGNAL( activated() ), m_edit, SLOT( copy() ) );

	a = new QAction( embed::getIconPixmap( "edit_cut" ), tr( "Cu&t" ),
									tb );
	a->setShortcut( tr( "Ctrl+X" ) );
	connect( a, SIGNAL( activated() ), m_edit, SLOT( cut() ) );

	a = new QAction( embed::getIconPixmap( "edit_paste" ), tr( "&Paste" ),
									tb );
	a->setShortcut( tr( "Ctrl+V" ) );
	connect( a, SIGNAL( activated() ), m_edit, SLOT( paste() ) );


	tb = new QToolBar( tr( "Format Actions" ), this );

	m_comboFont = new QComboBox( tb );
	m_comboFont->setEditable( TRUE );
	QFontDatabase db;
	m_comboFont->addItems( db.families() );
	connect( m_comboFont, SIGNAL( activated( const QString & ) ),
			m_edit, SLOT( setFontFamily( const QString & ) ) );
	m_comboFont->lineEdit()->setText( QApplication::font().family() );

	m_comboSize = new QComboBox( tb );
	m_comboSize->setEditable( TRUE );
	QList<int> sizes = db.standardSizes();
	QList<int>::Iterator it = sizes.begin();
	for ( ; it != sizes.end(); ++it )
	{
		m_comboSize->addItem( QString::number( *it ) );
	}
	connect( m_comboSize, SIGNAL( activated( const QString & ) ),
		     this, SLOT( textSize( const QString & ) ) );
	m_comboSize->lineEdit()->setText( QString::number(
					QApplication::font().pointSize() ) );

	m_actionTextBold = new QAction( embed::getIconPixmap( "text_bold" ),
					tr( "&Bold" ), tb );
	m_actionTextBold->setShortcut( tr( "Ctrl+B" ) );
	connect( m_actionTextBold, SIGNAL( activated() ), this,
							SLOT( textBold() ) );

	m_actionTextItalic = new QAction( embed::getIconPixmap( "text_italic" ),
						tr( "&Italic" ), tb );
	m_actionTextItalic->setShortcut( tr( "Ctrl+I" ) );
	connect( m_actionTextItalic, SIGNAL( activated() ), this,
							SLOT( textItalic() ) );

	m_actionTextUnderline = new QAction( embed::getIconPixmap(
								"text_under" ),
						tr( "&Underline" ), tb );
	m_actionTextUnderline->setShortcut( tr( "Ctrl+U" ) );
	connect( m_actionTextUnderline, SIGNAL( activated() ), this,
						SLOT( textUnderline() ) );


	QActionGroup * grp = new QActionGroup( tb );
	connect( grp, SIGNAL( selected( QAction* ) ), this,
						SLOT( textAlign( QAction* ) ) );

	m_actionAlignLeft = new QAction( embed::getIconPixmap( "text_left" ),
						tr( "&Left" ), grp );
	m_actionAlignLeft->setShortcut( tr( "Ctrl+L" ) );

	m_actionAlignCenter = new QAction( embed::getIconPixmap(
								"text_center" ),
						tr( "C&enter" ), grp );
	m_actionAlignCenter->setShortcut( tr( "Ctrl+E" ) );

	m_actionAlignRight = new QAction( embed::getIconPixmap( "text_right" ),
						tr( "&Right" ), grp );
	m_actionAlignRight->setShortcut( tr( "Ctrl+R" ) );

	m_actionAlignJustify = new QAction( embed::getIconPixmap(
								"text_block" ),
						tr( "&Justify" ), grp );
	m_actionAlignJustify->setShortcut( tr( "Ctrl+J" ) );


	QPixmap pix( 16, 16 );
	pix.fill( Qt::black );
	m_actionTextColor = new QAction( pix, tr( "&Color..." ), tb );
	connect( m_actionTextColor, SIGNAL( activated() ), this,
							SLOT( textColor() ) );

}




void projectNotes::textBold()
{
	m_edit->setFontWeight( m_actionTextBold->isChecked() );
	engine::getSongEditor()->setModified();
}




void projectNotes::textUnderline()
{
	m_edit->setFontUnderline( m_actionTextUnderline->isChecked() );
	engine::getSongEditor()->setModified();
}




void projectNotes::textItalic()
{
	m_edit->setFontItalic( m_actionTextItalic->isChecked() );
	engine::getSongEditor()->setModified();
}




void projectNotes::textFamily( const QString & _f )
{
	m_edit->setFontFamily( _f );
	m_edit->viewport()->setFocus();
	engine::getSongEditor()->setModified();
}




void projectNotes::textSize( const QString & _p )
{
	m_edit->setFontPointSize( _p.toInt() );
	m_edit->viewport()->setFocus();
	engine::getSongEditor()->setModified();
}




void projectNotes::textColor()
{
	QColor col = QColorDialog::getColor( m_edit->textColor(), this );
	if ( !col.isValid() )
	{
		return;
	}
	m_edit->setTextColor( col );
	QPixmap pix( 16, 16 );
	pix.fill( Qt::black );
	m_actionTextColor->setIcon( pix );
}




void projectNotes::textAlign( QAction * _a )
{
	if( _a == m_actionAlignLeft )
	{
		m_edit->setAlignment( Qt::AlignLeft );
	}
	else if( _a == m_actionAlignCenter )
	{
		m_edit->setAlignment( Qt::AlignHCenter );
	}
	else if( _a == m_actionAlignRight )
	{
		m_edit->setAlignment( Qt::AlignRight );
	}
	else if( _a == m_actionAlignJustify )
	{
		m_edit->setAlignment( Qt::AlignJustify );
	}
}




void projectNotes::fontChanged( const QFont & _f )
{
	m_comboFont->lineEdit()->setText( _f.family() );
	m_comboSize->lineEdit()->setText( QString::number( _f.pointSize() ) );
	m_actionTextBold->setChecked( _f.bold() );
	m_actionTextItalic->setChecked( _f.italic() );
	m_actionTextUnderline->setChecked( _f.underline() );
	engine::getSongEditor()->setModified();
}




void projectNotes::colorChanged( const QColor & _c )
{
	QPixmap pix( 16, 16 );
	pix.fill( _c );
	m_actionTextColor->setIcon( pix );
	engine::getSongEditor()->setModified();
}




void projectNotes::alignmentChanged( int _a )
{
	if ( _a & Qt::AlignLeft )
	{
		m_actionAlignLeft->setChecked( TRUE );
	}
	else if ( ( _a & Qt::AlignHCenter ) )
	{
		m_actionAlignCenter->setChecked( TRUE );
	}
	else if ( ( _a & Qt::AlignRight ) )
	{
		m_actionAlignRight->setChecked( TRUE );
	}
	else if ( ( _a & Qt::AlignJustify ) )
	{
		m_actionAlignJustify->setChecked( TRUE );
	}
	engine::getSongEditor()->setModified();
}




void projectNotes::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	mainWindow::saveWidgetState( this, _this );

	QDomCDATASection ds = _doc.createCDATASection( m_edit->toHtml() );
	_this.appendChild( ds );
}




void projectNotes::loadSettings( const QDomElement & _this )
{
	mainWindow::restoreWidgetState( this, _this );
	m_edit->setHtml( _this.text() );
}


#include "project_notes.moc"



#undef isChecked
#undef setChecked
#undef setFontWeight
#undef setFontUnderline
#undef setFontItalic
#undef setFontFamily
#undef setFontPointSize
#undef setTextColor
#undef setHtml
#undef toHtml


#endif
