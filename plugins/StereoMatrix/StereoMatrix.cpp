/*
 * StereoMatrix.cpp - stereo-matrix-effect-plugin
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


#include "StereoMatrix.h"

#include "embed.h"
#include "plugin_export.h"

namespace lmms
{


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT stereomatrix_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"Stereo Matrix",
	QT_TRANSLATE_NOOP( "PluginBrowser",
				"Plugin for freely manipulating stereo output" ),
	"Paul Giblock <drfaygo/at/gmail.com>",
	0x0100,
	Plugin::Type::Effect,
	new PluginPixmapLoader("logo"),
	nullptr,
	nullptr,
	nullptr,
};

}



StereoMatrixEffect::StereoMatrixEffect(
			Model * _parent,
			const Descriptor::SubPluginFeatures::Key * _key ) :
	Effect( &stereomatrix_plugin_descriptor, _parent, _key ),
	m_smControls( this )
{
}




Effect::ProcessStatus StereoMatrixEffect::processImpl(SampleFrame* buf, const fpp_t frames)
{
	for (fpp_t f = 0; f < frames; ++f)
	{	
		const float d = dryLevel();
		const float w = wetLevel();
		
		sample_t l = buf[f][0];
		sample_t r = buf[f][1];

		// Init with dry-mix
		buf[f][0] = l * d;
		buf[f][1] = r * d;

		// Add it wet
		buf[f][0] += ( m_smControls.m_llModel.value( f ) * l  +
					m_smControls.m_rlModel.value( f ) * r ) * w;

		buf[f][1] += ( m_smControls.m_lrModel.value( f ) * l  +
					m_smControls.m_rrModel.value( f ) * r ) * w;
	}

	return ProcessStatus::ContinueIfNotQuiet;
}




extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model * _parent, void * _data )
{
	return( new StereoMatrixEffect( _parent,
		static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>(
								_data ) ) );
}

}


} // namespace lmms
