/*
 * chordtable_editor.cpp - dialog to display information about installed CHORDTABLE
 *                      plugins
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


#include "chordtable_editor.h"


#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>

#include "Engine.h"
#include "gui_templates.h"
#include "Knob.h"
#include "TabBar.h"
#include "TabButton.h"
#include "ComboBox.h"
#include "AutomatableSlider.h"
#include "Model.h"
#include "volume.h"
#include "panning.h"
#include "ToolTip.h"
#include "LcdWidget.h"
#include "LedCheckbox.h"

#include "embed.cpp"

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT chordtableeditor_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"ChordTable Plugin Editor",
	QT_TRANSLATE_NOOP( "ChordTable Editor",
	"Edits the Chordtable" ),
	"Riki SLuga <jaz/at/rikis/dot/net>",
	0x0100,
	Plugin::Tool,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
} ;


// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model * _parent, void * _data )
{
	return new chordtableEditor;
}

}


/*****************************************************************************************************
 *
 * The chordtableEditor class
 *
******************************************************************************************************/

chordtableEditor::chordtableEditor() :
	ToolPlugin( &chordtableeditor_plugin_descriptor, NULL )
{
	m_chordTable = Engine::chordTable();
	m_chordsModel= new ComboBoxModel( this, tr( "Chord type" ) );
	for( int i = 0; i < m_chordTable->size(); ++i )
	{
		m_chordsModel->addItem( m_chordTable->at(i)->getName() );
	}

}


chordtableEditor::~chordtableEditor()
{
}


QString chordtableEditor::nodeName() const
{
	return chordtableeditor_plugin_descriptor.name;
}



/*****************************************************************************************************
 *
 * The chordtableEditorView class
 *
******************************************************************************************************/


chordtableEditorView::chordtableEditorView( ToolPlugin * _tool ) :
	ToolPluginView( _tool  ),
	m_chordTableEditor( castModel<chordtableEditor>() ),
	m_Chord(NULL),
	m_chordsComboBox( new ComboBox() )
{
	setWindowIcon( embed::getIconPixmap( "controller" ) ); //menjaj icono!!
	setWindowTitle( tr( "ChordTable Editor" ) );

	setAutoFillBackground( true );
	setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );


	//the top level all including dialog;
	QVBoxLayout* topLayout = new QVBoxLayout( this );
	topLayout->setMargin( 0 );
	setLayout(topLayout);

	//the upper top area of the dialog
	QWidget * upperWidget= new QWidget(this);
	QHBoxLayout *upperLayout = new QHBoxLayout(upperWidget);
	upperWidget->setLayout(upperLayout);

	QSizePolicy gp=m_chordsComboBox->sizePolicy();
	gp.setVerticalPolicy(QSizePolicy::Fixed);
	m_chordsComboBox->setMinimumSize(120,22);
	m_chordsComboBox->setSizePolicy(gp);

	//combobox data
	m_chordsComboBox->setModel(m_chordTableEditor->m_chordsModel);


	//Connecting the changing of data to the combobox
	connect(m_chordTableEditor->m_chordsModel,SIGNAL(dataChanged()),this,SLOT(loadChord()));

	QPushButton *button1 = new QPushButton(tr("Add SemiTone"));
	QPushButton *button2 = new QPushButton(tr("Delete chord"));
	QPushButton *button3 = new QPushButton(tr("Reset chords"));

	connect(button1, SIGNAL (clicked()), this, SLOT (addChordSemiTone()));
	connect(button3, SIGNAL (clicked()), this, SLOT (reset()));

	//adding combo and button
	upperLayout->addWidget(m_chordsComboBox);
	upperLayout->addStretch();
	upperLayout->addWidget(button3);
	upperLayout->addWidget(button2);
	upperLayout->addWidget(button1);

	//the lower area
	QWidget *lowerWidget=new QWidget(this);
	//	 lowerWidget->setMinimumSize(540,400);

	QHBoxLayout *lowerWidgetLayout=new QHBoxLayout(lowerWidget);
	lowerWidget->setLayout(lowerWidgetLayout);

	//the lower widget scroll area
	m_scrollArea = new QScrollArea(lowerWidget);
	m_scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
	m_scrollArea->setPalette( QApplication::palette( m_scrollArea ) );
	m_scrollArea->setFrameStyle( QFrame::NoFrame );
	lowerWidgetLayout->addWidget(m_scrollArea);

	//The widget inside the scroll area
	m_chordsWidget = new QWidget(m_scrollArea);
	m_scrollArea->setWidget(m_chordsWidget);

	m_chordsWidget->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);
	//	 m_chordsWidget->setStyleSheet("background-color:#f00;");
	//	 m_chordsWidget->setMinimumSize(20,20);
	//	 m_chordsWidgetLayout->setSizeConstraint(QLayout::SetMaximumSize);
	m_chordsWidgetLayout= new QHBoxLayout;

	m_chordsWidgetLayout->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
	m_chordsWidgetLayout->setSpacing(4);
	m_chordsWidgetLayout->setMargin(0);
	m_chordsWidgetLayout->setSizeConstraint(QLayout::SetFixedSize);


	m_chordsWidget->setLayout(m_chordsWidgetLayout);

	//the first node de prova!!
	loadChord();

	//setting the main layout
	topLayout->addWidget(upperWidget);
	//	 topLayout->addStretch();
	topLayout->addWidget(lowerWidget);

	//--------------



	setWhatsThis( tr(
									"This dialog displays information on all of the LADSPA plugins LMMS was "
									"able to locate. The plugins are divided into five categories based "
									"upon an interpretation of the port types and names.\n\n"
									) );


	hide();
	if( parentWidget() )
	{
		parentWidget()->hide();
		parentWidget()->layout()->setSizeConstraint(QLayout::SetDefaultConstraint );
		parentWidget()->setMinimumSize(580,430);
		
		Qt::WindowFlags flags = parentWidget()->windowFlags();
		//		flags |= Qt::MSWindowsFixedSizeDialogHint;
		flags &= ~Qt::WindowMaximizeButtonHint;
		parentWidget()->setWindowFlags( flags );
	}


}


void chordtableEditorView::loadChord()
{

	//taking selected value from the comboboxmodel
	int i= m_chordTableEditor->m_chordsModel->value();
	//getting the address of the selected chord
	m_Chord=m_chordTableEditor->m_chordTable->at(i);

	//connects changes to chord structure to reloading this view by recalling this function;
	connect(m_Chord,SIGNAL(emitStructureEdited()),this,SLOT(loadChord()));

	//Deleting all the widgets of the layout - Qt5 style

	m_chordsWidget->setUpdatesEnabled(false);
	QList<QWidget*> widgets= m_chordsWidget->findChildren<QWidget*>("chordNoteWidget", Qt::FindDirectChildrenOnly);
	foreach(QWidget * widget, widgets)
	{
		m_chordsWidgetLayout->removeWidget(widget);
		delete widget;
	}

//	qDeleteAll(m_chordsWidget->findChildren<QWidget*>("chordNoteWidget", Qt::FindDirectChildrenOnly));
	m_chordsWidget->setUpdatesEnabled(true);

	//adding the widgets from the menu

	ChordSemiTone *m_chordSemiTone;
	chordNoteModel *m_chordNoteModel;
	chordNoteWidget *m_chordNoteWidget;
	for (int i=0;i<m_Chord->getChordSemiTones()->size();i++)
	{
		m_chordSemiTone=m_Chord->getChordSemiTones()->at(i);
		m_chordNoteModel= new chordNoteModel(m_chordTableEditor, m_chordSemiTone,i);
		m_chordNoteWidget= new chordNoteWidget(m_chordNoteModel,m_chordsWidget);
		//Connects the nested delete pushbutton to the remove chordnote slot
		connect(m_chordNoteWidget,SIGNAL(emitDeletePosition(int)),this,SLOT(removeSemiTone(int)));
		//Connects the nested clone pushbutton to the clone the chordnote
		connect(m_chordNoteWidget,SIGNAL(emitClonePosition(int)),this,SLOT(cloneSemiTone(int)));
		//		lowerInsideLayout->addWidget(m_chordNoteWidget);
		m_chordsWidgetLayout->addWidget(m_chordNoteWidget);
	}
	m_chordsWidget->adjustSize();
}

void chordtableEditorView::reset()
{
	m_chordTableEditor->m_chordTable->reset();
	loadChord();
}

void chordtableEditorView::removeSemiTone(int i)
{
	m_Chord->removeSemiTone(i);
	loadChord();
}

void chordtableEditorView::addChordSemiTone()
{
	m_Chord->addSemiTone();
	loadChord();
}

void chordtableEditorView::cloneSemiTone(int i)
{
	ChordSemiTone *cst = new ChordSemiTone(m_Chord->at(i));
	m_Chord->insertSemiTone(cst,i);
	loadChord();
}

chordtableEditorView::~chordtableEditorView()
{
}


/*****************************************************************************************************
 *
 * The chordNoteModel class
 *
******************************************************************************************************/

chordNoteModel::chordNoteModel(Model *_parent, ChordSemiTone *_semiTone,int _position) :
	Model(_parent),
	m_semiTone(_semiTone),
	m_position(_position)
{
	//	connect(_semiTone->key,SIGNAL(dataChanged()),this,SLOT(changeData()));
	//	connect(_semiTone->vol,SIGNAL(dataChanged()),this,SLOT(changeData()));
}

//the position of the semitone in the semitones vector
int chordNoteModel::position() const
{
	return m_position;
}

void chordNoteModel::setPosition(int position)
{
	m_position = position;
}

//NON SEMBRA CAMBIARE I DATI DELLA CHORDTABLE


/*****************************************************************************************************
 *
 * The chordNoteWidget class
 *
******************************************************************************************************/

chordNoteWidget::chordNoteWidget(chordNoteModel * _model, QWidget *_parent) :
	QWidget(_parent),
	ModelView(_model,_parent),
	m_chordNoteModel( castModel<chordNoteModel>() )
{
	setObjectName(QStringLiteral("chordNoteWidget"));
	//the position in the vector
	m_position=m_chordNoteModel->position();
	setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding);

	QVBoxLayout *m_vLayout= new QVBoxLayout(this);
	m_vLayout->setSizeConstraint(QLayout::SetFixedSize);
	setLayout(m_vLayout);
//	setFixedSize(100,290);

	m_vLayout->setSizeConstraint(QLayout::SetMaximumSize);


	QFrame *frame=new QFrame;
	frame->setFrameStyle(QFrame::Panel | QFrame::Raised);
	frame->setLineWidth(2);
	//	frame->setMinimumWidth(100);
	//	frame->setMinimumHeight(280);

	m_vLayout->addWidget(frame);

	QGridLayout* gridLayout = new QGridLayout( frame );
	gridLayout->setAlignment(Qt::AlignHCenter);
	//	gridLayout->setContentsMargins( 8, 18, 8, 8 );
	//	gridLayout->setRowStretch( 4, 3);
	//	gridLayout->setColumnStretch(0,2);
	gridLayout->setHorizontalSpacing( 10 );
	gridLayout->setVerticalSpacing( 10 );

	frame->setLayout(gridLayout);

	//	setAutoFillBackground( true );
	setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );


	m_volumeKnob = new Knob( knobDark_28, this );
	m_volumeKnob->setLabel( tr( "Volume" ) );
	m_volumeKnob->setModel(m_chordNoteModel->m_semiTone->vol);
	//	m_volumeKnob->move( 27, 5 );
	m_volumeKnob->setEnabled( true );
	m_volumeKnob->setHintText( tr( "Volume knob:" ), "" );
	m_volumeKnob->setWhatsThis( tr( "The Wet/Dry knob sets the ratio between "
																	"the input signal and the effect signal that "
																	"forms the output." ) );

	m_panKnob = new Knob( knobDark_28, this );
	m_panKnob->setLabel( tr( "Panning" ) );
	m_panKnob->setModel(m_chordNoteModel->m_semiTone->pan);
	//	m_panKnob->move( 27, 5 );
	m_panKnob->setEnabled( true );
	m_panKnob->setHintText( tr( "Panning knob:" ), "" );
	m_panKnob->setWhatsThis( tr( "The Wet/Dry knob sets the ratio between "
															 "the input signal and the effect signal that "
															 "forms the output." ) );

	//----------------
	m_keySlider = new AutomatableSlider( this, tr( "Key note" ) );
	m_keySlider->setModel(m_chordNoteModel->m_semiTone->key );
	m_keySlider->setOrientation( Qt::Vertical );
	m_keySlider->setPageStep( 1 );
	m_keySlider->setTickPosition( QSlider::TicksLeft );
	m_keySlider->setFixedSize( 26, 60 );
	m_keySlider->setTickInterval( 50 );
	ToolTip::add( m_keySlider, tr( "Key note" ) );
	m_keySlider->setWhatsThis( tr("The key note"));

	m_keyLcd= new LcdWidget( 3, this );
	m_keyLcd->setValue( m_chordNoteModel->m_semiTone->key->value());
	connect( m_keySlider, SIGNAL( logicValueChanged( int ) ), this,	SLOT( setKeyLabel( int ) ) );

	m_activeLed= new LedCheckBox(this, tr("Active"));
	m_activeLed->setModel(m_chordNoteModel->m_semiTone->active);
	m_activeLed->setWhatsThis( tr("If the note is active or gets omitted"));
	m_activeLed->setEnabled(true);
	ToolTip::add( m_activeLed, tr( "Active note" ) );

	QLabel* m_activeLabel = new QLabel( tr( "Act.:" ) );
	m_activeLabel->setFont( pointSize<8>( m_activeLabel->font() ) );

	m_silencedLed= new LedCheckBox(this, tr("Silenced"));
	m_silencedLed->setModel(m_chordNoteModel->m_semiTone->silenced);
	m_silencedLed->setWhatsThis( tr("If the note is silenced"));
	m_silencedLed->setEnabled(true);
	ToolTip::add( m_silencedLed, tr( "Silenced note" ) );

	QLabel* m_silencedLabel = new QLabel( tr( "Sil.:" ) );
	m_silencedLabel->setFont( pointSize<8>( m_silencedLabel->font() ) );

	m_bareLed= new LedCheckBox(this, tr("Bare"));
	m_bareLed->setModel(m_chordNoteModel->m_semiTone->bare);
	m_bareLed->setWhatsThis( tr("If the arpeggio ignores the note volume or panning "));
	m_bareLed->setEnabled(true);
	ToolTip::add( m_bareLed, tr( "Bare note" ) );

	QLabel* m_bareLabel = new QLabel( tr( "Bar.:" ) );
	m_bareLabel->setFont( pointSize<8>( m_bareLabel->font() ) );

	//----------------


	gridLayout->addWidget(m_volumeKnob,0,0,1,2,Qt::AlignCenter);
	gridLayout->addWidget(m_panKnob,1,0,1,2,Qt::AlignCenter);
	gridLayout->addWidget(m_keyLcd,2,0,1,2,Qt::AlignCenter);
	gridLayout->addWidget(m_keySlider,3,0,1,2,Qt::AlignCenter);

	gridLayout->addWidget(m_activeLabel,4,0,1,1,Qt::AlignCenter);
	gridLayout->addWidget(m_activeLed,4,1,1,1,Qt::AlignCenter);

	gridLayout->addWidget(m_silencedLabel,5,0,1,1,Qt::AlignCenter);
	gridLayout->addWidget(m_silencedLed,5,1,1,1,Qt::AlignCenter);

	gridLayout->addWidget(m_bareLabel,6,0,1,1,Qt::AlignCenter);
	gridLayout->addWidget(m_bareLed,6,1,1,1,Qt::AlignCenter);

	//Connect it while instantiating this class!!
	m_cloneButton = new QPushButton(tr("Clone"));
	m_cloneButton->setWhatsThis( tr("Clones the SemiTone"));
	ToolTip::add( m_cloneButton, tr( "Clones the SemiTone" ) );
	//connects the pushBUtton to emitting position signal
	connect(m_cloneButton,SIGNAL(clicked()),this,SLOT(emitClonePosition()));

	gridLayout->addWidget(m_cloneButton,7,0,1,2,Qt::AlignCenter);

	//the first widget is the one
	m_delButton = new QPushButton(tr("Del"));
	m_delButton->setParent(this);

	if ( m_position == 0 )
	{
		m_delButton->setEnabled(false);
		m_delButton->setWhatsThis( tr("Delete disabled for base note"));
		ToolTip::add( m_delButton, tr( "Delete disabled for base note" ) );
	}
	else
	{
		m_delButton->setWhatsThis( tr("Deletes the SemiTone"));
		ToolTip::add( m_delButton, tr( "Deletes the SemiTone" ) );
		//connects the pushBUtton to emitting position signal
		connect(m_delButton,SIGNAL(clicked()),this,SLOT(emitDeletePosition()));
	}
	gridLayout->addWidget(m_delButton,8,0,1,2,Qt::AlignCenter);

}


void chordNoteWidget::setKeyLabel(int i)
{
	if (m_keyLcd)
	{
		m_keyLcd->setValue(i);
		m_keyLcd->update();
	}
}



int chordNoteWidget::position() const
{
	return m_position;
}

void chordNoteWidget::setPosition(int position)
{
	m_position = position;
}

