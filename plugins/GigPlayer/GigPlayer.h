/*
 * GigPlayer.h - a GIG player using libgig (based on Sf2 player plugin)
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


#ifndef GIG_PLAYER_H
#define GIG_PLAYER_H

#include <QList>
#include <QMutex>
#include <QMutexLocker>
#include <samplerate.h>

#include "Instrument.h"
#include "PixmapButton.h"
#include "InstrumentView.h"
#include "Knob.h"
#include "LcdSpinBox.h"
#include "LedCheckBox.h"
#include "gig.h"


class QLabel;


namespace lmms
{


class NotePlayHandle;

namespace gui
{
class PatchesDialog;
class GigInstrumentView;
}




struct GIGPluginData
{
	int midiNote;
} ;




// Load a GIG file using libgig
class GigInstance
{
public:
	GigInstance( QString filename ) :
		riff( filename.toUtf8().constData() ),
		gig( &riff )
	{}

private:
	RIFF::File riff;

public:
	gig::File gig;
} ;




// Stores options for the notes, e.g. velocity and release time
struct Dimension
{
	Dimension() :
		release( false )
	{
		for( int i = 0; i < 8; ++i )
		{
			DimValues[i] = 0;
		}
	}

	uint DimValues[8];
	bool release;
} ;




// Takes information from the GIG file for a certain note and provides the
// amplitude (0-1) to multiply the signal by (internally incrementing the
// position in the envelope when asking for the amplitude).
class ADSR
{
	// From the file
	float preattack; // initial amplitude (0-1)
	float attack; // 0-60s
	float decay1; // 0-60s
	float decay2; // 0-60s
	bool infiniteSustain; // i.e., no decay2
	float sustain; // sustain amplitude (0-1)
	float release; // 0-60s

	// Used to calculate current amplitude
	float amplitude;
	bool isAttack;
	bool isRelease;
	bool isDone;
	f_cnt_t attackPosition;
	f_cnt_t attackLength;
	f_cnt_t decayLength;
	f_cnt_t releasePosition;
	f_cnt_t releaseLength;

public:
	ADSR();
	ADSR( gig::DimensionRegion * region, int sampleRate );
	void keyup(); // We will begin releasing starting now
	bool done(); // Is this sample done playing?
	float value(); // What's the current amplitude
	void inc( f_cnt_t num ); // Increment internal positions by num
} ;




// The sample from the GIG file with our current position in both the sample
// and the envelope
class GigSample
{
public:
	GigSample( gig::Sample * pSample, gig::DimensionRegion * pDimRegion,
			float attenuation, int interpolation, float desiredFreq );
	~GigSample();

	// Needed when initially creating in QList
	GigSample( const GigSample& g );
	GigSample& operator=( const GigSample& g );

	// Needed since libsamplerate stores data internally between calls
	void updateSampleRate();
	bool convertSampleRate( SampleFrame & oldBuf, SampleFrame & newBuf,
		f_cnt_t oldSize, f_cnt_t newSize, float freq_factor, f_cnt_t& used );

	gig::Sample * sample;
	gig::DimensionRegion * region;
	float attenuation;
	ADSR adsr;

	// The position in sample
	f_cnt_t pos;

	// Whether to change the pitch of the samples, e.g. if there's only one
	// sample per octave and you want that sample pitch shifted for the rest of
	// the notes in the octave, this will be true
	bool pitchtrack;

	// Used to convert sample rates
	int interpolation;
	SRC_STATE * srcState;

	// Used changing the pitch of the note if desired
	float sampleFreq;
	float freqFactor;
} ;




// What portion of a note are we in?
enum class GigState
{
	// We just pressed the key
	KeyDown,
	// The note is currently playing
	PlayingKeyDown,
	// We just released the key
	KeyUp,
	// The note is being released, e.g. a release sample is playing
	PlayingKeyUp,
	// The note is done playing, you can delete this note now
	Completed
} ;




// Corresponds to a certain midi note pressed, but may contain multiple samples
class GigNote
{
public:
	int midiNote;
	int velocity;
	bool release; // Whether to trigger a release sample on key up
	bool isRelease; // Whether this is a release sample, changes when we delete it
	GigState state;
	float frequency;
	QList<GigSample> samples;

	// Used to determine which note should be released on key up
	//
	// Note: if accessing the data, be careful not to access it after the key
	// has been released since that's when it is deleted
	GIGPluginData * handle;

	GigNote( int midiNote, int velocity, float frequency, GIGPluginData * handle )
		: midiNote( midiNote ), velocity( velocity ),
		  release( false ), isRelease( false ), state( GigState::KeyDown ),
		  frequency( frequency ), handle( handle )
	{
	}
} ;



class GigInstrument : public Instrument
{
	Q_OBJECT

	mapPropertyFromModel( int, getBank, setBank, m_bankNum );
	mapPropertyFromModel( int, getPatch, setPatch, m_patchNum );

public:
	GigInstrument( InstrumentTrack * _instrument_track );
	~GigInstrument() override;

	void playImpl(CoreAudioDataMut out) override;
	void playNoteImpl(NotePlayHandle* _n, CoreAudioDataMut out) override;
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
	void openFile( const QString & _gigFile, bool updateTrackName = true );
	void updatePatch();
	void updateSampleRate();


private:
	// The GIG file and instrument we're using
	GigInstance * m_instance;
	gig::Instrument * m_instrument;

	// Part of the UI
	QString m_filename;

	gui::LcdSpinBoxModel m_bankNum;
	gui::LcdSpinBoxModel m_patchNum;

	FloatModel m_gain;

	// Locking for the data
	QMutex m_synthMutex;
	QMutex m_notesMutex;

	// Used for resampling
	int m_interpolation;

	// List of all the currently playing notes
	QList<GigNote> m_notes;

	// Used when determining which samples to use
	uint32_t m_RandomSeed;
	float m_currentKeyDimension;

private:
	// Delete the current GIG instance if one is open
	void freeInstance();

	// Open the instrument in the currently-open GIG file
	void getInstrument();

	// Create "dimension" to select desired samples from GIG file based on
	// parameters such as velocity
	Dimension getDimensions( gig::Region * pRegion, int velocity, bool release );

	// Load sample data from the Gig file, looping the sample where needed
	void loadSample( GigSample& sample, SampleFrame* sampleData, f_cnt_t samples );
	f_cnt_t getLoopedIndex( f_cnt_t index, f_cnt_t startf, f_cnt_t endf ) const;
	f_cnt_t getPingPongIndex( f_cnt_t index, f_cnt_t startf, f_cnt_t endf ) const;

	// Add the desired samples to the note, either normal samples or release
	// samples
	void addSamples( GigNote & gignote, bool wantReleaseSample );

	friend class gui::GigInstrumentView;

signals:
	void fileLoading();
	void fileChanged();
	void patchChanged();

} ;


namespace gui
{


class GigInstrumentView : public InstrumentViewFixedSize
{
	Q_OBJECT
public:
	GigInstrumentView( Instrument * _instrument,
					QWidget * _parent );
	~GigInstrumentView() override = default;

private:
	void modelChanged() override;

	PixmapButton * m_fileDialogButton;
	PixmapButton * m_patchDialogButton;

	LcdSpinBox * m_bankNumLcd;
	LcdSpinBox * m_patchNumLcd;

	QLabel * m_filenameLabel;
	QLabel * m_patchLabel;

	Knob * m_gainKnob;

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
