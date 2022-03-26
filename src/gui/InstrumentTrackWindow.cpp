/*
 * InstrumentTrackWindow.cpp - implementation of InstrumentTrackWindow class
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
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

#include "InstrumentTrackWindow.h"

#include <QDir>
#include <QDropEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

#include "ComboBox.h"
#include "ConfigManager.h"
#include "DataFile.h"
#include "EffectRackView.h"
#include "embed.h"
#include "Engine.h"
#include "FileBrowser.h"
#include "FileDialog.h"
#include "GroupBox.h"
#include "MixerLineLcdSpinBox.h"
#include "GuiApplication.h"
#include "gui_templates.h"
#include "Instrument.h"
#include "InstrumentFunctions.h"
#include "InstrumentFunctionViews.h"
#include "InstrumentMidiIOView.h"
#include "InstrumentMiscView.h"
#include "InstrumentSoundShapingView.h"
#include "InstrumentTrack.h"
#include "InstrumentTrackView.h"
#include "Knob.h"
#include "LcdSpinBox.h"
#include "LedCheckBox.h"
#include "LeftRightNav.h"
#include "MainWindow.h"
#include "PianoView.h"
#include "PluginFactory.h"
#include "PluginView.h"
#include "Song.h"
#include "StringPairDrag.h"
#include "SubWindow.h"
#include "TabWidget.h"
#include "ToolTip.h"
#include "TrackContainerView.h"
#include "TrackLabelButton.h"


const int INSTRUMENT_WIDTH	= 254;
const int INSTRUMENT_HEIGHT	= INSTRUMENT_WIDTH;
const int PIANO_HEIGHT		= 80;


InstrumentTrackWindow::InstrumentTrackWindow( InstrumentTrackView * _itv ) :
	QWidget(),
	ModelView( nullptr, this ),
	m_track( _itv->model() ),
	m_itv( _itv ),
	m_instrumentView( nullptr )
{
	setAcceptDrops( true );

	// init own layout + widgets
	setFocusPolicy( Qt::StrongFocus );
	QVBoxLayout * vlayout = new QVBoxLayout( this );
	vlayout->setMargin( 0 );
	vlayout->setSpacing( 0 );

	TabWidget* generalSettingsWidget = new TabWidget( tr( "GENERAL SETTINGS" ), this );

	QVBoxLayout* generalSettingsLayout = new QVBoxLayout( generalSettingsWidget );

	generalSettingsLayout->setContentsMargins( 8, 18, 8, 8 );
	generalSettingsLayout->setSpacing( 6 );

	QWidget* nameAndChangeTrackWidget = new QWidget( generalSettingsWidget );
	QHBoxLayout* nameAndChangeTrackLayout = new QHBoxLayout( nameAndChangeTrackWidget );
	nameAndChangeTrackLayout->setContentsMargins( 0, 0, 0, 0 );
	nameAndChangeTrackLayout->setSpacing( 2 );

	// setup line edit for changing instrument track name
	m_nameLineEdit = new QLineEdit;
	m_nameLineEdit->setFont( pointSize<9>( m_nameLineEdit->font() ) );
	connect( m_nameLineEdit, SIGNAL( textChanged( const QString & ) ),
				this, SLOT( textChanged( const QString & ) ) );

	m_nameLineEdit->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
	nameAndChangeTrackLayout->addWidget(m_nameLineEdit, 1);


	// set up left/right arrows for changing instrument
	m_leftRightNav = new LeftRightNav(this);
	connect( m_leftRightNav, SIGNAL( onNavLeft() ), this,
						SLOT( viewPrevInstrument() ) );
	connect( m_leftRightNav, SIGNAL( onNavRight() ), this,
						SLOT( viewNextInstrument() ) );
	// m_leftRightNav->setShortcuts();
	nameAndChangeTrackLayout->addWidget(m_leftRightNav);


	generalSettingsLayout->addWidget( nameAndChangeTrackWidget );



	QGridLayout* basicControlsLayout = new QGridLayout;
	basicControlsLayout->setHorizontalSpacing(3);
	basicControlsLayout->setVerticalSpacing(0);
	basicControlsLayout->setContentsMargins(0, 0, 0, 0);

	QString labelStyleSheet = "font-size: 6pt;";
	Qt::Alignment labelAlignment = Qt::AlignHCenter | Qt::AlignTop;
	Qt::Alignment widgetAlignment = Qt::AlignHCenter | Qt::AlignCenter;

	// set up volume knob
	m_volumeKnob = new Knob( knobBright_26, nullptr, tr( "Volume" ) );
	m_volumeKnob->setVolumeKnob( true );
	m_volumeKnob->setHintText( tr( "Volume:" ), "%" );

	basicControlsLayout->addWidget( m_volumeKnob, 0, 0 );
	basicControlsLayout->setAlignment( m_volumeKnob, widgetAlignment );

	QLabel *label = new QLabel( tr( "VOL" ), this );
	label->setStyleSheet( labelStyleSheet );
	basicControlsLayout->addWidget( label, 1, 0);
	basicControlsLayout->setAlignment( label, labelAlignment );


	// set up panning knob
	m_panningKnob = new Knob( knobBright_26, nullptr, tr( "Panning" ) );
	m_panningKnob->setHintText( tr( "Panning:" ), "" );

	basicControlsLayout->addWidget( m_panningKnob, 0, 1 );
	basicControlsLayout->setAlignment( m_panningKnob, widgetAlignment );

	label = new QLabel( tr( "PAN" ), this );
	label->setStyleSheet( labelStyleSheet );
	basicControlsLayout->addWidget( label, 1, 1);
	basicControlsLayout->setAlignment( label, labelAlignment );


	basicControlsLayout->setColumnStretch(2, 1);


	// set up pitch knob
	m_pitchKnob = new Knob( knobBright_26, nullptr, tr( "Pitch" ) );
	m_pitchKnob->setHintText( tr( "Pitch:" ), " " + tr( "cents" ) );

	basicControlsLayout->addWidget( m_pitchKnob, 0, 3 );
	basicControlsLayout->setAlignment( m_pitchKnob, widgetAlignment );

	m_pitchLabel = new QLabel( tr( "PITCH" ), this );
	m_pitchLabel->setStyleSheet( labelStyleSheet );
	basicControlsLayout->addWidget( m_pitchLabel, 1, 3);
	basicControlsLayout->setAlignment( m_pitchLabel, labelAlignment );


	// set up pitch range knob
	m_pitchRangeSpinBox= new LcdSpinBox( 2, nullptr, tr( "Pitch range (semitones)" ) );

	basicControlsLayout->addWidget( m_pitchRangeSpinBox, 0, 4 );
	basicControlsLayout->setAlignment( m_pitchRangeSpinBox, widgetAlignment );

	m_pitchRangeLabel = new QLabel( tr( "RANGE" ), this );
	m_pitchRangeLabel->setStyleSheet( labelStyleSheet );
	basicControlsLayout->addWidget( m_pitchRangeLabel, 1, 4);
	basicControlsLayout->setAlignment( m_pitchRangeLabel, labelAlignment );


	basicControlsLayout->setColumnStretch(5, 1);


	// setup spinbox for selecting Mixer-channel
	m_mixerChannelNumber = new MixerLineLcdSpinBox( 2, nullptr, tr( "Mixer channel" ), m_itv );

	basicControlsLayout->addWidget( m_mixerChannelNumber, 0, 6 );
	basicControlsLayout->setAlignment( m_mixerChannelNumber, widgetAlignment );

	label = new QLabel( tr( "CHANNEL" ), this );
	label->setStyleSheet( labelStyleSheet );
	basicControlsLayout->addWidget( label, 1, 6);
	basicControlsLayout->setAlignment( label, labelAlignment );

	QPushButton* saveSettingsBtn = new QPushButton( embed::getIconPixmap( "project_save" ), QString() );
	saveSettingsBtn->setMinimumSize( 32, 32 );

	connect( saveSettingsBtn, SIGNAL( clicked() ), this, SLOT( saveSettingsBtnClicked() ) );

	ToolTip::add( saveSettingsBtn, tr( "Save current instrument track settings in a preset file" ) );

	basicControlsLayout->addWidget( saveSettingsBtn, 0, 7 );

	label = new QLabel( tr( "SAVE" ), this );
	label->setStyleSheet( labelStyleSheet );
	basicControlsLayout->addWidget( label, 1, 7);
	basicControlsLayout->setAlignment( label, labelAlignment );

	generalSettingsLayout->addLayout( basicControlsLayout );


	m_tabWidget = new TabWidget( "", this, true, true );
	// "-1" :
	// in "TabWidget::addTab", under "Position tab's window", the widget is
	// moved up by 1 pixel
	m_tabWidget->setMinimumHeight( INSTRUMENT_HEIGHT + GRAPHIC_TAB_HEIGHT - 4 - 1 );


	// create tab-widgets
	m_ssView = new InstrumentSoundShapingView( m_tabWidget );

	// FUNC tab
	QWidget* instrumentFunctions = new QWidget( m_tabWidget );
	QVBoxLayout* instrumentFunctionsLayout = new QVBoxLayout( instrumentFunctions );
	instrumentFunctionsLayout->setMargin( 5 );
	m_noteStackingView = new InstrumentFunctionNoteStackingView( &m_track->m_noteStacking );
	m_arpeggioView = new InstrumentFunctionArpeggioView( &m_track->m_arpeggio );

	instrumentFunctionsLayout->addWidget( m_noteStackingView );
	instrumentFunctionsLayout->addWidget( m_arpeggioView );
	instrumentFunctionsLayout->addStretch();

	// MIDI tab
	m_midiView = new InstrumentMidiIOView( m_tabWidget );

	// FX tab
	m_effectView = new EffectRackView( m_track->m_audioPort.effects(), m_tabWidget );

	// MISC tab
	m_miscView = new InstrumentMiscView( m_track, m_tabWidget );


	m_tabWidget->addTab( m_ssView, tr( "Envelope, filter & LFO" ), "env_lfo_tab", 1 );
	m_tabWidget->addTab( instrumentFunctions, tr( "Chord stacking & arpeggio" ), "func_tab", 2 );
	m_tabWidget->addTab( m_effectView, tr( "Effects" ), "fx_tab", 3 );
	m_tabWidget->addTab( m_midiView, tr( "MIDI" ), "midi_tab", 4 );
	m_tabWidget->addTab( m_miscView, tr( "Miscellaneous" ), "misc_tab", 5 );
	adjustTabSize(m_ssView);
	adjustTabSize(instrumentFunctions);
	m_effectView->resize(EffectRackView::DEFAULT_WIDTH, INSTRUMENT_HEIGHT - 4 - 1);
	adjustTabSize(m_midiView);
	adjustTabSize(m_miscView);

	// setup piano-widget
	m_pianoView = new PianoView( this );
	m_pianoView->setMinimumHeight( PIANO_HEIGHT );
	m_pianoView->setMaximumHeight( PIANO_HEIGHT );

	vlayout->addWidget( generalSettingsWidget );
	// Use QWidgetItem explicitly to make the size hint change on instrument changes
	// QLayout::addWidget() uses QWidgetItemV2 with size hint caching
	vlayout->insertItem(1, new QWidgetItem(m_tabWidget));
	vlayout->addWidget( m_pianoView );
	setModel( _itv->model() );

	QMdiSubWindow* subWin = getGUI()->mainWindow()->addWindowedWidget( this );
	Qt::WindowFlags flags = subWin->windowFlags();
	flags |= Qt::MSWindowsFixedSizeDialogHint;
	flags &= ~Qt::WindowMaximizeButtonHint;
	subWin->setWindowFlags( flags );

	updateInstrumentView();

	// Hide the Size and Maximize options from the system menu
	// since the dialog size is fixed.
	QMenu * systemMenu = subWin->systemMenu();
	systemMenu->actions().at( 2 )->setVisible( false ); // Size
	systemMenu->actions().at( 4 )->setVisible( false ); // Maximize

	subWin->setWindowIcon( embed::getIconPixmap( "instrument_track" ) );
	subWin->setMinimumSize( subWin->size() );
	subWin->hide();
}




InstrumentTrackWindow::~InstrumentTrackWindow()
{
	delete m_instrumentView;

	if (parentWidget())
	{
		parentWidget()->hide();
		parentWidget()->deleteLater();
	}
}




void InstrumentTrackWindow::setInstrumentTrackView( InstrumentTrackView* view )
{
	if( m_itv && view )
	{
		m_itv->m_tlb->setChecked( false );
	}

	m_itv = view;
	m_mixerChannelNumber->setTrackView(m_itv);
}




void InstrumentTrackWindow::modelChanged()
{
	m_track = castModel<InstrumentTrack>();

	m_nameLineEdit->setText( m_track->name() );

	m_track->disconnect( SIGNAL( nameChanged() ), this );
	m_track->disconnect( SIGNAL( instrumentChanged() ), this );

	connect( m_track, SIGNAL( nameChanged() ),
			this, SLOT( updateName() ) );
	connect( m_track, SIGNAL( instrumentChanged() ),
			this, SLOT( updateInstrumentView() ) );

	m_volumeKnob->setModel( &m_track->m_volumeModel );
	m_panningKnob->setModel( &m_track->m_panningModel );
	m_mixerChannelNumber->setModel( &m_track->m_mixerChannelModel );
	m_pianoView->setModel( &m_track->m_piano );

	if( m_track->instrument() && m_track->instrument()->flags().testFlag( Instrument::IsNotBendable ) == false )
	{
		m_pitchKnob->setModel( &m_track->m_pitchModel );
		m_pitchRangeSpinBox->setModel( &m_track->m_pitchRangeModel );
		m_pitchKnob->show();
		m_pitchLabel->show();
		m_pitchRangeSpinBox->show();
		m_pitchRangeLabel->show();
	}
	else
	{
		m_pitchKnob->hide();
		m_pitchLabel->hide();
		m_pitchKnob->setModel( nullptr );
		m_pitchRangeSpinBox->hide();
		m_pitchRangeLabel->hide();
	}

	if (m_track->instrument() && m_track->instrument()->flags().testFlag(Instrument::IsMidiBased))
	{
		m_miscView->microtunerGroupBox()->hide();
		m_track->m_microtuner.enabledModel()->setValue(false);
	}
	else
	{
		m_miscView->microtunerGroupBox()->show();
	}

	m_ssView->setModel( &m_track->m_soundShaping );
	m_noteStackingView->setModel( &m_track->m_noteStacking );
	m_arpeggioView->setModel( &m_track->m_arpeggio );
	m_midiView->setModel( &m_track->m_midiPort );
	m_effectView->setModel( m_track->m_audioPort.effects() );
	m_miscView->pitchGroupBox()->setModel(&m_track->m_useMasterPitchModel);
	m_miscView->microtunerGroupBox()->setModel(m_track->m_microtuner.enabledModel());
	m_miscView->scaleCombo()->setModel(m_track->m_microtuner.scaleModel());
	m_miscView->keymapCombo()->setModel(m_track->m_microtuner.keymapModel());
	m_miscView->rangeImportCheckbox()->setModel(m_track->m_microtuner.keyRangeImportModel());
	updateName();
}




void InstrumentTrackWindow::saveSettingsBtnClicked()
{
	FileDialog sfd(this, tr("Save preset"), "", tr("XML preset file (*.xpf)"));

	QString presetRoot = ConfigManager::inst()->userPresetsDir();
	if(!QDir(presetRoot).exists())
	{
		QDir().mkdir(presetRoot);
	}
	if(!QDir(presetRoot + m_track->instrumentName()).exists())
	{
		QDir(presetRoot).mkdir(m_track->instrumentName());
	}

	sfd.setAcceptMode(FileDialog::AcceptSave);
	sfd.setDirectory(presetRoot + m_track->instrumentName());
	sfd.setFileMode( FileDialog::AnyFile );
	QString fname = m_track->name();
	sfd.selectFile(fname.remove(QRegExp(FILENAME_FILTER)));
	sfd.setDefaultSuffix( "xpf");

	if( sfd.exec() == QDialog::Accepted &&
		!sfd.selectedFiles().isEmpty() &&
		!sfd.selectedFiles().first().isEmpty() )
	{
		DataFile dataFile(DataFile::InstrumentTrackSettings);
		QDomElement& content(dataFile.content());

		m_track->setSimpleSerializing();
		m_track->saveSettings(dataFile, content);
		//We don't want to save muted & solo settings when we're saving a preset
		content.setAttribute("muted", 0);
		content.setAttribute("solo", 0);
		content.setAttribute("mutedBeforeSolo", 0);
		QString f = sfd.selectedFiles()[0];
		dataFile.writeFile(f);
	}
}





void InstrumentTrackWindow::updateName()
{
	setWindowTitle( m_track->name().length() > 25 ? ( m_track->name().left(24)+"..." ) : m_track->name() );

	if( m_nameLineEdit->text() != m_track->name() )
	{
		m_nameLineEdit->setText( m_track->name() );
	}
}





void InstrumentTrackWindow::updateInstrumentView()
{
	delete m_instrumentView;
	if( m_track->m_instrument != nullptr )
	{
		m_instrumentView = m_track->m_instrument->createView( m_tabWidget );
		m_tabWidget->addTab( m_instrumentView, tr( "Plugin" ), "plugin_tab", 0 );
		m_tabWidget->setActiveTab( 0 );

		m_ssView->setFunctionsHidden( m_track->m_instrument->flags().testFlag( Instrument::IsSingleStreamed ) );

		modelChanged(); 		// Get the instrument window to refresh
		m_track->dataChanged(); // Get the text on the trackButton to change

		adjustTabSize(m_instrumentView);
		m_pianoView->setVisible(m_track->m_instrument->hasNoteInput());
		// adjust window size
		layout()->invalidate();
		resize(sizeHint());
		if (parentWidget())
		{
			parentWidget()->resize(parentWidget()->sizeHint());
		}
		update();
		m_instrumentView->update();
	}
}




void InstrumentTrackWindow::textChanged( const QString& newName )
{
	m_track->setName( newName );
	Engine::getSong()->setModified();
}




void InstrumentTrackWindow::toggleVisibility( bool on )
{
	if( on )
	{
		show();
		parentWidget()->show();
		parentWidget()->raise();
	}
	else
	{
		parentWidget()->hide();
	}
}




void InstrumentTrackWindow::closeEvent( QCloseEvent* event )
{
	event->ignore();

	if( getGUI()->mainWindow()->workspace() )
	{
		parentWidget()->hide();
	}
	else
	{
		hide();
	}

	m_itv->m_tlb->setFocus();
	m_itv->m_tlb->setChecked( false );
}




void InstrumentTrackWindow::focusInEvent( QFocusEvent* )
{
	if(m_pianoView->isVisible()) {
		m_pianoView->setFocus();
	}
}




void InstrumentTrackWindow::dragEnterEventGeneric( QDragEnterEvent* event )
{
	StringPairDrag::processDragEnterEvent( event, "instrument,presetfile,pluginpresetfile" );
}




void InstrumentTrackWindow::dragEnterEvent( QDragEnterEvent* event )
{
	dragEnterEventGeneric( event );
}




void InstrumentTrackWindow::dropEvent( QDropEvent* event )
{
	QString type = StringPairDrag::decodeKey( event );
	QString value = StringPairDrag::decodeValue( event );

	if( type == "instrument" )
	{
		m_track->loadInstrument( value, nullptr, true /* DnD */ );

		Engine::getSong()->setModified();

		event->accept();
		setFocus();
	}
	else if( type == "presetfile" )
	{
		DataFile dataFile(value);
		m_track->replaceInstrument(dataFile);
		event->accept();
		setFocus();
	}
	else if( type == "pluginpresetfile" )
	{
		const QString ext = FileItem::extension( value );
		Instrument * i = m_track->instrument();

		if( !i->descriptor()->supportsFileType( ext ) )
		{
			PluginFactory::PluginInfoAndKey piakn =
				getPluginFactory()->pluginSupportingExtension(ext);
			i = m_track->loadInstrument(piakn.info.name(), &piakn.key);
		}

		i->loadFile( value );

		event->accept();
		setFocus();
	}
}




void InstrumentTrackWindow::saveSettings( QDomDocument& doc, QDomElement & thisElement )
{
	thisElement.setAttribute( "tab", m_tabWidget->activeTab() );
	MainWindow::saveWidgetState( this, thisElement );
}




void InstrumentTrackWindow::loadSettings( const QDomElement& thisElement )
{
	m_tabWidget->setActiveTab( thisElement.attribute( "tab" ).toInt() );
	MainWindow::restoreWidgetState( this, thisElement );
	if( isVisible() )
	{
		m_itv->m_tlb->setChecked( true );
	}
}

void InstrumentTrackWindow::viewInstrumentInDirection(int d)
{
	// helper routine for viewNextInstrument, viewPrevInstrument
	// d=-1 to view the previous instrument,
	// d=+1 to view the next instrument

	const QList<TrackView *> &trackViews = m_itv->trackContainerView()->trackViews();
	int idxOfMe = trackViews.indexOf(m_itv);

	// search for the next InstrumentTrackView (i.e. skip AutomationViews, etc)
	// sometimes, the next InstrumentTrackView may already be open, in which case
	//   replace our window contents with the *next* closed Instrument Track and
	//   give focus to the InstrumentTrackView we skipped.
	int idxOfNext = idxOfMe;
	InstrumentTrackView *newView = nullptr;
	InstrumentTrackView *bringToFront = nullptr;
	do
	{
		idxOfNext = (idxOfNext + d + trackViews.size()) % trackViews.size();
		newView = dynamic_cast<InstrumentTrackView*>(trackViews[idxOfNext]);
		// the window that should be brought to focus is the FIRST InstrumentTrackView that comes after us
		if (bringToFront == nullptr && newView != nullptr)
		{
			bringToFront = newView;
		}
		// if the next instrument doesn't have an active window, then exit loop & load that one into our window.
		if (newView != nullptr && !newView->m_tlb->isChecked())
		{
			break;
		}
	} while (idxOfNext != idxOfMe);

	// avoid reloading the window if there is only one instrument, as that will just change the active tab
	if (idxOfNext != idxOfMe)
	{
		// save current window pos and then hide the window by unchecking its button in the track list
		QPoint curPos = parentWidget()->pos();
		m_itv->m_tlb->setChecked(false);

		// enable the new window by checking its track list button & moving it to where our window just was
		newView->m_tlb->setChecked(true);
		newView->getInstrumentTrackWindow()->parentWidget()->move(curPos);

		// scroll the SongEditor/PatternEditor to make sure the new trackview label is visible
		bringToFront->trackContainerView()->scrollToTrackView(bringToFront);

		// get the instrument window to refresh
		modelChanged();
	}
	Q_ASSERT(bringToFront);
	bringToFront->getInstrumentTrackWindow()->setFocus();
}

void InstrumentTrackWindow::viewNextInstrument()
{
	viewInstrumentInDirection(+1);
}
void InstrumentTrackWindow::viewPrevInstrument()
{
	viewInstrumentInDirection(-1);
}

void InstrumentTrackWindow::adjustTabSize(QWidget *w)
{
	// "-1" :
	// in "TabWidget::addTab", under "Position tab's window", the widget is
	// moved up by 1 pixel
	w->setMinimumSize(INSTRUMENT_WIDTH - 4, INSTRUMENT_HEIGHT - 4 - 1);
}

