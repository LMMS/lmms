/*
 * MixerProfiler.cpp - class for profiling performance of Mixer
 *
 * Copyright (c) 2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#include "MixerProfiler.h"


MixerProfiler::MixerProfiler() :
	m_periodTimer(),
	m_cpuLoad( 0 ),
	m_outputFile()
{
}



MixerProfiler::~MixerProfiler()
{
}


void MixerProfiler::finishPeriod( sample_rate_t sampleRate, fpp_t framesPerPeriod )
{
	int periodElapsed = m_periodTimer.elapsed();

	const float newCpuLoad = periodElapsed / 10000.0f * sampleRate / framesPerPeriod;
    m_cpuLoad = qBound<int>( 0, ( newCpuLoad * 0.1f + m_cpuLoad * 0.9f ), 100 );

	if( m_outputFile.isOpen() )
	{
		m_outputFile.write( QString( "%1\n" ).arg( periodElapsed ).toLatin1() );
	}
}



void MixerProfiler::setOutputFile( const QString& outputFile )
{
	m_outputFile.close();
	m_outputFile.setFileName( outputFile );
	m_outputFile.open( QFile::WriteOnly | QFile::Truncate );
}

