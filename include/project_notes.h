/*
 * project_notes.h - header for project-notes-editor
 *
 * Copyright (c) 2005-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _PROJECT_NOTES_H
#define _PROJECT_NOTES_H

#include "qt3support.h"

#ifdef QT4

#include <QMainWindow>

#else

#include <qmainwindow.h>
#define textColor color

#endif

#include "settings.h"
#include "engine.h"


class QAction;
class QComboBox;
class QTextEdit;


class projectNotes : public QMainWindow, public settings, public engineObject
{
	Q_OBJECT
public:
	projectNotes( engine * _engine );
	virtual ~projectNotes();

	void clear( void );

	virtual void FASTCALL saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );
	inline virtual QString nodeName( void ) const
	{
		return( "projectnotes" );
	}


protected:
	void setupActions( void );


private slots:
	void textBold( void );
	void textUnderline( void );
	void textItalic( void );
	void textFamily( const QString & _f );
	void textSize( const QString & _p );
	void textColor( void );
	void textAlign( QAction * _a );

	void fontChanged( const QFont & _f );
	void colorChanged( const QColor & _c );
	void alignmentChanged( int _a );


private:
	QTextEdit * m_edit;
	QAction * m_actionTextBold,
		* m_actionTextUnderline,
		* m_actionTextItalic,
		* m_actionTextColor,
		* m_actionAlignLeft,
		* m_actionAlignCenter,
		* m_actionAlignRight,
		* m_actionAlignJustify;
	QComboBox * m_comboFont;
	QComboBox * m_comboSize;

} ;


#endif
