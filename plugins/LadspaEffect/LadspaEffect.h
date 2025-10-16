/*
 * LadspaEffect.h - class for handling LADSPA effect plugins
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _LADSPA_EFFECT_H
#define _LADSPA_EFFECT_H

#include <QMutex>

#include "Effect.h"
#include "ladspa.h"
#include "LadspaControls.h"
#include "LadspaManager.h"

namespace lmms
{

struct port_desc_t;
using multi_proc_t = QVector<port_desc_t*>;

class LadspaEffect : public Effect
{
	Q_OBJECT
public:
	LadspaEffect( Model * _parent,
			const Descriptor::SubPluginFeatures::Key * _key );
	~LadspaEffect() override;

	ProcessStatus processImpl(SampleFrame* buf, const fpp_t frames) override;

	void setControl( int _control, LADSPA_Data _data );

	EffectControls * controls() override
	{
		return m_controls;
	}

	inline const multi_proc_t & getPortControls()
	{
		return m_portControls;
	}

	ch_cnt_t processorCount() const
	{
		return m_processors;
	}

private slots:
	void changeSampleRate();


private:
	void pluginInstantiation();
	void pluginDestruction();

	static sample_rate_t maxSamplerate( const QString & _name );


	QMutex m_pluginMutex;
	LadspaControls * m_controls;

	sample_rate_t m_maxSampleRate;
	ladspa_key_t m_key;
	int m_portCount;
	bool m_inPlaceBroken;

	const LADSPA_Descriptor * m_descriptor;
	QVector<LADSPA_Handle> m_handles;

	QVector<multi_proc_t> m_ports;
	multi_proc_t m_portControls;

	ch_cnt_t m_processors = 1;
};


} // namespace lmms

#endif
