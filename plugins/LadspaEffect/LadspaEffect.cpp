/*
 * LadspaEffect.cpp - class for processing LADSPA effects
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2009-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QVarLengthArray>
#include <QMessageBox>

#include "LadspaEffect.h"
#include "DataFile.h"
#include "AudioEngine.h"
#include "Ladspa2LMMS.h"
#include "LadspaBase.h"
#include "LadspaControl.h"
#include "LadspaSubPluginFeatures.h"
#include "AutomationClip.h"
#include "ValueBuffer.h"
#include "Song.h"

#include "embed.h"

#include "plugin_export.h"

namespace lmms
{


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT ladspaeffect_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"LADSPA",
	QT_TRANSLATE_NOOP( "PluginBrowser",
				"plugin for using arbitrary LADSPA-effects "
				"inside LMMS." ),
	"Danny McRae <khjklujn/at/users.sourceforge.net>",
	0x0100,
	Plugin::Type::Effect,
	new PluginPixmapLoader("logo"),
	{},
	new LadspaSubPluginFeatures( Plugin::Type::Effect )
} ;

}


LadspaEffect::LadspaEffect( Model * _parent,
			const Descriptor::SubPluginFeatures::Key * _key ) :
	Effect( &ladspaeffect_plugin_descriptor, _parent, _key ),
	m_controls( nullptr ),
	m_maxSampleRate( 0 ),
	m_key( LadspaSubPluginFeatures::subPluginKeyToLadspaKey( _key ) )
{
	Ladspa2LMMS * manager = Engine::getLADSPAManager();
	if( manager->getDescription( m_key ) == nullptr )
	{
		Engine::getSong()->collectError(tr( "Unknown LADSPA plugin %1 requested." ).arg(
											m_key.second ) );
		setOkay( false );
		return;
	}

	setDisplayName( manager->getShortName( m_key ) );

	pluginInstantiation();

	connect( Engine::audioEngine(), SIGNAL( sampleRateChanged() ),
					this, SLOT( changeSampleRate() ) );
}




LadspaEffect::~LadspaEffect()
{
	pluginDestruction();
}




void LadspaEffect::changeSampleRate()
{
	DataFile dataFile( DataFile::Type::EffectSettings );
	m_controls->saveState( dataFile, dataFile.content() );

	LadspaControls * controls = m_controls;
	m_controls = nullptr;

	m_pluginMutex.lock();
	pluginDestruction();
	pluginInstantiation();
	m_pluginMutex.unlock();

	controls->effectModelChanged( m_controls );
	delete controls;

	m_controls->restoreState( dataFile.content().firstChild().toElement() );

	// the IDs of re-created controls have been saved and now need to be
	// resolved again
	AutomationClip::resolveAllIDs();
}




Effect::ProcessStatus LadspaEffect::processImpl(SampleFrame* buf, const fpp_t frames)
{
	m_pluginMutex.lock();
	if (!isOkay() || dontRun() || !isEnabled() || !isRunning())
	{
		m_pluginMutex.unlock();
		return ProcessStatus::Sleep;
	}

	auto outFrames = frames;
	SampleFrame* outBuf = nullptr;
	QVarLengthArray<SampleFrame> sBuf(frames);

	if( m_maxSampleRate < Engine::audioEngine()->outputSampleRate() )
	{
		outBuf = buf;
		buf = sBuf.data();
		sampleDown(outBuf, buf, m_maxSampleRate);
		outFrames = frames * m_maxSampleRate /
				Engine::audioEngine()->outputSampleRate();
	}

	// Copy the LMMS audio buffer to the LADSPA input buffer and initialize
	// the control ports.
	ch_cnt_t channel = 0;
	for( ch_cnt_t proc = 0; proc < processorCount(); ++proc )
	{
		for( int port = 0; port < m_portCount; ++port )
		{
			port_desc_t * pp = m_ports.at( proc ).at( port );
			switch( pp->rate )
			{
				case BufferRate::ChannelIn:
					for (fpp_t frame = 0; frame < outFrames; ++frame)
					{
						pp->buffer[frame] = buf[frame][channel];
					}
					++channel;
					break;
				case BufferRate::AudioRateInput:
				{
					ValueBuffer * vb = pp->control->valueBuffer();
					if( vb )
					{
						memcpy(pp->buffer, vb->values(), outFrames * sizeof(float));
					}
					else
					{
						pp->value = static_cast<LADSPA_Data>(
											pp->control->value() / pp->scale );
						// This only supports control rate ports, so the audio rates are
						// treated as though they were control rate by setting the
						// port buffer to all the same value.
						for (fpp_t frame = 0; frame < outFrames; ++frame)
						{
							pp->buffer[frame] = pp->value;
						}
					}
					break;
				}
				case BufferRate::ControlRateInput:
					if( pp->control == nullptr )
					{
						break;
					}
					pp->value = static_cast<LADSPA_Data>(
										pp->control->value() / pp->scale );
					pp->buffer[0] =
						pp->value;
					break;
				case BufferRate::ChannelOut:
				case BufferRate::AudioRateOutput:
				case BufferRate::ControlRateOutput:
					break;
				default:
					break;
			}
		}
	}


	// Process the buffers.
	for( ch_cnt_t proc = 0; proc < processorCount(); ++proc )
	{
		(m_descriptor->run)(m_handles[proc], outFrames);
	}

	// Copy the LADSPA output buffers to the LMMS buffer.
	channel = 0;
	const float d = dryLevel();
	const float w = wetLevel();
	for( ch_cnt_t proc = 0; proc < processorCount(); ++proc )
	{
		for( int port = 0; port < m_portCount; ++port )
		{
			port_desc_t * pp = m_ports.at( proc ).at( port );
			switch( pp->rate )
			{
				case BufferRate::ChannelIn:
				case BufferRate::AudioRateInput:
				case BufferRate::ControlRateInput:
					break;
				case BufferRate::ChannelOut:
					for (fpp_t frame = 0; frame < outFrames; ++frame)
					{
						buf[frame][channel] = d * buf[frame][channel] + w * pp->buffer[frame];
					}
					++channel;
					break;
				case BufferRate::AudioRateOutput:
				case BufferRate::ControlRateOutput:
					break;
				default:
					break;
			}
		}
	}

	if (outBuf != nullptr)
	{
		sampleBack(buf, outBuf, m_maxSampleRate);
	}

	m_pluginMutex.unlock();

	return ProcessStatus::ContinueIfNotQuiet;
}




void LadspaEffect::setControl( int _control, LADSPA_Data _value )
{
	if( !isOkay() )
	{
		return;
	}
	m_portControls[_control]->value = _value;
}




void LadspaEffect::pluginInstantiation()
{
	m_maxSampleRate = maxSamplerate( displayName() );

	Ladspa2LMMS * manager = Engine::getLADSPAManager();

	// Calculate how many processing units are needed.
	int effect_channels = manager->getDescription( m_key )->inputChannels;
	m_processors = DEFAULT_CHANNELS / effect_channels;

	// get inPlaceBroken property
	m_inPlaceBroken = manager->isInplaceBroken( m_key );

	// Categorize the ports, and create the buffers.
	m_portCount = manager->getPortCount( m_key );

	int inputch = 0;
	int outputch = 0;
	std::array<LADSPA_Data*, 2> inbuf;
	inbuf[0] = nullptr;
	inbuf[1] = nullptr;
	for( ch_cnt_t proc = 0; proc < processorCount(); proc++ )
	{
		multi_proc_t ports;
		for( int port = 0; port < m_portCount; port++ )
		{
			auto p = new port_desc_t;

			p->name = manager->getPortName( m_key, port );
			p->proc = proc;
			p->port_id = port;
			p->control = nullptr;
			p->buffer = nullptr;

			// Determine the port's category.
			if( manager->isPortAudio( m_key, port ) )
			{
				if( p->name.toUpper().contains( "IN" ) &&
					manager->isPortInput( m_key, port ) )
				{
					p->rate = BufferRate::ChannelIn;
					p->buffer = new LADSPA_Data[Engine::audioEngine()->framesPerPeriod()];
					inbuf[ inputch ] = p->buffer;
					inputch++;
				}
				else if( p->name.toUpper().contains( "OUT" ) &&
					manager->isPortOutput( m_key, port ) )
				{
					p->rate = BufferRate::ChannelOut;
					if( ! m_inPlaceBroken && inbuf[ outputch ] )
					{
						p->buffer = inbuf[ outputch ];
						outputch++;
					}
					else
					{
						p->buffer = new LADSPA_Data[Engine::audioEngine()->framesPerPeriod()];
						m_inPlaceBroken = true;
					}
				}
				else if( manager->isPortInput( m_key, port ) )
				{
					p->rate = BufferRate::AudioRateInput;
					p->buffer = new LADSPA_Data[Engine::audioEngine()->framesPerPeriod()];
				}
				else
				{
					p->rate = BufferRate::AudioRateOutput;
					p->buffer = new LADSPA_Data[Engine::audioEngine()->framesPerPeriod()];
				}
			}
			else
			{
				p->buffer = new LADSPA_Data[1];

				if( manager->isPortInput( m_key, port ) )
				{
					p->rate = BufferRate::ControlRateInput;
				}
				else
				{
					p->rate = BufferRate::ControlRateOutput;
				}
			}

			p->scale = 1.0f;
			if( manager->isEnum( m_key, port ) )
			{
				p->data_type = BufferDataType::Enum;
			}
			else if( manager->isPortToggled( m_key, port ) )
			{
				p->data_type = BufferDataType::Toggled;
			}
			else if( manager->isInteger( m_key, port ) )
			{
				p->data_type = BufferDataType::Integer;
			}
			else if( p->name.toUpper().contains( "(SECONDS)" ) )
			{
				p->data_type = BufferDataType::Time;
				p->scale = 1000.0f;
				int loc = p->name.toUpper().indexOf(
								"(SECONDS)" );
				p->name.replace( loc, 9, "(ms)" );
			}
			else if( p->name.toUpper().contains( "(S)" ) )
			{
				p->data_type = BufferDataType::Time;
				p->scale = 1000.0f;
				int loc = p->name.toUpper().indexOf( "(S)" );
				p->name.replace( loc, 3, "(ms)" );
			}
			else if( p->name.toUpper().contains( "(MS)" ) )
			{
				p->data_type = BufferDataType::Time;
				int loc = p->name.toUpper().indexOf( "(MS)" );
				p->name.replace( loc, 4, "(ms)" );
			}
			else
			{
				p->data_type = BufferDataType::Floating;
			}

			// Get the range and default values.
			p->max = manager->getUpperBound( m_key, port );
			if( p->max == NOHINT )
			{
				p->max = p->name.toUpper() == "GAIN" ? 10.0f :
					1.0f;
			}

			if( manager->areHintsSampleRateDependent(
								m_key, port ) )
			{
				p->max *= m_maxSampleRate;
			}

			p->min = manager->getLowerBound( m_key, port );
			if( p->min == NOHINT )
			{
				p->min = 0.0f;
			}

			if( manager->areHintsSampleRateDependent(
								m_key, port ) )
			{
				p->min *= m_maxSampleRate;
			}

			p->def = manager->getDefaultSetting( m_key, port );
			if( p->def == NOHINT )
			{
				if( p->data_type != BufferDataType::Toggled )
				{
					p->def = ( p->min + p->max ) / 2.0f;
				}
				else
				{
					p->def = 1.0f;
				}
			}
			else if( manager->areHintsSampleRateDependent( m_key, port ) )
			{
				p->def *= m_maxSampleRate;
			}


			p->max *= p->scale;
			p->min *= p->scale;
			p->def *= p->scale;

			p->value = p->def;

			p->suggests_logscale = manager->isLogarithmic( m_key, port );

			ports.append( p );

	// For convenience, keep a separate list of the ports that are used
	// to control the processors.
			if( p->rate == BufferRate::AudioRateInput ||
					p->rate == BufferRate::ControlRateInput )
			{
				p->control_id = m_portControls.count();
				m_portControls.append( p );
			}
		}
		m_ports.append( ports );
	}

	// Instantiate the processing units.
	m_descriptor = manager->getDescriptor( m_key );
	if( m_descriptor == nullptr )
	{
		QMessageBox::warning( 0, "Effect",
			"Can't get LADSPA descriptor function: " + m_key.second,
			QMessageBox::Ok, QMessageBox::NoButton );
		setOkay( false );
		return;
	}
	if( m_descriptor->run == nullptr )
	{
		QMessageBox::warning( 0, "Effect",
			"Plugin has no processor: " + m_key.second,
			QMessageBox::Ok, QMessageBox::NoButton );
		setDontRun( true );
	}
	for( ch_cnt_t proc = 0; proc < processorCount(); proc++ )
	{
		LADSPA_Handle effect = manager->instantiate( m_key,
							m_maxSampleRate );
		if( effect == nullptr )
		{
			QMessageBox::warning( 0, "Effect",
				"Can't get LADSPA instance: " + m_key.second,
				QMessageBox::Ok, QMessageBox::NoButton );
			setOkay( false );
			return;
		}
		m_handles.append( effect );
	}

	// Connect the ports.
	for( ch_cnt_t proc = 0; proc < processorCount(); proc++ )
	{
		for( int port = 0; port < m_portCount; port++ )
		{
			port_desc_t * pp = m_ports.at( proc ).at( port );
			if( !manager->connectPort( m_key,
						m_handles[proc],
						port,
						pp->buffer ) )
			{
				QMessageBox::warning( 0, "Effect",
				"Failed to connect port: " + m_key.second,
				QMessageBox::Ok, QMessageBox::NoButton );
				setDontRun( true );
				return;
			}
		}
	}

	// Activate the processing units.
	for( ch_cnt_t proc = 0; proc < processorCount(); proc++ )
	{
		manager->activate( m_key, m_handles[proc] );
	}
	m_controls = new LadspaControls( this );
}




void LadspaEffect::pluginDestruction()
{
	if( !isOkay() )
	{
		return;
	}

	delete m_controls;

	for( ch_cnt_t proc = 0; proc < processorCount(); proc++ )
	{
		Ladspa2LMMS * manager = Engine::getLADSPAManager();
		manager->deactivate( m_key, m_handles[proc] );
		manager->cleanup( m_key, m_handles[proc] );
		for( int port = 0; port < m_portCount; port++ )
		{
			port_desc_t * pp = m_ports.at( proc ).at( port );
			if( m_inPlaceBroken || pp->rate != BufferRate::ChannelOut )
			{
				if( pp->buffer) delete[] pp->buffer;
			}
			delete pp;
		}
		m_ports[proc].clear();
	}
	m_ports.clear();
	m_handles.clear();
	m_portControls.clear();
}






static QMap<QString, sample_rate_t> __buggy_plugins;

sample_rate_t LadspaEffect::maxSamplerate( const QString & _name )
{
	if( __buggy_plugins.isEmpty() )
	{
		__buggy_plugins["C* AmpVTS"] = 88200;
		__buggy_plugins["Chorus2"] = 44100;
		__buggy_plugins["Notch Filter"] = 96000;
		__buggy_plugins["TAP Reflector"] = 192000;
	}
	if( __buggy_plugins.contains( _name ) )
	{
		return( __buggy_plugins[_name] );
	}
	return( Engine::audioEngine()->outputSampleRate() );
}




extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model * _parent, void * _data )
{
	return new LadspaEffect( _parent,
		static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>(
								_data ) );
}

}


} // namespace lmms
