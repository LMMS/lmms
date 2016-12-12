/*
 * chordtable_editor.h - dialog to display information about installed CHORDTABLE
 *					plugins
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _CHORDTABLE_EDITOR_H
#define _CHORDTABLE_EDITOR_H

#include "ToolPlugin.h"
#include "ToolPluginView.h"
#include "InstrumentFunctions.h"

class ComboBox;
class QScrollArea;
class QVBoxLayout;
class chordtableEditor;

class chordtableEditorView : public ToolPluginView
{
	Q_OBJECT
public:
	chordtableEditorView( ToolPlugin * _tool );
	virtual ~chordtableEditorView();


public slots:


private:

	chordtableEditor * m_chordTableEditor;

	QScrollArea * m_scrollArea;
	QVBoxLayout * m_scrollAreaLayout;


	//The combobox of the available chord combinations
	ComboBox * m_chordsComboBox;

} ;


class chordtableEditor : public ToolPlugin
{
public:
	chordtableEditor();
	virtual ~chordtableEditor();

	virtual PluginView * instantiateView( QWidget * )
	{
		return new chordtableEditorView( this );
	}

	virtual QString nodeName() const;

	virtual void saveSettings( QDomDocument& doc, QDomElement& element )
	{
		Q_UNUSED(doc)
		Q_UNUSED(element)
	}

	virtual void loadSettings( const QDomElement& element )
	{
		Q_UNUSED(element)
	}

	InstrumentFunctionNoteStacking::ChordTable * m_chordtable;
	ComboBoxModel *m_chordsModel;

} ;


#endif
