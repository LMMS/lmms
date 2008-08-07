/*
 * sid_instrument.h - ResID based software-synthesizer
 *
 * Copyright (c) 2008 Csaba Hruska <csaba.hruska/at/gmail.com>
 *                    Attila Herman <attila589/at/gmail.com>
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


#ifndef _SID_H
#define _SID_H

#include <QtCore/QObject>
#include "instrument.h"
#include "instrument_view.h"
#include "knob.h"


class sidInstrumentView;
class notePlayHandle;
class automatableButtonGroup;
class pixmapButton;

class voiceObject : public model
{
	Q_OBJECT
public:
	enum WaveForm {
		SquareWave = 0,
		TriangleWave,
		SawWave,
		NoiseWave,
		NumWaveShapes
	};
	voiceObject( model * _parent, int _idx );
	virtual ~voiceObject();


private:
	knobModel m_pulseWidthModel;
	knobModel m_attackModel;
	knobModel m_decayModel;
	knobModel m_sustainModel;
	knobModel m_releaseModel;
	knobModel m_coarseModel;
	intModel m_waveFormModel;
	boolModel m_syncModel;
	boolModel m_ringModModel;
	boolModel m_filteredModel;

	friend class sidInstrument;
	friend class sidInstrumentView;
} ;

class sidInstrument : public instrument
{
	Q_OBJECT
public:
	enum FilerType {
		HighPass = 0,
		BandPass,
		LowPass,
		NumFilterTypes
	};
	
	enum ChipModel {
		sidMOS6581 = 0,
		sidMOS8580,
		NumChipModels
	};


	sidInstrument( instrumentTrack * _instrument_track );
	virtual ~sidInstrument();

	virtual void playNote( notePlayHandle * _n, bool _try_parallelizing,
						sampleFrame * _working_buffer );
	virtual void deleteNotePluginData( notePlayHandle * _n );


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual QString nodeName( void ) const;

	virtual f_cnt_t desiredReleaseFrames( void ) const;

	virtual pluginView * instantiateView( QWidget * _parent );


/*public slots:
	void updateKnobHint( void );
	void updateKnobToolTip( void );*/

private:
	// voices
	voiceObject * m_voice[3];

	// filter	
	knobModel m_filterFCModel;
	knobModel m_filterResonanceModel;
	intModel m_filterModeModel;
	
	// misc
	boolModel m_voice3OffModel;
	knobModel m_volumeModel;

	intModel m_chipModel;

	friend class sidInstrumentView;

} ;



class sidInstrumentView : public instrumentView
{
	Q_OBJECT
public:
	sidInstrumentView( instrument * _instrument, QWidget * _parent );
	virtual ~sidInstrumentView();

private:
	virtual void modelChanged( void );
	
	automatableButtonGroup * m_passBtnGrp;
	automatableButtonGroup * m_sidTypeBtnGrp;

	struct voiceKnobs
	{
		voiceKnobs( knob * a,
					knob * d,
					knob * s,
					knob * r,
					knob * pw,
					knob * crs,
					automatableButtonGroup * wfbg,
					pixmapButton * syncb,
					pixmapButton * ringb,
					pixmapButton * filterb ) :
			m_attKnob( a ),
			m_decKnob( d ),
			m_sustKnob( s ),
			m_relKnob( r ),
			m_pwKnob( pw ),
			m_crsKnob( crs ),
			m_waveFormBtnGrp( wfbg ),
			m_syncButton( syncb ),
			m_ringModButton( ringb ),
			m_filterButton( filterb )
		{
		}
		voiceKnobs()
		{
		}
		knob * m_attKnob;
		knob * m_decKnob;
		knob * m_sustKnob;
		knob * m_relKnob;
		knob * m_pwKnob;
		knob * m_crsKnob;
		automatableButtonGroup * m_waveFormBtnGrp;
		pixmapButton * m_syncButton;
		pixmapButton * m_ringModButton;
		pixmapButton * m_filterButton;
	} ;

	voiceKnobs m_voiceKnobs[3];

	knob * m_volKnob;
	knob * m_resKnob;
	knob * m_cutKnob;
	pixmapButton * m_offButton;

protected slots:
	void updateKnobHint( void );
	void updateKnobToolTip( void );
} ;


#endif
