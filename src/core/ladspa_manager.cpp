/*
 * ladspa_manager.cpp - a class to manage loading and instantiation
 *                      of ladspa plugins
 *
 * Copyright (c) 2005-2008 Danny McRae <khjklujn@netscape.net>
 * Copyright (c) 2011-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QLibrary>

#include <math.h>

#include "config_mgr.h"
#include "ladspa_manager.h"



ladspaManager::ladspaManager()
{
	QStringList ladspaDirectories = QString( getenv( "LADSPA_PATH" ) ).
								split( LADSPA_PATH_SEPERATOR );
	ladspaDirectories += configManager::inst()->ladspaDir().split( ',' );

	ladspaDirectories.push_back( configManager::inst()->pluginDir() + "ladspa" );
#ifndef LMMS_BUILD_WIN32
	ladspaDirectories.push_back( qApp->applicationDirPath() + '/' + LIB_DIR + "ladspa" );
	ladspaDirectories.push_back( "/usr/lib/ladspa" );
	ladspaDirectories.push_back( "/usr/lib64/ladspa" );
	ladspaDirectories.push_back( "/usr/local/lib/ladspa" );
	ladspaDirectories.push_back( "/usr/local/lib64/ladspa" );
	ladspaDirectories.push_back( "/Library/Audio/Plug-Ins/LADSPA" );
#endif

	for( QStringList::iterator it = ladspaDirectories.begin(); 
			 		   it != ladspaDirectories.end(); ++it )
	{
		QDir directory( ( *it ) );
		QFileInfoList list = directory.entryInfoList();
		for( QFileInfoList::iterator file = list.begin();
						file != list.end(); ++file )
		{
			const QFileInfo & f = *file;
			if( !f.isFile() ||
				 f.fileName().right( 3 ).toLower() !=
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
				LADSPA_Descriptor_Function descriptorFunction =
			( LADSPA_Descriptor_Function ) plugin_lib.resolve(
							"ladspa_descriptor" );
				if( descriptorFunction != NULL )
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
	for( l_ladspa_key_t::iterator it = keys.begin();
		    it != keys.end(); it++ )
	{
		m_sortedPlugins.append( qMakePair( getName( *it ), *it ) );
	}
	qSort( m_sortedPlugins );
}




ladspaManager::~ladspaManager()
{
	for( ladspaManagerMapType::iterator it = m_ladspaManagerMap.begin();
					it != m_ladspaManagerMap.end(); ++it )
	{
		delete it.value();
	}
}




ladspaManagerDescription * ladspaManager::getDescription( 
						const ladspa_key_t & _plugin )
{
	if( m_ladspaManagerMap.contains( _plugin ) )
	{
		return( m_ladspaManagerMap[_plugin] );
	}
	else
	{
		return( NULL );
	}
}




void ladspaManager::addPlugins(
		LADSPA_Descriptor_Function _descriptor_func,
						const QString & _file )
{
	const LADSPA_Descriptor * descriptor;

	for( long pluginIndex = 0;
		( descriptor = _descriptor_func( pluginIndex ) ) != NULL;
								++pluginIndex )
	{
		ladspa_key_t key( _file, QString( descriptor->Label ) );
		if( m_ladspaManagerMap.contains( key ) )
		{
			continue;
		}

		ladspaManagerDescription * plugIn = 
				new ladspaManagerDescription;
		plugIn->descriptorFunction = _descriptor_func;
		plugIn->index = pluginIndex;
		plugIn->inputChannels = getPluginInputs( descriptor );
		plugIn->outputChannels = getPluginOutputs( descriptor );

		if( plugIn->inputChannels == 0 && plugIn->outputChannels > 0 )
		{
			plugIn->type = SOURCE;
		}
		else if( plugIn->inputChannels > 0 &&
				       plugIn->outputChannels > 0 )
		{
			plugIn->type = TRANSFER;
		}
		else if( plugIn->inputChannels > 0 &&
				       plugIn->outputChannels == 0 )
		{
			plugIn->type = SINK;
		}
		else
		{
			plugIn->type = OTHER;
		}

		m_ladspaManagerMap[key] = plugIn;
	}
}




uint16_t ladspaManager::getPluginInputs( 
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




uint16_t ladspaManager::getPluginOutputs( 
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




l_sortable_plugin_t ladspaManager::getSortedPlugins()
{
	return( m_sortedPlugins );
}




QString ladspaManager::getLabel( const ladspa_key_t & _plugin )
{
	if( m_ladspaManagerMap.contains( _plugin ) )
	{
		LADSPA_Descriptor_Function descriptorFunction =
			m_ladspaManagerMap[_plugin]->descriptorFunction;
		const LADSPA_Descriptor * descriptor = 
				descriptorFunction(
					m_ladspaManagerMap[_plugin]->index );
		return( QString( descriptor->Label ) );
	}
	else
	{
		return( QString( "" ) );
	}
}




bool ladspaManager::hasRealTimeDependency( 
					const ladspa_key_t &  _plugin )
{
	if( m_ladspaManagerMap.contains( _plugin ) )
	{
		LADSPA_Descriptor_Function descriptorFunction =
			m_ladspaManagerMap[_plugin]->descriptorFunction;
		const LADSPA_Descriptor * descriptor =
				descriptorFunction(
					m_ladspaManagerMap[_plugin]->index );
		return( LADSPA_IS_REALTIME( descriptor->Properties ) );
	}
	else
	{
		return( false );
	}
}




bool ladspaManager::isInplaceBroken( const ladspa_key_t &  _plugin )
{
	if( m_ladspaManagerMap.contains( _plugin ) )
	{
		LADSPA_Descriptor_Function descriptorFunction =
			m_ladspaManagerMap[_plugin]->descriptorFunction;
		const LADSPA_Descriptor * descriptor =
				descriptorFunction(
					m_ladspaManagerMap[_plugin]->index );
		return( LADSPA_IS_INPLACE_BROKEN( descriptor->Properties ) );
	}
	else
	{
		return( false );
	}
}




bool ladspaManager::isRealTimeCapable( 
					const ladspa_key_t &  _plugin )
{
	if( m_ladspaManagerMap.contains( _plugin ) )
	{
		LADSPA_Descriptor_Function descriptorFunction =
			m_ladspaManagerMap[_plugin]->descriptorFunction;
		const LADSPA_Descriptor * descriptor =
				descriptorFunction(
					m_ladspaManagerMap[_plugin]->index );
		return( LADSPA_IS_HARD_RT_CAPABLE( descriptor->Properties ) );
	}
	else
	{
		return( false );
	}
}




QString ladspaManager::getName( const ladspa_key_t & _plugin )
{
	if( m_ladspaManagerMap.contains( _plugin ) )
	{
		LADSPA_Descriptor_Function descriptorFunction =
			m_ladspaManagerMap[_plugin]->descriptorFunction;
		const LADSPA_Descriptor * descriptor =
				descriptorFunction(
					m_ladspaManagerMap[_plugin]->index );
		return( QString( descriptor->Name ) );
	}
	else
	{
		return( QString( "" ) );
	}
}




QString ladspaManager::getMaker( const ladspa_key_t & _plugin )
{
	if( m_ladspaManagerMap.contains( _plugin ) )
	{
		LADSPA_Descriptor_Function descriptorFunction =
			m_ladspaManagerMap[_plugin]->descriptorFunction;
		const LADSPA_Descriptor * descriptor =
				descriptorFunction(
					m_ladspaManagerMap[_plugin]->index );
		return( QString( descriptor->Maker ) );
	}
	else
	{
		return( QString( "" ) );
	}
}




QString ladspaManager::getCopyright( const ladspa_key_t & _plugin )
{
	if( m_ladspaManagerMap.contains( _plugin ) )
	{
		LADSPA_Descriptor_Function descriptorFunction =
			m_ladspaManagerMap[_plugin]->descriptorFunction;
		const LADSPA_Descriptor * descriptor =
				descriptorFunction(
					m_ladspaManagerMap[_plugin]->index );
		return( QString( descriptor->Copyright ) );
	}
	else
	{
		return( QString( "" ) );
	}
}




uint32_t ladspaManager::getPortCount( const ladspa_key_t & _plugin )
{
	if( m_ladspaManagerMap.contains( _plugin ) )
	{
		LADSPA_Descriptor_Function descriptorFunction =
			m_ladspaManagerMap[_plugin]->descriptorFunction;
		const LADSPA_Descriptor * descriptor =
				descriptorFunction(
					m_ladspaManagerMap[_plugin]->index );
		return( descriptor->PortCount );
	}
	else
	{
		return( 0 );
	}
}




bool ladspaManager::isPortInput( const ladspa_key_t & _plugin, 
								uint32_t _port )
{
	if( m_ladspaManagerMap.contains( _plugin ) 
		&& _port < getPortCount( _plugin ) )
	{
		LADSPA_Descriptor_Function descriptorFunction =
			m_ladspaManagerMap[_plugin]->descriptorFunction;
		const LADSPA_Descriptor * descriptor =
				descriptorFunction(
					m_ladspaManagerMap[_plugin]->index );
		
		return( LADSPA_IS_PORT_INPUT
				( descriptor->PortDescriptors[_port] ) );
	}
	else
	{
		return( false );
	}
}




bool ladspaManager::isPortOutput( const ladspa_key_t & _plugin, 
								uint32_t _port )
{
	if( m_ladspaManagerMap.contains( _plugin ) 
		   && _port < getPortCount( _plugin ) )
	{
		LADSPA_Descriptor_Function descriptorFunction =
			m_ladspaManagerMap[_plugin]->descriptorFunction;
		const LADSPA_Descriptor * descriptor =
				descriptorFunction(
					m_ladspaManagerMap[_plugin]->index );
		
		return( LADSPA_IS_PORT_OUTPUT
				( descriptor->PortDescriptors[_port] ) );
	}
	else
	{
		return( false );
	}
}




bool ladspaManager::isPortAudio( const ladspa_key_t & _plugin,
								uint32_t _port )
{
	if( m_ladspaManagerMap.contains( _plugin ) 
		   && _port < getPortCount( _plugin ) )
	{
		LADSPA_Descriptor_Function descriptorFunction =
			m_ladspaManagerMap[_plugin]->descriptorFunction;
		const LADSPA_Descriptor * descriptor =
				descriptorFunction(
					m_ladspaManagerMap[_plugin]->index );
		
		return( LADSPA_IS_PORT_AUDIO
				( descriptor->PortDescriptors[_port] ) );
	}
	else
	{
		return( false );
	}
}




bool ladspaManager::isPortControl( const ladspa_key_t & _plugin, 
								uint32_t _port )
{
	if( m_ladspaManagerMap.contains( _plugin ) 
		   && _port < getPortCount( _plugin ) )
	{
		LADSPA_Descriptor_Function descriptorFunction =
			m_ladspaManagerMap[_plugin]->descriptorFunction;
		const LADSPA_Descriptor * descriptor =
				descriptorFunction(
					m_ladspaManagerMap[_plugin]->index );
		
		return( LADSPA_IS_PORT_CONTROL
				( descriptor->PortDescriptors[_port] ) );
	}
	else
	{
		return( false );
	}
}




bool ladspaManager::areHintsSampleRateDependent(
						const ladspa_key_t & _plugin, 
								uint32_t _port )
{
	if( m_ladspaManagerMap.contains( _plugin ) 
		   && _port < getPortCount( _plugin ) )
	{
		LADSPA_Descriptor_Function descriptorFunction =
			m_ladspaManagerMap[_plugin]->descriptorFunction;
		const LADSPA_Descriptor * descriptor =
				descriptorFunction(
					m_ladspaManagerMap[_plugin]->index );
		LADSPA_PortRangeHintDescriptor hintDescriptor =
			descriptor->PortRangeHints[_port].HintDescriptor;
		return( LADSPA_IS_HINT_SAMPLE_RATE ( hintDescriptor ) );
	}
	else
	{
		return( false );
	}
}




float ladspaManager::getLowerBound( const ladspa_key_t & _plugin,
								uint32_t _port )
{
	if( m_ladspaManagerMap.contains( _plugin ) 
		   && _port < getPortCount( _plugin ) )
	{
		LADSPA_Descriptor_Function descriptorFunction =
			m_ladspaManagerMap[_plugin]->descriptorFunction;
		const LADSPA_Descriptor * descriptor =
				descriptorFunction(
					m_ladspaManagerMap[_plugin]->index );
		LADSPA_PortRangeHintDescriptor hintDescriptor =
			descriptor->PortRangeHints[_port].HintDescriptor;
		if( LADSPA_IS_HINT_BOUNDED_BELOW( hintDescriptor ) )
		{
			return( descriptor->PortRangeHints[_port].LowerBound );
		}
		else
		{
			return( NOHINT );
		}
	}
	else
	{
		return( NOHINT );
	}
}




float ladspaManager::getUpperBound( const ladspa_key_t & _plugin,									uint32_t _port )
{
	if( m_ladspaManagerMap.contains( _plugin ) 
		   && _port < getPortCount( _plugin ) )
	{
		LADSPA_Descriptor_Function descriptorFunction =
			m_ladspaManagerMap[_plugin]->descriptorFunction;
		const LADSPA_Descriptor * descriptor =
				descriptorFunction(
					m_ladspaManagerMap[_plugin]->index );
		LADSPA_PortRangeHintDescriptor hintDescriptor =
			descriptor->PortRangeHints[_port].HintDescriptor;
		if( LADSPA_IS_HINT_BOUNDED_ABOVE( hintDescriptor ) )
		{
			return( descriptor->PortRangeHints[_port].UpperBound );
		}
		else
		{
			return( NOHINT );
		}
	}
	else
	{
		return( NOHINT );
	}
}




bool ladspaManager::isPortToggled( const ladspa_key_t & _plugin, 
								uint32_t _port )
{
	if( m_ladspaManagerMap.contains( _plugin ) 
		   && _port < getPortCount( _plugin ) )
	{
		LADSPA_Descriptor_Function descriptorFunction =
			m_ladspaManagerMap[_plugin]->descriptorFunction;
		const LADSPA_Descriptor * descriptor =
				descriptorFunction(
					m_ladspaManagerMap[_plugin]->index );
		LADSPA_PortRangeHintDescriptor hintDescriptor =
			descriptor->PortRangeHints[_port].HintDescriptor;
		return( LADSPA_IS_HINT_TOGGLED( hintDescriptor ) );
	}
	else
	{
		return( false );
	}
}




float ladspaManager::getDefaultSetting( const ladspa_key_t & _plugin, 
							uint32_t _port )
{
	if( m_ladspaManagerMap.contains( _plugin ) 
		   && _port < getPortCount( _plugin ) )
	{
		LADSPA_Descriptor_Function descriptorFunction =
			m_ladspaManagerMap[_plugin]->descriptorFunction;
		const LADSPA_Descriptor * descriptor =
				descriptorFunction(
					m_ladspaManagerMap[_plugin]->index );
		LADSPA_PortRangeHintDescriptor hintDescriptor =
			descriptor->PortRangeHints[_port].HintDescriptor;
		switch( hintDescriptor & LADSPA_HINT_DEFAULT_MASK ) 
		{
			case LADSPA_HINT_DEFAULT_NONE:
				return( NOHINT );
			case LADSPA_HINT_DEFAULT_MINIMUM:
				return( descriptor->PortRangeHints[_port].
								LowerBound );
			case LADSPA_HINT_DEFAULT_LOW:
				if( LADSPA_IS_HINT_LOGARITHMIC
							( hintDescriptor ) )
				{
					return( exp( log( descriptor->PortRangeHints[_port].LowerBound ) 
						* 0.75
						+ log( descriptor->PortRangeHints[_port].UpperBound ) 
						* 0.25 ) );
				}
				else 
				{
					return( descriptor->PortRangeHints[_port].LowerBound
						* 0.75
						+ descriptor->PortRangeHints[_port].UpperBound
						* 0.25 );
				}
			case LADSPA_HINT_DEFAULT_MIDDLE:
				if( LADSPA_IS_HINT_LOGARITHMIC
						( hintDescriptor ) ) 
				{
					return( sqrt( descriptor->PortRangeHints[_port].LowerBound
						* descriptor->PortRangeHints[_port].UpperBound ) );
				}
				else 
				{
					return( 0.5 * ( descriptor->PortRangeHints[_port].LowerBound
							+ descriptor->PortRangeHints[_port].UpperBound ) );
				}
			case LADSPA_HINT_DEFAULT_HIGH:
				if( LADSPA_IS_HINT_LOGARITHMIC
						( hintDescriptor ) ) 
				{
					return( exp( log( descriptor->PortRangeHints[_port].LowerBound ) 
						* 0.25
						+ log( descriptor->PortRangeHints[_port].UpperBound ) 
						* 0.75 ) );
				}
				else 
				{
					return( descriptor->PortRangeHints[_port].LowerBound
						* 0.25
						+ descriptor->PortRangeHints[_port].UpperBound
						* 0.75 );
				}
			case LADSPA_HINT_DEFAULT_MAXIMUM:
				return( descriptor->PortRangeHints[_port].UpperBound );
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




bool ladspaManager::isLogarithmic( const ladspa_key_t & _plugin,
								uint32_t _port )
{
	if( m_ladspaManagerMap.contains( _plugin ) 
		   && _port < getPortCount( _plugin ) )
	{
		LADSPA_Descriptor_Function descriptorFunction =
			m_ladspaManagerMap[_plugin]->descriptorFunction;
		const LADSPA_Descriptor * descriptor =
				descriptorFunction(
					m_ladspaManagerMap[_plugin]->index );
		LADSPA_PortRangeHintDescriptor hintDescriptor =
			descriptor->PortRangeHints[_port].HintDescriptor;
		return( LADSPA_IS_HINT_LOGARITHMIC( hintDescriptor ) );
	}
	else
	{
		return( false );
	}
}




bool ladspaManager::isInteger( const ladspa_key_t & _plugin,
								uint32_t _port )
{
	if( m_ladspaManagerMap.contains( _plugin ) 
		   && _port < getPortCount( _plugin ) )
	{
		LADSPA_Descriptor_Function descriptorFunction =
			m_ladspaManagerMap[_plugin]->descriptorFunction;
		const LADSPA_Descriptor * descriptor =
				descriptorFunction(
					m_ladspaManagerMap[_plugin]->index );
		LADSPA_PortRangeHintDescriptor hintDescriptor =
			descriptor->PortRangeHints[_port].HintDescriptor;
		return( LADSPA_IS_HINT_INTEGER( hintDescriptor ) );
	}
	else
	{
		return( false );
	}
}




QString ladspaManager::getPortName( const ladspa_key_t & _plugin,
								uint32_t _port )
{
	if( m_ladspaManagerMap.contains( _plugin ) &&
					_port < getPortCount( _plugin ) )
	{
		LADSPA_Descriptor_Function descriptorFunction =
			m_ladspaManagerMap[_plugin]->descriptorFunction;
		const LADSPA_Descriptor * descriptor =
				descriptorFunction(
					m_ladspaManagerMap[_plugin]->index );

		return( QString( descriptor->PortNames[_port] ) );
	}
	else
	{
		return( QString( "" ) );
	}
}




const void * ladspaManager::getImplementationData(
						const ladspa_key_t & _plugin )
{
	if( m_ladspaManagerMap.contains( _plugin ) )
	{
		LADSPA_Descriptor_Function descriptorFunction =
			m_ladspaManagerMap[_plugin]->descriptorFunction;
		const LADSPA_Descriptor * descriptor =
				descriptorFunction(
					m_ladspaManagerMap[_plugin]->index );
		return( descriptor->ImplementationData );
	}
	else
	{
		return( NULL );
	}
}




const LADSPA_Descriptor * ladspaManager::getDescriptor( 
						const ladspa_key_t & _plugin )
{
	if( m_ladspaManagerMap.contains( _plugin ) )
	{
		LADSPA_Descriptor_Function descriptorFunction =
			m_ladspaManagerMap[_plugin]->descriptorFunction;
		const LADSPA_Descriptor * descriptor =
				descriptorFunction(
					m_ladspaManagerMap[_plugin]->index );
		return( descriptor );
	}
	else
	{
		return( NULL );
	}
}




LADSPA_Handle ladspaManager::instantiate( 
					const ladspa_key_t & _plugin, 
							uint32_t _sample_rate )
{
	if( m_ladspaManagerMap.contains( _plugin ) )
	{
		LADSPA_Descriptor_Function descriptorFunction =
			m_ladspaManagerMap[_plugin]->descriptorFunction;
		const LADSPA_Descriptor * descriptor =
				descriptorFunction(
					m_ladspaManagerMap[_plugin]->index );
		return( ( descriptor->instantiate )
						( descriptor, _sample_rate ) );
	}
	else
	{
		return( NULL );
	}
}




bool ladspaManager::connectPort( const ladspa_key_t & _plugin, 
						LADSPA_Handle _instance, 
						uint32_t _port,
						LADSPA_Data * _data_location )
{
	if( m_ladspaManagerMap.contains( _plugin ) 
		&& _port < getPortCount( _plugin ) )
	{
		LADSPA_Descriptor_Function descriptorFunction =
			m_ladspaManagerMap[_plugin]->descriptorFunction;
		const LADSPA_Descriptor * descriptor =
				descriptorFunction(
					m_ladspaManagerMap[_plugin]->index );
		if( descriptor->connect_port != NULL )
		{
			( descriptor->connect_port )
					( _instance, _port, _data_location );
			return( true );
		}
	}
	return( false );
}




bool ladspaManager::activate( const ladspa_key_t & _plugin, 
					LADSPA_Handle _instance )
{
	if( m_ladspaManagerMap.contains( _plugin ) )
	{
		LADSPA_Descriptor_Function descriptorFunction =
			m_ladspaManagerMap[_plugin]->descriptorFunction;
		const LADSPA_Descriptor * descriptor =
				descriptorFunction(
					m_ladspaManagerMap[_plugin]->index );
		if( descriptor->activate != NULL )
		{
			( descriptor->activate ) ( _instance );
			return( true );
		}
	}
	return( false );
}




bool ladspaManager::run( const ladspa_key_t & _plugin,
							LADSPA_Handle _instance,
							uint32_t _sample_count )
{
	if( m_ladspaManagerMap.contains( _plugin ) )
	{
		LADSPA_Descriptor_Function descriptorFunction =
			m_ladspaManagerMap[_plugin]->descriptorFunction;
		const LADSPA_Descriptor * descriptor =
				descriptorFunction(
					m_ladspaManagerMap[_plugin]->index );
		if( descriptor->run != NULL )
		{
			( descriptor->run ) ( _instance, _sample_count );
			return( true );
		}
	}
	return( false );
}




bool ladspaManager::runAdding( const ladspa_key_t & _plugin, 
						LADSPA_Handle _instance,
						uint32_t _sample_count )
{
	if( m_ladspaManagerMap.contains( _plugin ) )
	{
		LADSPA_Descriptor_Function descriptorFunction =
			m_ladspaManagerMap[_plugin]->descriptorFunction;
		const LADSPA_Descriptor * descriptor =
				descriptorFunction(
					m_ladspaManagerMap[_plugin]->index );
		if( descriptor->run_adding != NULL &&
			  	descriptor->set_run_adding_gain != NULL )
		{
			( descriptor->run_adding ) ( _instance, _sample_count );
			return( true );
		}
	}
	return( false );
}




bool ladspaManager::setRunAddingGain( const ladspa_key_t & _plugin, 
						LADSPA_Handle _instance,
						LADSPA_Data _gain )
{
	if( m_ladspaManagerMap.contains( _plugin ) )
	{
		LADSPA_Descriptor_Function descriptorFunction =
			m_ladspaManagerMap[_plugin]->descriptorFunction;
		const LADSPA_Descriptor * descriptor =
				descriptorFunction(
					m_ladspaManagerMap[_plugin]->index );
		if( descriptor->run_adding != NULL &&
				  descriptor->set_run_adding_gain != NULL )
		{
			( descriptor->set_run_adding_gain )
							( _instance, _gain );
			return( true );
		}
	}
	return( false );
}




bool ladspaManager::deactivate( const ladspa_key_t & _plugin, 
						LADSPA_Handle _instance )
{
	if( m_ladspaManagerMap.contains( _plugin ) )
	{
		LADSPA_Descriptor_Function descriptorFunction =
			m_ladspaManagerMap[_plugin]->descriptorFunction;
		const LADSPA_Descriptor * descriptor =
				descriptorFunction(
					m_ladspaManagerMap[_plugin]->index );
		if( descriptor->deactivate != NULL )
		{
			( descriptor->deactivate ) ( _instance );
			return( true );
		}
	}
	return( false );
}




bool ladspaManager::cleanup( const ladspa_key_t & _plugin, 
						LADSPA_Handle _instance )
{
	if( m_ladspaManagerMap.contains( _plugin ) )
	{
		LADSPA_Descriptor_Function descriptorFunction =
			m_ladspaManagerMap[_plugin]->descriptorFunction;
		const LADSPA_Descriptor * descriptor =
				descriptorFunction(
					m_ladspaManagerMap[_plugin]->index );
		if( descriptor->cleanup != NULL )
		{
			( descriptor->cleanup ) ( _instance );
			return( true );
		}
	}
	return( false );
}
