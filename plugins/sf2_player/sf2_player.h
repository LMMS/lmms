/*
 * sf2_player.h - a soundfont2 player using fluidSynth
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
 * Copyright (c) 2009-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef SF2_PLAYER_H
#define SF2_PLAYER_H

#include <QtCore/QMutex>

#include "Instrument.h"
#include "pixmap_button.h"
#include "InstrumentView.h"
#include "knob.h"
#include "LcdSpinBox.h"
#include "led_checkbox.h"
#include "fluidsynth.h"
#include "SampleBuffer.h"

class sf2InstrumentView;
class sf2Font;
class NotePlayHandle;

class patchesDialog;
class QLabel;


class sf2Instrument : public Instrument
{
	Q_OBJECT
	mapPropertyFromModel(int,getBank,setBank,m_bankNum);
	mapPropertyFromModel(int,getPatch,setPatch,m_patchNum);

public:
	sf2Instrument( InstrumentTrack * _instrument_track );
	virtual ~sf2Instrument();

	virtual void play( sampleFrame * _working_buffer );

	virtual void playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer );
	virtual void deleteNotePluginData( NotePlayHandle * _n );


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual void loadFile( const QString & _file );

	virtual AutomatableModel * childModel( const QString & _modelName );

	virtual QString nodeName() const;

	virtual f_cnt_t desiredReleaseFrames() const
	{
		return 0;
	}

	virtual Flags flags() const
	{
		return IsSingleStreamed;
	}

	virtual PluginView * instantiateView( QWidget * _parent );
	
	QString getCurrentPatchName();


	void setParameter( const QString & _param, const QString & _value );


public slots:
	void openFile( const QString & _sf2File, bool updateTrackName = true );
	void updatePatch();
	void updateSampleRate();
	
	// We can't really support sample-exact with the way IPH and FS work.
	// So, sig/slots work just fine for the synth settings right now.
	void updateReverbOn();
	void updateReverb();
	void updateChorusOn();
	void updateChorus();
	void updateGain();


private:
	static QMutex s_fontsMutex;
	static QMap<QString, sf2Font*> s_fonts;
	static int (* s_origFree)( fluid_sfont_t * );

	SRC_STATE * m_srcState;

	fluid_settings_t* m_settings;
	fluid_synth_t* m_synth;

	sf2Font* m_font;

	int m_fontId;
	QString m_filename;

	// Protect the array of active notes
	QMutex m_notesRunningMutex;

	// Protect synth when we are re-creating it.
	QMutex m_synthMutex;
	QMutex m_loadMutex;

	int m_notesRunning[128];
	sample_rate_t m_internalSampleRate;
	int m_lastMidiPitch;
	int m_lastMidiPitchRange;
	int m_channel;

	LcdSpinBoxModel m_bankNum;
	LcdSpinBoxModel m_patchNum;

	FloatModel m_gain;

	BoolModel m_reverbOn;
	FloatModel m_reverbRoomSize;
	FloatModel m_reverbDamping;
	FloatModel m_reverbWidth;
	FloatModel m_reverbLevel;

	BoolModel m_chorusOn;
	FloatModel m_chorusNum;
	FloatModel m_chorusLevel;
	FloatModel m_chorusSpeed;
	FloatModel m_chorusDepth;


private:
	void freeFont();

	friend class sf2InstrumentView;

signals:
	void fileLoading();
	void fileChanged();
	void patchChanged();

} ;



// A soundfont in our font-map
class sf2Font
{
public:
	sf2Font( fluid_sfont_t * f ) :
		fluidFont( f ),
		refCount( 1 )
	{};

	fluid_sfont_t * fluidFont;
	int refCount;
};



class sf2InstrumentView : public InstrumentView
{
	Q_OBJECT
public:
	sf2InstrumentView( Instrument * _instrument,
					QWidget * _parent );
	virtual ~sf2InstrumentView();

private:
	virtual void modelChanged();

	pixmapButton * m_fileDialogButton;
	pixmapButton * m_patchDialogButton;

	LcdSpinBox * m_bankNumLcd;
	LcdSpinBox * m_patchNumLcd;

	QLabel * m_filenameLabel;
	QLabel * m_patchLabel;

	knob	* m_gainKnob;

	pixmapButton * m_reverbButton;
	knob	* m_reverbRoomSizeKnob;
	knob	* m_reverbDampingKnob;
	knob	* m_reverbWidthKnob;
	knob	* m_reverbLevelKnob;

	pixmapButton * m_chorusButton;
	knob * m_chorusNumKnob;
	knob * m_chorusLevelKnob;
	knob * m_chorusSpeedKnob;
	knob * m_chorusDepthKnob;

	static patchesDialog * s_patchDialog;

protected slots:
	void invalidateFile();
	void showFileDialog();
	void showPatchDialog();
	void updateFilename();
	void updatePatchName();
} ;



#endif
