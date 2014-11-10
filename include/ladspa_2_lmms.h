/*
 * ladspa_2_lmms.h - class that identifies and instantiates LADSPA effects
 *                   for use with LMMS
 *
 * Copyright (c) 2005-2008 Danny McRae <khjklujn@netscape.net>
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

#ifndef _LADSPA_2_LMMS_H
#define _LADSPA_2_LMMS_H


#include "ladspa_manager.h"


class EXPORT ladspa2LMMS : public ladspaManager
{
public:
	
	inline l_sortable_plugin_t getInstruments()
	{
		return( m_instruments );
	}
	
	inline l_sortable_plugin_t getValidEffects()
	{
		return( m_validEffects );
	}
	
	inline l_sortable_plugin_t getInvalidEffects()
	{
		return( m_invalidEffects );
	}
	
	inline l_sortable_plugin_t getAnalysisTools()
	{
		return( m_analysisTools );
	}
	
	inline l_sortable_plugin_t getOthers()
	{
		return( m_otherPlugins );
	}
	
	QString getShortName( const ladspa_key_t & _key );

private:
	ladspa2LMMS();
	virtual ~ladspa2LMMS();

	l_sortable_plugin_t m_instruments;
	l_sortable_plugin_t m_validEffects;
	l_sortable_plugin_t m_invalidEffects;
	l_sortable_plugin_t m_analysisTools;
	l_sortable_plugin_t m_otherPlugins;
	
	friend class engine;

} ;

#endif
