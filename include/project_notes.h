/*
 * project_notes.h - header for project-notes-editor
 *
 * Copyright (c) 2005-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _PROJECT_NOTES_H
#define _PROJECT_NOTES_H

#include <QtGui/QMainWindow>

#include "JournallingObject.h"

class QAction;
class QComboBox;
class QTextCharFormat;
class QTextEdit;


class EXPORT projectNotes : public QMainWindow, public SerializingObject
{
	Q_OBJECT
public:
	projectNotes();
	virtual ~projectNotes();

	void clear();
	void setText( const QString & _text );

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	inline virtual QString nodeName() const
	{
		return "projectnotes";
	}


protected:
	void setupActions();


private slots:
	void textBold();
	void textUnderline();
	void textItalic();
	void textFamily( const QString & _f );
	void textSize( const QString & _p );
	void textColor();
	void textAlign( QAction * _a );

	void formatChanged( const QTextCharFormat & _f );
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
