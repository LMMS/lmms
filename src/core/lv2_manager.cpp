/*
 * lv2_manager.cpp - a class to manage loading and instantiation of lv2 plugins
 *
 * Copyright (c) 2009 Martin Andrews <mdda/at/users.sourceforge.net>
 *
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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


#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QLibrary>
#include <QtCore/QFile>
#include <QtCore/QTextStream>

#include <math.h>

#include "config_mgr.h"
#include "lv2_manager.h"

#ifdef LMMS_HAVE_LV2


lv2Manager * static_lv2_manager=(lv2Manager *)NULL; // There is only one of these...



LV2World::LV2World()
{
	world=slv2_world_new();
	slv2_world_load_all( world );

	input_class = slv2_value_new_uri( world, SLV2_PORT_CLASS_INPUT );
	output_class = slv2_value_new_uri(world, SLV2_PORT_CLASS_OUTPUT );
	control_class = slv2_value_new_uri( world, SLV2_PORT_CLASS_CONTROL );
	audio_class = slv2_value_new_uri( world, SLV2_PORT_CLASS_AUDIO );
	event_class = slv2_value_new_uri( world, SLV2_PORT_CLASS_EVENT );
	midi_class = slv2_value_new_uri( world, SLV2_EVENT_CLASS_MIDI );
	in_place_broken = slv2_value_new_uri( world, SLV2_NAMESPACE_LV2 "inPlaceBroken" );
	integer = slv2_value_new_uri( world, SLV2_NAMESPACE_LV2 "integer" );
	toggled = slv2_value_new_uri( world, SLV2_NAMESPACE_LV2 "toggled" );
	srate = slv2_value_new_uri( world, SLV2_NAMESPACE_LV2 "sampleRate" );
	gtk_gui = slv2_value_new_uri( world, "http://lv2plug.in/ns/extensions/ui#GtkUI" );
}




LV2World::~LV2World()
{
	slv2_value_free( input_class );
	slv2_value_free( output_class );
	slv2_value_free( control_class );
	slv2_value_free( audio_class );
	slv2_value_free( event_class );
	slv2_value_free( midi_class );
	slv2_value_free( in_place_broken );

	slv2_world_free( world );
}




lv2Manager::lv2Manager()
{
	if( m_lv2_bundle.world == NULL )
	{
		printf( "Failed to Initialize slv2_world\n" );
		return;
	}
	printf( "Initialized slv2_world\n" );

	m_cacheFile = configManager::inst()->workingDir() + ".lv2_cache.txt";

	loadFromCacheFile();

	if( false )
	{
		// This is to test the loading from the cache file
		l_lv2_key_t keys = m_lv2ManagerMap.keys();
		for( l_lv2_key_t::iterator it = keys.begin();
						it != keys.end(); it++ )
		{
			printf( "Read from file : '%s'\n", getName( *it ).toAscii().data() );
		}
	}

	// slv2_world_load_all( m_world ); // No special path apart from LV2_PATH
	m_pluginList = slv2_world_get_all_plugins( m_lv2_bundle.world );

	for( unsigned i=0; i < slv2_plugins_size( m_pluginList ); ++i )
	{
		SLV2Plugin p = slv2_plugins_get_at( m_pluginList, i );
		addPlugins( p );  // This will just return if the plugin information is cached (after being loaded)
	}

	saveToCacheFile();

	l_lv2_key_t keys = m_lv2ManagerMap.keys();
	for( l_lv2_key_t::iterator it = keys.begin(); it != keys.end(); ++it )
	{
		m_sortedPlugins.append( qMakePair( getName( *it ), *it ) );
	}

	qSort( m_sortedPlugins );
}




lv2Manager::~lv2Manager()
{
	for( lv2ManagerMapType::iterator it = m_lv2ManagerMap.begin();
					it != m_lv2ManagerMap.end(); ++it )
	{
		delete it.value();
	}

	slv2_plugins_free( m_lv2_bundle.world, m_pluginList );
}




lv2ManagerDescription * lv2Manager::getDescription( const lv2_key_t & _plugin )
{
	if( m_lv2ManagerMap.contains( _plugin ) )
	{
		return m_lv2ManagerMap[_plugin];
	}
	return NULL; // If it doesn't exist
}




void lv2Manager::ensureLV2DataExists( lv2ManagerDescription *desc )
{
	if( desc->plugin == NULL )
	{
		printf( " Need to load actual plugin data for '%s'\n", (desc->uri).toAscii().constData() );

		// use uri to get data
		SLV2Value uri = slv2_value_new_uri( m_lv2_bundle.world, (desc->uri).toAscii().constData() );
		desc->plugin = slv2_plugins_get_by_uri( m_pluginList, uri );
		slv2_value_free( uri );

		if( desc->plugin == NULL ) // Still empty??
		{
			printf( " Failed to load actual plugin data for '%s'\n", (desc->uri).toAscii().constData() );
		}
	}
}




void lv2Manager::addPlugins( SLV2Plugin _plugin )
{
	QString uri= QString( slv2_value_as_uri( slv2_plugin_get_uri( _plugin ) ) );
	lv2_key_t key = lv2_key_t( uri );

	if( m_lv2ManagerMap.contains( key ) )
	{
		printf( "Already in Cache LV2 plugin URI : '%s'\n", uri.toAscii().constData() );
		return;
	}

/*
// Speed things up (before caching of data implemented)
 if( ! uri.contains( "Organ" ) )
 {
  printf( "Skipping LV2 plugin URI : '%s'\n", uri.toAscii().constData() );
  return;
 }
*/
 printf( "Examining LV2 plugin URI : '%s'\n", uri.toAscii().constData() );

 lv2ManagerDescription * description = new lv2ManagerDescription;
 description->plugin = _plugin;

 // Investigate this plugin
 description->uri = uri;

// Any data accessed within the plugin itself causes the whole thing to be 'unwrapped'
 SLV2Value data = slv2_plugin_get_name( _plugin );
 description->name = QString( slv2_value_as_string( data ) );
// printf( " LV2 plugin Name : '%s'\n", slv2_value_as_string( data ) );
 slv2_value_free( data );

 description->inputChannels = slv2_plugin_get_num_ports_of_class( _plugin,
  m_lv2_bundle.input_class, m_lv2_bundle.audio_class, NULL);

	description->outputChannels = slv2_plugin_get_num_ports_of_class( _plugin,
  m_lv2_bundle.output_class, m_lv2_bundle.audio_class, NULL);

// This always seems to return 'Plugin', which isn't so useful to us
//	SLV2PluginClass pclass = slv2_plugin_get_class( _plugin );
//	SLV2Value label = slv2_plugin_class_get_label( pclass );
//	printf( "Plugin Class is : '%s'\n", slv2_value_as_string( label ) );

 printf( "  Audio (input, output)=(%d,%d)\n", description->inputChannels, description->outputChannels );

 if( description->inputChannels == 0 && description->outputChannels > 0 )
 {
  description->type = SOURCE;
 }
 else if( description->inputChannels > 0 &&
          description->outputChannels > 0 )
 {
  description->type = TRANSFER;
 }
 else if( description->inputChannels > 0 &&
          description->outputChannels == 0 )
 {
  description->type = SINK;
 }
 else
 {
  description->type = OTHER;
 }

 m_lv2ManagerMap[key] = description;

 printf( "  Finished that plugin : type=%d\n", (int)description->type );

/*
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

		plugIn->index = pluginIndex;

		m_ladspaManagerMap[key] = plugIn;
	}
*/

}




l_sortable_plugin_t lv2Manager::getSortedPlugins()
{
	return m_sortedPlugins;
}



void lv2Manager::saveToCacheFile()
{
 QFile file( m_cacheFile );
 QString line;
 if ( file.open( QIODevice::WriteOnly ) ) {
  QTextStream stream( &file );
  stream << "; This file is auto-regenerated by lmms\n\n";

 	l_lv2_key_t keys = m_lv2ManagerMap.keys();
 	for( l_lv2_key_t::iterator it = keys.begin(); it != keys.end(); it++ )
	 {
   stream << "[" << (*it) << "]\n"; // Plugin URI

  	lv2ManagerDescription * description=getDescription( (*it) );
		 stream << "name=" << description->name << "\n"; // Plugin name
		 stream << "type=" << (int)description->type << "\n";
   stream << "channels.audio.input=" << description->inputChannels << "\n";
   stream << "channels.audio.output=" << description->outputChannels << "\n";

   stream << "\n";
	 }

  file.close();
 }
 return;
}




void lv2Manager::loadFromCacheFile()
{
 QFile file( m_cacheFile );
 QString line;
 if ( file.open( QIODevice::ReadOnly ) ) { // file opened successfully
  QTextStream stream( &file );
  while ( !stream.atEnd() ) {    // until end of file...
   line = stream.readLine().trimmed();
   if( line.startsWith( "[" ) && line.endsWith( "]" ) )    // Skip over everything else
   {
    lv2ManagerDescription * desc=new lv2ManagerDescription;
    desc->plugin = NULL; // To ensure that we don't attempt to use it
    desc->uri = line.mid( 1, line.length()-2 ); // Extract the URI
    printf( "Reading Data for '%s' from the Cache\n", desc->uri.toAscii().data() );

    // Read in data for this uri - until we get to a blank line
    bool finished = false;
    while ( !stream.atEnd() && !finished )
    {
     line = stream.readLine().trimmed();
     if( line.length() == 0 )
     {
      finished = true;
      continue;
     }
     int equals = line.indexOf( "=" );
     if( equals > 0 )
     {
      QString name = line.left(equals);
      QString value = line.mid(equals+1);
      printf( " Found '%s' = '%s'\n", name.toAscii().data(), value.toAscii().data() );

      if( name == "name" )
      {
       desc->name = value;
      }
      else if( name == "type" )
      {
       desc->type = (lv2PluginType)value.toInt();
      }
      else if( name == "channels.audio.input" )
      {
       desc->inputChannels = value.toInt();
      }
      else if( name == "channels.audio.output" )
      {
       desc->outputChannels = value.toInt();
      }
      else
      {
       printf( "Could not parse line : '%s' in lv2.cache\n", line.toAscii().data() );
      }
     }
    }
    // Dump the discovered data into the manager
    lv2_key_t key = lv2_key_t( desc->uri );
    m_lv2ManagerMap[key] = desc;
   }
   // Skip lines that don't match ^[.*]$
  }
  // Close the file
  file.close();
 }
 return;
}




bool lv2Manager::isInplaceBroken( const lv2_key_t &  _plugin )
{
	if( m_lv2ManagerMap.contains( _plugin ) )
	{
		lv2ManagerDescription * d = m_lv2ManagerMap[_plugin];
		ensureLV2DataExists( d );
		return slv2_plugin_has_feature( d->plugin, m_lv2_bundle.in_place_broken );
	}
	return false;
}




QString lv2Manager::getName( const lv2_key_t & _plugin )
{
	if( m_lv2ManagerMap.contains( _plugin ) )
	{
		// This is a cached entry - so it will be here
		// whether the plugin rdf is fully loaded or not
		return m_lv2ManagerMap[_plugin]->name;
	}
	return QString();
}




QString lv2Manager::getMaker( const lv2_key_t & _plugin )
{
	if( m_lv2ManagerMap.contains( _plugin ) )
	{
		lv2ManagerDescription * d = m_lv2ManagerMap[_plugin];
		ensureLV2DataExists( d );
		SLV2Value data = slv2_plugin_get_author_name( d->plugin );
		QString ret = QString( data ?  slv2_value_as_string( data ) : "Unknown" );
		slv2_value_free( data );
		return ret;
	}
	return QString();
}




QString lv2Manager::getCopyright( const lv2_key_t & _plugin )
{
	if( m_lv2ManagerMap.contains( _plugin ) )
	{
		lv2ManagerDescription * d = m_lv2ManagerMap[_plugin];
		ensureLV2DataExists( d );
		SLV2Value data = slv2_plugin_get_author_homepage( d->plugin );
		QString ret = QString( data ?  slv2_value_as_string( data ) : "Unknown" );
		slv2_value_free( data );
		return ret;
	}
	return QString();
}




Uint32 lv2Manager::getPortCount( const lv2_key_t & _plugin )
{
	if( m_lv2ManagerMap.contains( _plugin ) )
	{
		lv2ManagerDescription * d = m_lv2ManagerMap[_plugin];
		ensureLV2DataExists( d );
		return (Uint32) slv2_plugin_get_num_ports( d->plugin );
	}
	return 0;
}



bool lv2Manager::isPortClass( const lv2_key_t & _plugin,
				Uint32 _port, SLV2Value _class )
{
	if( m_lv2ManagerMap.contains( _plugin ) )
	{
		lv2ManagerDescription * d = m_lv2ManagerMap[_plugin];
		ensureLV2DataExists( d );
		if( _port < slv2_plugin_get_num_ports( d->plugin ) )
		{
			SLV2Port port = slv2_plugin_get_port_by_index( d->plugin, _port );
			return slv2_port_is_a( d->plugin, port, _class );
		}
	}
	return false; // Not found
}



bool lv2Manager::isPortInput( const lv2_key_t & _plugin, Uint32 _port )
{
	return isPortClass( _plugin, _port, m_lv2_bundle.input_class );
}



bool lv2Manager::isPortOutput( const lv2_key_t & _plugin, Uint32 _port )
{
	return isPortClass( _plugin, _port, m_lv2_bundle.output_class );
}




bool lv2Manager::isPortAudio( const lv2_key_t & _plugin, Uint32 _port )
{
	return isPortClass( _plugin, _port, m_lv2_bundle.audio_class );
}




bool lv2Manager::isPortControl( const lv2_key_t & _plugin, Uint32 _port )
{
	return isPortClass( _plugin, _port, m_lv2_bundle.control_class );
}



void lv2Manager::getRanges( const lv2_key_t & _plugin,
			Uint32 _port, float * _def, float * _min, float * _max )
{
	if( m_lv2ManagerMap.contains( _plugin ) )
	{
		lv2ManagerDescription * d = m_lv2ManagerMap[_plugin];
		ensureLV2DataExists( d );
		if( _port < slv2_plugin_get_num_ports( d->plugin ) )
		{
			SLV2Port port = slv2_plugin_get_port_by_index( d->plugin, _port );
			SLV2Value def, min,  max;
			slv2_port_get_range( d->plugin, port, &def, &min, &max );
			*_def = NOHINT;
			if( def != NULL )
			{
				*_def = slv2_value_as_float(def);
				slv2_value_free(def);
			}
			*_min = NOHINT;
			if( min != NULL )
			{
				*_min = slv2_value_as_float(min);
				slv2_value_free(min);
			}
			*_max = NOHINT;
			if( max != NULL )
			{
				*_max = slv2_value_as_float(max);
				slv2_value_free(max);
			}
		}
	}
}



float lv2Manager::getLowerBound( const lv2_key_t & _plugin, Uint32 _port )
{
	float def, min, max;
	getRanges( _plugin, _port, &def, &min, &max );
	return min;
}



float lv2Manager::getUpperBound( const lv2_key_t & _plugin, Uint32 _port )
{
	float def, min, max;
	getRanges( _plugin, _port, &def, &min, &max );
	return max;
}



float lv2Manager::getDefaultSetting( const lv2_key_t & _plugin, Uint32 _port )
{
	float def, min, max;
	getRanges( _plugin, _port, &def, &min, &max );
	return def;
}



QStringList lv2Manager::listEnumeration( const lv2_key_t & _plugin, Uint32 _port )
{
	QStringList list;
	if( m_lv2ManagerMap.contains( _plugin ) )
	{
		lv2ManagerDescription * d = m_lv2ManagerMap[_plugin];
		ensureLV2DataExists( d );
		if( _port < slv2_plugin_get_num_ports( d->plugin ) )
		{
			SLV2Port port = slv2_plugin_get_port_by_index( d->plugin, _port );
			SLV2ScalePoints sp_list = slv2_port_get_scale_points( d->plugin, port );
			if( sp_list != NULL )
			{
				printf( "Got %d ScalePoints for port=%d\n",
				slv2_scale_points_size( sp_list ), _port );

				for( unsigned i = 0; i < slv2_scale_points_size( sp_list ); i++ )
				{
					SLV2ScalePoint sp = slv2_scale_points_get_at( sp_list, i );
					SLV2Value label = slv2_scale_point_get_label( sp );
					list.append( QString( slv2_value_as_string( label ) ) );
					// slv2_value_free( label );
					// slv2_scale_point_free( sp );
				}
				slv2_scale_points_free( sp_list );
			}
		}
	}
	return list;
}






bool lv2Manager::isInteger( const lv2_key_t & _plugin, Uint32 _port )
{
	if( listEnumeration( _plugin, _port ).size() >= 2 )
	{
		return true;
	}
	return false;
}



bool lv2Manager::isPortToggled( const lv2_key_t & _plugin, Uint32 _port )
{
	if( listEnumeration( _plugin, _port ).size() == 2 )
	{
		return true;
	}
	return false;
}






QString lv2Manager::getPortName( const lv2_key_t & _plugin,
								Uint32 _port )
{
	if( m_lv2ManagerMap.contains( _plugin ) )
	{
		lv2ManagerDescription * d = m_lv2ManagerMap[_plugin];
		ensureLV2DataExists( d );
		if( _port < slv2_plugin_get_num_ports( d->plugin ) )
		{
			SLV2Port port = slv2_plugin_get_port_by_index( d->plugin, _port );
			return QString( slv2_value_as_string( slv2_port_get_name( d->plugin, port ) ) );
		}
	}
	return QString();
}

#endif

