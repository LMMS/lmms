/*
 * ProcessHandle.h - Base class for ProcessHandles - any threadable jobs that do
 * 					things other than render audio
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
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
 
#ifndef PROCESS_HANDLE_H
#define PROCESS_HANDLE_H

#include <QtCore/QThread>
#include <QtCore/QVector>
#include <QtCore/QMutex>

#include "ThreadableJob.h"
#include "lmms_basics.h"

class track;

class ProcessHandle : public ThreadableJob
{
	MM_OPERATORS
public:
	enum Type
	{
		ControllerProcessHandle,
		AutomationProcessHandle,
		InstrumentProcessHandle,
		BbTrackProcessHandle,
		SampleTrackProcessHandle,
		NumProcessHandleTypes
	};
	
	ProcessHandle( Type type ) : m_type( type )
	{}
	
	virtual ~ProcessHandle() {}
	
	virtual bool affinityMatters() const
	{
		return false;
	}
	
	const QThread* affinity() const
	{
		return m_affinity;
	}

	Type type() const
	{
		return m_type;
	}
	
	virtual void doProcessing() = 0;

	virtual bool requiresProcessing() const
	{
		return true;
	}
	
private:
	Type m_type;
	QThread* m_affinity;
	
typedef QList<ProcessHandle *> ProcessHandleList;
typedef QList<const ProcessHandle *> ConstProcessHandleList;	
};

#endif
