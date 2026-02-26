/*
 * RenderManager.h - Provides a uniform interface for rendering the project or
 *                   individual tracks for the CLI and GUI.
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

#ifndef LMMS_RENDER_MANAGER_H
#define LMMS_RENDER_MANAGER_H

#include <memory>
#include <queue>

#include "ProjectRenderer.h"
#include "OutputSettings.h"


namespace lmms
{


class RenderManager : public QObject
{
	Q_OBJECT
public:
	RenderManager(const OutputSettings& outputSettings, ProjectRenderer::ExportFileFormat fmt);
	~RenderManager() override;

	/// Export all unmuted tracks into a single file
	void renderProject(const QString& outputPath);

	/// Export all unmuted tracks into individual file
	void renderTracks(const QString& outputPath);

	/// Export a a track into a single file
	void renderTrack(Track* track, const QString& outputPath);

	void abortProcessing();

signals:
	void progressChanged( int );
	void finished();

private slots:
	void updateConsoleProgress();

private:
	struct RenderJob
	{
		QString path;
		std::vector<Track*> tracksToRender;
	};

	void render();
	void startRender();
	void renderFinished();

	void storeMuteStates();
	void restoreMuteStates();

	const OutputSettings m_outputSettings;
	ProjectRenderer::ExportFileFormat m_format;
	std::unique_ptr<ProjectRenderer> m_activeRenderer;

	std::queue<RenderJob> m_renderJobQueue;
	std::unordered_map<Track*, bool> m_muteStates;
	int m_totalRenderJobs = 0;
} ;


} // namespace lmms

#endif // LMMS_RENDER_MANAGER_H
