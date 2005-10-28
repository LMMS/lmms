/*
 * vestige.h - instrument VeSTige for hosting VST-plugins
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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


#ifndef _VESTIGE_H
#define _VESTIGE_H

#include "instrument.h"
#include "midi.h"

#ifdef QT4

#include <QVector>

#else

#include <qvaluevector.h>

#endif


#include <fst.h>
#include <vst/aeffectx.h>

#include "spc_bg_hndl_widget.h"


class pixmapButton;
class QPixmap;


class vestigeInstrument : public instrument, public specialBgHandlingWidget
{
	Q_OBJECT
public:
	vestigeInstrument( channelTrack * _channel_track );
	virtual ~vestigeInstrument();

	virtual void play( void );

	virtual void FASTCALL playNote( notePlayHandle * _n );
	virtual void FASTCALL deleteNotePluginData( notePlayHandle * _n );


	virtual void FASTCALL saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );

	virtual QString nodeName( void ) const;

	virtual void FASTCALL setParameter( const QString & _param,
						const QString & _value );


protected slots:
	void openPlugin( void );


protected:
	virtual void paintEvent( QPaintEvent * _pe );


private:
	void closePlugin( void );

	void enqueueEvent( const midiEvent & _e, Uint32 _frames_ahead = 0 );

	static long hostCallback( AEffect *, long, long, long, void *, float );


	static bool s_initialized;
	static bool s_threadAdopted;
	static QPixmap * s_artwork;


	FSTHandle * m_handle;
	FST * m_fst;

	vvector<VstMidiEvent> m_midiEvents;


	pixmapButton * m_openPluginButton;

	QString m_plugin;


} ;


#endif
