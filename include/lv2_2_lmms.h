/*
 * lv2_2_lmms.h - class that identifies and instantiates LV2 effects
 *                   for use with LMMS
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

#ifndef _LV2_2_LMMS_H
#define _LV2_2_LMMS_H


#include "lv2_manager.h"

#ifdef LMMS_HAVE_LV2

class EXPORT lv22LMMS : public lv2Manager
{
public:
	inline l_sortable_plugin_t getInstruments()
	{
		return  m_instruments;
	}

	inline l_sortable_plugin_t getValidEffects()
	{
		return  m_validEffects;
	}

	inline l_sortable_plugin_t getInvalidEffects()
	{
		return  m_invalidEffects;
	}

	inline l_sortable_plugin_t getAnalysisTools()
	{
		return  m_analysisTools;
	}

	inline l_sortable_plugin_t getOthers()
	{
		return  m_otherPlugins;
	}

	QString getShortName( const lv2_key_t & _key );


private:
	lv22LMMS();
	virtual ~lv22LMMS();

	l_sortable_plugin_t m_instruments;
	l_sortable_plugin_t m_validEffects;
	l_sortable_plugin_t m_invalidEffects;
	l_sortable_plugin_t m_analysisTools;
	l_sortable_plugin_t m_otherPlugins;

	friend class engine;

} ;

#endif

#endif
