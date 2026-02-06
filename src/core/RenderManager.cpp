/*
 * RenderManager - exporting logic common between the CLI and GUI.
 *
 * Copyright (c) 2015 Ryan Roden-Corrent <ryan/at/rcorre.net>
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

#include "RenderManager.h"

#include <QDir>
#include <QRegularExpression>
#include <iostream>
#include <ranges>

#include "Engine.h"
#include "Song.h"

namespace lmms
{

RenderManager::RenderManager(
	const OutputSettings& outputSettings, ProjectRenderer::ExportFileFormat fmt)
	: m_outputSettings(outputSettings)
	, m_format(fmt)
{
	Engine::audioEngine()->storeAudioDevice();
}

RenderManager::~RenderManager()
{
	Engine::audioEngine()->restoreAudioDevice();  // Also deletes audio dev.
}

void RenderManager::abortProcessing()
{
	if (m_activeRenderer)
	{
		m_activeRenderer->abortProcessing();
		m_activeRenderer.reset();
	}

	restoreMuteStates();
}

// Render the song into individual tracks
void RenderManager::renderTracks(const QString& outputPath)
{
	auto trackNum = 1;

	// TODO: Currently, only the song is exported (it will require changes and refactors in Song), but in the future we
	// may want to generalize this function to work with any track container (e.g. the pattern store)
	for (const auto& track : Engine::getSong()->tracks() | std::views::filter(&Track::isRenderable))
	{
		auto extension = ProjectRenderer::getFileExtensionFromFormat(m_format);
		auto name = track->name();
		name = name.remove(QRegularExpression(FILENAME_FILTER));
		name = QString{"%1_%2%3"}.arg(trackNum++).arg(name).arg(extension);

		const auto pathForTrack = QDir{outputPath}.filePath(name);
		m_renderJobQueue.emplace(pathForTrack, std::vector{track});
	}

	startRender();
}

// Render the song into a single track
void RenderManager::renderProject(const QString& outputPath)
{
	auto tracks = std::vector<Track*>{};

	// TODO: Currently, only the song is exported (it will require changes and refactors in Song), but in the future we
	// may want to generalize this function to work with any track container (e.g. the pattern store)
	for (const auto& track : Engine::getSong()->tracks() | std::views::filter(&Track::isRenderable))
	{
		tracks.emplace_back(track);
	}

	m_renderJobQueue.emplace(outputPath, tracks);
	startRender();
}

void RenderManager::renderTrack(Track* track, const QString& outputPath)
{
	assert(track);
	assert(track->trackContainer());

	if (!track->isRenderable())
	{
		qDebug("Track is not renderable");
		return;
	}

	// TODO: Currently, only the song is exported (it will require changes and refactors in Song), but in the future we
	// may want to generalize this function to work with any track container (e.g. the pattern store)
	if (!dynamic_cast<Song*>(track->trackContainer()))
	{
		qDebug("Can only export tracks from the song");
		return;
	}

	m_renderJobQueue.emplace(outputPath, std::vector{track});
	startRender();
}

void RenderManager::startRender()
{
	m_totalRenderJobs = m_renderJobQueue.size();
	storeMuteStates();
	render();
}

void RenderManager::renderFinished()
{
	restoreMuteStates();
	render();
}

void RenderManager::render()
{
	if (m_renderJobQueue.empty())
	{
		emit finished();
		return;
	}

	const auto job = m_renderJobQueue.front();
	m_renderJobQueue.pop();

	for (const auto& track : Engine::getSong()->tracks() | std::views::filter(&Track::isRenderable))
	{
		track->setMuted(
			std::find(job.tracksToRender.begin(), job.tracksToRender.end(), track) == job.tracksToRender.end());
	}

	// TODO: We shouldn't need to allocate a new ProjectRenderer each time... we might also want to remove
	// ProjectRenderer, merge it with RenderManager and use ThreadPool instead
	m_activeRenderer = std::make_unique<ProjectRenderer>(m_outputSettings, m_format, job.path);

	if (m_activeRenderer->isReady())
	{
		connect(m_activeRenderer.get(), &ProjectRenderer::progressChanged, this, &RenderManager::progressChanged);
		connect(m_activeRenderer.get(), &ProjectRenderer::finished, this, &RenderManager::renderFinished);
		m_activeRenderer->startProcessing();
	}
	else
	{
		qDebug("Renderer failed to acquire a file device");
		m_renderJobQueue = std::queue<RenderJob>{};
	}
}

void RenderManager::updateConsoleProgress()
{
	if (m_activeRenderer)
	{
		m_activeRenderer->updateConsoleProgress();
		std::cerr << "(" << (m_totalRenderJobs - m_renderJobQueue.size()) << "/" << m_totalRenderJobs << ")";
	}
}

void RenderManager::storeMuteStates()
{
	// TODO: Currently, only the song is exported (it will require changes and refactors in Song), but in the future we
	// may want to generalize this function to work with any track container (e.g. the pattern store)
	for (const auto& track : Engine::getSong()->tracks() | std::views::filter(&Track::isRenderable))
	{
		m_muteStates[track] = track->isMuted();
	}
}

void RenderManager::restoreMuteStates()
{
	for (const auto& [track, muted] : m_muteStates)
	{
		track->setMuted(muted);
	}
}


} // namespace lmms