/*
 * SampleTrack.h - class SampleTrack, a track which provides arrangement of samples
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef SAMPLE_TRACK_H
#define SAMPLE_TRACK_H

#include <QDialog>
#include <QLayout>

#include "AudioPort.h"
#include "FxMixer.h"
#include "FxLineLcdSpinBox.h"
#include "Track.h"

class EffectRackView;
class Knob;
class SampleBuffer;
class SampleTrackWindow;
class TrackLabelButton;
class QLineEdit;


class SampleTCO : public TrackContentObject
{
	Q_OBJECT
	mapPropertyFromModel(bool,isRecord,setRecord,m_recordModel);
public:
	SampleTCO( Track * _track );
	virtual ~SampleTCO();

	virtual void changeLength( const MidiTime & _length );
	const QString & sampleFile() const;

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );
	inline virtual QString nodeName() const
	{
		return "sampletco";
	}

	SampleBuffer* sampleBuffer()
	{
		return m_sampleBuffer;
	}

	MidiTime sampleLength() const;
	void setSampleStartFrame( f_cnt_t startFrame );
	void setSamplePlayLength( f_cnt_t length );
	virtual TrackContentObjectView * createView( TrackView * _tv );


	bool isPlaying() const;
	void setIsPlaying(bool isPlaying);

public slots:
	void setSampleBuffer( SampleBuffer* sb );
	void setSampleFile( const QString & _sf );
	void updateLength();
	void toggleRecord();
	void playbackPositionChanged();
	void updateTrackTcos();


private:
	SampleBuffer* m_sampleBuffer;
	BoolModel m_recordModel;
	bool m_isPlaying;


	friend class SampleTCOView;


signals:
	void sampleChanged();

} ;



class SampleTCOView : public TrackContentObjectView
{
	Q_OBJECT

public:
	SampleTCOView( SampleTCO * _tco, TrackView * _tv );
	virtual ~SampleTCOView() = default;

public slots:
	void updateSample();



protected:
	virtual void contextMenuEvent( QContextMenuEvent * _cme );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );
	virtual void mouseDoubleClickEvent( QMouseEvent * );
	virtual void paintEvent( QPaintEvent * );


private:
	SampleTCO * m_tco;
	QPixmap m_paintPixmap;
} ;




class SampleTrack : public Track
{
	Q_OBJECT
public:
	SampleTrack( TrackContainer* tc );
	virtual ~SampleTrack();

	virtual bool play( const MidiTime & _start, const fpp_t _frames,
						const f_cnt_t _frame_base, int _tco_num = -1 );
	virtual TrackView * createView( TrackContainerView* tcv );
	virtual TrackContentObject * createTCO(const MidiTime & pos);


	virtual void saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void loadTrackSpecificSettings( const QDomElement & _this );

	inline IntModel * effectChannelModel()
	{
		return &m_effectChannelModel;
	}

	inline AudioPort * audioPort()
	{
		return &m_audioPort;
	}

	virtual QString nodeName() const
	{
		return "sampletrack";
	}

public slots:
	void updateTcos();
	void setPlayingTcos( bool isPlaying );
	void updateEffectChannel();

private:
	FloatModel m_volumeModel;
	FloatModel m_panningModel;
	IntModel m_effectChannelModel;
	AudioPort m_audioPort;



	friend class SampleTrackView;
	friend class SampleTrackWindow;

} ;



class SampleTrackView : public TrackView
{
	Q_OBJECT
public:
	SampleTrackView( SampleTrack* Track, TrackContainerView* tcv );
	virtual ~SampleTrackView();

	SampleTrackWindow * getSampleTrackWindow()
	{
		return m_window;
	}

	SampleTrack * model()
	{
		return castModel<SampleTrack>();
	}

	const SampleTrack * model() const
	{
		return castModel<SampleTrack>();
	}


	virtual QMenu * createFxMenu( QString title, QString newFxLabel );


public slots:
	void showEffects();


protected:
	void modelChanged();
	virtual QString nodeName() const
	{
		return "SampleTrackView";
	}

	void dragEnterEvent(QDragEnterEvent *dee);
	void dropEvent(QDropEvent *de);

private slots:
	void assignFxLine( int channelIndex );
	void createFxLine();


private:
	SampleTrackWindow * m_window;
	Knob * m_volumeKnob;
	Knob * m_panningKnob;

	TrackLabelButton * m_tlb;


	friend class SampleTrackWindow;

} ;



class SampleTrackWindow : public QWidget, public ModelView, public SerializingObjectHook
{
	Q_OBJECT
public:
	SampleTrackWindow(SampleTrackView * tv);
	virtual ~SampleTrackWindow();

	SampleTrack * model()
	{
		return castModel<SampleTrack>();
	}

	const SampleTrack * model() const
	{
		return castModel<SampleTrack>();
	}

	void setSampleTrackView(SampleTrackView * tv);

	SampleTrackView *sampleTrackView()
	{
		return m_stv;
	}


public slots:
	void textChanged(const QString & new_name);
	void toggleVisibility(bool on);
	void updateName();


protected:
	// capture close-events for toggling sample-track-button
	virtual void closeEvent(QCloseEvent * ce);

	virtual void saveSettings(QDomDocument & doc, QDomElement & element);
	virtual void loadSettings(const QDomElement & element);

private:
	virtual void modelChanged();

	SampleTrack * m_track;
	SampleTrackView * m_stv;

	// widgets on the top of an sample-track-window
	QLineEdit * m_nameLineEdit;
	Knob * m_volumeKnob;
	Knob * m_panningKnob;
	FxLineLcdSpinBox * m_effectChannelNumber;

	EffectRackView * m_effectRack;

} ;


#endif
