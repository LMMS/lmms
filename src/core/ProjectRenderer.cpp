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

#include "ProjectRenderer.h"
#include "Song.h"

namespace lmms
{

ProjectRenderer::ProjectRenderer( const AudioEngine::qualitySettings & qualitySettings,
					const OutputSettings & outputSettings,
					LmmsExporter::ExportAudioFileFormat exportFileFormat,
					const QString & outputFilename ) :
	m_qualitySettings(qualitySettings),
	m_exporter(LmmsExporter::ExportFileType::Audio, outputFilename),
	m_progress(0)
{
	m_timer = nullptr;
	m_exporter.setupAudioRendering(outputSettings, exportFileFormat, Engine::audioEngine()->framesPerPeriod(),
		&nextOutputBuffer, &endRendering, this);
}


void ProjectRenderer::startProcessing()
{
	m_timer = std::make_unique<PerfLogTimer>("Project Render");

	Engine::getSong()->startExport();
	// Skip first empty buffer.
	Engine::audioEngine()->nextBuffer();
	Engine::audioEngine()->startExporting(m_qualitySettings);

	m_progress = 0;

	m_exporter.startExporting();
}

void ProjectRenderer::abortProcessing()
{
	m_exporter.stopExporting();
}

void ProjectRenderer::endRendering(void* dataIn)
{
	ProjectRenderer* thisRenderer = reinterpret_cast<ProjectRenderer*>(dataIn);

	Engine::audioEngine()->stopProcessing();
	Engine::getSong()->stopExport();

	thisRenderer->m_timer->end();
	thisRenderer->m_timer = nullptr;
	
	emit thisRenderer->finished();
}

void ProjectRenderer::updateConsoleProgress()
{
	const int cols = 50;
	static int rot = 0;
	auto buf = std::array<char, 80>{};
	auto prog = std::array<char, cols + 1>{};

	for( int i = 0; i < cols; ++i )
	{
		prog[i] = ( i*100/cols <= m_progress ? '-' : ' ' );
	}
	prog[cols] = 0;

	const auto activity = (const char*)"|/-\\";
	std::fill(buf.begin(), buf.end(), 0);
	sprintf(buf.data(), "\r|%s|    %3d%%   %c  ", prog.data(), m_progress,
							activity[rot] );
	rot = ( rot+1 ) % 4;

	fprintf( stderr, "%s", buf.data() );
	fflush( stderr );
}

// gets a buffer and some data as input
// the sender who constructed lmms::LmmsExporter decides what is dataIn (in this case it is ProjectRenderer)
// fills the provided buffer with AudioEngine::nextBuffer() data and sets the correct size
// this class doesn't own bufferOut
// bufferOut can not be nullptr
// dataIn can be nullptr
void ProjectRenderer::nextOutputBuffer(std::vector<SampleFrame>* bufferOut, void* dataIn)
{
	ProjectRenderer* thisRenderer = reinterpret_cast<ProjectRenderer*>(dataIn);

	fpp_t curFrames = Engine::audioEngine()->framesPerPeriod();
	if (bufferOut->size() != curFrames)
	{
		bufferOut->resize(curFrames);
	}
	
	// get next buffer
	const SampleFrame* newBuffer = Engine::audioEngine()->nextBuffer();

	if (newBuffer != nullptr && Engine::getSong()->isExportDone() == false)
	{
		// copy new buffer to bufferOut
		for (size_t i = 0; i < bufferOut->size(); i++)
		{
			(*bufferOut)[i] = newBuffer[i];
		}

		// update progress
		const int nprog = Engine::getSong()->getExportProgress();
		if (thisRenderer->m_progress != nprog)
		{
			thisRenderer->m_progress = nprog;
			emit thisRenderer->progressChanged(thisRenderer->m_progress);
		}

		// delete source buffer
		if (Engine::audioEngine()->hasFifoWriter()) { delete[] newBuffer; }
	}
	else
	{
		// this will stop LmmsExporter exporting
		bufferOut->clear();
	}
}


} // namespace lmms
