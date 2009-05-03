/*
 * vst_effect.h - class for handling VST effect plugins
 *
 * Copyright (c) 2006-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _VST_EFFECT_H
#define _VST_EFFECT_H

#include <QtCore/QMutex>

#include "effect.h"
#include "vst_plugin.h"
#include "vst_effect_control_dialog.h"
#include "vst_effect_controls.h"



class vstEffect : public effect
{
public:
	vstEffect( model * _parent,
			const descriptor::subPluginFeatures::key * _key );
	virtual ~vstEffect();

	virtual bool processAudioBuffer( sampleFrame * _buf,
							const fpp_t _frames );

	virtual effectControls * getControls( void )
	{
		return &m_vstControls;
	}


	virtual inline QString publicName( void ) const
	{
		return m_plugin->name();
	}


private:
	void openPlugin( const QString & _plugin );
	void closePlugin( void );

	vstPlugin * m_plugin;
	QMutex m_pluginMutex;
	effectKey m_key;

	vstEffectControls m_vstControls;


	friend class vstEffectControls;
	friend class vstEffectControlDialog;

} ;



#endif
