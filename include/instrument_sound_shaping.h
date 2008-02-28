/*
 * instrument_sound_shaping.h - class instrumentSoundShaping
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _INSTRUMENT_SOUND_SHAPING_H
#define _INSTRUMENT_SOUND_SHAPING_H

#include "mixer.h"
#include "automatable_model.h"
#include "combobox.h"


class instrumentTrack;
class envelopeAndLFOParameters;
class notePlayHandle;


class instrumentSoundShaping : public model, public journallingObject
{
public:
	instrumentSoundShaping( instrumentTrack * _instrument_track );
	virtual ~instrumentSoundShaping();

	void processAudioBuffer( sampleFrame * _ab, const fpp_t _frames,
							notePlayHandle * _n );

	enum Targets
	{
		Volume,
		Cut,
		Resonance,
		NumTargets
	} ;

	f_cnt_t envFrames( const bool _only_vol = FALSE ) const;
	f_cnt_t releaseFrames( const bool _only_vol = FALSE ) const;

	float volumeLevel( notePlayHandle * _n, const f_cnt_t _frame );


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );
	inline virtual QString nodeName( void ) const
	{
		return( "eldata" );
	}


private:
	envelopeAndLFOParameters * m_envLFOParameters[NumTargets];
	instrumentTrack * m_instrumentTrack;

	boolModel m_filterEnabledModel;
	comboBoxModel m_filterModel;
	floatModel m_filterCutModel;
	floatModel m_filterResModel;


	friend class instrumentSoundShapingView;
	friend class flpImport;

} ;


extern const QString __targetNames[instrumentSoundShaping::NumTargets][2];


#endif
