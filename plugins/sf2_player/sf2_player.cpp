/*
 * sf2_player.cpp - a soundfont2 player using fluidSynth
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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


#include <QtGui/QLayout>
#include <QtGui/QLabel>
#include <QtGui/QFileDialog>
#include <Qt/QtXml>

#include "sf2_player.h"
#include "engine.h"
#include "instrument_track.h"
#include "instrument_play_handle.h"
#include "note_play_handle.h"
#include "knob.h"
#include "song.h"
#include "automatable_model_templates.h"

#include "main_window.h"
#include "patches_dialog.h"
#include "tooltip.h"
#include "lcd_spinbox.h"

#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"


extern "C"
{

plugin::descriptor sf2player_plugin_descriptor =
{
	STRINGIFY_PLUGIN_NAME( PLUGIN_NAME ),
	"Sf2Player",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"SoundFont synthesizer" ),
	"Paul Giblock <drfaygo/at/gmail/dot/com>",
	0x0100,
	plugin::Instrument,
	new QPixmap( PLUGIN_NAME::getIconPixmap( "logo" ) ),
	NULL
} ;

}


sf2Instrument::sf2Instrument( instrumentTrack * _instrument_track ) :
	instrument( _instrument_track, &sf2player_plugin_descriptor ),
	m_fontId( 0 ),
	m_filename( "" ),
	m_bankNum( -1, -1, 999, 1, this ),
	m_patchNum( -1, -1, 127, 1, this )
{
	for( int i = 0; i < 128; ++i )
	{
		m_notesRunning[i] = 0;
	} 
	m_settings = new_fluid_settings();

	/* Set the synthesizer settings, if necessary */


	m_synth = new_fluid_synth( m_settings );


	//fluid_settings_setstr(settings, "audio.driver", "jack");

	//adriver = new_fluid_audio_driver(settings, synth);

	instrumentPlayHandle * iph = new instrumentPlayHandle( this );
	engine::getMixer()->addPlayHandle( iph );

	connect( &m_bankNum, SIGNAL( dataChanged() ),
				this, SLOT( updatePatch() ) );

	connect( &m_patchNum, SIGNAL( dataChanged() ),
				this, SLOT( updatePatch() ) );
}




sf2Instrument::~sf2Instrument()
{
    engine::getMixer()->removePlayHandles( getInstrumentTrack() );
    delete_fluid_synth( m_synth );
    delete_fluid_settings( m_settings );
}




void sf2Instrument::saveSettings( QDomDocument & _doc,
							QDomElement & _this )
{
	_this.setAttribute( "src", m_filename );
	m_patchNum.saveSettings( _doc, _this, "patch" );
	m_bankNum.saveSettings( _doc, _this, "bank" );
}




void sf2Instrument::loadSettings( const QDomElement & _this )
{
	openFile( _this.attribute( "src" ) );
	m_patchNum.loadSettings( _this, "patch" );
	m_bankNum.loadSettings( _this, "bank" );
}




QString sf2Instrument::nodeName( void ) const
{
	return( sf2player_plugin_descriptor.name );
}


void sf2Instrument::openFile( const QString & _sf2File )
{
	if( m_filename != "")
	{
		fluid_synth_sfunload( m_synth, m_fontId, TRUE);
	}

	m_fontId = fluid_synth_sfload( m_synth, _sf2File.toLocal8Bit(), TRUE );

	if( m_fontId >= 0)
	{
		m_patchNum.setValue(0);
		m_bankNum.setValue(0);
		m_filename = _sf2File;

		emit fileChanged();
	}
}

void sf2Instrument::updatePatch( void )
{
	printf("update patch\n");
	if( m_bankNum.value() >= 0 && m_patchNum.value() >= 0 ) {
		fluid_synth_program_select( m_synth, 1, m_fontId,
				m_bankNum.value(), m_patchNum.value() );
	}
}
	


void sf2Instrument::playNote( notePlayHandle * _n, bool )
{
	const float LOG440 = 2.64345267649f;

	const f_cnt_t tfp = _n->totalFramesPlayed();

	int midiNote = (int)floor( ( log2( _n->frequency() ) - LOG440 ) * 12+69-58)+0.5;
	// out of range?
	if( midiNote <= 0 || midiNote >= 128 )
	{
		return;
	}

	if ( tfp == 0 )
	{
		_n->m_pluginData = new int( midiNote );
		fluid_synth_noteon( m_synth, 1, midiNote, _n->getVolume() );
		m_notesRunningMutex.lock();
		++m_notesRunning[midiNote];
		m_notesRunningMutex.unlock();
	}
	else if( _n->released() )
	{
        // Doesn't happen with release frames = 0
	}

}




void sf2Instrument::play( bool _try_parallelizing )
{
	const fpp_t frames = engine::getMixer()->framesPerPeriod();

	sampleFrame * buf = new sampleFrame[frames];

	// Assumes stereo and float sample_t

	fluid_synth_write_float( m_synth, frames, buf, 0, 2, buf, 1, 2 );

	getInstrumentTrack()->processAudioBuffer( buf, frames, NULL );

	delete[] buf;
}



void sf2Instrument::deleteNotePluginData( notePlayHandle * _n )
{
	int * midiNote = static_cast<int *>( _n->m_pluginData );
	m_notesRunningMutex.lock();
	const int n = --m_notesRunning[*midiNote];
	m_notesRunningMutex.unlock();
	if( n <= 0 )
	{
		fluid_synth_noteoff( m_synth, 1, *midiNote );
	}

	delete midiNote;
}


pluginView * sf2Instrument::instantiateView( QWidget * _parent )
{
	return( new sf2InstrumentView( this, _parent ) );
}




sf2InstrumentView::sf2InstrumentView( instrument * _instrument,
							QWidget * _parent ) :
	instrumentView( _instrument, _parent )
{
	QVBoxLayout * vl = new QVBoxLayout( this );
	QHBoxLayout * hl = new QHBoxLayout();
	
	// File Button
	m_fileDialogButton = new pixmapButton( this, NULL );
	m_fileDialogButton->setCursor( QCursor( Qt::PointingHandCursor ) );
	m_fileDialogButton->setActiveGraphic( embed::getIconPixmap(
							"project_open_down" ) );
	m_fileDialogButton->setInactiveGraphic( embed::getIconPixmap(
							"project_open" ) );
	connect( m_fileDialogButton, SIGNAL( clicked() ),
				this, SLOT( showFileDialog() ) );
	toolTip::add( m_fileDialogButton, tr( "Open other SoundFont file" ) );

	m_fileDialogButton->setWhatsThis(
		tr( "Click here to open another SF2 file" ) );

	// Patch Button
	m_patchDialogButton = new pixmapButton( this, NULL );
	m_patchDialogButton->setCursor( QCursor( Qt::PointingHandCursor ) );
	m_patchDialogButton->setActiveGraphic( embed::getIconPixmap(
							"track_op_menu" ) );
	m_patchDialogButton->setInactiveGraphic( embed::getIconPixmap(
							"track_op_menu" ) );
	connect( m_patchDialogButton, SIGNAL( clicked() ),
				this, SLOT( showPatchDialog() ) );
	toolTip::add( m_patchDialogButton, tr( "Choose the patch" ) );


	// LCDs
	m_bankNumLcd = new lcdSpinBox( 3, this, "Bank" );
	m_bankNumLcd->setLabel( "Bank:" );
	m_bankNumLcd->addTextForValue( -1, "---" );
	m_bankNumLcd->setEnabled( FALSE );

	m_patchNumLcd = new lcdSpinBox( 3, this, "Patch" );
	m_patchNumLcd->setLabel( "Patch:" );
	m_patchNumLcd->addTextForValue( -1, "---" );
	m_patchNumLcd->setEnabled( FALSE );

	hl->addWidget( m_fileDialogButton );
	hl->addWidget( m_bankNumLcd );
	hl->addWidget( m_patchNumLcd );
	hl->addWidget( m_patchDialogButton );

	vl->addLayout( hl );

	// Next row

	hl = new QHBoxLayout();
	
	m_filenameLabel = new QLabel( this );
	
	hl->addWidget( m_filenameLabel );

	vl->addLayout( hl );

	setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	setPalette( pal );

	updateFilename();
}




sf2InstrumentView::~sf2InstrumentView()
{
}


void sf2InstrumentView::modelChanged( void )
{
	sf2Instrument * k = castModel<sf2Instrument>();
	m_bankNumLcd->setModel( &k->m_bankNum );
	m_patchNumLcd->setModel( &k->m_patchNum );

	connect(k, SIGNAL( fileChanged( void ) ),
		this, SLOT( updateFilename( void ) ) );
	
	updateFilename();

}


void sf2InstrumentView::updateFilename( void )
{
	m_filenameLabel->setText("File: " + 
		castModel<sf2Instrument>()->m_filename +
		"\nPatch: TODO");
	update();
}


void sf2InstrumentView::showFileDialog( void )
{
	sf2Instrument * k = castModel<sf2Instrument>();

	QFileDialog ofd( NULL, tr( "Open SoundFont file" ) );
	ofd.setFileMode( QFileDialog::ExistingFiles );

	QStringList types;
	types << tr( "SoundFont2-Files (*.sf2)" );
	ofd.setFilters( types );

	QString dir;
	if( k->m_filename != "" )
	{
		QString f = k->m_filename;
		if( QFileInfo( f ).isRelative() )
		{
			f = configManager::inst()->userSamplesDir() + f;
			if( QFileInfo( f ).exists() == FALSE )
			{
				f = configManager::inst()->factorySamplesDir() +
								k->m_filename;
			}
		}
		ofd.setDirectory( QFileInfo( f ).absolutePath() );
		ofd.selectFile( QFileInfo( f ).fileName() );
	}
	else
	{
		ofd.setDirectory( configManager::inst()->userSamplesDir() );
	}

	if( ofd.exec() == QDialog::Accepted && !ofd.selectedFiles().isEmpty() )
	{
		QString f = ofd.selectedFiles()[0];
		if( f != "" )
		{
			k->openFile( f );
			engine::getSong()->setModified();
		}
	}


}

patchesDialog * sf2InstrumentView::s_patchDialog = NULL;

void sf2InstrumentView::showPatchDialog( void ) {

	sf2Instrument * k = castModel<sf2Instrument>();

	if( s_patchDialog == NULL ) {
		printf("Creating patchDialog\n");
		s_patchDialog = new patchesDialog(this);
	}
    s_patchDialog->setup( k->m_synth, 1, k->getInstrumentTrack()->name(),
		&k->m_bankNum, &k->m_patchNum );
    
	s_patchDialog->exec();
}


extern "C"
{

// neccessary for getting instance out of shared lib
plugin * lmms_plugin_main( model *, void * _data )
{
	return( new sf2Instrument(
				static_cast<instrumentTrack *>( _data ) ) );
}


}

#include "sf2_player.moc"

