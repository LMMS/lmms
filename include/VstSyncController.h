/*
 * VstSyncController.h - type declarations needed for VST to lmms host sync
 *
 * Copyright (c) 2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2013 Mike Choi <rdavidian71/at/gmail/dot/com>
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

#ifndef LMMS_VST_SYNC_CONTROLLER_H
#define LMMS_VST_SYNC_CONTROLLER_H

#include <QObject>

#include "SharedMemory.h"
#include "VstSyncData.h"  // IWYU pragma: keep

namespace lmms
{

class VstSyncController : public QObject
{
	Q_OBJECT
public:
	VstSyncController();

	void setAbsolutePosition(double ticks);
	void setPlaybackState(bool enabled);
	void setTempo(int newTempo);
	void setTimeSignature(int num, int denom);
	void startCycle(int startTick, int endTick);
	void stopCycle();
	void setPlaybackJumped(bool jumped);
	void update();

	const std::string& sharedMemoryKey() const noexcept { return m_syncData.key(); }

private slots:
	void updateSampleRate();

private:
	SharedMemory<VstSyncData> m_syncData;
};


} // namespace lmms

#endif // LMMS_VST_SYNC_CONTROLLER_H
