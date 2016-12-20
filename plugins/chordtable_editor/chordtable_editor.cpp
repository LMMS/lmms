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
	setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );


	//the top level all including dialog;
	QVBoxLayout* topLayout = new QVBoxLayout( this );
	topLayout->setMargin( 0 );

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
	QPushButton *button1 = new QPushButton(tr("New chord"));
	QPushButton *button2 = new QPushButton(tr("Delete chord"));

	//adding combo and button
	upperLayout->addWidget(m_chordsComboBox);
	upperLayout->addStretch();
	upperLayout->addWidget(button2);
	upperLayout->addWidget(button1);

	//the lower area
	 QWidget *lowerWidget=new QWidget(this);
	 lowerWidget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
	 lowerWidget->setMinimumSize(400,300);
	 QHBoxLayout *lowerLayout= new QHBoxLayout(lowerWidget);
	 lowerLayout->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
//	 lowerLayout->setSizeConstraint( QLayout::SetMinimumSize );
//	 lowerLayout->setSpacing( 0 );
//	 lowerLayout->setMargin( 0 );
	 lowerWidget->setLayout(lowerLayout);

	//the first node de prova!!
	InstrumentFunctionNoteStacking::Chord cc=m_chordTableEditor->m_chordTable->at(0);
	InstrumentFunctionNoteStacking::ChordSemiTone st=cc.at(0);
	chordNoteModel *cm= new chordNoteModel(m_chordTableEditor, &st);
	chordNoteModel *cm1= new chordNoteModel(m_chordTableEditor, &st);
	chordNoteWidget *cn= new chordNoteWidget(cm,this);
	chordNoteWidget *cn1= new chordNoteWidget(cm1,this);
	lowerLayout->addWidget(cn);
	lowerLayout->addWidget(cn1);

//setting the main layout
	 topLayout->addWidget(upperWidget);
	 topLayout->addStretch();
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




chordtableEditorView::~chordtableEditorView()
{
}


//---------------------------------------------
//---------------------------------------------
//---------------------------------------------

chordNoteModel::chordNoteModel(Model *_parent, InstrumentFunctionNoteStacking::ChordSemiTone *_semiTone) :
	Model(_parent),
	m_semiTone( _semiTone	),
	m_volumeModel( DefaultVolume, MinVolume, MaxVolume, 0.1f, this, tr( "Volume" ) ),
	m_panningModel( DefaultPanning, PanningLeft, PanningRight, 0.1f, this, tr( "Panning" ) ),
	m_keyModel(KeyCenter,KeyMin,KeyMax,this, tr("Key")),
	m_activeModel(true,this,tr("Active")),
	m_silencedModel(false,this,tr("Silenced")),
	m_bareModel(false,this,tr("Bare"))
{

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

	m_vLayout->addWidget(frame);

	QVBoxLayout *vl = new QVBoxLayout(frame);
	frame->setLayout(vl);


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

	m_silencedLed= new LedCheckBox(this, tr("Silenced"));
	m_silencedLed->setModel(&m_chordNoteModel->m_silencedModel);
	m_silencedLed->setWhatsThis( tr("If the note is silenced"));
	m_silencedLed->setEnabled(true);
	ToolTip::add( m_silencedLed, tr( "Silenced note" ) );

	m_bareLed= new LedCheckBox(this, tr("Bare"));
	m_bareLed->setModel(&m_chordNoteModel->m_bareModel);
	m_bareLed->setWhatsThis( tr("If the arpeggio ignores the note volume or panning "));
	m_bareLed->setEnabled(true);
	ToolTip::add( m_bareLed, tr( "Bare note" ) );


//	connect( m_keySlider, SIGNAL( sliderPressed() ), this,
//			SLOT( showMasterVolumeFloat()) );
//	connect( m_keySlider, SIGNAL( logicSliderMoved( int ) ), this,
//			SLOT( updateMasterVolumeFloat( int ) ) );
//	connect( m_keySlider, SIGNAL( sliderReleased() ), this,
//			SLOT( hideMasterVolumeFloat() ) );
	//----------------


	vl->addWidget(m_volumeKnob);
	vl->addWidget(m_panKnob);
	vl->addWidget(m_keyLcd);
	vl->addWidget(m_keySlider);
	vl->addWidget(m_activeLed);
	vl->addWidget(m_silencedLed);
	vl->addWidget(m_bareLed);

}

void chordNoteWidget::setKeyLabel(int i)
{
	if (m_keyLcd)
	{
		m_keyLcd->setValue(i);
		m_keyLcd->update();
	}
}

