/*
 * ProjectRenderer.h - ProjectRenderer class for easily rendering projects
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_PROJECT_RENDERER_H
#define LMMS_PROJECT_RENDERER_H

#include "lmmsconfig.h"
#include "LmmsExporter.h"
#include "AudioEngine.h"
#include "OutputSettings.h"
#include "PerfLog.h"

#include "lmms_export.h"

namespace lmms
{


class LMMS_EXPORT ProjectRenderer : public QObject
{
	Q_OBJECT
public:
	ProjectRenderer( const AudioEngine::qualitySettings & _qs,
				const OutputSettings & _os,
				LmmsExporter::ExportAudioFileFormat exportFileFormat,
				const QString & _out_file );
	~ProjectRenderer() override = default;

	inline bool isReady()
	{
		return m_exporter.canExportAutioFile();
	}

public slots:
	void startProcessing();
	void abortProcessing();

	void updateConsoleProgress();


signals:
	void progressChanged( int );
	void finished();

private:
	// fill m_activeRenderer's buffer
	// provided as function pointer to m_activeRenderer
	static void nextOutputBuffer(std::vector<SampleFrame>* bufferOut, void* dataIn);
	static void endRendering(void* dataIn);

	const AudioEngine::qualitySettings m_qualitySettings;

	LmmsExporter m_exporter;

	volatile int m_progress;
	std::unique_ptr<PerfLogTimer> m_timer;

	friend class LmmsExporter;
} ;


} // namespace lmms

#endif // LMMS_PROJECT_RENDERER_H
