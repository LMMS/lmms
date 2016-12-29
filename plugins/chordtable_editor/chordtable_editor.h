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
#include <QFrame>

#include "ToolPlugin.h"
#include "ToolPluginView.h"
#include "InstrumentFunctions.h"

class ComboBox;
class QScrollArea;
class Knob;
class AutomatableSlider;
class LcdWidget;
class QVBoxLayout;
class QHBoxLayout;
class chordtableEditor;
class chordNoteModel;
class LedCheckBox;


/*****************************************************************************************************
 *
 * The chordNoteWidget class
 *
******************************************************************************************************/
class chordNoteWidget : public QWidget, public ModelView
{
	Q_OBJECT
public:
	chordNoteWidget(chordNoteModel * _model, QWidget *_parent);
	~chordNoteWidget(){}

	chordNoteModel * m_chordNoteModel;
	Knob *m_volumeKnob;
	Knob *m_panKnob;
	//
	QLabel *m_keyLabel;
	LcdWidget *m_keyLcd;
	AutomatableSlider *m_keySlider;
	LedCheckBox *m_activeLed;
	LedCheckBox *m_silencedLed;
	LedCheckBox *m_bareLed;

public slots:

	//changes m_keyLCD value
	void setKeyLabel(int i);

};


/*****************************************************************************************************
 *
 * The chordNoteModel class
 *
******************************************************************************************************/
class chordNoteModel : public Model
{
	Q_OBJECT
public:
	chordNoteModel(Model *_parent, ChordSemiTone *_semiTone);
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
	ChordSemiTone *m_semiTone;

public slots:
	//transfer model data to the m_semiTone
	void changeData();


} ;

//---------------------------------------------------



/*****************************************************************************************************
 *
 * The chordtableEditorView class
 *
******************************************************************************************************/
class chordtableEditorView : public ToolPluginView
{
	Q_OBJECT
public:
	chordtableEditorView( ToolPlugin * _tool );
	virtual ~chordtableEditorView();

public slots:
	//loads the chord into the widget
	void loadChord();

private:

	//the existing chordtable
	chordtableEditor * m_chordTableEditor;

	//the chosen chord
	Chord * m_Chord;

	QVector<chordNoteWidget *> m_chordnoteWidgets;


	QScrollArea * m_scrollArea;
	QVBoxLayout * m_scrollAreaLayout;

	//the widget layout where to put all the chordNoteWidgets
	QHBoxLayout *m_chordsWidgetLayout;
	QWidget *m_chordsWidget;


	//The combobox of the available chord combinations
	ComboBox * m_chordsComboBox;

} ;


/*****************************************************************************************************
 *
 * The chordtableEditor class
 *
******************************************************************************************************/
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

	ChordTable * m_chordTable;

	//single chord notes
	QList <chordNoteModel> * m_notes;

	ComboBoxModel * m_chordsModel;

} ;

//-----------------------------------------------------------------------


#endif
