/*
 * LadspaManager.cpp - a class to manage loading and instantiation
 *                      of ladspa plugins
 *
 * Copyright (c) 2005-2008 Danny McRae <khjklujn@netscape.net>
 * Copyright (c) 2011-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QLibrary>
#include <QList>
#include <QRegularExpression>

#include <cmath>

#include "ConfigManager.h"
#include "LadspaManager.h"
#include "PluginFactory.h"
#include "lmms_constants.h"


namespace lmms
{


LadspaManager::LadspaManager()
{
	// Make sure plugin search paths are set up
	PluginFactory::setupSearchPaths();

	QList<QRegularExpression> excludePatterns = PluginFactory::getExcludePatterns("LMMS_EXCLUDE_LADSPA");

	QStringList ladspaDirectories = QString( getenv( "LADSPA_PATH" ) ).
								split( LADSPA_PATH_SEPERATOR );
	ladspaDirectories += ConfigManager::inst()->ladspaDir().split( ',' );

	ladspaDirectories.push_back( "plugins:ladspa" );
#ifndef LMMS_BUILD_WIN32
	ladspaDirectories.push_back( qApp->applicationDirPath() + '/' + LIB_DIR + "ladspa" );
	ladspaDirectories.push_back( "/usr/lib/ladspa" );
	ladspaDirectories.push_back( "/usr/lib64/ladspa" );
	ladspaDirectories.push_back( "/usr/local/lib/ladspa" );
	ladspaDirectories.push_back( "/usr/local/lib64/ladspa" );
	ladspaDirectories.push_back( "/Library/Audio/Plug-Ins/LADSPA" );
#endif

	for (const auto& ladspaDirectory : ladspaDirectories)
	{
		// Skip empty entries as QDir will interpret it as the working directory
		if (ladspaDirectory.isEmpty()) { continue; }
		QDir directory(ladspaDirectory);
		QFileInfoList list = directory.entryInfoList();
		for (const auto& f : list)
		{
			bool exclude = false;
			for (const auto& pattern : excludePatterns) {
				if (pattern.match(f.filePath()).hasMatch()) {
					exclude = true;
					break;
				}
			}

			if (exclude || !f.isFile() || f.fileName().right(3).toLower() !=
#ifdef LMMS_BUILD_WIN32
													"dll"
#else
				 									".so"
#endif
								)
			{
				continue;
			}

			QLibrary plugin_lib( f.absoluteFilePath() );

			if( plugin_lib.load() == true )
			{
				auto descriptorFunction = (LADSPA_Descriptor_Function)plugin_lib.resolve("ladspa_descriptor");
				if( descriptorFunction != nullptr )
				{
					addPlugins( descriptorFunction,
							f.fileName() );
				}
			}
			else
			{
				qWarning() << plugin_lib.errorString();
			}
		}
	}
	
	l_ladspa_key_t keys = m_ladspaManagerMap.keys();
	for (const auto& key : keys)
	{
		m_sortedPlugins.append(qMakePair(getName(key), key));
	}
	std::sort( m_sortedPlugins.begin(), m_sortedPlugins.end() );
}




LadspaManager::~LadspaManager()
{
	for( LadspaManagerMapType::iterator it = m_ladspaManagerMap.begin();
					it != m_ladspaManagerMap.end(); ++it )
	{
		delete it.value();
	}
}




LadspaManagerDescription * LadspaManager::getDescription(
						const ladspa_key_t & _plugin )
{
	auto const it = m_ladspaManagerMap.find(_plugin);
	return it != m_ladspaManagerMap.end() ? *it : nullptr;
}




void LadspaManager::addPlugins(
		LADSPA_Descriptor_Function _descriptor_func,
						const QString & _file )
{
	for (long pluginIndex = 0; const auto descriptor = _descriptor_func(pluginIndex); ++pluginIndex)
	{
		ladspa_key_t key( _file, QString( descriptor->Label ) );
		if( m_ladspaManagerMap.contains( key ) )
		{
			continue;
		}

		auto plugIn = new LadspaManagerDescription;
		plugIn->descriptorFunction = _descriptor_func;
		plugIn->index = pluginIndex;
		plugIn->inputChannels = getPluginInputs( descriptor );
		plugIn->outputChannels = getPluginOutputs( descriptor );

		if( plugIn->inputChannels == 0 && plugIn->outputChannels > 0 )
		{
			plugIn->type = LadspaPluginType::Source;
		}
		else if( plugIn->inputChannels > 0 &&
				       plugIn->outputChannels > 0 )
		{
			plugIn->type = LadspaPluginType::Transfer;
		}
		else if( plugIn->inputChannels > 0 &&
				       plugIn->outputChannels == 0 )
		{
			plugIn->type = LadspaPluginType::Sink;
		}
		else
		{
			plugIn->type = LadspaPluginType::Other;
		}

		m_ladspaManagerMap[key] = plugIn;
	}
}




uint16_t LadspaManager::getPluginInputs(
		const LADSPA_Descriptor * _descriptor )
{
	uint16_t inputs = 0;
	
	for( uint16_t port = 0; port < _descriptor->PortCount; port++ )
	{
		if( LADSPA_IS_PORT_INPUT( 
				_descriptor->PortDescriptors[port] ) &&
			LADSPA_IS_PORT_AUDIO( 
				_descriptor->PortDescriptors[port] ) )
		{
			QString name = QString( 
					_descriptor->PortNames[port] );
			if( name.toUpper().contains( "IN" ) )
			{
				inputs++;
			}
		}
	}
	return inputs;
}




uint16_t LadspaManager::getPluginOutputs(
		const LADSPA_Descriptor * _descriptor )
{
	uint16_t outputs = 0;
	
	for( uint16_t port = 0; port < _descriptor->PortCount; port++ )
	{
		if( LADSPA_IS_PORT_OUTPUT( 
				_descriptor->PortDescriptors[port] ) &&
			LADSPA_IS_PORT_AUDIO( 
				_descriptor->PortDescriptors[port] ) )
		{
			QString name = QString( 
					_descriptor->PortNames[port] );
			if( name.toUpper().contains( "OUT" ) )
			{
				outputs++;
			}
		}
	}
	return outputs;
}

const LADSPA_PortDescriptor* LadspaManager::getPortDescriptor(const ladspa_key_t &_plugin, uint32_t _port)
{
	const LADSPA_Descriptor * descriptor = getDescriptor( _plugin );
	if( descriptor && _port < getPortCount( _plugin ) )
	{
		return( & descriptor->PortDescriptors[_port] );
	}
	return( nullptr );
}

const LADSPA_PortRangeHint *LadspaManager::getPortRangeHint(const ladspa_key_t &_plugin, uint32_t _port)
{
	const LADSPA_Descriptor * descriptor = getDescriptor( _plugin );
	if( descriptor && _port < getPortCount( _plugin ) )
	{
		return( & descriptor->PortRangeHints[_port] );
	}
	return( nullptr );
}




l_sortable_plugin_t LadspaManager::getSortedPlugins()
{
	return( m_sortedPlugins );
}




QString LadspaManager::getLabel( const ladspa_key_t & _plugin )
{
	const LADSPA_Descriptor * descriptor = getDescriptor( _plugin );
	return( descriptor ? descriptor->Label : "" );
}




bool LadspaManager::hasRealTimeDependency(
					const ladspa_key_t &  _plugin )
{
	const LADSPA_Descriptor * descriptor = getDescriptor( _plugin );
	return( descriptor ? LADSPA_IS_REALTIME( descriptor->Properties )
					   : false );
}




bool LadspaManager::isInplaceBroken( const ladspa_key_t &  _plugin )
{
	const LADSPA_Descriptor * descriptor = getDescriptor( _plugin );
	return( descriptor ? LADSPA_IS_INPLACE_BROKEN( descriptor->Properties )
					   : false );
}




bool LadspaManager::isRealTimeCapable(
					const ladspa_key_t &  _plugin )
{
	const LADSPA_Descriptor * descriptor = getDescriptor( _plugin );
	return( descriptor ? LADSPA_IS_HARD_RT_CAPABLE( descriptor->Properties )
					   : false );
}




QString LadspaManager::getName( const ladspa_key_t & _plugin )
{
	const LADSPA_Descriptor * descriptor = getDescriptor( _plugin );
	return( descriptor ? descriptor->Name : "" );
}




QString LadspaManager::getMaker( const ladspa_key_t & _plugin )
{
	const LADSPA_Descriptor * descriptor = getDescriptor( _plugin );
	return( descriptor ? descriptor->Maker : "" );
}




QString LadspaManager::getCopyright( const ladspa_key_t & _plugin )
{
	const LADSPA_Descriptor * descriptor = getDescriptor( _plugin );
	return( descriptor ? descriptor->Copyright : "" );
}




uint32_t LadspaManager::getPortCount( const ladspa_key_t & _plugin )
{
	const LADSPA_Descriptor * descriptor = getDescriptor( _plugin );
	return( descriptor ? descriptor->PortCount : 0 );
}




bool LadspaManager::isPortInput( const ladspa_key_t & _plugin,
								uint32_t _port )
{
	const auto * descriptor = getPortDescriptor( _plugin, _port );
	return( descriptor && LADSPA_IS_PORT_INPUT( * descriptor ) );
}




bool LadspaManager::isPortOutput( const ladspa_key_t & _plugin,
								uint32_t _port )
{
	const auto * descriptor = getPortDescriptor( _plugin, _port );
	return( descriptor && LADSPA_IS_PORT_OUTPUT( * descriptor ) );
}




bool LadspaManager::isPortAudio( const ladspa_key_t & _plugin,
								uint32_t _port )
{
	const auto * descriptor = getPortDescriptor( _plugin, _port );
	return( descriptor && LADSPA_IS_PORT_AUDIO( * descriptor ) );
}




bool LadspaManager::isPortControl( const ladspa_key_t & _plugin,
								uint32_t _port )
{
	const auto * descriptor = getPortDescriptor( _plugin, _port );
	return( descriptor && LADSPA_IS_PORT_CONTROL( * descriptor ) );
}




bool LadspaManager::areHintsSampleRateDependent(
						const ladspa_key_t & _plugin, 
								uint32_t _port )
{
	const auto* portRangeHint = getPortRangeHint( _plugin, _port );
	return portRangeHint && LADSPA_IS_HINT_SAMPLE_RATE( portRangeHint->HintDescriptor );
}




float LadspaManager::getLowerBound( const ladspa_key_t & _plugin,
								uint32_t _port )
{
	const auto* portRangeHint = getPortRangeHint( _plugin, _port );
	if( portRangeHint && LADSPA_IS_HINT_BOUNDED_BELOW( portRangeHint->HintDescriptor ) )
	{
		return( portRangeHint->LowerBound );
	}
	return( NOHINT );
}




float LadspaManager::getUpperBound( const ladspa_key_t & _plugin,
									uint32_t _port )
{
	const auto* portRangeHint = getPortRangeHint( _plugin, _port );
	if( portRangeHint && LADSPA_IS_HINT_BOUNDED_ABOVE( portRangeHint->HintDescriptor ) )
	{
		return( portRangeHint->UpperBound );
	}
	return( NOHINT );
}




bool LadspaManager::isPortToggled( const ladspa_key_t & _plugin,
								uint32_t _port )
{
	const auto* portRangeHint = getPortRangeHint( _plugin, _port );
	return( portRangeHint && LADSPA_IS_HINT_TOGGLED( portRangeHint->HintDescriptor ) );
}




float LadspaManager::getDefaultSetting( const ladspa_key_t & _plugin,
							uint32_t _port )
{
	const auto* portRangeHint = getPortRangeHint( _plugin, _port );
	if( portRangeHint )
	{
		LADSPA_PortRangeHintDescriptor hintDescriptor = portRangeHint->HintDescriptor;
		switch( hintDescriptor & LADSPA_HINT_DEFAULT_MASK ) 
		{
			case LADSPA_HINT_DEFAULT_NONE:
				return( NOHINT );
			case LADSPA_HINT_DEFAULT_MINIMUM:
				return( portRangeHint->LowerBound );
			case LADSPA_HINT_DEFAULT_LOW:
				if( LADSPA_IS_HINT_LOGARITHMIC
							( hintDescriptor ) )
				{
					return std::exp(std::log(portRangeHint->LowerBound)
						* 0.75 + std::log(portRangeHint->UpperBound) * 0.25);
				}
				else 
				{
					return( portRangeHint->LowerBound * 0.75 +
							portRangeHint->UpperBound * 0.25 );
				}
			case LADSPA_HINT_DEFAULT_MIDDLE:
				if( LADSPA_IS_HINT_LOGARITHMIC
						( hintDescriptor ) ) 
				{
					return std::sqrt(portRangeHint->LowerBound * portRangeHint->UpperBound);
				}
				else 
				{
					return( 0.5 * ( portRangeHint->LowerBound
							+ portRangeHint->UpperBound ) );
				}
			case LADSPA_HINT_DEFAULT_HIGH:
				if( LADSPA_IS_HINT_LOGARITHMIC
						( hintDescriptor ) ) 
				{
					return std::exp(std::log(portRangeHint->LowerBound)
						* 0.25 + std::log(portRangeHint->UpperBound) * 0.75);
				}
				else 
				{
					return( portRangeHint->LowerBound * 0.25 +
							portRangeHint->UpperBound * 0.75 );
				}
			case LADSPA_HINT_DEFAULT_MAXIMUM:
				return( portRangeHint->UpperBound );
			case LADSPA_HINT_DEFAULT_0:
				return( 0.0 );
			case LADSPA_HINT_DEFAULT_1:
				return( 1.0 );
			case LADSPA_HINT_DEFAULT_100:
				return( 100.0 );
			case LADSPA_HINT_DEFAULT_440:
				return( 440.0 );
			default:
				return( NOHINT );
		}
	}
	else
	{
		return( NOHINT );
	}
}




bool LadspaManager::isLogarithmic( const ladspa_key_t & _plugin,
								uint32_t _port )
{
	const auto* portRangeHint = getPortRangeHint( _plugin, _port );
	return( portRangeHint && LADSPA_IS_HINT_LOGARITHMIC( portRangeHint->HintDescriptor ) );
}




bool LadspaManager::isInteger( const ladspa_key_t & _plugin,
								uint32_t _port )
{
	const auto* portRangeHint = getPortRangeHint( _plugin, _port );
	return( portRangeHint && LADSPA_IS_HINT_INTEGER( portRangeHint->HintDescriptor ) );
}




bool LadspaManager::isEnum( const ladspa_key_t & _plugin, uint32_t _port )
{
	auto const * desc = getDescriptor(_plugin);
	if (desc && _port < desc->PortCount)
	{
		LADSPA_PortRangeHintDescriptor hintDescriptor =
			desc->PortRangeHints[_port].HintDescriptor;
		// This is an LMMS extension to ladspa
		return LADSPA_IS_HINT_INTEGER(hintDescriptor) && LADSPA_IS_HINT_TOGGLED(hintDescriptor);
	}

	return false;
}




QString LadspaManager::getPortName( const ladspa_key_t & _plugin,
								uint32_t _port )
{
	const LADSPA_Descriptor * descriptor = getDescriptor( _plugin );
	return( descriptor ? descriptor->PortNames[_port] : QString( "" ) );
}




const void * LadspaManager::getImplementationData(
						const ladspa_key_t & _plugin )
{
	const LADSPA_Descriptor * descriptor = getDescriptor( _plugin );
	return( descriptor ? descriptor->ImplementationData : nullptr );
}




const LADSPA_Descriptor * LadspaManager::getDescriptor(const ladspa_key_t & _plugin)
{
	auto const it = m_ladspaManagerMap.find(_plugin);
	if (it != m_ladspaManagerMap.end())
	{
		auto const plugin = *it;

		LADSPA_Descriptor_Function descriptorFunction = plugin->descriptorFunction;
		const LADSPA_Descriptor* descriptor = descriptorFunction(plugin->index);

		return descriptor;
	}

	return nullptr;
}




LADSPA_Handle LadspaManager::instantiate(
					const ladspa_key_t & _plugin, 
							uint32_t _sample_rate )
{
	const LADSPA_Descriptor * descriptor = getDescriptor( _plugin );
	return( descriptor ?
				( descriptor->instantiate )( descriptor, _sample_rate ) :
				nullptr );
}




bool LadspaManager::connectPort( const ladspa_key_t & _plugin,
						LADSPA_Handle _instance, 
						uint32_t _port,
						LADSPA_Data * _data_location )
{
	const LADSPA_Descriptor * descriptor = getDescriptor( _plugin );
	if( descriptor && descriptor->connect_port != nullptr &&
			_port < getPortCount( _plugin ) )
	{
		( descriptor->connect_port )
				( _instance, _port, _data_location );
		return( true );
	}
	return( false );
}




bool LadspaManager::activate( const ladspa_key_t & _plugin,
					LADSPA_Handle _instance )
{
	const LADSPA_Descriptor * descriptor = getDescriptor( _plugin );
	if( descriptor && descriptor->activate != nullptr )
	{
		( descriptor->activate ) ( _instance );
		return( true );
	}
	return( false );
}




bool LadspaManager::run( const ladspa_key_t & _plugin,
							LADSPA_Handle _instance,
							uint32_t _sample_count )
{
	const LADSPA_Descriptor * descriptor = getDescriptor( _plugin );
	if( descriptor && descriptor->run!= nullptr )
	{
		( descriptor->run ) ( _instance, _sample_count );
		return( true );
	}
	return( false );
}




bool LadspaManager::runAdding( const ladspa_key_t & _plugin,
						LADSPA_Handle _instance,
						uint32_t _sample_count )
{
	const LADSPA_Descriptor * descriptor = getDescriptor( _plugin );
	if( descriptor && descriptor->run_adding!= nullptr
			&& descriptor->set_run_adding_gain != nullptr )
	{
		( descriptor->run_adding ) ( _instance, _sample_count );
		return( true );
	}
	return( false );
}




bool LadspaManager::setRunAddingGain( const ladspa_key_t & _plugin,
						LADSPA_Handle _instance,
						LADSPA_Data _gain )
{
	const LADSPA_Descriptor * descriptor = getDescriptor( _plugin );
	if( descriptor && descriptor->run_adding!= nullptr
			&& descriptor->set_run_adding_gain != nullptr )
	{
		( descriptor->set_run_adding_gain ) ( _instance, _gain );
		return( true );
	}
	return( false );
}




bool LadspaManager::deactivate( const ladspa_key_t & _plugin,
						LADSPA_Handle _instance )
{
	const LADSPA_Descriptor * descriptor = getDescriptor( _plugin );
	if( descriptor && descriptor->deactivate!= nullptr )
	{
		( descriptor->deactivate ) ( _instance );
		return( true );
	}
	return( false );
}




bool LadspaManager::cleanup( const ladspa_key_t & _plugin,
						LADSPA_Handle _instance )
{
	const LADSPA_Descriptor * descriptor = getDescriptor( _plugin );
	if( descriptor && descriptor->cleanup!= nullptr )
	{
		( descriptor->cleanup ) ( _instance );
		return( true );
	}
	return( false );
}


} // namespace lmms
