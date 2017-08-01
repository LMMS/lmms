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
	m_volCurrent(0),
	m_panCurrent(0)
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
	const ValueBuffer * panTimeBuf  = m_gdxControls.m_panTimeModel.valueBuffer();

	const ValueBuffer * attTypeBuf  = m_gdxControls.m_attackTypeModel.valueBuffer();
	const ValueBuffer * desTypeBuf  = m_gdxControls.m_descentTypeModel.valueBuffer();
	const ValueBuffer * panTypeBuf  = m_gdxControls.m_panTypeModel.valueBuffer();

	const ValueBuffer * attTempoBuf = m_gdxControls.m_attackTempoModel.valueBuffer();
	const ValueBuffer * desTempoBuf = m_gdxControls.m_descentTempoModel.valueBuffer();
	const ValueBuffer * panTempoBuf = m_gdxControls.m_panTempoModel.valueBuffer();

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
		uint32_t tempo3=(uint32_t)(panTempoBuf
					   ? panTempoBuf->value( f )
					   : m_gdxControls.m_panTempoModel.value());
		uint32_t step1=(uint32_t)(44100.f*60.f/tempo1*(attTimeBuf
						   ? attTimeBuf->value( f )
						   : m_gdxControls.m_attackTimeModel.value()));
		uint32_t step2=(uint32_t)(44100.f*60.f/tempo2*(desTimeBuf
						   ? desTimeBuf->value( f )
						   : m_gdxControls.m_descentTimeModel.value()));
		uint32_t step3=(uint32_t)(44100.f*60.f/tempo3*(panTimeBuf
						   ? panTimeBuf->value( f )
						   : m_gdxControls.m_panTimeModel.value()));
		int type1=(int)(attTypeBuf
				? attTypeBuf->value( f )
				: m_gdxControls.m_attackTypeModel.value());
		int type2=(int)(desTypeBuf
				? desTypeBuf->value( f )
				: m_gdxControls.m_descentTypeModel.value());
		int type3=(int)(panTypeBuf
				? panTypeBuf->value( f )
				: m_gdxControls.m_panTypeModel.value());

		float volume,pan;

		m_volCurrent++;
		if(m_volCurrent>=step1+step2+1) m_volCurrent=0;

		if(m_volCurrent<=step1)
		{
			volume=1.f;
			     if(type1==0) volume=0.f;
			else if(type1==1) volume=1.f;
			else if(step1>0)
			{
				float t=((float)m_volCurrent)/(float)step1;
				switch(type1)
				{
				case 2:	volume=f2(t); break;
				case 3: volume=f3(t); break;
				case 4: volume=f4(t); break;
				case 5: volume=f5(t); break;
				case 6: volume=f6(t); break;
				}
			}
		}
		else
		{
			volume=0.f;
			     if(type2==0) volume=0.f;
			else if(type2==1) volume=1.f;
			else if(step2>0)
			{
				float t=1.f-((float)(m_volCurrent-step1))/(float)step2;
				switch(type2)
				{
				case 2:	volume=f2(t); break;
				case 3:	volume=f3(t); break;
				case 4: volume=f4(t); break;
				case 5: volume=f5(t); break;
				case 6: volume=f6(t); break;
				}
			}
		}

		if(volume<1.f)
		{
			curVal0*=volume;
			curVal1*=volume;
		}

		if(step3>0)
		{
			m_panCurrent++;
			if(m_panCurrent>=step3+step3+1) m_panCurrent=0;

			if(m_panCurrent<=step3)
			{
				pan=0.f;
				     if(type3==0) pan=0.f;
				else if(type3==1) pan=0.f;
				else if(type3==2) pan=0.5f;
				else if(type3==3) pan=1.0f;
				else if(step3>0)
				{
					float t=((float)m_panCurrent)/(float)step3;
					switch(type3)
					{
					case 4:	pan=f2(t); break;
					case 5:	pan=f3(t); break;
					case 6: pan=f4(t); break;
					case 7: pan=f5(t); break;
					case 8: pan=f6(t); break;
					}
				}
			}
			else
			{
				pan=0.f;
				     if(type3==0) pan=0.f;
				else if(type3==1) pan=1.f;
				else if(type3==2) pan=0.5f;
				else if(type3==3) pan=1.f;
				else if(step3>0)
				{
					float t=1.f-((float)(m_panCurrent-step3))/(float)step3;
					switch(type3)
					{
					case 4:	pan=f2(t); break;
					case 5:	pan=f3(t); break;
					case 6: pan=f4(t); break;
					case 7: pan=f5(t); break;
					case 8: pan=f6(t); break;
					}
				}
			}

			if(pan<1.f)
			{
				float newVal0=(pan*curVal0)+(1.f-pan)*curVal1;
				float newVal1=(pan*curVal1)+(1.f-pan)*curVal0;
				curVal0=newVal0;
				curVal1=newVal1;
			}
		}

		buf[f][0] = d * buf[f][0] + w * curVal0;
		buf[f][1] = d * buf[f][1] + w * curVal1;
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

