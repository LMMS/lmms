/*
 * audio_file_processor.h - declaration of class audioFileProcessor
 *                          (instrument-plugin for using audio-files)
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _AUDIO_FILE_PROCESSOR_H
#define _AUDIO_FILE_PROCESSOR_H

#include "qt3support.h"

#ifdef QT4

#include <QtGui/QPixmap>

#else

#include <qpixmap.h>

#endif

#include "instrument.h"
#include "sample_buffer.h"
#include "spc_bg_hndl_widget.h"


class knob;
class pixmapButton;


class audioFileProcessor : public instrument, public specialBgHandlingWidget
{
	Q_OBJECT
public:
	audioFileProcessor( instrumentTrack * _channel_track );
	virtual ~audioFileProcessor();

	virtual void FASTCALL playNote( notePlayHandle * _n );
	virtual void FASTCALL deleteNotePluginData( notePlayHandle * _n );

	virtual void FASTCALL saveSettings( QDomDocument & _doc,
						QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );

	virtual void FASTCALL setParameter( const QString & _param,
						const QString & _value );

	virtual QString nodeName( void ) const;

	virtual Uint32 FASTCALL getBeatLen( notePlayHandle * _n ) const;


public slots:
	void setAudioFile( const QString & _audio_file );


protected slots:
	void openAudioFile( void );
	void reverseBtnToggled( bool _on );
	void ampKnobChanged( float _new_value );
	void startKnobChanged( float _new_value );
	void endKnobChanged( float _new_value );
	void lineDrawBtnToggled( bool _on );
	void dotDrawBtnToggled( bool _on );
	void sampleUpdated( void );


protected:
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );
	virtual void paintEvent( QPaintEvent * );


private:
	static QPixmap * s_artwork;


	sampleBuffer m_sampleBuffer;
	
	sampleBuffer::drawMethods m_drawMethod;

	QPixmap m_graph;
	knob * m_ampKnob;
	knob * m_startKnob;
	knob * m_endKnob;
	pixmapButton * m_openAudioFileButton;
	pixmapButton * m_viewLinesPB;
	pixmapButton * m_viewDotsPB;
	pixmapButton * m_reverseButton;
	pixmapButton * m_loopButton;


	void updateSample( void );
	void FASTCALL setStartAndEndKnob( float _s, float _e );

} ;


#endif
