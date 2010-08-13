/*
 * sample_track.h - class sampleTrack, a track which provides arrangement of
 *                  samples
 *
 * Copyright (c) 2005-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _SAMPLE_TRACK_H
#define _SAMPLE_TRACK_H

#include <QtGui/QDialog>

#include "AudioPort.h"
#include "track.h"

class EffectRackView;
class knob;
class sampleBuffer;


class sampleTCO : public trackContentObject
{
	Q_OBJECT
	mapPropertyFromModel(bool,isRecord,setRecord,m_recordModel);
public:
	sampleTCO( track * _track );
	virtual ~sampleTCO();

	virtual void changeLength( const midiTime & _length );
	const QString & sampleFile() const;

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );
	inline virtual QString nodeName() const
	{
		return "sampletco";
	}

	sampleBuffer * getSampleBuffer()
	{
		return m_sampleBuffer;
	}

	midiTime sampleLength() const;

	virtual trackContentObjectView * createView( trackView * _tv );


public slots:
	void setSampleBuffer( sampleBuffer * _sb );
	void setSampleFile( const QString & _sf );
	void updateLength( bpm_t = 0 );
	void toggleRecord();


private:
	sampleBuffer * m_sampleBuffer;
	BoolModel m_recordModel;


	friend class sampleTCOView;


signals:
	void sampleChanged();

} ;



class sampleTCOView : public trackContentObjectView
{
	Q_OBJECT
public:
	sampleTCOView( sampleTCO * _tco, trackView * _tv );
	virtual ~sampleTCOView();


public slots:
	void updateSample();


protected:
	virtual void contextMenuEvent( QContextMenuEvent * _cme );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );
	virtual void mouseDoubleClickEvent( QMouseEvent * );
	virtual void paintEvent( QPaintEvent * );


private:
	sampleTCO * m_tco;

} ;




class sampleTrack : public track
{
	Q_OBJECT
public:
	sampleTrack( trackContainer * _tc );
	virtual ~sampleTrack();

	virtual bool play( const midiTime & _start, const fpp_t _frames,
						const f_cnt_t _frame_base,
							Sint16 _tco_num = -1 );
	virtual trackView * createView( trackContainerView * _tcv );
	virtual trackContentObject * createTCO( const midiTime & _pos );


	virtual void saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void loadTrackSpecificSettings( const QDomElement & _this );

	inline AudioPort * audioPort()
	{
		return &m_audioPort;
	}

	virtual QString nodeName() const
	{
		return "sampletrack";
	}


private:
	AudioPort m_audioPort;
	FloatModel m_volumeModel;


	friend class sampleTrackView;

} ;



class sampleTrackView : public trackView
{
	Q_OBJECT
public:
	sampleTrackView( sampleTrack * _track, trackContainerView * _tcv );
	virtual ~sampleTrackView();


public slots:
	void showEffects();


protected:
	void modelChanged();
	virtual QString nodeName() const
	{
		return "SampleTrackView";
	}


private:
	EffectRackView * m_effectRack;
	QWidget * m_effWindow;
	knob * m_volumeKnob;

} ;


#endif
