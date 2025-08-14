/*
 * Patman.cpp - a GUS-compatible patch instrument plugin
 *
 * Copyright (c) 2007-2008 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
 * Copyright (c) 2009-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "Patman.h"

#include <QDragEnterEvent>
#include <QPainter>
#include <QDomElement>

#include "ConfigManager.h"
#include "endian_handling.h"
#include "Engine.h"
#include "FileDialog.h"
#include "FontHelper.h"
#include "InstrumentTrack.h"
#include "NotePlayHandle.h"
#include "PathUtil.h"
#include "PixmapButton.h"
#include "Song.h"
#include "StringPairDrag.h"
#include "Clipboard.h"

#include "embed.h"

#include "plugin_export.h"

namespace lmms
{


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT patman_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"PatMan",
	QT_TRANSLATE_NOOP( "PluginBrowser",
				"GUS-compatible patch instrument" ),
	"Javier Serrano Polo <jasp00/at/users.sourceforge.net>",
	0x0100,
	Plugin::Type::Instrument,
	new PluginPixmapLoader( "logo" ),
	"pat",
	"patchfile",
	nullptr,
} ;


// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model *m, void * )
{
	return new PatmanInstrument( static_cast<InstrumentTrack *>( m ) );
}

}




PatmanInstrument::PatmanInstrument( InstrumentTrack * _instrument_track ) :
	Instrument( _instrument_track, &patman_plugin_descriptor ),
	m_loopedModel( true, this ),
	m_tunedModel( true, this )
{
}




PatmanInstrument::~PatmanInstrument()
{
	unloadCurrentPatch();
}




void PatmanInstrument::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "src", m_patchFile );
	m_loopedModel.saveSettings( _doc, _this, "looped" );
	m_tunedModel.saveSettings( _doc, _this, "tuned" );
}




void PatmanInstrument::loadSettings( const QDomElement & _this )
{
	setFile( _this.attribute( "src" ), false );
	m_loopedModel.loadSettings( _this, "looped" );
	m_tunedModel.loadSettings( _this, "tuned" );
}




void PatmanInstrument::loadFile( const QString & _file )
{
	setFile( _file );
}




QString PatmanInstrument::nodeName() const
{
	return( patman_plugin_descriptor.name );
}




void PatmanInstrument::playNote( NotePlayHandle * _n,
						SampleFrame* _working_buffer )
{
	if( m_patchFile == "" )
	{
		return;
	}

	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();

	if (!_n->m_pluginData)
	{
		selectSample( _n );
	}
	auto hdata = (handle_data*)_n->m_pluginData;

	float play_freq = hdata->tuned ? _n->frequency() :
						hdata->sample->frequency();

	if (hdata->sample->play(_working_buffer + offset, hdata->state, frames,
					play_freq, m_loopedModel.value() ? Sample::Loop::On : Sample::Loop::Off))
	{
		applyRelease( _working_buffer, _n );
	}
	else
	{
		zeroSampleFrames(_working_buffer, frames + offset);
	}
}




void PatmanInstrument::deleteNotePluginData( NotePlayHandle * _n )
{
	auto hdata = (handle_data*)_n->m_pluginData;
	delete hdata->state;
	delete hdata;
}




void PatmanInstrument::setFile( const QString & _patch_file, bool _rename )
{
	if( _patch_file.size() <= 0 )
	{
		m_patchFile = QString();
		return;
	}

	// is current instrument-track-name equal to previous-filename??
	if( _rename &&
		( instrumentTrack()->name() ==
					QFileInfo( m_patchFile ).fileName() ||
				   	m_patchFile == "" ) )
	{
		// then set it to new one
		instrumentTrack()->setName( PathUtil::cleanName( _patch_file ) );
	}
	// else we don't touch the instrument-track-name, because the user
	// named it self

	m_patchFile = PathUtil::toShortestRelative( _patch_file );
	LoadError error = loadPatch( PathUtil::toAbsolute( _patch_file ) );
	if( error != LoadError::OK )
	{
		printf("Load error\n");
	}

	emit fileChanged();
}




PatmanInstrument::LoadError PatmanInstrument::loadPatch(
						const QString & _filename )
{
	unloadCurrentPatch();

	FILE * fd = fopen( _filename.toUtf8().constData() , "rb" );
	if( !fd )
	{
		perror( "fopen" );
		return( LoadError::Open );
	}

	auto header = std::array<unsigned char, 239>{};

	if (fread(header.data(), 1, 239, fd ) != 239 ||
			(memcmp(header.data(), "GF1PATCH110\0ID#000002", 22)
			&& memcmp(header.data(), "GF1PATCH100\0ID#000002", 22)))
	{
		fclose( fd );
		return( LoadError::NotGUS );
	}

	if( header[82] != 1 && header[82] != 0 )
	{
		fclose( fd );
		return( LoadError::Instruments );
	}

	if( header[151] != 1 && header[151] != 0 )
	{
		fclose( fd );
		return( LoadError::Layers );
	}

	int sample_count = header[198];
	for( int i = 0; i < sample_count; ++i )
	{
		unsigned short tmpshort;

#define SKIP_BYTES( x ) \
		if ( fseek( fd, x, SEEK_CUR ) == -1 ) \
		{ \
			fclose( fd ); \
			return( LoadError::IO ); \
		}

#define READ_SHORT( x ) \
		if ( fread( &tmpshort, 2, 1, fd ) != 1 ) \
		{ \
			fclose( fd ); \
			return( LoadError::IO ); \
		} \
		x = (unsigned short)swap16IfBE( tmpshort );

#define READ_LONG( x ) \
		if ( fread( &x, 4, 1, fd ) != 1 ) \
		{ \
			fclose( fd ); \
			return( LoadError::IO ); \
		} \
		x = (unsigned)swap32IfBE( x );

		// skip wave name, fractions
		SKIP_BYTES( 7 + 1 );
		unsigned data_length;
		READ_LONG( data_length );
		unsigned loop_start;
		READ_LONG( loop_start );
		unsigned loop_end;
		READ_LONG( loop_end );
		unsigned sample_rate;
		READ_SHORT( sample_rate );
		// skip low_freq, high_freq
		SKIP_BYTES( 4 + 4 );
		unsigned root_freq;
		READ_LONG( root_freq );
		// skip tuning, panning, envelope, tremolo, vibrato
		SKIP_BYTES( 2 + 1 + 12 + 3 + 3 );
		unsigned char modes;
		if ( fread( &modes, 1, 1, fd ) != 1 )
		{
			fclose( fd );
			return( LoadError::IO );
		}
		// skip scale frequency, scale factor, reserved space
		SKIP_BYTES( 2 + 2 + 36 );

		f_cnt_t frames;
		std::unique_ptr<sample_t[]> wave_samples;
		if( modes & MODES_16BIT )
		{
			frames = data_length >> 1;
			wave_samples = std::make_unique<sample_t[]>(frames);
			for( f_cnt_t frame = 0; frame < frames; ++frame )
			{
				short sample;
				if ( fread( &sample, 2, 1, fd ) != 1 )
				{
					fclose( fd );
					return( LoadError::IO );
				}
				sample = swap16IfBE( sample );
				if( modes & MODES_UNSIGNED )
				{
					sample ^= 0x8000;
				}
				wave_samples[frame] = sample / 32767.0f;
			}

			loop_start >>= 1;
			loop_end >>= 1;
		}
		else
		{
			frames = data_length;
			wave_samples = std::make_unique<sample_t[]>(frames);
			for( f_cnt_t frame = 0; frame < frames; ++frame )
			{
				char sample;
				if ( fread( &sample, 1, 1, fd ) != 1 )
				{
					fclose( fd );
					return( LoadError::IO );
				}
				if( modes & MODES_UNSIGNED )
				{
					sample ^= 0x80;
				}
				wave_samples[frame] = sample / 127.0f;
			}
		}

		auto data = new SampleFrame[frames];

		for( f_cnt_t frame = 0; frame < frames; ++frame )
		{
			for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS;
									++chnl )
			{
				data[frame][chnl] = wave_samples[frame];
			}
		}

		auto psample = std::make_shared<Sample>(data, frames, sample_rate);
		psample->setFrequency(root_freq / 1000.0f);

		if( modes & MODES_LOOPING )
		{
			psample->setLoopStartFrame( loop_start );
			psample->setLoopEndFrame( loop_end );
		}

		m_patchSamples.push_back(psample);

		delete[] data;
	}
	fclose( fd );
	return( LoadError::OK );
}




void PatmanInstrument::unloadCurrentPatch()
{
	while( !m_patchSamples.empty() )
	{
		m_patchSamples.pop_back();
	}
}




void PatmanInstrument::selectSample( NotePlayHandle * _n )
{
	const float freq = _n->frequency();

	float min_dist = HUGE_VALF;
	std::shared_ptr<Sample> sample = nullptr;

	for (const auto& patchSample : m_patchSamples)
	{
		float patch_freq = patchSample->frequency();
		float dist = freq >= patch_freq ? freq / patch_freq :
							patch_freq / freq;

		if( dist < min_dist )
		{
			min_dist = dist;
			sample = patchSample;
		}
	}

	auto hdata = new handle_data;
	hdata->tuned = m_tunedModel.value();
	hdata->sample = sample ? sample : std::make_shared<Sample>();
	hdata->state = new Sample::PlaybackState(_n->hasDetuningInfo());

	_n->m_pluginData = hdata;
}




gui::PluginView * PatmanInstrument::instantiateView( QWidget * _parent )
{
	return( new gui::PatmanView( this, _parent ) );
}






namespace gui
{


PatmanView::PatmanView( Instrument * _instrument, QWidget * _parent ) :
	InstrumentViewFixedSize( _instrument, _parent ),
	m_pi(castModel<PatmanInstrument>())
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(),
				PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );


	m_openFileButton = new PixmapButton( this, nullptr );
	m_openFileButton->setObjectName( "openFileButton" );
	m_openFileButton->setCursor( QCursor( Qt::PointingHandCursor ) );
	m_openFileButton->move( 227, 86 );
	m_openFileButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"select_file_on" ) );
	m_openFileButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"select_file" ) );
	connect( m_openFileButton, SIGNAL( clicked() ),
				this, SLOT( openFile() ) );
	m_openFileButton->setToolTip(tr("Open patch"));

	m_loopButton = new PixmapButton( this, tr( "Loop" ) );
	m_loopButton->setObjectName("loopButton");
	m_loopButton->setCheckable( true );
	m_loopButton->move( 195, 138 );
	m_loopButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"loop_on" ) );
	m_loopButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
								"loop_off" ) );
	m_loopButton->setToolTip(tr("Loop mode"));

	m_tuneButton = new PixmapButton( this, tr( "Tune" ) );
	m_tuneButton->setObjectName("tuneButton");
	m_tuneButton->setCheckable( true );
	m_tuneButton->move( 223, 138 );
	m_tuneButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"tune_on" ) );
	m_tuneButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
								"tune_off" ) );
	m_tuneButton->setToolTip(tr("Tune mode"));


	if (m_pi->m_patchFile.isEmpty())
	{
		m_displayFilename = tr("No file selected");
	}
	else
	{
		updateFilename();
	}

	setAcceptDrops( true );
}




void PatmanView::openFile()
{
	FileDialog ofd( nullptr, tr( "Open patch file" ) );
	ofd.setFileMode( FileDialog::ExistingFiles );

	QStringList types;
	types << tr( "Patch-Files (*.pat)" );
	ofd.setNameFilters( types );

	if( m_pi->m_patchFile == "" )
	{
		if( QDir( "/usr/share/midi/freepats" ).exists() )
		{
			ofd.setDirectory( "/usr/share/midi/freepats" );
		}
		else
		{
			ofd.setDirectory(
				ConfigManager::inst()->userSamplesDir() );
		}
	}
	else if( QFileInfo( m_pi->m_patchFile ).isRelative() )
	{
		QString f = ConfigManager::inst()->userSamplesDir()
							+ m_pi->m_patchFile;
		if( QFileInfo( f ).exists() == false )
		{
			f = ConfigManager::inst()->factorySamplesDir()
							+ m_pi->m_patchFile;
		}

		ofd.selectFile( f );
	}
	else
	{
		ofd.selectFile( m_pi->m_patchFile );
	}

	if( ofd.exec() == QDialog::Accepted && !ofd.selectedFiles().isEmpty() )
	{
		QString f = ofd.selectedFiles()[0];
		if( f != "" )
		{
			m_pi->setFile( f );
			Engine::getSong()->setModified();
		}
	}
}




void PatmanView::updateFilename()
{
 	m_displayFilename = "";
	int idx = m_pi->m_patchFile.length();

	QFontMetrics fm(adjustedToPixelSize(font(), SMALL_FONT_SIZE));

	// simple algorithm for creating a text from the filename that
	// matches in the white rectangle
	while( idx > 0 && fm.size( Qt::TextSingleLine,
				m_displayFilename + "..." ).width() < 225 )
	{
		m_displayFilename = m_pi->m_patchFile[--idx] +
							m_displayFilename;
	}

	if( idx > 0 )
	{
		m_displayFilename = "..." + m_displayFilename;
	}

	update();
}




void PatmanView::dragEnterEvent(QDragEnterEvent* _dee)
{
	StringPairDrag::processDragEnterEvent(_dee, {"samplefile"});
}




void PatmanView::dropEvent( QDropEvent * _de )
{
	const auto [type, value] = Clipboard::decodeMimeData(_de->mimeData());

	if (type == "samplefile")
	{
		m_pi->setFile(value);
		_de->accept();
		return;
	}

	_de->ignore();
}




void PatmanView::paintEvent( QPaintEvent * )
{
	QPainter p( this );

	p.setFont(adjustedToPixelSize(font(), SMALL_FONT_SIZE));
	p.drawText( 8, 116, 235, 16,
			Qt::AlignLeft | Qt::TextSingleLine | Qt::AlignVCenter,
			m_displayFilename );
}




void PatmanView::modelChanged()
{
	m_pi = castModel<PatmanInstrument>();
	m_loopButton->setModel( &m_pi->m_loopedModel );
	m_tuneButton->setModel( &m_pi->m_tunedModel );
	connect( m_pi, SIGNAL( fileChanged() ),
			this, SLOT( updateFilename() ) );
}


} // namespace gui

} // namespace lmms
