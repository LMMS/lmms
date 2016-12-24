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


//---------------------------------------------
//---------------------------------------------
//---------------------------------------------

chordtableEditor::chordtableEditor() :
	ToolPlugin( &chordtableeditor_plugin_descriptor, NULL ),
	m_chordTable(&InstrumentFunctionNoteStacking::ChordTable::getInstance())
{
	m_chordsModel= new ComboBoxModel( this, tr( "Chord type" ) );
	for( int i = 0; i < m_chordTable->size(); ++i )
	{
		m_chordsModel->addItem( m_chordTable->at(i).getName() );
	}

}




chordtableEditor::~chordtableEditor()
{
}




QString chordtableEditor::nodeName() const
{
	return chordtableeditor_plugin_descriptor.name;
}



//---------------------------------------------
//---------------------------------------------
//---------------------------------------------


chordtableEditorView::chordtableEditorView( ToolPlugin * _tool ) :
	ToolPluginView( _tool  ),
	m_chordTableEditor( castModel<chordtableEditor>() ),
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

	QPushButton *button1 = new QPushButton(tr("New chord"));
	QPushButton *button2 = new QPushButton(tr("Delete chord"));

	//adding combo and button
	upperLayout->addWidget(m_chordsComboBox);
	upperLayout->addStretch();
	upperLayout->addWidget(button2);
	upperLayout->addWidget(button1);

	//the lower area
	 QWidget *lowerWidget=new QWidget(this);
	 lowerWidget->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
//	 lowerWidget->setMinimumSize(500,350);

	 QHBoxLayout *lowerWidgetLayout=new QHBoxLayout(lowerWidget);
	 lowerWidget->setLayout(lowerWidgetLayout);
	 lowerWidget->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );

	 //the lower widget scroll area
	 m_scrollArea = new QScrollArea(lowerWidget);
//	 m_scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
//	 m_scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
//	 m_scrollArea->setPalette( QApplication::palette( m_scrollArea ) );
//	 m_scrollArea->setFrameStyle( QFrame::NoFrame );

	 lowerWidgetLayout->addWidget(m_scrollArea);
	 QHBoxLayout *scrollLayout= new QHBoxLayout(m_scrollArea);
	 m_scrollArea->setLayout(scrollLayout);



	 //The widget inside the scroll area
	 m_chordsWidget = new QWidget(m_scrollArea);
	 m_chordsWidget->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);

	 scrollLayout->addWidget(m_chordsWidget);

	 m_chordsWidgetLayout= new QHBoxLayout(m_chordsWidget);
	 m_chordsWidgetLayout->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
//	 lowerLayout->setSizeConstraint( QLayout::SetMinimumSize );
//	 lowerLayout->setSpacing( 0 );
//	 lowerLayout->setMargin( 0 );
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
		parentWidget()->setMinimumSize(500,400);
		
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

	//Deleting all the widgets of the layout Qt4 style
//	QLayoutItem *wItem;
//	while ((wItem = m_chordsWidget->layout()->takeAt(0)) != 0)
//	{
//		m_chordsWidget->layout()->removeItem(wItem);
//		delete wItem;
//	}

	//Deleting all the widgets of the layout - Qt5 style
	m_chordsWidget->setUpdatesEnabled(false);
	qDeleteAll(m_chordsWidget->findChildren<QWidget*>("", Qt::FindDirectChildrenOnly));
	m_chordsWidget->setUpdatesEnabled(true);

	//adding the widgets from the menu
	foreach(InstrumentFunctionNoteStacking::ChordSemiTone m_chordSemiTone,m_Chord.getChordSemiTones())
	{
		chordNoteModel *m_chordNoteModel= new chordNoteModel(m_chordTableEditor, &m_chordSemiTone);
		chordNoteWidget *m_chordNoteWidget= new chordNoteWidget(m_chordNoteModel,this);
//		lowerInsideLayout->addWidget(m_chordNoteWidget);
		m_chordsWidgetLayout->addWidget(m_chordNoteWidget);
	}
}

chordtableEditorView::~chordtableEditorView()
{
}


//---------------------------------------------
//---------------------------------------------
//---------------------------------------------

chordNoteModel::chordNoteModel(Model *_parent, InstrumentFunctionNoteStacking::ChordSemiTone *_semiTone) :
	Model(_parent),
	m_volumeModel( _semiTone->vol, MinVolume, MaxVolume, 0.1f, this, tr( "Volume" ) ),
	m_panningModel( _semiTone->pan, PanningLeft, PanningRight, 0.1f, this, tr( "Panning" ) ),
	m_keyModel(_semiTone->key->value(),KeyMin,KeyMax,this, tr("Key")),
	m_activeModel(_semiTone->active,this,tr("Active")),
	m_silencedModel(_semiTone->silenced,this,tr("Silenced")),
	m_bareModel(_semiTone->bare,this,tr("Bare"))
{
	m_semiTone=_semiTone;
	connect(&m_volumeModel,SIGNAL(dataChanged()),this,SLOT(changeData()));
	connect(&m_keyModel,SIGNAL(dataChanged()),this,SLOT(changeData()));
}

//NON SEMBRA CAMBIARE I DATI DELLA CHORDTABLE
void chordNoteModel::changeData(){
	m_semiTone->vol=m_volumeModel.value();
	m_semiTone->key=m_keyModel.value();
	m_semiTone->pan=m_panningModel.value();
	m_semiTone->active=m_activeModel.value();
	m_semiTone->silenced=m_silencedModel.value();
	m_semiTone->bare=m_bareModel.value();
}

//---------------------------------------------
//---------------------------------------------
//---------------------------------------------


chordNoteWidget::chordNoteWidget(chordNoteModel * _model, QWidget *_parent) :
	QWidget(_parent),
	ModelView(_model,_parent),
	m_chordNoteModel( castModel<chordNoteModel>() )
{
	QVBoxLayout *m_vLayout= new QVBoxLayout(this);
	setLayout(m_vLayout);

	QFrame *frame=new QFrame;
	frame->setFrameStyle(QFrame::Panel | QFrame::Raised);
	frame->setLineWidth(2);
	frame->setMinimumWidth(62);
	frame->setMinimumHeight(250);

	m_vLayout->addWidget(frame);

	QGridLayout* gridLayout = new QGridLayout( frame );
	gridLayout->setAlignment(Qt::AlignHCenter);
//	gridLayout->setContentsMargins( 8, 18, 8, 8 );
//	gridLayout->setRowStretch( 4, 3);
//	gridLayout->setColumnStretch(0,2);
gridLayout->setHorizontalSpacing( 10 );
gridLayout->setVerticalSpacing( 10 );

	frame->setLayout(gridLayout);


	setAutoFillBackground( true );
	setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );


	m_volumeKnob = new Knob( knobDark_28, this );
	m_volumeKnob->setLabel( tr( "Volume" ) );
	m_volumeKnob->setModel(&m_chordNoteModel->m_volumeModel);
//	m_volumeKnob->move( 27, 5 );
	m_volumeKnob->setEnabled( true );
	m_volumeKnob->setHintText( tr( "Volume knob:" ), "" );
	m_volumeKnob->setWhatsThis( tr( "The Wet/Dry knob sets the ratio between "
					"the input signal and the effect signal that "
					"forms the output." ) );

	m_panKnob = new Knob( knobDark_28, this );
	m_panKnob->setLabel( tr( "Panning" ) );
	m_panKnob->setModel(&m_chordNoteModel->m_panningModel);
//	m_panKnob->move( 27, 5 );
	m_panKnob->setEnabled( true );
	m_panKnob->setHintText( tr( "Panning knob:" ), "" );
	m_panKnob->setWhatsThis( tr( "The Wet/Dry knob sets the ratio between "
					"the input signal and the effect signal that "
					"forms the output." ) );

	//----------------
	m_keySlider = new AutomatableSlider( this, tr( "Key note" ) );
	m_keySlider->setModel( &m_chordNoteModel->m_keyModel );
	m_keySlider->setOrientation( Qt::Vertical );
	m_keySlider->setPageStep( 1 );
	m_keySlider->setTickPosition( QSlider::TicksLeft );
	m_keySlider->setFixedSize( 26, 60 );
	m_keySlider->setTickInterval( 50 );
	ToolTip::add( m_keySlider, tr( "Key note" ) );
	m_keySlider->setWhatsThis( tr("The key note"));

	m_keyLcd= new LcdWidget( 3, this );
	m_keyLcd->setValue( m_chordNoteModel->m_keyModel.value());
	connect( m_keySlider, SIGNAL( logicValueChanged( int ) ), this,	SLOT( setKeyLabel( int ) ) );

	m_activeLed= new LedCheckBox(this, tr("Active"));
	m_activeLed->setModel(&m_chordNoteModel->m_activeModel);
	m_activeLed->setWhatsThis( tr("If the note is active or gets omitted"));
	m_activeLed->setEnabled(true);
	ToolTip::add( m_activeLed, tr( "Active note" ) );

	QLabel* m_activeLabel = new QLabel( tr( "Act.:" ) );
	m_activeLabel->setFont( pointSize<8>( m_activeLabel->font() ) );

	m_silencedLed= new LedCheckBox(this, tr("Silenced"));
	m_silencedLed->setModel(&m_chordNoteModel->m_silencedModel);
	m_silencedLed->setWhatsThis( tr("If the note is silenced"));
	m_silencedLed->setEnabled(true);
	ToolTip::add( m_silencedLed, tr( "Silenced note" ) );

	QLabel* m_silencedLabel = new QLabel( tr( "Sil.:" ) );
	m_silencedLabel->setFont( pointSize<8>( m_silencedLabel->font() ) );

	m_bareLed= new LedCheckBox(this, tr("Bare"));
	m_bareLed->setModel(&m_chordNoteModel->m_bareModel);
	m_bareLed->setWhatsThis( tr("If the arpeggio ignores the note volume or panning "));
	m_bareLed->setEnabled(true);
	ToolTip::add( m_bareLed, tr( "Bare note" ) );

	QLabel* m_bareLabel = new QLabel( tr( "Bar.:" ) );
	m_bareLabel->setFont( pointSize<8>( m_bareLabel->font() ) );


//	connect( m_keySlider, SIGNAL( sliderPressed() ), this,
//			SLOT( showMasterVolumeFloat()) );
//	connect( m_keySlider, SIGNAL( logicSliderMoved( int ) ), this,
//			SLOT( updateMasterVolumeFloat( int ) ) );
//	connect( m_keySlider, SIGNAL( sliderReleased() ), this,
//			SLOT( hideMasterVolumeFloat() ) );
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

//	gridLayout->addWidget(m_volumeKnob,0,0,0,2);
//	gridLayout->addWidget(m_panKnob,1,0,0,2);
//	gridLayout->addWidget(m_keyLcd,2,0,0,2);
//	gridLayout->addWidget(m_keySlider,3,0,3,2);
//	gridLayout->addWidget(m_activeLed,7,0,0,0);
//	gridLayout->addWidget(m_silencedLed,8,0,0,0);
//	gridLayout->addWidget(m_bareLed,9,0,0,0);

}

void chordNoteWidget::setKeyLabel(int i)
{
	if (m_keyLcd)
	{
		m_keyLcd->setValue(i);
		m_keyLcd->update();
	}
}

