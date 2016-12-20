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
class chordtableEditor;
class chordNoteModel;
class LedCheckBox;

//new constants for key notes in arpeggios, used by the Int Automated model
const int KeyMax = ( 0 + 20 );
const int KeyMin = - KeyMax;
const int KeyCenter = 0;
const int DefaultKey = 0;

//the widget for the single semitone
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

	FloatModel m_volumeModel;
	FloatModel m_panningModel;
	IntModel m_keyModel;
	BoolModel m_activeModel;
	BoolModel m_silencedModel;
	BoolModel m_bareModel;

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
