/*
 * chordtable_editor.cpp - dialog to display information about installed CHORDTABLE
 * plugins
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


#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QMessageBox>

#include "gui_templates.h"
#include "Knob.h"
#include "ComboBox.h"
#include "AutomatableSlider.h"
#include "Model.h"
#include "ToolTip.h"
#include "LcdWidget.h"
#include "LedCheckbox.h"
#include "GuiApplication.h"
#include "Engine.h"
#include "Song.h"
#include "FileDialog.h"

#include "ChordTable.h"
#include "chordtable_editor.h"

#include "embed.cpp"


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT chordtableeditor_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"ChordTable Editor",
	QT_TRANSLATE_NOOP( "ChordTable Editor","Edits the Chords of the ChordTable "
	"and allows automation of the seminote parameters" ),
	"Riki Sluga <jaz/at/rikis/dot/net>",
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
	ToolPlugin( & chordtableeditor_plugin_descriptor, NULL )
{
	m_chordTable = Engine::chordTable();
	m_chordsComboModel= new ComboBoxModel( this, tr( "Chord type" ) );
	//loads data into comboModel
	reloadComboModel();
}


chordtableEditor::~chordtableEditor()
{
}


QString chordtableEditor::nodeName() const
{
	return chordtableeditor_plugin_descriptor.name;
}

void chordtableEditor::reloadComboModel()
{
	int i=m_chordsComboModel->value();

	m_chordsComboModel->clear();
	for( int i = 0; i < m_chordTable->size(); ++i )
	{
		m_chordsComboModel->addItem( m_chordTable->at( i )->getName() );
	}
	m_chordsComboModel->setValue( i );
}



/*****************************************************************************************************
 *
 * The chordtableEditorView class
 *
******************************************************************************************************/

ChordTableEditorView::ChordTableEditorView( ToolPlugin * _tool ) :
	ToolPluginView( _tool ),
	m_chordTableEditor( castModel<chordtableEditor>() ),
	m_chordTable( m_chordTableEditor->m_chordTable ),
	m_chord( NULL ),
	m_chordsComboBox( new ComboBox() )
{
	setWindowIcon( PluginPixmapLoader( "logo" ).pixmap() );
	setWindowTitle( tr( "ChordTable Editor" ) );

	setAutoFillBackground( true );
	setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );

	QVBoxLayout * topLayout = new QVBoxLayout( this );
	topLayout->setMargin( 0 );
	setLayout( topLayout );

	QWidget * upperWidget= new QWidget( this );
	QGridLayout* upperLayout = new QGridLayout( upperWidget );
	upperLayout->setContentsMargins( 8, 8, 8, 8 );
	upperLayout->setColumnStretch( 0, 1 );
	upperLayout->setHorizontalSpacing( 3 );
	upperLayout->setVerticalSpacing( 1 );
	upperWidget->setLayout( upperLayout );

	QSizePolicy gp=m_chordsComboBox->sizePolicy();
	gp.setVerticalPolicy( QSizePolicy::Fixed );
	m_chordsComboBox->setMinimumSize( 130, 22 );
	m_chordsComboBox->setSizePolicy( gp );

	//combobox data
	m_chordsComboBox->setModel( m_chordTableEditor->m_chordsComboModel );

	//Connecting combobox signal to widget data
	connect( m_chordTableEditor->m_chordsComboModel, SIGNAL( dataChanged() ), this, SLOT( loadChord() ) );

	//Connecting the changing of data to the combobox
	connect( m_chordTable, SIGNAL( chordTableChanged() ), this, SLOT( loadChord() ) );
	connect( m_chordTable, SIGNAL( chordTableChanged() ), m_chordTableEditor, SLOT( reloadComboModel() ) );

	QPushButton * addSemiBut = new QPushButton( tr( "Add SemiTone" ) );
	addSemiBut->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

	QPushButton * delChordBut = new QPushButton( tr( "Delete chord" ) );
	QPushButton * resetChordsBut = new QPushButton( tr( "Reset chords" ) );
	QPushButton * saveFileBut = new QPushButton( tr( "Save File" ) );
	QPushButton * openFileBut = new QPushButton( tr( "Open File" ) );
	QPushButton * newChordBut = new QPushButton( tr( "New chord" ) );
	QPushButton * cloneChordBut = new QPushButton( tr( "Clone chord" ) );

	connect( addSemiBut, SIGNAL ( clicked() ), this, SLOT ( addChordSemiTone() ) );
	connect( delChordBut, SIGNAL ( clicked() ), this, SLOT ( removeChord() ) );
	connect( resetChordsBut, SIGNAL ( clicked() ), this, SLOT ( resetChords() ) );
	connect( saveFileBut, SIGNAL ( clicked() ), this, SLOT ( saveFile() ) );
	connect( openFileBut, SIGNAL ( clicked() ), this, SLOT ( openFile() ) );
	connect( newChordBut, SIGNAL ( clicked() ), this, SLOT ( newChord() ) );
	connect( cloneChordBut, SIGNAL ( clicked() ), this, SLOT ( cloneChord() ) );

	// setup line edit for changing instrument track name
	m_nameLineEdit = new QLineEdit;
	m_nameLineEdit->setFont( pointSize<9>( m_nameLineEdit->font() ) );
	connect( m_nameLineEdit, SIGNAL( textChanged( const QString & ) ),
					 this, SLOT( changeText( const QString & ) ) );
	m_nameLineEdit->setWhatsThis(tr(""));
	m_nameLineEdit->setToolTip(tr(""));
	//lineedit change
	connect( this, SIGNAL( lineEditChange() ),this,SLOT( reloadCombo() ) );

	upperLayout->addWidget( m_nameLineEdit, 0, 0 );
	upperLayout->addWidget( m_chordsComboBox, 1, 0 );
	upperLayout->addWidget( openFileBut, 0, 1 );
	upperLayout->addWidget( saveFileBut, 1, 1 );
	upperLayout->addWidget( cloneChordBut, 0, 2 );
	upperLayout->addWidget( newChordBut, 1 ,2 );
	upperLayout->addWidget( resetChordsBut, 0, 3 );
	upperLayout->addWidget( delChordBut,1, 3 );
	upperLayout->addWidget( addSemiBut, 0, 4, 2, 2 );

	//the lower area
	QWidget * lowerWidget = new QWidget( this );

	QHBoxLayout * lowerWidgetLayout = new QHBoxLayout( lowerWidget );
	lowerWidget->setLayout( lowerWidgetLayout );

	//the lower widget scroll area
	m_scrollArea = new QScrollArea( lowerWidget );
	m_scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
	m_scrollArea->setPalette( QApplication::palette( m_scrollArea ) );
	m_scrollArea->setFrameStyle( QFrame::NoFrame );
	lowerWidgetLayout->addWidget( m_scrollArea );

	//The widget inside the scroll area
	m_chordsWidget = new QWidget( lowerWidget );

	m_chordsWidget->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	m_chordsWidgetLayout = new QHBoxLayout();

	m_chordsWidgetLayout->setAlignment( Qt::AlignLeft | Qt::AlignBottom );
	m_chordsWidgetLayout->setSpacing( 2 );
	m_chordsWidgetLayout->setMargin( 0 );
	m_chordsWidgetLayout->setSizeConstraint( QLayout::SetFixedSize );

	m_chordsWidget->setLayout( m_chordsWidgetLayout );
	m_scrollArea->setWidget( m_chordsWidget );

	//the first node de prova!!
	//	loadChord();

	lowerWidget->setMinimumHeight( m_chordsWidget->height() + 20 );

	//setting the main layout
	topLayout->addWidget( upperWidget );
	//	 topLayout->addStretch();
	topLayout->addWidget( lowerWidget );

	//--------------

	//changes the combobox in the plugin
	connect( gui, SIGNAL( genericSignal_1( int ) ), this, SLOT( setChordSelection( int ) ) );

	//loads the Chord
	loadChord();

	setWhatsThis( tr( "This dialog allows editing and automation of the chord table." ) );


	hide();
	if( parentWidget() )
	{
		parentWidget()->hide();
//		parentWidget()->layout()->setSizeConstraint( QLayout::SetDefaultConstraint );
		parentWidget()->layout()->setSizeConstraint( QLayout::SetNoConstraint );
		parentWidget()->setMinimumSize( 750,500 );
		parentWidget()->adjustSize();

		Qt::WindowFlags flags = parentWidget()->windowFlags();
		//		flags |= Qt::MSWindowsFixedSizeDialogHint;
		flags &= ~Qt::WindowMaximizeButtonHint;
		parentWidget()->setWindowFlags( flags );
	}


}


void ChordTableEditorView::loadChord()
{

	//taking selected value from the comboboxmodel
	int i = m_chordTableEditor->m_chordsComboModel->value();
	//eliminating problems while deleting last chord
	if ( i >= m_chordTable->size() )
	{
		i = m_chordTable->size() - 1;
	}
	//getting the address of the selected chord
	m_chord = m_chordTable->at( i );
	//setting chord name to the editline
	m_nameLineEdit->blockSignals( true );
	m_nameLineEdit->setText( m_chord->m_name );
	m_nameLineEdit->blockSignals( false );

	QLayoutItem * child;
	while ( ( child = m_chordsWidgetLayout->takeAt( 0 ) ) != 0 ) {
		m_chordsWidgetLayout->removeItem( child );
		delete child->widget();
		delete child;
	}

	//adding the widgets from the menu
	ChordSemiTone * m_chordSemiTone;
	chordNoteModel * m_chordNoteModel;
	chordNoteWidget * m_chordNoteWidget;
	for ( int i = 0; i < m_chord->size(); i++ )
	{
		m_chordSemiTone = m_chord->at( i );
		m_chordNoteModel = new chordNoteModel( m_chordTableEditor, m_chordSemiTone, i );
		m_chordNoteWidget = new chordNoteWidget( m_chordNoteModel, m_chordsWidget );
		//Connects the nested delete pushbutton to the remove chordnote slot
		connect( m_chordNoteWidget, SIGNAL( emitDeletePosition( int ) ), this, SLOT( removeSemiTone( int ) ) );
		//Connects the nested clone pushbutton to the clone the chordnote
		connect( m_chordNoteWidget, SIGNAL( emitClonePosition( int ) ), this, SLOT( cloneSemiTone( int ) ) );
		m_chordsWidgetLayout->addWidget( m_chordNoteWidget );
	}

	m_chordsWidget->adjustSize();
}

void ChordTableEditorView::reloadCombo()
{
	m_chordTableEditor->reloadComboModel();
	//emits signal combo model data has changed
	Engine::chordTable()->emitChordNameChanged();
}

void ChordTableEditorView::resetChords()
{
	if ( confirmDialog( tr( "Resetting chords" ), tr( "All the chords and their automation are being reset "
																										"to their factory values. All the newly created chords "
																										"will be deleted. \nAre you sure?" ) ) == QMessageBox::Ok )
	{
		Engine::getSong()->stop();
		//setting combomodel value to 0;
		m_chordTableEditor->m_chordsComboModel->setValue( 0 );
		m_chordTable->reset();
		reloadCombo();
	}
}

void ChordTableEditorView::removeSemiTone( int i )
{
	if ( confirmDialog( tr( "Remove Semitone" ), tr( "Are you sure?" ) ) == QMessageBox::Ok )
	{
		Engine::getSong()->stop();
		m_chord->removeSemiTone( i );
	}
}

void ChordTableEditorView::addChordSemiTone()
{
	m_chord->addSemiTone();
}

void ChordTableEditorView::cloneSemiTone( int i )
{
	ChordSemiTone * cst = new ChordSemiTone( m_chord->at( i ) );
	m_chord->insertSemiTone( cst, i );
}

void ChordTableEditorView::saveFile()
{
	FileDialog sfd( this, tr( "Save preset" ), "", tr( "Chord Table XML preset file ( *.ctd )" ) );

	QString presetRoot = ConfigManager::inst()->userPresetsDir();
	if( !QDir( presetRoot ).exists() )
	{
		QDir().mkdir( presetRoot );
	}
	if( !QDir( presetRoot + "ChordTable" ).exists() )
	{
		QDir( presetRoot ).mkdir( "ChordTable" );
	}

	sfd.setAcceptMode( FileDialog::AcceptSave );
	sfd.setDirectory( presetRoot + "ChordTable" );
	sfd.setFileMode( FileDialog::AnyFile );
	QString fname = "ChordTable";
	sfd.selectFile( fname.remove( QRegExp( "[^a-zA-Z0-9_\\-\\d\\s]" ) )+".ctd" );

	if( sfd.exec() == QDialog::Accepted &&
			!sfd.selectedFiles().isEmpty() &&
			!sfd.selectedFiles().first().isEmpty() )
	{
		//		DataFile::LocaleHelper localeHelper( DataFile::LocaleHelper::ModeSave );

		DataFile dataFile( DataFile::ChordTable );
		m_chordTable->saveSettings( dataFile, dataFile.content() );
		QString f = sfd.selectedFiles()[0];
		dataFile.writeFile( f );
	}
}

void ChordTableEditorView::openFile()
{
	if ( confirmDialog( tr( "Open File" ), tr( "By opening a new file all changes to this ChordTable"
																						 " will be lost. \nAre you sure?" ) ) == QMessageBox::Ok )
	{

		Engine::getSong()->stop();
		FileDialog sfd( this, tr( "Open preset" ), "", tr( "Chord Table preset file ( *.ctd )" ) );

		QString presetRoot = ConfigManager::inst()->userPresetsDir();
		if( !QDir( presetRoot ).exists() )
		{
			QDir().mkdir( presetRoot );
		}
		if( !QDir( presetRoot + "ChordTable" ).exists() )
		{
			QDir( presetRoot ).mkdir( "ChordTable" );
		}

		sfd.setAcceptMode( FileDialog::AcceptOpen );
		sfd.setDirectory( presetRoot + "ChordTable" );
		sfd.setFileMode( FileDialog::AnyFile );
		QString fname = "";
		sfd.selectFile( fname.remove( QRegExp( "[^a-zA-Z0-9_\\-\\d\\s]" ) ) + ".ctd" );

		if( sfd.exec() == QDialog::Accepted &&
				!sfd.selectedFiles().isEmpty() &&
				!sfd.selectedFiles().first().isEmpty() )
		{
			//DataFile::LocaleHelper localeHelper( DataFile::LocaleHelper::ModeLoad );

			QString f = sfd.selectedFiles()[0];
			DataFile dataFile( f );

			m_chordTable->loadSettings( dataFile.content() );
		}
	}
}

void ChordTableEditorView::newChord()
{
	Engine::getSong()->stop();
	m_chordTable->cloneChord( -1 );
	reloadCombo();
	m_chordsComboBox->model()->setValue( m_chordTable->size() );
}

void ChordTableEditorView::cloneChord()
{
	Engine::getSong()->stop();
	m_chordTable->cloneChord( m_chordsComboBox->model()->value() );
	reloadCombo();
	m_chordsComboBox->model()->setValue( m_chordTable->size() );
}

void ChordTableEditorView::removeChord()
{
	if ( confirmDialog( tr( "Remove Chord" ), tr( "This chord will be removed. \nAre you sure?" ) ) == QMessageBox::Ok )
	{
		Engine::getSong()->stop();
		m_chordTable->removeChord( m_chordsComboBox->model()->value() );
		reloadCombo();
	}
}

void ChordTableEditorView::changeText( QString _text )
{
	m_chord->m_name =_text;
	emit lineEditChange();
}

void ChordTableEditorView::setChordSelection( int i )
{
	m_chordsComboBox->model()->setValue( i );
}

ChordTableEditorView::~ChordTableEditorView()
{
}

int ChordTableEditorView::confirmDialog(QString _title, QString _text)
{
	QMessageBox msgBox;
	msgBox.setIcon( QMessageBox::Warning );
	msgBox.setWindowTitle(_title);
	msgBox.setText(_text);
	msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
	msgBox.setDefaultButton(QMessageBox::Ok);
	return msgBox.exec();
}



/*****************************************************************************************************
 *
 * The chordNoteModel class
 *
******************************************************************************************************/

chordNoteModel::chordNoteModel( Model *_parent, ChordSemiTone *_semiTone, int _position ) :
	Model( _parent ),
	m_semiTone( _semiTone ),
	m_position( _position )
{
}

//the position of the SemiTone in the semitones vector
int chordNoteModel::position() const
{
	return m_position;
}

void chordNoteModel::setPosition( int position )
{
	m_position = position;
}


/*****************************************************************************************************
 *
 * The chordNoteWidget class
 *
******************************************************************************************************/

chordNoteWidget::chordNoteWidget( chordNoteModel * _model, QWidget *_parent ) :
	QWidget( _parent ),
	ModelView( _model, _parent ),
	m_chordNoteModel( castModel<chordNoteModel>() )
{
	setObjectName( "chordNoteWidget" );
	//the position in the vector
	m_position = m_chordNoteModel->position();
	setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

	m_vLayout = new QVBoxLayout( this );
	m_vLayout->setSizeConstraint( QLayout::SetFixedSize );
	setLayout( m_vLayout );

	m_Frame = new QFrame( this );
	m_Frame->setFrameStyle( QFrame::Panel | QFrame::Raised );
	m_Frame->setLineWidth( 2 );
	m_vLayout->addWidget( m_Frame );

	m_gridLayout = new QGridLayout( m_Frame );
	m_gridLayout->setAlignment( Qt::AlignHCenter );
	m_gridLayout->setHorizontalSpacing( 10 );
	m_gridLayout->setVerticalSpacing( 10 );

	m_Frame->setLayout( m_gridLayout );

	setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );

	m_volumeKnob = new Knob( knobDark_28, m_Frame );
	m_volumeKnob->setLabel( tr( "Volume" ) );
	m_volumeKnob->setModel( m_chordNoteModel->m_semiTone->vol );
	m_volumeKnob->setEnabled( true );
	m_volumeKnob->setHintText( tr( "Volume knob:" ), "" );
	m_volumeKnob->setWhatsThis( tr( "This automatable knob sets the volume "
																	"of the corresponding semitone. \n"
																	"It is expressed in percentage related to the volume"
																	" of the base note: a value of 100 means 2x the volume"
																	" of the base note." ) );

	m_panKnob = new Knob( knobDark_28, m_Frame );
	m_panKnob->setLabel( tr( "Panning" ) );
	m_panKnob->setModel( m_chordNoteModel->m_semiTone->pan );
	m_panKnob->setEnabled( true );
	m_panKnob->setHintText( tr( "Panning knob:" ), "" );
	m_panKnob->setWhatsThis( tr( "This automatable knob sets the panning of the "
															 "corresponding semitone" ) );

	//----------------
	m_keySlider = new AutomatableSlider( m_Frame, tr( "Key note" ) );
	m_keySlider->setModel( m_chordNoteModel->m_semiTone->key );
	m_keySlider->setOrientation( Qt::Vertical );
	m_keySlider->setPageStep( 1 );
	m_keySlider->setTickPosition( QSlider::TicksLeft );
	m_keySlider->setFixedSize( 26, 60 );
	m_keySlider->setTickInterval( 50 );
	ToolTip::add( m_keySlider, tr( "Key note slider" ) );
	m_keySlider->setWhatsThis( tr( "This automatable slider sets the arpeggio semitone" ) );

	m_keyLcd = new LcdWidget( 3, m_Frame );
	m_keyLcd->setValue( m_chordNoteModel->m_semiTone->key->value() );
	ToolTip::add( m_keyLcd, tr( "The key note value" ) );
	m_keyLcd->setWhatsThis( tr( "The distance in semitones from the base note" ) );
	connect( m_keySlider, SIGNAL( logicValueChanged( int ) ), this,	SLOT( setKeyLabel( int ) ) );

	//TODO: until the "Active" action isn't fixed the led won't work
	m_activeLed = new LedCheckBox( m_Frame, tr( "Active" ) );
	m_activeLed->setModel( m_chordNoteModel->m_semiTone->active );
	m_activeLed->setWhatsThis( tr( "It toggles the note to active state (it's played) or to inactive state (it's skipped)/n/n"
																 "FUNCTION TEMPORARILY DISABLED" ) );
	m_activeLed->setEnabled( false );
	//disabling signals to prevent action
	m_activeLed->blockSignals( true );
	ToolTip::add( m_activeLed, tr( "Active note status - FUNCTION DISABLED" ) );

	QLabel * m_activeLabel = new QLabel( tr( "Act.:" ) );
	m_activeLabel->setParent( m_Frame );
	m_activeLabel->setFont( pointSize<8>( m_activeLabel->font() ) );

	m_silencedLed = new LedCheckBox( m_Frame, tr( "Silenced" ) );
	m_silencedLed->setModel( m_chordNoteModel->m_semiTone->silenced );
	m_silencedLed->setWhatsThis( tr( "while playing the note is taken into account but silenced, also during automation" ) );
	m_silencedLed->setEnabled( true );
	ToolTip::add( m_silencedLed, tr( "Silenced note" ) );

	QLabel * m_randomLabel = new QLabel( tr( "Rand:" ) );
	m_randomLabel->setParent( m_Frame );
	m_randomLabel->setFont( pointSize<8>( m_randomLabel->font() ) );

	m_randomLed = new LedCheckBox( m_Frame, tr( "Random" ) );
	m_randomLed->setModel( m_chordNoteModel->m_semiTone->rand );
	m_randomLed->setWhatsThis( tr( "The chord semitone will be a random value from the -key to +key value.\n"
																 "The panning will also be random, ranging from -pan to +pan value." ) );
	m_randomLed->setEnabled( true );
	ToolTip::add( m_randomLed, tr( "Random note" ) );

	QLabel * m_silencedLabel = new QLabel( tr( "Sil.:" ) );
	m_silencedLabel->setParent( m_Frame );
	m_silencedLabel->setFont( pointSize<8>( m_silencedLabel->font() ) );

	m_bareLed = new LedCheckBox( m_Frame, tr( "Bare" ) );
	m_bareLed->setModel( m_chordNoteModel->m_semiTone->bare );
	m_bareLed->setWhatsThis( tr( "Only the key SemiTone is processed. Silence toggle, Volume and panning discarded, also during automation" ) );
	m_bareLed->setEnabled( true );
	ToolTip::add( m_bareLed, tr( "Bare note" ) );

	QLabel * m_bareLabel = new QLabel( tr( "Bar.:" ) );
	m_bareLabel->setParent( m_Frame );
	m_bareLabel->setFont( pointSize<8>( m_bareLabel->font() ) );

	//----------------


	m_gridLayout->addWidget( m_volumeKnob, 0, 0, 1, 2, Qt::AlignCenter );
	m_gridLayout->addWidget( m_panKnob, 1, 0, 1, 2, Qt::AlignCenter );
	m_gridLayout->addWidget( m_keyLcd, 2, 0, 1, 2, Qt::AlignCenter );
	m_gridLayout->addWidget( m_keySlider, 3, 0, 1, 2, Qt::AlignCenter );

	m_gridLayout->addWidget( m_activeLabel, 4, 0, 1, 1, Qt::AlignCenter );
	m_gridLayout->addWidget( m_activeLed, 4, 1, 1, 1, Qt::AlignCenter );

	m_gridLayout->addWidget( m_silencedLabel, 5, 0, 1, 1, Qt::AlignCenter );
	m_gridLayout->addWidget( m_silencedLed, 5, 1, 1, 1, Qt::AlignCenter );

	m_gridLayout->addWidget( m_randomLabel, 6, 0, 1, 1, Qt::AlignCenter );
	m_gridLayout->addWidget( m_randomLed, 6, 1, 1, 1, Qt::AlignCenter );

	m_gridLayout->addWidget( m_bareLabel, 7, 0, 1, 1, Qt::AlignCenter );
	m_gridLayout->addWidget( m_bareLed, 7, 1, 1, 1, Qt::AlignCenter );

	//Connect it while instantiating this class!!
	m_cloneButton = new QPushButton( tr( "Clone" ) );
	m_cloneButton->setParent( m_Frame );
	m_cloneButton->setWhatsThis( tr( "Duplicates the SemiTone next to the original" ) );
	ToolTip::add( m_cloneButton, tr( "Clones the SemiTone" ) );
	//connects the pushBUtton to emitting position signal
	connect( m_cloneButton, SIGNAL( clicked() ), this, SLOT( emitClonePosition() ) );

	m_gridLayout->addWidget( m_cloneButton, 8, 0, 1, 2, Qt::AlignCenter );

	//the first widget is the one
	m_delButton = new QPushButton( tr( "Del" ) );
	m_delButton->setParent( m_Frame );

	if ( m_position == 0 )
	{
		m_delButton->setEnabled( false );
		m_delButton->setStyleSheet( QString::fromUtf8( "QPushButton:disabled"
																								 "{ color: gray }"
																								 ) );
		m_delButton->setWhatsThis( tr( "The base note can't be deleted." ) );
		ToolTip::add( m_delButton, tr( "Delete disabled for base note" ) );
	}
	else
	{
		m_delButton->setWhatsThis( tr( "Deletes the SemiTone" ) );
		ToolTip::add( m_delButton, tr( "Deletes the SemiTone" ) );
		//connects the pushBUtton to emitting position signal
		connect( m_delButton, SIGNAL( clicked() ), this, SLOT( emitDeletePosition() ) );
	}
	m_gridLayout->addWidget( m_delButton, 9, 0, 1, 2, Qt::AlignCenter );

}

chordNoteWidget::~chordNoteWidget()
{
	//don't think this is necessary, the parent widget deletes the children automatically
	delete m_volumeKnob;
	delete m_keyLcd;
	delete m_keySlider;
	delete m_activeLed;
	delete m_silencedLed;
	delete m_bareLed;
	delete m_panKnob;
	delete m_delButton;
	delete m_cloneButton;
	delete m_gridLayout;
	delete m_Frame;
	delete m_vLayout;
}


void chordNoteWidget::setKeyLabel( int i )
{
	if ( m_keyLcd )
	{
		m_keyLcd->setValue( i );
		m_keyLcd->update();
	}
}

int chordNoteWidget::position() const
{
	return m_position;
}

void chordNoteWidget::setPosition( int position )
{
	m_position = position;
}

