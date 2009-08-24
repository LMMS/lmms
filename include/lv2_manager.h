/*
 * lv2_manager.h - declaration of class lv2Manager a class to manage lv2 plugins
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


#ifndef _LV2_MANAGER_H
#define _LV2_MANAGER_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_LV2

#include <slv2/world.h>
#include <slv2/plugin.h>
#include <slv2/scalepoint.h>

#ifdef LMMS_HAVE_SLV2_SCALEPOINTS_H
#include <slv2/scalepoints.h>
#endif

#include <QtCore/QMap>
#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtCore/QStringList>


#include "export.h"
#include "lmms_basics.h"

const float NOHINT = -99342.2243f;

//typedef QPair<QString, QString> lv2_key_t;
typedef QString lv2_key_t; // Just need the URI for LV2...
typedef QPair<QString, lv2_key_t> sortable_plugin_t;
typedef QList<sortable_plugin_t> l_sortable_plugin_t;
typedef QList<lv2_key_t> l_lv2_key_t;

/* lv2Manager provides a database of LV2 plug-ins.  Upon instantiation,
it loads all of the plug-ins found in the LV2_PATH environmental variable
and stores their access descriptors according in a dictionary keyed on
the filename the plug-in was loaded from and the label of the plug-in.

The can be retrieved by using lv2_key_t (which is really just the LV2 URI) :

// lv2_key_t key( "lalala" )

as the plug-in key. */


/** The SLV2World, and various cached (as symbols, fast) URIs.
 *
 * This object represents everything ardour 'knows' about LV2
 * (ie understood extensions/features/etc)
 */
struct LV2World {
	LV2World();
	~LV2World();

	SLV2World world;
	SLV2Value input_class; ///< Input port
	SLV2Value output_class; ///< Output port
	SLV2Value audio_class; ///< Audio port
	SLV2Value control_class; ///< Control port
	SLV2Value event_class; ///< Event port
	SLV2Value midi_class; ///< MIDI event
	SLV2Value in_place_broken;
	SLV2Value integer;
	SLV2Value toggled;
	SLV2Value srate;
	SLV2Value gtk_gui;
};


enum lv2PluginType
{
	SOURCE,
	TRANSFER,
	VALID,
	INVALID,
	SINK,
	OTHER
};

typedef struct lv2ManagerStorage
{
	SLV2Plugin plugin;
	QString uri;
	QString name;
	lv2PluginType type;
	Uint16 inputChannels;
	Uint16 outputChannels;
} lv2ManagerDescription;


class EXPORT lv2Manager
{
public:
	lv2Manager();
	virtual ~lv2Manager();

	void loadFromCacheFile();
	void saveToCacheFile();

	l_sortable_plugin_t getSortedPlugins();
	lv2ManagerDescription * getDescription( const lv2_key_t & _plugin );

	/* This identifier can be used as a unique, case-sensitive
	identifier for the plugin type within the plugin file. Plugin
	types should be identified by file and label rather than by index
	or plugin name, which may be changed in new plugin
	versions. Labels must not contain white-space characters. */
	QString  getLabel( const lv2_key_t & _plugin );

	/* Indicates that the plugin has a real-time dependency
	(e.g. listens to a MIDI device) and so its output must not
	be cached or subject to significant latency. */
//	bool  hasRealTimeDependency( const lv2_key_t & _plugin );

	/* Indicates that the plugin may cease to work correctly if the
	host elects to use the same data location for both input and output
	(see connectPort). */
	bool  isInplaceBroken( const lv2_key_t & _plugin );

	/* Indicates that the plugin is capable of running not only in a
	conventional host but also in a 'hard real-time' environment. */
//	bool  isRealTimeCapable( const lv2_key_t & _plugin );

	/* Returns the name of the plug-in */
	QString  getName( const lv2_key_t & _plugin );

	/* Returns the the plug-in's author */
	QString  getMaker( const lv2_key_t & _plugin );

	/* Returns the copyright for the plug-in */
	QString  getCopyright( const lv2_key_t & _plugin );

  	/* This indicates the number of ports (input AND output) present on
	the plugin. */
	Uint32  getPortCount( const lv2_key_t & _plugin );


	/* Indicates that the port is an input. */
	bool  isPortInput( const lv2_key_t & _plugin, Uint32 _port );

	/* Indicates that the port is an output. */
	bool  isPortOutput( const lv2_key_t & _plugin, Uint32 _port );

	/* Indicates that the port is an audio. */
	bool  isPortAudio( const lv2_key_t & _plugin, Uint32 _port );

	/* Indicates that the port is an control. */
	bool  isPortControl( const lv2_key_t & _plugin, Uint32 _port );

	/* Indicates that any bounds specified should be interpreted as
	multiples of the sample rate. For instance, a frequency range from
	0Hz to the Nyquist frequency (half the sample rate) could be requested
	by this hint in conjunction with LowerBound = 0 and UpperBound = 0.5.
	Hosts that support bounds at all must support this hint to retain
	meaning. */
//	bool  areHintsSampleRateDependent( const ladspa_key_t & _plugin,
//								Uint32 _port );

  	/* Returns the lower boundary value for the given port. If
	no lower bound is provided by the plug-in, returns -999e-99. When
	areHintsSampleRateDependent() is also true then this value should be
	multiplied by the relevant sample rate. */
	float  getLowerBound( const lv2_key_t & _plugin, Uint32 _port );

  	/* Returns the upper boundary value for the given port. If
	no upper bound is provided by the plug-in, returns -999e-99. When
	areHintsSampleRateDependent() is also true then this value should be
	multiplied by the relevant sample rate. */
	float  getUpperBound( const lv2_key_t & _plugin, Uint32 _port );

	/* Retrieves any default setting hints offered by the plug-in for
	the given port. */
	float  getDefaultSetting( const lv2_key_t & _plugin, Uint32 _port );

	/* Indicates that a user interface would probably wish to provide a
	stepped control taking only integer values. Any bounds set should be
	slightly wider than the actual integer range required to avoid floating
	point rounding errors. For instance, the integer set {0,1,2,3} might
	be described as [-0.1, 3.1]. */
	bool  isInteger( const lv2_key_t & _plugin, Uint32 _port );

	/* Indicates whether the given port should be considered 0 or 1
	boolean switch. */
	bool  isPortToggled( const lv2_key_t & _plugin, Uint32 _port );

	/* return the labels to be applied on the UI */
	QStringList listEnumeration( const lv2_key_t & _plugin, Uint32 _port );

	/* Indicates that it is likely that the user will find it more
	intuitive to view values using a logarithmic scale. This is
	particularly useful for frequencies and gains. */
//	bool  isLogarithmic( const ladspa_key_t & _plugin, Uint32 _port );


	/* Returns the name of the port. */
	QString  getPortName( const lv2_key_t & _plugin, Uint32 _port );


	/* This may be used by the plugin developer to pass any custom
	implementation data into an instantiate call. It must not be used
	or interpreted by the host. It is expected that most plugin
	writers will not use this facility as LADSPA_Handle should be
	used to hold instance data. */
//	const void *  getImplementationData(
//						const ladspa_key_t & _plugin );


	/* Returns a pointer to the plug-in's descriptor from which control
	of the plug-in is accessible */
//	const LADSPA_Descriptor *  getDescriptor(
//						const ladspa_key_t & _plugin );


	/* The following methods are convenience functions for use during
	development.  A real instrument should use the getDescriptor()
	method and implement the plug-in manipulations internally to avoid
	the overhead associated with QMap lookups. */


	/* Returns a handle to an instantiation of the given plug-in. */
	//LADSPA_Handle  instantiate( const ladspa_key_t & _plugin,
	//					Uint32 _sample_rate );

  	/* This method calls a function pointer that connects a port on an
	instantiated plugin to a memory location at which a block of data
	for the port will be read/written. The data location is expected
	to be an array of LADSPA_Data for audio ports or a single
	LADSPA_Data value for control ports. Memory issues will be
	managed by the host. The plugin must read/write the data at these
	locations every time run() or runAdding() is called and the data
	present at the time of this connection call should not be
	considered meaningful.

	connectPort() may be called more than once for a plugin instance
	to allow the host to change the buffers that the plugin is
	reading or writing. These calls may be made before or after
	activate() or deactivate() calls.

	connectPort() must be called at least once for each port before
	run() or runAdding() is called. */
//	bool connectPort( const ladspa_key_t & _plugin,
//					LADSPA_Handle _instance,
//					Uint32 _port,
//					LADSPA_Data * _data_location );

  	/* This method calls a function pointer that initialises a plugin
	instance and activates it for use. This is separated from
	instantiate() to aid real-time support and so that hosts can
	reinitialise a plugin instance by calling deactivate() and then
	activate(). In this case the plugin instance must reset all state
	information dependent on the history of the plugin instance
	except for any data locations provided by connectPort() and any
	gain set by setRunAddingGain(). If there is nothing for
	activate() to do then the plugin writer may provide a NULL rather
	than an empty function.

	When present, hosts must call this function once before run() (or
	runAdding()) is called for the first time. This call should be
	made as close to the run() call as possible and indicates to
	real-time plugins that they are now live. Plugins should not rely
	on a prompt call to run() after activate(). activate() may not be
	called again unless deactivate() is called first. Note that
	connectPort() may be called before or after a call to
	activate(). */
//	bool  activate( const ladspa_key_t & _plugin,
//						LADSPA_Handle _instance );

	/* This method calls a function pointer that runs an instance of a
	plugin for a block. Two parameters are required: the first is a
	handle to the particular instance to be run and the second
	indicates the block size (in samples) for which the plugin
	instance may run.

	Note that if an activate() function exists then it must be called
	before run() or run_adding(). If deactivate() is called for a
	plugin instance then the plugin instance may not be reused until
	activate() has been called again. */
//	bool  run( const ladspa_key_t & _plugin,
//					LADSPA_Handle _instance,
//					Uint32 _sample_count );

	/* This method calls a function pointer that runs an instance of a
	plugin for a block. This has identical behaviour to run() except
	in the way data is output from the plugin. When run() is used,
	values are written directly to the memory areas associated with
	the output ports. However when runAdding() is called, values
	must be added to the values already present in the memory
	areas. Furthermore, output values written must be scaled by the
	current gain set by setRunAddingGain() (see below) before
	addition.

	runAdding() is optional. When it is not provided by a plugin,
	this function pointer must be set to NULL. When it is provided,
	the function setRunAddingGain() must be provided also. */
//	bool  runAdding( const ladspa_key_t & _plugin,
//						LADSPA_Handle _instance,
//						Uint32 _sample_count );

  	/* This method calls a function pointer that sets the output gain for
	use when runAdding() is called (see above). If this function is
	never called the gain is assumed to default to 1. Gain
	information should be retained when activate() or deactivate()
	are called.

	This function should be provided by the plugin if and only if the
	runAdding() function is provided. When it is absent this
	function pointer must be set to NULL. */
//	bool  setRunAddingGain( const ladspa_key_t & _plugin,
//						LADSPA_Handle _instance,
//						LADSPA_Data _gain );

	/* This is the counterpart to activate() (see above). If there is
	nothing for deactivate() to do then the plugin writer may provide
	a NULL rather than an empty function.

	Hosts must deactivate all activated units after they have been
	run() (or run_adding()) for the last time. This call should be
	made as close to the last run() call as possible and indicates to
	real-time plugins that they are no longer live. Plugins should
	not rely on prompt deactivation. Note that connect_port() may be
	called before or after a call to deactivate().

	Deactivation is not similar to pausing as the plugin instance
	will be reinitialised when activate() is called to reuse it. */
//	bool  deactivate( const ladspa_key_t & _plugin,
//						LADSPA_Handle _instance );

	/* Once an instance of a plugin has been finished with it can be
	deleted using the following function. The instance handle passed
	ceases to be valid after this call.

	If activate() was called for a plugin instance then a
	corresponding call to deactivate() must be made before cleanup()
	is called. */
//	bool  cleanup( const ladspa_key_t & _plugin,
//						LADSPA_Handle _instance );

private:
	void addPlugins( SLV2Plugin _plugin );
	void ensureLV2DataExists( lv2ManagerDescription *desc );

	bool isPortClass( const lv2_key_t & _plugin,
				Uint32 _port, SLV2Value _class );

	void getRanges( const lv2_key_t & _plugin,
			Uint32 _port, float * _def, float * _min, float * _max );

	SLV2Plugins m_pluginList;
	typedef QMap<lv2_key_t, lv2ManagerDescription *> lv2ManagerMapType;
	lv2ManagerMapType m_lv2ManagerMap;
	l_sortable_plugin_t m_sortedPlugins;

	LV2World m_lv2_bundle;

	QString m_cacheFile;

} ;


// This is declared static in lv2_manager.cpp
// TODO: integrate into LMMS engine
extern lv2Manager * static_lv2_manager; // There is only one of these...

#endif

#endif
