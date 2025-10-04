/*
 * ProjectRenderer.cpp - ProjectRenderer-class for easily rendering projects
 *
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QFile>

#include "ProjectRenderer.h"
#include "PathUtil.h"
#include "Song.h"
#include "PerfLog.h"

namespace lmms
{

ProjectRenderer::ProjectRenderer(const AudioEngine::qualitySettings& qualitySettings,
	const OutputSettings& outputSettings, AudioFileFormat audioFileFormat, const QString& outputFilename)
	: QThread(Engine::audioEngine())
	, m_audioFile(PathUtil::fsConvert(outputFilename), audioFileFormat, outputSettings)
	, m_qualitySettings(qualitySettings)
	, m_progress(0)
	, m_abort(false)
{
}

void ProjectRenderer::startProcessing()
{
	start(
#ifndef LMMS_BUILD_WIN32
		QThread::HighPriority
#endif
	);
}


void ProjectRenderer::run()
{
	PerfLogTimer perfLog("Project Render");

	Engine::getSong()->startExport();
	// Skip first empty buffer.
	Engine::audioEngine()->nextBuffer();

	m_progress = 0;

	// Now start processing
	Engine::audioEngine()->startProcessing(false);

	// Continually track and emit progress percentage to listeners.
	while (!Engine::getSong()->isExportDone() && !m_abort)
	{
		processNextBuffer();

		const int nprog = Engine::getSong()->getExportProgress();
		if (m_progress != nprog)
		{
			m_progress = nprog;
			emit progressChanged( m_progress );
		}
	}

	// Notify the audio engine of the end of processing.
	Engine::audioEngine()->stopProcessing();

	Engine::getSong()->stopExport();

	perfLog.end();

	// If the user aborted export-process, the file has to be deleted.
	if (m_abort) { std::filesystem::remove(m_audioFile.path()); }
}

void ProjectRenderer::processNextBuffer()
{
	assert(m_audioFile.channels() == 1 || m_audioFile.channels == 2 && "invalid channel count");
	const auto framesPerPeriod = Engine::audioEngine()->framesPerPeriod();

	if (m_audioFile.channels() == 1)
	{
		const auto src = Engine::audioEngine()->renderNextBuffer();
		auto dst = std::vector<float>(framesPerPeriod);
		std::transform(src, src + framesPerPeriod, dst.begin(), [](auto& frame) { return frame.average(); });
		m_audioFile.write({dst.data(), 2, framesPerPeriod});
	}
	else if (m_audioFile.channels() == 2)
	{
		const auto src = Engine::audioEngine()->renderNextBuffer();
		m_audioFile.write({&src[0][0], 2, framesPerPeriod});
	}
}


void ProjectRenderer::abortProcessing()
{
	m_abort = true;
	wait();
}



void ProjectRenderer::updateConsoleProgress()
{
	constexpr int cols = 50;
	static int rot = 0;
	auto buf = std::array<char, 80>{};
	auto prog = std::array<char, cols + 1>{};

	for( int i = 0; i < cols; ++i )
	{
		prog[i] = ( i*100/cols <= m_progress ? '-' : ' ' );
	}
	prog[cols] = 0;

	const auto activity = "|/-\\";
	std::fill(buf.begin(), buf.end(), 0);
	std::snprintf(buf.data(), buf.size(), "\r|%s|    %3d%%   %c  ", prog.data(), m_progress,
							activity[rot] );
	rot = ( rot+1 ) % 4;

	fprintf( stderr, "%s", buf.data() );
	fflush( stderr );
}


} // namespace lmms
