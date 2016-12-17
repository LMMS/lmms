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

#include <QLabel>

#include "ToolPlugin.h"
#include "ToolPluginView.h"
#include "InstrumentFunctions.h"

class ComboBox;
class QScrollArea;
class Knob;
class AutomatableSlider;
class LcdWidget;
class QVBoxLayout;
class chordtableEditor;
class chordNoteModel;


//the widget for the single semitone
class chordNoteWidget : public QWidget, public ModelView
{
	Q_OBJECT
public:
	chordNoteWidget(chordNoteModel * _model, QWidget *_parent);
	~chordNoteWidget(){}

	Knob *m_volumeKnob;
	Knob *m_panKnob;
	//
	QLabel *m_keyLabel;
	AutomatableSlider *m_keySlider;
	LcdWidget *m_activeLcd;
	LcdWidget *m_silencedLcd;
	LcdWidget *m_bareLcd;

};


//the model behind the single chord semitone
class chordNoteModel : public Model
{
	Q_OBJECT
public:
	chordNoteModel(Model *_parent, InstrumentFunctionNoteStacking::ChordSemiTone *_semiTone);
	~chordNoteModel(){}

	virtual chordNoteWidget * instantiateView( QWidget * _parent)
	{
		return new chordNoteWidget( this,  _parent);
	}

	virtual void saveSettings( QDomDocument& doc, QDomElement& element )
	{
		Q_UNUSED(doc)
		Q_UNUSED(element)
	}

	virtual void loadSettings( const QDomElement& element )
	{
		Q_UNUSED(element)
	}


	//The chordsemitone it's referring to
	InstrumentFunctionNoteStacking::ChordSemiTone *m_semiTone;

	//Single note data
//	int8_t m_key; // the semitone
//	volume_t m_vol; // its volume : in percentage of the volume, from 0% (silence)
//	// to 200% or more, to emboss volume. No control for high volumes yet;
//	panning_t m_pan; // the panning from -100 to +100

//	bool m_active; // the note is played -> true: yes, false: skipped
//	bool m_silenced; // the note is processed but silenced -> true: normally
//	// played, false: muted
//	bool m_bare; // The note only has the key value. True: bare, we will
	// discard al the rest, taking data from the base note
	// false: the note has all data, volume, panning, silenced...

} ;

//---------------------------------------------------



class chordtableEditorView : public ToolPluginView
{
	Q_OBJECT
public:
	chordtableEditorView( ToolPlugin * _tool );
	virtual ~chordtableEditorView();


public slots:


private:

	chordtableEditor * m_chordTableEditor;

	QVector<chordNoteWidget *> m_chordnoteWidgets;


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

	InstrumentFunctionNoteStacking::ChordTable * m_chordTable;
	InstrumentFunctionNoteStacking::Chord * m_chord;

	//single chord notes
	QList <chordNoteModel> * m_notes;

	ComboBoxModel * m_chordsModel;

} ;

//-----------------------------------------------------------------------


#endif
