/*
 * DataprocLauncher.h - QThread::create workaround for older Qt version
 *
 * Copyright (c) 2019 Martin Pavelek <he29/dot/HS/at/gmail/dot/com>
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

#ifndef DATAPROCLAUNCHER_H
#define DATAPROCLAUNCHER_H

#include <QThread>
#include <QWaitCondition>

#include "SaProcessor.h"
#include "../../src/3rdparty/ringbuffer/include/ringbuffer/ringbuffer.h"

class DataprocLauncher : public QThread
{
public:
	explicit DataprocLauncher(SaProcessor &proc, ringbuffer_t<sampleFrame> &buffer, QWaitCondition &notifier)
		: m_processor(&proc),
		m_inputBuffer(&buffer),
		m_notifier(&notifier)
	{
	}

private:
	void run() override
	{
		m_processor->analyze(*m_inputBuffer, *m_notifier);
	}

	SaProcessor *m_processor;
	ringbuffer_t<sampleFrame> *m_inputBuffer;
	QWaitCondition *m_notifier;
};

#endif // DATAPROCLAUNCHER_H
