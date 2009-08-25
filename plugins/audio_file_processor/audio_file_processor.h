/*
 * audio_file_processor.h - declaration of class audioFileProcessor
 *						  (instrument-plugin for using audio-files)
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _AUDIO_FILE_PROCESSOR_H
#define _AUDIO_FILE_PROCESSOR_H

#include <QtGui/QPixmap>

#include "Instrument.h"
#include "InstrumentView.h"
#include "sample_buffer.h"
#include "knob.h"
#include "pixmap_button.h"



class audioFileProcessor : public Instrument
{
	Q_OBJECT
public:
	audioFileProcessor( InstrumentTrack * _instrument_track );
	virtual ~audioFileProcessor();

	virtual void playNote( notePlayHandle * _n, 
						sampleFrame * _working_buffer );
	virtual void deleteNotePluginData( notePlayHandle * _n );

	virtual void saveSettings( QDomDocument & _doc,
						QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual void loadResource( const ResourceItem * _resourceItem );

	virtual QString nodeName() const;

	virtual Uint32 getBeatLen( notePlayHandle * _n ) const;

	virtual f_cnt_t desiredReleaseFrames() const
	{
		return 128;
	}

	virtual PluginView * instantiateView( QWidget * _parent );


public slots:
	void setAudioFile( const QString & _audio_file, bool _rename = true );


private slots:
	void reverseModelChanged();
	void ampModelChanged();
	void loopPointChanged();


private:
	typedef sampleBuffer::handleState handleState;

	sampleBuffer m_sampleBuffer;
	
	FloatModel m_ampModel;
	FloatModel m_startPointModel;
	FloatModel m_endPointModel;
	BoolModel m_reverseModel;
	BoolModel m_loopModel;


	friend class AudioFileProcessorView;

} ;



class AudioFileProcessorView : public InstrumentView
{
	Q_OBJECT
public:
	AudioFileProcessorView( Instrument * _instrument, QWidget * _parent );
	virtual ~AudioFileProcessorView();


protected slots:
	void sampleUpdated();
	void openAudioFile();


protected:
	virtual void paintEvent( QPaintEvent * );


private:
	virtual void modelChanged();

	static QPixmap * s_artwork;

	QPixmap m_graph;
	knob * m_ampKnob;
	knob * m_startKnob;
	knob * m_endKnob;
	pixmapButton * m_openAudioFileButton;
	pixmapButton * m_reverseButton;
	pixmapButton * m_loopButton;

} ;


#endif

/* vim: set tw=0 noexpandtab: */
