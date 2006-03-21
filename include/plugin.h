/*
 * plugin.h - class plugin, the base-class and generic interface for all plugins
 *
 * Copyright (c) 2005-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _PLUGIN_H
#define _PLUGIN_H


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "qt3support.h"

#ifdef QT4

#include <QString>
#include <QVector>

#else

#include <qstring.h>
#include <qvaluevector.h>

#endif


#include "types.h"
#include "journalling_object.h"
#include "embed.h"


#define STRINGIFY_PLUGIN_NAME(s) STR(s)
#define STR(PN)	#PN


class QPixmap;


class plugin : public journallingObject
{
public:
	enum pluginTypes
	{
		INSTRUMENT,	// instrument being used in channel-track
		EFFECT,		// effect-plugin for effect-board
		IMPORT_FILTER,	// filter for importing a file
		EXPORT_FILTER,	// filter for exporting a file
		UNDEFINED = 255
	} ;

	// descriptor holds information about a plugin - every external plugin
	// has to instantiate such a descriptor in an extern "C"-section so that
	// the plugin-loader is able to access information about the plugin
	struct descriptor
	{
		const char * name;
		const char * public_name;
		const char * description;
		const char * author;
		int version;
		pluginTypes type;
		const QPixmap * logo;
	} ;

	// contructor of a plugin
	plugin( const descriptor * _descriptor, engine * _engine );
	virtual ~plugin();

	// returns public-name out of descriptor
	inline QString publicName( void ) const
	{
		return( m_descriptor->public_name );
	}

	// return plugin-type
	inline pluginTypes type( void ) const
	{
		return( m_descriptor->type );
	}

	// return plugin-descriptor for further information
	inline const descriptor * getDescriptor( void ) const
	{
		return( m_descriptor );
	}

	// plugins can overload this for making other classes able to change
	// settings of the plugin without knowing the actual class
	virtual void FASTCALL setParameter( const QString & _param,
						const QString & _value );

	// plugins can overload this for making other classes able to query
	// settings of the plugin without knowing the actual class
	virtual QString FASTCALL getParameter( const QString & _param );


	// returns an instance of a plugin whose name matches to given one
	// if specified plugin couldn't be loaded, it creates a dummy-plugin
	static plugin * FASTCALL instantiate( const QString & _plugin_name,
							void * _data );

	// fills given vector with descriptors for all available plugins
	static void FASTCALL getDescriptorsOfAvailPlugins(
					vvector<descriptor> & _plugin_descs );

private:
	const descriptor * m_descriptor;

	// pointer to instantiation-function in plugin
	typedef plugin * ( * instantiationHook )( void * );

} ;


#endif
