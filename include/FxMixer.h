/*
 * FxMixer.h - effect-mixer for LMMS
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _FX_MIXER_H
#define _FX_MIXER_H

#include "Model.h"
#include "Mixer.h"
#include "EffectChain.h"
#include "JournallingObject.h"


const int NumFxChannels = 64;


struct FxChannel
{
	FxChannel( Model * _parent );
	~FxChannel();

	EffectChain m_fxChain;
	bool m_used;
	bool m_stillRunning;
	float m_peakLeft;
	float m_peakRight;
	sampleFrame * m_buffer;
	BoolModel m_muteModel;
	FloatModel m_volumeModel;
	QString m_name;
	QMutex m_lock;

} ;



class FxMixer : public JournallingObject, public Model
{
public:
	FxMixer();
	virtual ~FxMixer();

	void mixToChannel( const sampleFrame * _buf, fx_ch_t _ch );
	void processChannel( fx_ch_t _ch, sampleFrame * _buf = NULL );

	void prepareMasterMix();
	void masterMix( sampleFrame * _buf );


	void clear();

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual QString nodeName() const
	{
		return "fxmixer";
	}

	FxChannel * effectChannel( int _ch )
	{
		if( _ch >= 0 && _ch <= NumFxChannels )
		{
			return m_fxChannels[_ch];
		}
		return NULL;
	}


private:
	FxChannel * m_fxChannels[NumFxChannels+1];	// +1 = master


	friend class MixerWorkerThread;
	friend class FxMixerView;

} ;


#endif
