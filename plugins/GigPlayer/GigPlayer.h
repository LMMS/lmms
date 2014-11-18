/*
 * GigPlayer.h - a GIG player using libgig (based on Sf2 player plugin)
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
 * Copyright (c) 2009-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef GIG_PLAYER_H
#define GIG_PLAYER_H

#include <QList>
#include <QMutex>
#include <QMutexLocker>

#include "Instrument.h"
#include "pixmap_button.h"
#include "InstrumentView.h"
#include "knob.h"
#include "LcdSpinBox.h"
#include "led_checkbox.h"
#include "SampleBuffer.h"
#include "gig.h"

class GigInstrumentView;
class NotePlayHandle;

class PatchesDialog;
class QLabel;


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
	int attackPosition;
	int attackLength;
	int decayLength;
	int releasePosition;
	int releaseLength;

public:
	ADSR();
	ADSR( gig::DimensionRegion * region, int sampleRate );
	void keyup(); // We will begin releasing starting now
	bool done(); // Is this sample done playing?
	float value(); // What's the current amplitude
	void inc( int num ); // Increment internal positions by num
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
	bool convertSampleRate( sampleFrame & oldBuf, sampleFrame & newBuf,
		int oldSize, int newSize, float freq_factor, int& used );

	gig::Sample * sample;
	float attenuation;
	ADSR adsr;

	// The position in sample
	int pos;

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
enum GigState
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
	GigState state;
	float frequency;
	QList<GigSample> samples;

	GigNote( int midiNote, int velocity, float frequency )
		: midiNote( midiNote ), velocity( velocity ),
		  release( false ), state( KeyDown ), frequency( frequency )
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
	virtual ~GigInstrument();

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
		return IsSingleStreamed|IsNotBendable;
	}

	virtual PluginView * instantiateView( QWidget * _parent );

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

	LcdSpinBoxModel m_bankNum;
	LcdSpinBoxModel m_patchNum;

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

	// Add the desired samples to the note, either normal samples or release
	// samples
	void addSamples( GigNote & gignote, bool wantReleaseSample );

	friend class GigInstrumentView;

signals:
	void fileLoading();
	void fileChanged();
	void patchChanged();

} ;




class GigInstrumentView : public InstrumentView
{
	Q_OBJECT
public:
	GigInstrumentView( Instrument * _instrument,
					QWidget * _parent );
	virtual ~GigInstrumentView();

private:
	virtual void modelChanged();

	pixmapButton * m_fileDialogButton;
	pixmapButton * m_patchDialogButton;

	LcdSpinBox * m_bankNumLcd;
	LcdSpinBox * m_patchNumLcd;

	QLabel * m_filenameLabel;
	QLabel * m_patchLabel;

	knob * m_gainKnob;

	static PatchesDialog * s_patchDialog;

protected slots:
	void invalidateFile();
	void showFileDialog();
	void showPatchDialog();
	void updateFilename();
	void updatePatchName();
} ;


#endif
