/*
 * BounceManager - bouncing clips to wavs/loops for play with audio file processor.
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

#include <QDir>

#include "BounceManager.h"

#include "FileDialog.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "PatternStore.h"
#include "Song.h"

using namespace lmms::gui;

namespace lmms
{


BounceManager::BounceManager() :
	m_qualitySettings(),
	m_oldQualitySettings( Engine::audioEngine()->currentQualitySettings() ),
	m_outputSettings(),
	m_format(),
	m_outputPath()
{
	Engine::audioEngine()->storeAudioDevice();
}

BounceManager::~BounceManager()
{
	Engine::audioEngine()->restoreAudioDevice();  // Also deletes audio dev.
	Engine::audioEngine()->changeQuality( m_oldQualitySettings );
	if ( m_qualitySettings )
	{
		delete m_qualitySettings;
	}
	if ( m_outputSettings )
	{
		delete m_outputSettings;
	}
}

// set the file tou output and the file format settings based on defaults
void BounceManager::setOutputDefaults()
{
	// TODO defaults from settings
	QString defaultDir = QFileInfo( Engine::getSong()->projectFileName() ).absolutePath();
	QString suffix = "wav";

	FileDialog bfd( getGUI()->mainWindow() );
	bfd.setFileMode( FileDialog::AnyFile );
	bfd.setDirectory( defaultDir );
	bfd.setDefaultSuffix( suffix );
	bfd.setAcceptMode( FileDialog::AcceptSave );
	if( bfd.exec() == QDialog::Accepted && !bfd.selectedFiles().isEmpty() &&
					 !bfd.selectedFiles()[0].isEmpty() )
	{
		m_outputPath = bfd.selectedFiles()[0];
	}
	else {
		return;
	}

	// TODO merge output prefs and allow users to configure
	int bitrate = 256;
	int bitdepth = 16;
	OutputSettings::StereoMode stereomode = OutputSettings::StereoMode::Stereo;
	int samplerate = 44100;
	int interpolation = 3;
	int oversampling = 1;


	// TODO memory leak if called twice
	m_qualitySettings =
			new AudioEngine::qualitySettings(
					static_cast<AudioEngine::qualitySettings::Interpolation>(interpolation),
					static_cast<AudioEngine::qualitySettings::Oversampling>(oversampling));

	OutputSettings::BitRateSettings bitRateSettings(bitrate , false);
	m_outputSettings = new OutputSettings(
			samplerate,
			bitRateSettings,
			static_cast<OutputSettings::BitDepth>( bitdepth ),
			stereomode );

}

// set the part of the timeline to export based on selected clips
void BounceManager::setLoopPoints()
{
	Song * song = Engine::getSong();
	int startTick = 0;

	song->getPlayPos().setTicks(startTick);
}

// mute all tracks except those selected
void BounceManager::muteUnsuedTracks()
{

}

void BounceManager::abortProcessing()
{
	if ( m_renderer ) {
		m_renderer->abortProcessing();
	}
	restoreMutedState();
}


// Bounce the clips into a .wav
void BounceManager::renderProject()
{
	Song * song = Engine::getSong();
	song->setExportLoop( true );
	song->setRenderBetweenMarkers( true );
	song->setLoopRenderCount( 1 );
	render( );
}

void BounceManager::render()
{
	m_renderer = new ProjectRenderer(
			*m_qualitySettings,
			*m_outputSettings,
			m_format,
			m_outputPath);

	if ( m_renderer->isReady() )
	{
		m_renderer->startProcessing();
	}
	else
	{
		qDebug( "Renderer failed to acquire a file device!" );
	}
}

// Unmute all tracks that were muted while rendering clips
void BounceManager::restoreMutedState()
{
	while ( !m_muted.empty() )
	{
		Track* restoreTrack = m_muted.back();
		m_muted.pop_back();
		restoreTrack->setMuted( false );
	}
}


} // namespace lmms
