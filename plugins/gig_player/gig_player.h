/*
 * gig_player.h - a gig player using libgig
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


#ifndef _GIG_PLAYER_H
#define _GIG_PLAYER_H

#include <QtCore/QMutex>
#include <list>

#include "Instrument.h"
#include "pixmap_button.h"
#include "InstrumentView.h"
#include "knob.h"
#include "LcdSpinBox.h"
#include "led_checkbox.h"
#include "SampleBuffer.h"
#include "gig.h"

class gigInstrumentView;
class gigInstance;
class NotePlayHandle;

class patchesDialog;
class QLabel;

class gigInstance
{
public:
	gigInstance( QString filename ) :
		riff( filename.toUtf8().constData() ),
		gig( &riff ),
		refCount( 1 )
	{}

private:
	RIFF::File riff;

public:
	gig::File gig;
	int refCount;
};



struct Dimension
{
	Dimension() :
		release( false )
	{
		for( int i = 0; i < 8; ++i)
			DimValues[i] = 0;
	}

	uint DimValues[8];
	bool release;
};



class gigNote
{
public:
	gigNote();
	gigNote(int size, bool release );
	gigNote( const gigNote& g );
	~gigNote();

	// Move constructor
	gigNote( gigNote&& g );

	// Move assignment
	gigNote& operator=( gigNote&& g );

	int position;
	int midiNote;
	sampleFrame* note;
	int size; // Don't try changing this...
	bool release; // Whether to trigger a release sample on key up
};




class gigInstrument : public Instrument
{
	Q_OBJECT
	mapPropertyFromModel(int,getBank,setBank,m_bankNum);
	mapPropertyFromModel(int,getPatch,setPatch,m_patchNum);

public:
	gigInstrument( InstrumentTrack * _instrument_track );
	virtual ~gigInstrument();

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
		return IsSingleStreamed | IsMidiBased;
	}

	virtual PluginView * instantiateView( QWidget * _parent );

	QString getCurrentPatchName();


	void setParameter( const QString & _param, const QString & _value );


public slots:
	void openFile( const QString & _gigFile, bool updateTrackName = true );
	void updatePatch();
	void updateSampleRate();


private:
	static QMutex s_instancesMutex;
	static QMap<QString, gigInstance*> s_instances;
	std::list<gigNote> m_notes;

	SRC_STATE * m_srcState;

	gigInstance* m_instance;
	gig::Instrument* m_instrument;
	uint32_t m_RandomSeed;

	QString m_filename;

	// Protect synth when we are re-creating it.
	QMutex m_synthMutex;
	QMutex m_loadMutex;

	sample_rate_t m_internalSampleRate;
	int m_lastMidiPitch;
	int m_lastMidiPitchRange;
	int m_channel;

	LcdSpinBoxModel m_bankNum;
	LcdSpinBoxModel m_patchNum;

	FloatModel m_gain;

private:
	void freeInstance();
	void getInstrument();
	gigNote sampleToNote( gig::Sample* pSample, int midiNote, float attenuation, bool release );
	Dimension getDimensions( gig::Region* pRegion, int velocity, bool release );
	gigNote convertSampleRate( gigNote& old, int oldRate, int newRate );
	void addNotes( int midiNote, int velocity, bool release ); // Locks m_synthMutex internally

	friend class gigInstrumentView;

signals:
	void fileLoading();
	void fileChanged();
	void patchChanged();

} ;



class gigInstrumentView : public InstrumentView
{
	Q_OBJECT
public:
	gigInstrumentView( Instrument * _instrument,
					QWidget * _parent );
	virtual ~gigInstrumentView();

private:
	virtual void modelChanged();

	pixmapButton * m_fileDialogButton;
	pixmapButton * m_patchDialogButton;

	LcdSpinBox * m_bankNumLcd;
	LcdSpinBox * m_patchNumLcd;

	QLabel * m_filenameLabel;
	QLabel * m_patchLabel;

	knob	* m_gainKnob;

	static patchesDialog * s_patchDialog;

protected slots:
	void invalidateFile();
	void showFileDialog();
	void showPatchDialog();
	void updateFilename();
	void updatePatchName();
} ;



#endif
