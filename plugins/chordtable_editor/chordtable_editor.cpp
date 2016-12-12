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
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>


#include "gui_templates.h"
#include "TabBar.h"
#include "TabButton.h"
#include "ComboBox.h"

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




chordtableEditor::chordtableEditor() :
	ToolPlugin( &chordtableeditor_plugin_descriptor, NULL ),
	m_chordtable(&InstrumentFunctionNoteStacking::ChordTable::getInstance())
{
	m_chordsModel= new ComboBoxModel( this, tr( "Chord type" ) );
	for( int i = 0; i < m_chordtable->size(); ++i )
	{
		m_chordsModel->addItem( m_chordtable->at(i).getName() );
	}

}




chordtableEditor::~chordtableEditor()
{
}




QString chordtableEditor::nodeName() const
{
	return chordtableeditor_plugin_descriptor.name;
}






chordtableEditorView::chordtableEditorView( ToolPlugin * _tool ) :
	ToolPluginView( _tool  ),
	m_chordTableEditor( castModel<chordtableEditor>() ),
	m_chordsComboBox( new ComboBox() )
{
	setWindowIcon( embed::getIconPixmap( "controller" ) ); //menjaj icono!!
	setWindowTitle( tr( "ChordTable Editor" ) );

	//the top level all including dialog;
	QVBoxLayout* topLayout = new QVBoxLayout( this );
	topLayout->setMargin( 0 );

	//the upper top area of the dialog
	QWidget * upperWidget= new QWidget(this);
	QHBoxLayout *upperLayout = new QHBoxLayout(upperWidget);
	upperWidget->setLayout(upperLayout);

	QSizePolicy gp=m_chordsComboBox->sizePolicy();
	gp.setVerticalPolicy(QSizePolicy::Fixed);
	m_chordsComboBox->setMinimumSize(120,20);
	m_chordsComboBox->setSizePolicy(gp);

	QPushButton *button1 = new QPushButton("One");

	upperLayout->addWidget(m_chordsComboBox);
	upperLayout->addStretch();
	upperLayout->addWidget(button1);

	//the lower area

	 QWidget *lowerWidget=new QWidget(this);
	 lowerWidget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
	 lowerWidget->setMinimumSize(400,300);
	 QHBoxLayout *lowerLayout= new QHBoxLayout(lowerWidget);
	 lowerWidget->setLayout(lowerLayout);


//setting the main layout
	 topLayout->addWidget(upperWidget);
	 topLayout->addWidget(lowerWidget);
	 topLayout->addStretch();

//--------------

	m_chordsComboBox->setModel(m_chordTableEditor->m_chordsModel);


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





