/*
 * Sf2Player.h - a soundfont2 player using fluidSynth
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
 * Copyright (c) 2009-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef SF2_PLAYER_H
#define SF2_PLAYER_H

#include <array>
#include <fluidsynth/types.h>
#include <QMutex>
#include <samplerate.h>

#include "Instrument.h"
#include "InstrumentView.h"
#include "LcdSpinBox.h"

class QLabel;

namespace lmms
{


struct Sf2PluginData;
class NotePlayHandle;

namespace gui
{
class Knob;
class PixmapButton;
class Sf2InstrumentView;
class PatchesDialog;
} // namespace gui


class Sf2Instrument : public Instrument
{
	Q_OBJECT
	mapPropertyFromModel(int,getBank,setBank,m_bankNum);
	mapPropertyFromModel(int,getPatch,setPatch,m_patchNum);

public:
	Sf2Instrument( InstrumentTrack * _instrument_track );
	~Sf2Instrument() override;

	void play( SampleFrame* _working_buffer ) override;

	void playNote( NotePlayHandle * _n,
						SampleFrame* _working_buffer ) override;
	void deleteNotePluginData( NotePlayHandle * _n ) override;


	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;

	void loadFile( const QString & _file ) override;

	AutomatableModel * childModel( const QString & _modelName ) override;

	QString nodeName() const override;

	gui::PluginView* instantiateView( QWidget * _parent ) override;
	
	QString getCurrentPatchName();


	void setParameter( const QString & _param, const QString & _value );


public slots:
	void openFile( const QString & _sf2File, bool updateTrackName = true );
	void updatePatch();
	void reloadSynth();
	
	// We can't really support sample-exact with the way IPH and FS work.
	// So, sig/slots work just fine for the synth settings right now.
	void updateReverbOn();
	void updateReverb();
	void updateChorusOn();
	void updateChorus();
	void updateGain();
	void updateTuning();

private:
	SRC_STATE * m_srcState;

	fluid_settings_t* m_settings;
	fluid_synth_t* m_synth;

	fluid_sfont_t* m_font;

	int m_fontId;
	QString m_filename;

	// Protect the array of active notes
	QMutex m_notesRunningMutex;

	// Protect synth when we are re-creating it.
	QMutex m_synthMutex;
	QMutex m_loadMutex;

	std::array<int, 128> m_notesRunning = {};
	sample_rate_t m_internalSampleRate;
	int m_lastMidiPitch;
	int m_lastMidiPitchRange;
	int m_channel;

	gui::LcdSpinBoxModel m_bankNum;
	gui::LcdSpinBoxModel m_patchNum;

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

	QVector<NotePlayHandle *> m_playingNotes;
	QMutex m_playingNotesMutex;

private:
	void freeFont();
	void noteOn( Sf2PluginData * n );
	void noteOff( Sf2PluginData * n );
	void renderFrames( f_cnt_t frames, SampleFrame* buf );

	friend class gui::Sf2InstrumentView;

signals:
	void fileLoading();
	void fileChanged();
	void patchChanged();

} ;


namespace gui
{


class Sf2InstrumentView : public InstrumentViewFixedSize
{
	Q_OBJECT
public:
	Sf2InstrumentView( Instrument * _instrument,
					QWidget * _parent );
	~Sf2InstrumentView() override = default;

private:
	void modelChanged() override;

	PixmapButton * m_fileDialogButton;
	PixmapButton * m_patchDialogButton;

	LcdSpinBox * m_bankNumLcd;
	LcdSpinBox * m_patchNumLcd;

	QLabel * m_filenameLabel;
	QLabel * m_patchLabel;

	Knob	* m_gainKnob;

	PixmapButton * m_reverbButton;
	Knob	* m_reverbRoomSizeKnob;
	Knob	* m_reverbDampingKnob;
	Knob	* m_reverbWidthKnob;
	Knob	* m_reverbLevelKnob;

	PixmapButton * m_chorusButton;
	Knob * m_chorusNumKnob;
	Knob * m_chorusLevelKnob;
	Knob * m_chorusSpeedKnob;
	Knob * m_chorusDepthKnob;

	static PatchesDialog * s_patchDialog;

protected slots:
	void invalidateFile();
	void showFileDialog();
	void showPatchDialog();
	void updateFilename();
	void updatePatchName();
} ;


} // namespace gui

} // namespace lmms

#endif
