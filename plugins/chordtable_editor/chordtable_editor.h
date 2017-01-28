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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#ifndef CHORDTABLE_EDITOR_H
#define CHORDTABLE_EDITOR_H

#include <QLabel>
#include <QFrame>
#include <QGridLayout>
#include <QLineEdit>

#include "ToolPlugin.h"
#include "ToolPluginView.h"
#include "InstrumentFunctions.h"

class ChordSemiTone;
class Chord;
class ChordTable;
class QVBoxLayout;
class QHBoxLayout;
class QPushButton;
class QScrollArea;
class chordtableEditor;
class ComboBox;
class ComboBoxModel;
class AutomatableSlider;
class Knob;
class LcdWidget;
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
	chordNoteWidget( chordNoteModel * _model, QWidget * _parent );
	~chordNoteWidget();

	QVBoxLayout * m_vLayout;
	QFrame * m_Frame;
	QGridLayout * m_gridLayout;

	chordNoteModel * m_chordNoteModel;
	Knob * m_volumeKnob;
	Knob * m_panKnob;

	LcdWidget * m_keyLcd;
	AutomatableSlider * m_keySlider;
	LedCheckBox * m_activeLed;
	LedCheckBox * m_silencedLed;
	LedCheckBox * m_randomLed;
	LedCheckBox * m_bareLed;

	QPushButton * m_delButton;
	QPushButton * m_cloneButton;


	//the position of the semitone in the vector
	int position() const;
	void setPosition( int position );

public slots:

	//changes m_keyLCD value
	void setKeyLabel( int i );

	//emits the position of the semitone in the vector
	void emitDeletePosition()
	{
		emit emitDeletePosition( m_position );
	}

	//emits the position of the semitone in the vector
	void emitClonePosition()
	{
		emit emitClonePosition( m_position );
	}

private:

	//the position of the semitone in the vector
	int m_position;

signals:

	//emits the positions of the semitone in the vector when edited/pushed etc..
	void emitDeletePosition( int i );
	void emitClonePosition( int i );

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
	chordNoteModel( Model * _parent, ChordSemiTone * _semiTone, int _position );
	~chordNoteModel(){}

	virtual chordNoteWidget * instantiateView( QWidget * _parent )
	{
		return new chordNoteWidget( this, _parent );
	}

	//Don't need this
	virtual void saveSettings( QDomDocument & doc, QDomElement & element )
	{
		Q_UNUSED( doc )
		Q_UNUSED( element )
	}

	virtual void loadSettings( const QDomElement & element )
	{
		Q_UNUSED( element )
	}


	//The chordsemitone it's referring to
	ChordSemiTone * m_semiTone;

	//the position of the semitone in the semitones vector
	int position() const;
	void setPosition( int position );

private:

	//the position in the vector if there's one, -1 if not present
	int m_position;

} ;

//---------------------------------------------------



/*****************************************************************************************************
 *
 * The chordtableEditorView class
 *
******************************************************************************************************/
class ChordTableEditorView : public ToolPluginView
{
	Q_OBJECT
public:
	ChordTableEditorView( ToolPlugin * _tool );
	virtual ~ChordTableEditorView();

	//displays a confirmation dialogue
	int confirmDialog( QString _title, QString _text );

public slots:
	//loads the chord into the widget
	void loadChord();
	//reloads the combo box
	void reloadCombo();
	//loads the preset chordtable
	void resetChords();
	//removes the semitone which sent the signal
	void removeSemiTone( int i );
	void addChordSemiTone();
	//clones the semitone which sent the signal
	void cloneSemiTone( int i );
	void saveFile();
	void openFile();
	void newChord();
	void cloneChord();
	void removeChord();
	//when the text of the chord Name changes
	void changeText( QString _text );

	//sets the combobox to the selected chord
	void setChordSelection( int i );


private:

	chordtableEditor * m_chordTableEditor;
	//the existing chordtable
	ChordTable * m_chordTable;

	//the chosen chord
	Chord * m_chord;

	//the chord name
	QLineEdit * m_nameLineEdit;


	QVector<chordNoteWidget *> m_chordnoteWidgets;


	QScrollArea * m_scrollArea;
	QVBoxLayout * m_scrollAreaLayout;

	//the widget layout where to put all the chordNoteWidgets
	QHBoxLayout * m_chordsWidgetLayout;
	QWidget * m_chordsWidget;

	//The combobox of the available chord combinations
	ComboBox * m_chordsComboBox;

signals:
	void lineEditChange();

} ;


/*****************************************************************************************************
 *
 * The chordtableEditor class
 *
******************************************************************************************************/
class chordtableEditor : public ToolPlugin
{
	Q_OBJECT

public:
	chordtableEditor();
	virtual ~chordtableEditor();

	virtual PluginView * instantiateView( QWidget * )
	{
		return new ChordTableEditorView( this );
	}

	virtual QString nodeName() const;

	virtual void saveSettings( QDomDocument & doc, QDomElement & element )
	{
		Q_UNUSED( doc )
		Q_UNUSED( element )
	}

	virtual void loadSettings( const QDomElement& element )
	{
		Q_UNUSED( element )
	}

	ChordTable * m_chordTable;
	ComboBoxModel * m_chordsComboModel;

public slots:

	//reloads data from the chordTable;
	void reloadComboModel();

} ;


#endif
