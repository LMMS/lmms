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
#include <QSet>
#include <QMessageBox>

#include "BounceManager.h"

#include "FileDialog.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "PatternStore.h"
#include "PatternEditor.h"
#include "SongEditor.h"
#include "ClipView.h"
#include "TrackView.h"
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

// set the file to output and the file format settings based on defaults
bool BounceManager::setOutputDefaults()
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
		restoreMutedState();
		return false;
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

	return true;
}

// set the part of the timeline to export based on selected clips
bool BounceManager::setExportPoints()
{
	Song * song = Engine::getSong();

	// find part of song to render
	TimePos begin = TimePos(2147483647);
	TimePos end = TimePos(0);

	// get selected clips
	SongEditorWindow* sew = getGUI()->songEditor();
	QVector<selectableObject *> sos = sew->m_editor->selectedObjects();
	QSet<Track *> tracks;
	for ( auto so: sos )
	{
		auto clipv = dynamic_cast<ClipView*>(so);
		if ( clipv != nullptr )
		{
			auto clip = clipv->getClip();
			begin = clip->startPosition() < begin ? clip->startPosition() : begin;
			end = clip->endPosition() > end ? clip->endPosition() : end;
			tracks << clip->getTrack();
		}
		so->setSelected(false);
	}

	if (tracks.size() == 0)
	{
		QMessageBox::information(
			sew,
			tr("LMMS"),
			tr("Select clips to bounce") );
		return false;
	}

	for (const auto& track : song->tracks())
	{
		if ( !tracks.contains(track) && track->isMuted() == false )
		{
			track->setMuted(true);
			m_muted.push_back(track);
		}
	}

	song->setRenderClips(begin, end);
	return true;
}

// Bounce the clips into a .wav

void BounceManager::render()
{
	if ( setExportPoints() && setOutputDefaults() )
	{
		m_renderer = new ProjectRenderer(
				*m_qualitySettings,
				*m_outputSettings,
				m_format,
				m_outputPath);

		connect( m_renderer, SIGNAL(finished()), this, SLOT(finished()));

		if ( m_renderer->isReady() )
		{
			m_renderer->startProcessing();
		}
		else
		{
			qDebug( "Renderer failed to acquire a file device!" );
		}
	}
}

void BounceManager::finished()
{
	for ( auto so: getGUI()->songEditor()->m_editor->selectedObjects() )
	{
		so->setSelected(false);
	}
	restoreMutedState();
	// this works provided RenderManager does nothing after emit finished()
	delete this;
}

// Unmute all tracks that were muted while rendering clips
void BounceManager::restoreMutedState()
{
	while ( !m_muted.empty() )
	{
		Track* track = m_muted.back();
		m_muted.pop_back();
		track->setMuted( false );
	}
}

} // namespace lmms
