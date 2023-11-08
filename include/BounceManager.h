/*
 * RenderManager.h - No questions asked bouncing of clips  to wav loops
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

#ifndef LMMS_BOUNCE_MANAGER_H
#define LMMS_BOUNCE_MANAGER_H

#include <memory>

#include "ProjectRenderer.h"
#include "OutputSettings.h"
#include "TimePos.h"


namespace lmms
{


class BounceManager : public QObject
{
	Q_OBJECT
public:
	BounceManager();

	~BounceManager() override;

	bool setExportPoints( );

	/// Export all unmuted tracks into a single file
	void render();

	void abortProcessing();

signals:
	void finished();

private slots:

private:
	QString pathForTrack( const Track *track, int num );
	void setOutputDefaults( );
	void muteUnusedTracks( );
	void restoreMutedState( );

	const AudioEngine::qualitySettings * m_qualitySettings;
	const AudioEngine::qualitySettings m_oldQualitySettings;
	const OutputSettings * m_outputSettings;
	ProjectRenderer::ExportFileFormat m_format;
	QString m_outputPath;
	ProjectRenderer * m_renderer;

	std::vector<Track*> m_muted;
	TimePos m_start;
	TimePos m_end;
} ;


} // namespace lmms

#endif // LMMS_BOUNCE_MANAGER_H
