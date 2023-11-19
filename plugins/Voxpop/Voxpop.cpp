/*
 * Voxpop.cpp - instrument for playing synchronized voice samples
 *
 * Copyright (c) 2023 teknopaul <teknopaul/at/fastmail.es>
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

#include "Voxpop.h"

#include <cmath>

#include <QPainter>
#include <QFileInfo>
#include <QDropEvent>
#include <QLabel>
#include <QTextStream>

#include <samplerate.h>

#include "AudioEngine.h"
#include "ComboBox.h"
#include "ConfigManager.h"
#include "DataFile.h"
#include "Engine.h"
#include "FileDialog.h"
#include "gui_templates.h"
#include "InstrumentTrack.h"
#include "LcdSpinBox.h"
#include "LedCheckBox.h"
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

Plugin::Descriptor PLUGIN_EXPORT voxpop_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"Voxpop",
	QT_TRANSLATE_NOOP( "PluginBrowser",
				"Voice sampler for "
				"using samples with a queue sheet "
				"*.txt file" ),
	"Teknopaul <teknopaul/at/fastmail.es>",
	0x0100,
	Plugin::Type::Instrument,
	new PluginPixmapLoader( "logo" ),
	"wav,ogg,ds,spx,au,voc,aif,aiff,flac,raw"
#ifdef LMMS_HAVE_SNDFILE_MP3
	",mp3"
#endif
	,
	nullptr,
} ;

}

Voxpop::Voxpop( InstrumentTrack * _instrument_track ) :
	Instrument( _instrument_track, &voxpop_plugin_descriptor ),

	m_mode( CueSelectionMode::Automation ),
	m_respectEndpointModel( true ),
	m_ampModel( 100, 0, 500, 1, this, tr( "Amplify" ) ),
	m_cueIndexModel( 0, 0, 99 , this, tr("Cue index") ),
	m_stutterModel( false, this, tr( "Stutter" ) ),
	m_interpolationModel( this, tr( "Interpolation mode" ) ),
	m_modeModel( this, tr( "Mode" )),

	m_audioFile(""),
	m_cuesheetFile(""),
	m_cueCount( 0 ),
	m_sampleBuffer(),
	m_sampleBuffers( 0 ),
	m_cueTexts( 0 ),
	m_cueOffsets( 0 ),
	m_nextPlayStartPoint( 0 )
{
	connect( &m_ampModel, SIGNAL( dataChanged() ),
				this, SLOT( ampModelChanged() ), Qt::DirectConnection );
	connect( &m_stutterModel, SIGNAL( dataChanged() ),
				this, SLOT( stutterModelChanged() ), Qt::DirectConnection );
	connect( &m_cueIndexModel, SIGNAL( dataChanged() ),
				this, SLOT( cueIndexChanged() ), Qt::DirectConnection );
	connect( &m_modeModel, SIGNAL( dataChanged() ),
				this, SLOT( modeChanged() ), Qt::DirectConnection );

//interpolation modes
	m_interpolationModel.addItem( tr( "None" ) );
	m_interpolationModel.addItem( tr( "Linear" ) );
	m_interpolationModel.addItem( tr( "Sinc" ) );
	m_interpolationModel.setValue( 1 );

	m_modeModel.addItem( tr( "Automation" ) );
	m_modeModel.addItem( tr( "Piano" ) );
	m_modeModel.addItem( tr( "Sequential" ) );
	m_modeModel.setValue( 0 );

}




void Voxpop::playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer )
{
	// Magic key - idea copied from AudioFileProcessor
	if ( m_stutterModel.value() == true && _n->frequency() < 20.0 )
	{
		resetStutter();
		return;
	}

	if (m_cueCount == 0)
	{
		return;
	}

	int cue = 0;
	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();

	if ( !_n->m_pluginData )
	{
		switch( m_mode )
		{
			case CueSelectionMode::Piano :
				cue = _n->key() % m_cueCount;
				break;
			case CueSelectionMode::Automation :
				cue = m_cueIndexModel.value();
				break;
			case CueSelectionMode::Sequential :
				cue = m_cueIndexModel.value();
				if (cue + 1 < m_cueCount - 1)
				{
					m_cueIndexModel.setValue(cue + 1);
				}
				else {
					m_cueIndexModel.setValue(0);
				}
		}

		if ( m_stutterModel.value() == true && m_nextPlayStartPoint[cue] >= m_sampleBuffers[cue]->endFrame() )
		{
			// Restart playing the note if in stutter mode and we're at the end of the sample.
			m_nextPlayStartPoint[cue] = m_sampleBuffers[cue]->startFrame() + m_cueOffsets[cue];
		}

		int srcMode = SRC_LINEAR;
		switch( m_interpolationModel.value() )
		{
			case 0:
				srcMode = SRC_ZERO_ORDER_HOLD;
				break;
			case 1:
				srcMode = SRC_LINEAR;
				break;
			case 2:
				srcMode = SRC_SINC_MEDIUM_QUALITY;
				break;
		}
		_n->m_pluginData = new VoxpopHandleState(cue, _n->hasDetuningInfo(), srcMode );
		((handleState *)_n->m_pluginData)->setFrameIndex( m_nextPlayStartPoint[cue] );

	}
	else
	{
		cue = ((VoxpopHandleState *)_n->m_pluginData)->cue;
	}

	if ( ! _n->isFinished() )
	{
		float freq = 440.f;
		if ( m_mode == CueSelectionMode::Automation)
		{
			freq = _n->frequency();
		}

		if ( m_sampleBuffers[cue]->play( _working_buffer + offset,
						(handleState *)_n->m_pluginData,
						frames, freq, SampleBuffer::LoopMode::Off ) )
		{
			applyRelease( _working_buffer, _n );
		}
		else
		{
			memset( _working_buffer, 0, ( frames + offset ) * sizeof( sampleFrame ) );
		}
	}

	if ( m_stutterModel.value() == true )
	{
		m_nextPlayStartPoint[cue] = ((handleState *)_n->m_pluginData)->frameIndex();
	}
}




void Voxpop::deleteNotePluginData( NotePlayHandle * _n )
{
	delete (handleState *)_n->m_pluginData;
}




void Voxpop::saveSettings(QDomDocument& doc, QDomElement& elem)
{
	m_cueIndexModel.saveSettings(doc, elem, "cueindex");
	m_ampModel.saveSettings(doc, elem, "amp");
	m_stutterModel.saveSettings(doc, elem, "stutter");
	m_interpolationModel.saveSettings(doc, elem, "interp");
	m_modeModel.saveSettings(doc, elem, "mode");
	m_cueIndexModel.saveSettings(doc, elem, "cueindex");
	if (m_cueCount > 0)
	{
		elem.setAttribute("audio", m_audioFile);
		elem.setAttribute("cuesheet", m_cuesheetFile);
	}
	m_respectEndpointModel.saveSettings(doc, elem, "respectendpoint");
}




void Voxpop::loadSettings(const QDomElement& elem)
{
	QString src = elem.attribute("cuesheet");
	if ( !elem.attribute("cuesheet").isEmpty() )
	{
		QString absolutePath = PathUtil::toAbsolute(src);
		if (!QFileInfo(absolutePath).exists())
		{
			QString message = tr("Cue sheet not found: %1").arg(absolutePath);
			Engine::getSong()->collectError(message);
		}
		else
		{
			setCuesheetFile(src, false);
		}
	}

	src = elem.attribute("audio");
	if ( !src.isEmpty() )
	{
		QString absolutePath = PathUtil::toAbsolute(src);
		if (!QFileInfo(absolutePath).exists())
		{
			QString message = tr("Sample not found: %1").arg(absolutePath);
			Engine::getSong()->collectError(message);
		}
		else
		{
			setAudioFile(src, false);
		}
	}

	m_ampModel.loadSettings(elem, "amp");
	m_stutterModel.loadSettings(elem, "stutter");
	if (elem.hasAttribute("interp") || !elem.firstChildElement("interp").isNull())
	{
		m_interpolationModel.loadSettings(elem, "interp");
	}
	else
	{
		m_interpolationModel.setValue(1.0f); // linear by default
	}
	if (elem.hasAttribute("mode") || !elem.firstChildElement("mode").isNull())
	{
		m_modeModel.loadSettings(elem, "mode");
	}
	else
	{
		m_modeModel.setValue(1.0f); // Automation bty default
	}
	m_respectEndpointModel.loadSettings(elem, "respectendpoint");
	modeChanged();
	if (m_cueCount > 0)
	{
		m_cueIndexModel.loadSettings(elem, "cueindex");
		cueIndexChanged();
	}
}




void Voxpop::loadAudioFile( const QString & _file )
{
	setAudioFile( _file );
}


void Voxpop::loadCuesheetFile( const QString & _file )
{
	setCuesheetFile( _file );
}


QString Voxpop::nodeName() const
{
	return voxpop_plugin_descriptor.name;
}



auto Voxpop::beatLen(NotePlayHandle* note) const -> int
{
	// If we can play indefinitely, use the default beat note duration
	return 0;
}



gui::PluginView* Voxpop::instantiateView( QWidget * _parent )
{
	return new gui::VoxpopView( this, _parent );
}



void Voxpop::setAudioFile( const QString & _audioFile, bool _rename )
{
	if ( _rename )
	{
		instrumentTrack()->setName( PathUtil::cleanName( _audioFile ) );
	}
	m_sampleBuffer.setAudioFile(_audioFile);
	m_sampleBuffer.setAllPointFrames( 0, m_sampleBuffer.frames(), 0, m_sampleBuffer.frames() );
	m_audioFile = _audioFile;
	reloadCuesheet();
}


void Voxpop::setCuesheetFile( const QString & _cuesheetFile, bool _rename )
{
	m_cuesheetFile = _cuesheetFile;
	reloadCuesheet();
}


void Voxpop::ampModelChanged()
{
	m_sampleBuffer.setAmplification( m_ampModel.value() / 100.0f );
	if ( m_cueCount > 0)
	{
		for (int i = 1 ; i < m_cueCount ; i++ )
		{
			m_sampleBuffers[i]->setAmplification( m_ampModel.value() / 100.0f );
		}
	}
}

void Voxpop::stutterModelChanged()
{
	for (int i = 0 ; i < m_cueCount ; i++ )
	{
		m_nextPlayStartPoint[i] = m_cueOffsets[i];
	}
}


void Voxpop::cueIndexChanged()
{
	if (m_cueCount > 0 && m_cueTexts.size() == m_cueCount)
	{
		int cue = m_cueIndexModel.value();
		emit cueChanged(cue, m_cueTexts[cue]);
	}
}

void Voxpop::modeChanged()
{
	m_mode = static_cast<Voxpop::CueSelectionMode>(m_modeModel.value());
}

f_cnt_t Voxpop::cuePointToFrames(QString field)
{
	return m_sampleBuffer.sampleRate() * field.toDouble();
}


bool Voxpop::reloadCuesheet()
{
	// TODO memory leaks, clean up needed first

	if ( !m_cuesheetFile.isEmpty() && ! m_audioFile.isEmpty() )
	{
		QString cuesheetPath = PathUtil::toAbsolute(m_cuesheetFile);
		if (!QFileInfo(cuesheetPath).exists())
		{
			return false;
		}
		QString audioPath = PathUtil::toAbsolute(m_audioFile);
		if (!QFileInfo(audioPath).exists())
		{
			return false;
		}

		// TODO how to check if a sample really loaded properly??
		QFile cueFile(cuesheetPath);
		if ( !cueFile.open(QFile::ReadOnly | QFile::Text)) 
		{
			return false;
		}
		QStringList quePoints;
		QTextStream in(&cueFile);
		QString line;
		while ( in.readLineInto(&line) )
		{
			if ( line.startsWith("#") ) continue;
			if ( line.trimmed() == "" ) continue;
			if ( line.split( "\t" ).length() == 3)
			{
				quePoints << line;
			}
		}
		if (quePoints.length() > VOXPOP_MAX_CUES)
		{
			qWarning("too many queue points");
			return false;
		}
		
		if ( quePoints.length() > 0 )
		{
			// dont play during relaod of samples
			Engine::getSong()->stop();

			int oldCueCount = m_cueCount;
			int newCueCount = quePoints.length();
			bool respectEndpoint = m_respectEndpointModel.value();

			deleteSamples(oldCueCount);

			f_cnt_t veryEndFrame = 0;
			for ( int i = 0 ; i < newCueCount ; i++ )
			{
				QStringList fields = quePoints[i].split( "\t" );
				m_cueTexts.push_back( new QString( fields[2].trimmed() ) );
				f_cnt_t startFrame = cuePointToFrames( fields[0] );
				f_cnt_t endFrame = respectEndpoint ? cuePointToFrames( fields[1] ) : m_sampleBuffer.frames();
				m_cueOffsets.push_back( startFrame );
				m_nextPlayStartPoint.push_back( startFrame );
				m_sampleBuffers.push_back(  new SampleBuffer() );
				m_sampleBuffers.at(i)->setAudioFile( audioPath ) ;
				m_sampleBuffers.at(i)->setAmplification( m_ampModel.value() / 100.0f );
				m_sampleBuffers.at(i)->setAllPointFrames( startFrame, endFrame, startFrame, endFrame );
				veryEndFrame = endFrame > veryEndFrame ? endFrame : veryEndFrame;
			}
			m_sampleBuffer.setAllPointFrames( 0, veryEndFrame, 0, 0 );

			m_cueIndexModel.setValue(0);
			m_cueIndexModel.setRange(0, newCueCount - 1);
			m_cueCount = newCueCount;
			return true;
		}
	}
	return false;
}

void Voxpop::deleteSamples(int count)
{
	// make sure View does not have dangling popinters
	emit cueChanged(-1, (QString *) &VOXPOP_DEFAULT_TEXT);

	m_cueCount = 0; // TODO race conditions?
	for ( int i = count - 1 ; i >= 0 ; i-- )
	{
		delete m_cueTexts.at(i);
		m_cueTexts.pop_back();
		delete m_sampleBuffers.at(i); // TODO memory leak
		m_sampleBuffers.pop_back();
	}
	m_cueOffsets.clear();
	m_nextPlayStartPoint.clear();
}

namespace gui
{


QPixmap * VoxpopView::s_artwork = nullptr;


VoxpopView::VoxpopView( Instrument * _instrument, QWidget * _parent ) :
	InstrumentViewFixedSize( _instrument, _parent )
{
	if ( s_artwork == nullptr )
	{
		s_artwork = new QPixmap( PLUGIN_NAME::getIconPixmap( "artwork" ) );
	}
	m_cuelabel = (QString *) &VOXPOP_DEFAULT_TEXT;

	m_respectEnpointsCheckBox = new LedCheckBox(this);
	m_respectEnpointsCheckBox->setToolTip( tr("Respect cue endpoints") );
	m_respectEnpointsCheckBox->move(3, 86);

	m_openAudioFileButton = new PixmapButton( this );
	m_openAudioFileButton->setCursor( QCursor( Qt::PointingHandCursor ) );
	m_openAudioFileButton->move( 227, 102 );
	m_openAudioFileButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "select_file" ) );
	m_openAudioFileButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "select_file" ) );	
	m_openAudioFileButton->setToolTip(tr("Open sample"));
	connect( m_openAudioFileButton, SIGNAL( clicked() ), this, SLOT( openAudioFile() ) );
	
	m_openCuesheetFileButton = new PixmapButton( this );
	m_openCuesheetFileButton->setCursor( QCursor( Qt::PointingHandCursor ) );
	m_openCuesheetFileButton->move( 227, 122 );
	m_openCuesheetFileButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "select_file" ) );
	m_openCuesheetFileButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "select_file" ) );	
	m_openCuesheetFileButton->setToolTip(tr("Open cue sheet"));
	connect( m_openCuesheetFileButton, SIGNAL( clicked() ), this, SLOT( openCuesheetFile() ) );

	int baseline = 145;
	m_ampKnob = new Knob( KnobType::Bright26, this );
	m_ampKnob->setVolumeKnob( true );
	m_ampKnob->move( 3, baseline - 2 );
	m_ampKnob->setHintText( tr( "Amplify:" ), "%" );

	m_cueIndexControl = new LcdSpinBox( 2, this, "cue index" );
	m_cueIndexControl->move( 35, baseline );

	m_stutterButton = new PixmapButton( this );
	m_stutterButton->setCheckable( true );
	m_stutterButton->move( 72, baseline + 1);
	m_stutterButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap( "stutter_on" ) );
	m_stutterButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "stutter_off" ) );
	m_stutterButton->setToolTip( tr( "Continue sample playback across notes" ) );

	m_modeBox = new ComboBox( this );
	m_modeBox->setGeometry( 96, baseline, 87, ComboBox::DEFAULT_HEIGHT );
	m_modeBox->setFont( pointSize<8>( m_modeBox->font() ) );

	m_interpBox = new ComboBox( this );
	m_interpBox->setGeometry( 186, baseline, 60, ComboBox::DEFAULT_HEIGHT );
	m_interpBox->setFont( pointSize<8>( m_interpBox->font() ) );

	m_waveView = 0;
	newWaveView();
	qRegisterMetaType<lmms::f_cnt_t>( "lmms::f_cnt_t" );
}


void VoxpopView::newWaveView()
{
	if ( m_waveView )
	{
		delete m_waveView;
		m_waveView = 0;
	}
	m_waveView = new VoxpopWaveView( this, 245, 75, castModel<Voxpop>()->m_sampleBuffer );
	m_waveView->move( 2, 172 );
	m_waveView->show();
}


void VoxpopView::paintEvent( QPaintEvent * )
{
	QPainter p( this );

	p.drawPixmap( 0, 0, *s_artwork );

	auto a = castModel<Voxpop>();

	QString file_name = "";
	int idx = a->m_audioFile.length();

	p.setFont( pointSize<8>( font() ) );
	p.setPen( QColor( 255, 255, 255 ) );
	
	QFontMetrics fm( p.font() );

	// simple algorithm for creating a text from the filename that
	// matches in the white rectangle
	while( idx > 0 && fm.size( Qt::TextSingleLine, file_name + "..." ).width() < 210 )
	{
		file_name = a->m_audioFile[--idx] + file_name;
	}
	if ( idx > 0 )
	{
		file_name = "..." + file_name;
	}
	p.drawText( 8, 113, file_name );
	
	file_name = "";
	idx = a->m_cuesheetFile.length();
	while( idx > 0 && fm.size( Qt::TextSingleLine, file_name + "..." ).width() < 210 )
	{
		file_name = a->m_cuesheetFile[--idx] + file_name;
	}
	if ( idx > 0 )
	{
		file_name = "..." + file_name;
	}
	p.drawText( 8, 133, file_name );

	file_name = "";
	if (!m_cuelabel->isEmpty())
	{
		p.drawText( 5, 60, file_name + m_cuelabel );
	}

	updateCuePoints();
}


void VoxpopView::sampleUpdated()
{
	m_waveView->update();
	update();
}

void VoxpopView::cueSheetChanged()
{
	m_waveView->update();
}


void VoxpopView::cueChanged(int cue, QString * text)
{
	m_cuelabel = text;
	update();
}


void VoxpopView::openAudioFile()
{
	Voxpop * voxpop = castModel<Voxpop>();
	QString af = voxpop->m_sampleBuffer.openAudioFile();
	if (af.isEmpty())
	{
		return;
	}
	voxpop->setAudioFile(af);
	Engine::getSong()->setModified();
	update();
	m_waveView->update();
}



void VoxpopView::openCuesheetFile()
{
	Voxpop * voxpop = castModel<Voxpop>();

	gui::FileDialog ofd(nullptr, tr("Open cue sheet file"));

	if ( !voxpop->m_audioFile.isEmpty())
	{
		QString absolutePath = PathUtil::toAbsolute(voxpop->m_audioFile);
		QFileInfo afInfo(absolutePath);
		if (afInfo.exists())
		{
			ofd.setDirectory(afInfo.absoluteDir());
		}
	}
	QStringList types;
	types << tr("Text Files (*.txt)");
	types << tr("All Files (*.*)");
	ofd.setNameFilters(types);

	if (ofd.exec() == QDialog::Accepted && !ofd.selectedFiles().isEmpty() && !ofd.selectedFiles()[0].isEmpty() )
	{
		voxpop->setCuesheetFile(ofd.selectedFiles()[0]);
		Engine::getSong()->setModified();
		update();
		m_waveView->update();
	}
}

void VoxpopView::modelChanged()
{
	auto voxpop = castModel<Voxpop>();
	connect( &voxpop->m_sampleBuffer, SIGNAL( sampleUpdated() ), this, SLOT( sampleUpdated() ) );
	m_ampKnob->setModel( &voxpop->m_ampModel );
	m_stutterButton->setModel( &voxpop->m_stutterModel );
	m_interpBox->setModel( &voxpop->m_interpolationModel );
	m_modeBox->setModel( &voxpop->m_modeModel );
	m_cueIndexControl->setModel( &voxpop->m_cueIndexModel );
	m_respectEnpointsCheckBox->setModel( &voxpop->m_respectEndpointModel );
	connect( voxpop, SIGNAL( cueChanged(int, QString *) ), this, SLOT( cueChanged(int, QString *) ) );
	sampleUpdated();
}

void VoxpopView::updateCuePoints()
{
	Voxpop * voxpop = castModel<Voxpop>();
	if (voxpop->m_cueCount < 1)
	{
		return;
	}

	QPainter p( this );
	p.setPen( QColor( 60, 255, 60, 150 ) );

	double graphWidth = 241;
	for ( int i = 0 ; i < voxpop->m_cueCount ; i++)
	{
		int linePx = voxpop->m_cueOffsets[i] * graphWidth / voxpop->m_sampleBuffer.frames();
		p.drawLine( linePx + 4, 173, linePx + 4, 243 );
	}
}


VoxpopWaveView::VoxpopWaveView( QWidget * _parent, int _w, int _h, SampleBuffer& buf ) :
	QWidget( _parent ),
	m_sampleBuffer( buf ),
	m_graph( QPixmap( _w - 2 * s_padding, _h - 2 * s_padding ) ),
	m_from( 0 ),
	m_to( m_sampleBuffer.frames() )
{
	setFixedSize( _w, _h );
	setMouseTracking( true );

	m_graph.fill( Qt::transparent );
	updateGraph();
	update();
}




void VoxpopWaveView::isPlaying( f_cnt_t _current_frame )
{
	update();
}


void VoxpopWaveView::paintEvent( QPaintEvent * _pe )
{
	QPainter p( this );
	p.drawPixmap( s_padding, s_padding, m_graph );
}




void VoxpopWaveView::updateGraph()
{

	if( m_from > m_sampleBuffer.startFrame() )
	{
		m_from = m_sampleBuffer.startFrame();
	}

	if( m_to < m_sampleBuffer.endFrame() )
	{
		m_to = m_sampleBuffer.endFrame();
	}

	m_graph.fill( Qt::transparent );
	QPainter p( &m_graph );
	p.setPen( QColor( 200, 255, 200 ) );

	m_sampleBuffer.visualize(
		p,
		QRect( 0, 0, m_graph.width(), m_graph.height() ),
		m_from, m_to
	);

}

} // namespace gui




extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main(Model * model, void *)
{
	return new Voxpop(static_cast<InstrumentTrack *>(model));
}


}


} // namespace lmms
