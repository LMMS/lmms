/*
 * ClickGDX.cpp - A click remover
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <math.h>
#include "ClickGDX.h"
#include "embed.h"


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT clickgdx_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"ClickGDX",
	QT_TRANSLATE_NOOP( "pluginBrowser", "A click remover plugin" ),
	"gi0e5b06 (on github.com)",
	0x0100,
	Plugin::Effect,
	new PluginPixmapLoader("logo"),
	NULL,
	NULL
} ;

}



ClickGDXEffect::ClickGDXEffect( Model* parent, const Descriptor::SubPluginFeatures::Key* key ) :
	Effect( &clickgdx_plugin_descriptor, parent, key ),
	m_gdxControls( this ),
	m_current(0)
{
}




ClickGDXEffect::~ClickGDXEffect()
{
}




bool ClickGDXEffect::processAudioBuffer( sampleFrame* buf, const fpp_t frames )
{
	if( !isEnabled() || !isRunning () )
	{
		return( false );
	}

	float curVal0;
	float curVal1;

	double outSum = 0.0;

	const float d = dryLevel();
	const float w = wetLevel();

	const ValueBuffer * attTimeBuf  = m_gdxControls.m_attackTimeModel.valueBuffer();
	const ValueBuffer * desTimeBuf  = m_gdxControls.m_descentTimeModel.valueBuffer();
	const ValueBuffer * attTypeBuf  = m_gdxControls.m_attackTypeModel.valueBuffer();
	const ValueBuffer * desTypeBuf  = m_gdxControls.m_descentTypeModel.valueBuffer();
	const ValueBuffer * attTempoBuf = m_gdxControls.m_attackTempoModel.valueBuffer();
	const ValueBuffer * desTempoBuf = m_gdxControls.m_descentTempoModel.valueBuffer();

	for( fpp_t f = 0; f < frames; ++f )
	{
		curVal0=buf[f][0];
		curVal1=buf[f][1];
		outSum += curVal0*curVal0+curVal1*curVal1;

		uint32_t tempo1=(uint32_t)(attTempoBuf
					   ? attTempoBuf->value( f )
					   : m_gdxControls.m_attackTempoModel.value());
		uint32_t tempo2=(uint32_t)(desTempoBuf
					   ? desTempoBuf->value( f )
					   : m_gdxControls.m_descentTempoModel.value());
		uint32_t step1=(uint32_t)(44100.f*60.f/tempo1*(attTimeBuf
						   ? attTimeBuf->value( f )
						   : m_gdxControls.m_attackTimeModel.value()));
		uint32_t step2=(uint32_t)(44100.f*60.f/tempo2*(desTimeBuf
						   ? desTimeBuf->value( f )
						   : m_gdxControls.m_descentTimeModel.value()));
		int type1=(int)(attTypeBuf
				? attTypeBuf->value( f )
				: m_gdxControls.m_attackTypeModel.value());
		int type2=(int)(desTypeBuf
				? desTypeBuf->value( f )
				: m_gdxControls.m_descentTypeModel.value());

		float volume;

		m_current++;
		if(m_current>=step1+step2) m_current=0;

		if(m_current<step1)
		{
			volume=1.f;
			if((type1>0)&&(step1>0))
			{
				float t=((float)m_current)/(float)step1;
				switch(type1)
				{
				case 1:
					volume=t;
					break;
				}
			}
		}
		else
		{
			volume=0.f;
			if((type2>0)&&(step2>0))
			{
				float t=((float)(m_current-step1))/(float)step2;
				switch(type2)
				{
				case 1:
					volume=1.f-t;
					break;
				}
			}
		}

		curVal0*=volume;
		curVal1*=volume;

		buf[f][0] = d * buf[f][0] + w * curVal0;
		buf[f][1] = d * buf[f][1] + w * curVal1;

		// convert pan values to left/right values
		/*
		const float pan = panBuf 
			? panBuf->value( f ) 
			: m_gdxControls.m_panModel.value();
		const float left1 = pan <= 0
			? 1.0
			: 1.0 - pan * 0.01f;
		const float right1 = pan >= 0
			? 1.0
			: 1.0 + pan * 0.01f;

		// second stage amplification
		const float left2 = leftBuf
			? leftBuf->value( f ) 
			: m_gdxControls.m_leftModel.value();
		const float right2 = rightBuf
			? rightBuf->value( f ) 
			: m_gdxControls.m_rightModel.value();

		s[0] *= left1 * left2 * 0.01;
		s[1] *= right1 * right2 * 0.01;

		buf[f][0] = d * buf[f][0] + w * s[0];
		buf[f][1] = d * buf[f][1] + w * s[1];
		*/

	}

	checkGate( outSum / frames );

	return isRunning();
}





extern "C"
{

// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model* parent, void* data )
{
	return new ClickGDXEffect( parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>( data ) );
}

}

