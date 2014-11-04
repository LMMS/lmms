/*
 * InstrumentSoundShaping.h - declaration of class InstrumentSoundShaping
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _INSTRUMENT_SOUND_SHAPING_H
#define _INSTRUMENT_SOUND_SHAPING_H

#include "Mixer.h"
#include "ComboBoxModel.h"


class InstrumentTrack;
class EnvelopeAndLfoParameters;
class NotePlayHandle;


class InstrumentSoundShaping : public Model, public JournallingObject
{
	Q_OBJECT
public:
	InstrumentSoundShaping( InstrumentTrack * _instrument_track );
	virtual ~InstrumentSoundShaping();

	void processAudioBuffer( sampleFrame * _ab, const fpp_t _frames,
							NotePlayHandle * _n );

	enum Targets
	{
		Volume,
		Cut,
		Resonance,
		NumTargets
	} ;

	f_cnt_t envFrames( const bool _only_vol = false ) const;
	f_cnt_t releaseFrames() const;

	float volumeLevel( NotePlayHandle * _n, const f_cnt_t _frame );


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );
	inline virtual QString nodeName() const
	{
		return "eldata";
	}


private:
	EnvelopeAndLfoParameters * m_envLfoParameters[NumTargets];
	InstrumentTrack * m_instrumentTrack;

	BoolModel m_filterEnabledModel;
	ComboBoxModel m_filterModel;
	FloatModel m_filterCutModel;
	FloatModel m_filterResModel;


	friend class InstrumentSoundShapingView;
	friend class FlpImport;

} ;


extern const QString __targetNames[InstrumentSoundShaping::NumTargets][3];


#endif
