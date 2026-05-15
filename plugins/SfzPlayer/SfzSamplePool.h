/*
 * SfzSamplePool.h - Helper class for handling loading of sample files
 *
 * Copyright (c) 2026 Keratin
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

#ifndef LMMS_SFZ_SAMPLE_POOL_H
#define LMMS_SFZ_SAMPLE_POOL_H

#include "SfzSampleBuffer.h"

#include <QString>
#include <memory>
#include <map>

namespace lmms
{

class SfzSamplePool
{
public:
	const SfzSampleBuffer* loadSample(const QString& path);

	//! Returns the number of samples currently loaded in the pool
	const int sampleCount() const { return m_samplePool.size(); }

private:
	std::map<QString, std::unique_ptr<SfzSampleBuffer>> m_samplePool;
};

} // namespace lmms

#endif // LMMS_SFZ_SAMPLE_POOL_H
