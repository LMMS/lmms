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

#include <QTextStream>

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


// Static map of current sfonts
QMap<QString, sf2Font*> sf2Instrument::s_fonts;

// Static functor to fluid's built-in free function.  Used when we want
// to really delete the soundfont, instead of just changing refCount
int (* sf2Instrument::s_origFree)( fluid_sfont_t * );


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

	// This is just our starting instance of synth.  It is recreated
	// everytime we load a new soundfont.
	m_synth = new_fluid_synth( m_settings );

	// Install our callbacks
	fluid_sfloader_t * pLoader
		= (fluid_sfloader_t *) malloc( sizeof( fluid_sfloader_t ) );

	pLoader->data = (void *) this;
	pLoader->load = sf2Instrument::sfloaderLoad;
	pLoader->free = sf2Instrument::sfloaderFree;
	fluid_synth_add_sfloader( m_synth, pLoader );

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
	emit fileLoading();

	// Remove synth from this fluidSynth (but not necessarily memory)
	if ( m_filename != "" )
	{
		// Recreate synth
		m_synthMutex.lock();
		delete_fluid_synth( m_synth );
		m_synth = new_fluid_synth( m_settings );
		m_synthMutex.unlock();

		// No need to explicitly delete font.  Deleting fluidSynth calls free
		// on all fonts.  This, causes sfloaderFreeFont to run.
	}

	char * sf2Ascii = qstrdup( qPrintable( _sf2File ) );
	m_fontId = fluid_synth_sfload( m_synth, sf2Ascii, TRUE );

	QString sf2Key( sf2Ascii );

	// Add to map, if doesn't exist.  
	// We can't do this in callback because fluid hasn't created the sfont yet
	if( !s_fonts.contains( sf2Key ) && fluid_synth_sfcount( m_synth ) > 0 )
	{
		// Grab this sf from the top of the stack
		fluid_sfont_t * sfont = fluid_synth_get_sfont( m_synth, 0 );

		// Hold on to real free function for executing later.
		s_origFree = sfont->free;
		sfont->free = sf2Instrument::sfloaderFreeFont;

		// Finally, add font
		s_fonts.insert( sf2Key, new sf2Font( sfont ) );
	}

	if( m_fontId >= 0 )
	{
		m_patchNum.setValue( 0 );
		m_bankNum.setValue( 0 );
		m_filename = _sf2File;

		emit fileChanged();
	}

	delete[] sf2Ascii;
}



void sf2Instrument::updatePatch( void )
{
	if( m_bankNum.value() >= 0 && m_patchNum.value() >= 0 ) {
		fluid_synth_program_select( m_synth, 1, m_fontId,
				m_bankNum.value(), m_patchNum.value() );
	}
}



void sf2Instrument::playNote( notePlayHandle * _n, bool )
{
	const float LOG440 = 2.64345267649f;

	const f_cnt_t tfp = _n->totalFramesPlayed();

	int midiNote = (int)floor( 12 * ( log2( _n->frequency() ) - LOG440 ) - 4 );

	// out of range?
	if( midiNote <= 0 || midiNote >= 128 )
	{
		return;
	}

	if ( tfp == 0 )
	{
		_n->m_pluginData = new int( midiNote );

		m_synthMutex.lock();
		fluid_synth_noteon( m_synth, 1, midiNote, _n->getVolume() );
		m_synthMutex.unlock();

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

	m_synthMutex.lock();
	fluid_synth_write_float( m_synth, frames, buf, 0, 2, buf, 1, 2 );
	m_synthMutex.unlock();

	getInstrumentTrack()->processAudioBuffer( buf, frames, NULL );

	delete[] buf;
}



int sf2Instrument::sfloaderFree( fluid_sfloader_t * _loader )
{
	if( _loader != NULL )
	{
		free( _loader );
	}
	return 0;
}



int sf2Instrument::sfloaderFreeFont( fluid_sfont_t * _sfont )
{
	QString key = QString( _sfont->get_name(_sfont) );

	sf2Font * f = sf2Instrument::s_fonts[key];

	--(f->refCount);

	// No more references
	if( f->refCount <= 0 )
	{
		QTextStream cout( stdout, QIODevice::WriteOnly );
		cout << "Really deleting " << key << endl;

		sf2Instrument::s_fonts.remove( key );
		delete f;

		// Let fluidsynth free the font, as if we were not here
		return s_origFree( _sfont );
	}

	// Make fluid think we freed the sfont
	return 0;
}



fluid_sfont_t * sf2Instrument::sfloaderLoad(
		fluid_sfloader_t * _loader, const char *_filename )
{
	if( _loader == NULL )
	{
		return NULL;
	}

	QString filename = _filename;

	if( sf2Instrument::s_fonts.contains( filename ) )
	{
		QTextStream cout( stdout, QIODevice::WriteOnly );
		cout << "Using existing reference to " << filename << endl;

		sf2Font * font = sf2Instrument::s_fonts[ filename ];

		font->refCount++;
		return font->fluidFont;
	}

	// fluidsynth will call next (default) loader...
	return NULL;
}



void sf2Instrument::deleteNotePluginData( notePlayHandle * _n )
{
	int * midiNote = static_cast<int *>( _n->m_pluginData );
	m_notesRunningMutex.lock();
	const int n = --m_notesRunning[*midiNote];
	m_notesRunningMutex.unlock();
	if( n <= 0 )
	{
		m_synthMutex.lock();
		fluid_synth_noteoff( m_synth, 1, *midiNote );
		m_synthMutex.unlock();
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
	m_patchDialogButton->setEnabled( FALSE );

	connect( m_patchDialogButton, SIGNAL( clicked() ),
			this, SLOT( showPatchDialog() ) );
	toolTip::add( m_patchDialogButton, tr( "Choose the patch" ) );


	// LCDs
	m_bankNumLcd = new lcdSpinBox( 3, this, "Bank" );
	m_bankNumLcd->setLabel( "BANK" );
	m_bankNumLcd->addTextForValue( -1, "---" );
	m_bankNumLcd->setEnabled( FALSE );

	m_patchNumLcd = new lcdSpinBox( 3, this, "Patch" );
	m_patchNumLcd->setLabel( "PATCH" );
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

	connect(k, SIGNAL( fileLoading( void ) ),
			this, SLOT( invalidateFile( void ) ) );

	updateFilename();

}



void sf2InstrumentView::updateFilename( void )
{
	sf2Instrument * i = castModel<sf2Instrument>();
	m_filenameLabel->setText( "File: " +
		i->m_filename +
		"\nPatch: TODO" );

	m_patchDialogButton->setEnabled( !i->m_filename.isEmpty() );

	update();
}

void sf2InstrumentView::invalidateFile( void )
{
	m_patchDialogButton->setEnabled( false );
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

	m_fileDialogButton->setEnabled( false );
	
	if( ofd.exec() == QDialog::Accepted && !ofd.selectedFiles().isEmpty() )
	{
		QString f = ofd.selectedFiles()[0];
		if( f != "" )
		{
			k->openFile( f );
			engine::getSong()->setModified();
		}
	}

	m_fileDialogButton->setEnabled( true );
}



// Single instance of the patch dialog
patchesDialog * sf2InstrumentView::s_patchDialog = NULL;

void sf2InstrumentView::showPatchDialog( void ) {

	sf2Instrument * k = castModel<sf2Instrument>();

	if( s_patchDialog == NULL )
	{
		s_patchDialog = new patchesDialog( this );
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

