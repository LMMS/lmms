/*
 * lv2_2_lmms.cpp - class that identifies and instantiates LV2 effects
 *                  for use with LMMS
 *
 * Copyright (c) 2009 Martin Andrews <mdda@sourceforge.net>
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


#include "lv2_2_lmms.h"


#ifdef LMMS_HAVE_LV2

lv22LMMS::lv22LMMS()
{
	l_sortable_plugin_t plugins = getSortedPlugins();

	for( l_sortable_plugin_t::iterator it = plugins.begin();
		    it != plugins.end(); it++ )
	{
		lv2_key_t key = (*it).second;
		lv2ManagerDescription * desc = getDescription( key );

		if( desc->type == SOURCE )
		{
			m_instruments.append( qMakePair( getName( key ),
								key ) );
		}
		else if( desc->type == TRANSFER &&
			( desc->inputChannels == desc->outputChannels &&
			( desc->inputChannels == 1 ||
			desc->inputChannels == 2 ||
			desc->inputChannels == 4 ) ) )
		{
			m_validEffects.append( qMakePair( getName( key ),
								key ) );
		}
		else if( desc->type == TRANSFER &&
			( desc->inputChannels != desc->outputChannels ||
			( desc->inputChannels != 1 &&
			desc->inputChannels != 2 &&
			desc->inputChannels != 4 ) ) )
		{
			m_invalidEffects.append( qMakePair( getName( key ),
								key ) );
		}
		else if( desc->type == SINK )
		{
			m_analysisTools.append( qMakePair( getName( key ),
								key ) );
		}
		else if( desc->type == OTHER )
		{
			m_otherPlugins.append( qMakePair( getName( key ),
								key ) );
		}
	}
}




lv22LMMS::~lv22LMMS()
{
}




QString lv22LMMS::getShortName( const lv2_key_t & _key )
{
	QString name = getName( _key );

	if( name.indexOf( "(" ) > 0 )
	{
		name = name.left( name.indexOf( "(" ) );
	}
	if( name.indexOf( " - " ) > 0 )
	{
		name = name.left( name.indexOf( " - " ) );
	}
	if( name.indexOf( "  " ) > 0 )
	{
		name = name.left( name.indexOf( "  " ) );
	}
	Qt::CaseSensitivity cs = Qt::CaseInsensitive;
	if( name.indexOf( " with ", 0, cs ) > 0 )
	{
		name = name.left( name.indexOf( " with ", 0, cs ) );
	}
	if( name.indexOf( ",", 0, cs ) > 0 )
	{
		name = name.left( name.indexOf( ",", 0, cs ) );
	}
	if( name.length() > 40 )
	{
		Uint8 i = 40;
		while( name[i] != ' ' && i != 0 )
		{
			i--;
		}
		name = name.left( i );
	}
	if( name.length() == 0 )
	{
		name = "LV2 Plugin";
	}

	return name;
}

#endif

