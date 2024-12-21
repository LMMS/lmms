/*
 * StereoMatrix.h - stereo-matrix-effect-plugin
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
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


#ifndef _STEREO_MATRIX_H
#define _STEREO_MATRIX_H

#include "AudioPluginInterface.h"
#include "StereoMatrixControls.h"

namespace lmms
{


class StereoMatrixEffect : public DefaultEffectPluginInterface
{
public:
	StereoMatrixEffect( Model * parent, 
	                      const Descriptor::SubPluginFeatures::Key * _key );
	~StereoMatrixEffect() override = default;

	ProcessStatus processImpl(CoreAudioDataMut inOut) override;

	EffectControls* controls() override
	{
		return( &m_smControls );
	}


private:
	StereoMatrixControls m_smControls;

	friend class StereoMatrixControls;
} ;


} // namespace lmms

#endif
