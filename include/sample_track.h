/*
 * sample_track.h - class sampleTrack, a track which provides arrangement of
 *                  samples
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


#ifndef _SAMPLE_TRACK_H
#define _SAMPLE_TRACK_H

#include "qt3support.h"

#ifdef QT4

#include <QDialog>

#else

#include <qdialog.h>

#endif


#include "track.h"
#include "sample_buffer.h"


class nameLabel;
class QLabel;
//class sampleTCOSettingsDialog;


class sampleTCO : public trackContentObject
{
	Q_OBJECT
public:
	sampleTCO( track * _track );
	virtual ~sampleTCO();

	virtual void FASTCALL changeLength( const midiTime & _length );
	void FASTCALL play( sampleFrame * _ab, Uint32 _start_frame,
							Uint32 _frames );
	const QString & sampleFile( void ) const;

	virtual void FASTCALL saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );
	inline virtual QString nodeName( void ) const
	{
		return( "sampletco" );
	}


public slots:
	void setSampleFile( const QString & _sf );
	void updateLength( int = 0 );


protected:
	void paintEvent( QPaintEvent * );
	void mouseDoubleClickEvent( QMouseEvent * );
	midiTime getSampleLength( void ) const;


private:
	sampleBuffer m_sampleBuffer;


	//friend class sampleTCOSettingsDialog;

} ;



/*
class sampleTCOSettingsDialog : public QDialog
{
	Q_OBJECT
public:
	sampleTCOSettingsDialog( sampleTCO * _stco );
	~sampleTCOSettingsDialog();


protected slots:
	void openSampleFile( void );
	void setSampleFile( const QString & _f );


private:
	sampleTCO * m_sampleTCO;
	QLabel * m_fileLbl;

} ;
*/



class sampleTrack : public QObject, public track
{
	Q_OBJECT
public:
	sampleTrack( trackContainer * _tc );
	virtual ~sampleTrack();

	virtual trackTypes trackType( void ) const;
	virtual bool FASTCALL play( const midiTime & _start,
					Uint32 _start_frame, Uint32 _frames,
					Uint32 _frame_base,
							Sint16 _tco_num = -1 );
	virtual trackContentObject * FASTCALL createTCO( const midiTime &
									_pos );


	virtual void FASTCALL saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadTrackSpecificSettings( const QDomElement &
									_this );


public slots:
	virtual QString nodeName( void ) const
	{
		return( "sampletrack" );
	}


private:
	nameLabel * m_trackLabel;

} ;


#endif
