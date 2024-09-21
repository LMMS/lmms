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

#ifndef RENDER_MANAGER_H
#define RENDER_MANAGER_H

#include <memory>

#include "ProjectRenderer.h"
#include "OutputSettings.h"


class RenderManager : public QObject
{
	Q_OBJECT
public:
	RenderManager(
		const Mixer::qualitySettings & qualitySettings,
		const OutputSettings & outputSettings,
		ProjectRenderer::ExportFileFormats fmt,
		QString outputPath);

	virtual ~RenderManager();

	/// Export all unmuted tracks into a single file
	void renderProject();

	/// Export all unmuted tracks into individual file
	void renderTracks();

	void abortProcessing();

signals:
	void progressChanged( int );
	void finished();

private slots:
	void renderNextTrack();
	void updateConsoleProgress();

private:
	QString pathForTrack( const Track *track, int num );
	void restoreMutedState();

	void render( QString outputPath );

	const Mixer::qualitySettings m_qualitySettings;
	const Mixer::qualitySettings m_oldQualitySettings;
	const OutputSettings m_outputSettings;
	ProjectRenderer::ExportFileFormats m_format;
	QString m_outputPath;

	std::unique_ptr<ProjectRenderer> m_activeRenderer;

	QVector<Track*> m_tracksToRender;
	QVector<Track*> m_unmuted;
} ;

#endif
