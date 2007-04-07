/*
 * patman.h - header for a GUS-compatible patch instrument plugin
 *
 * Copyright (c) 2007 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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


#ifndef _PATMAN_H_
#define _PATMAN_H_


#include "instrument.h"
#include "sample_buffer.h"
#include "spc_bg_hndl_widget.h"


class pixmapButton;


#define MODES_16BIT	( 1 << 0 )
#define MODES_UNSIGNED	( 1 << 1 )
#define MODES_LOOPING	( 1 << 2 )
#define MODES_PINGPONG	( 1 << 3 )
#define MODES_REVERSE	( 1 << 4 )
#define MODES_SUSTAIN	( 1 << 5 )
#define MODES_ENVELOPE	( 1 << 6 )
#define MODES_CLAMPED	( 1 << 7 )


class patmanSynth : public instrument, public specialBgHandlingWidget
{
	Q_OBJECT
public:
	class subPluginFeatures : public plugin::descriptor::subPluginFeatures
	{
	public:
		subPluginFeatures( plugin::pluginTypes _type );

		virtual const QStringList & supportedExtensions( void )
		{
			return( supported_extensions() );
		}

		static const QStringList & supported_extensions( void );

	} ;


	patmanSynth( instrumentTrack * _track );
	virtual ~patmanSynth();

	virtual void FASTCALL playNote( notePlayHandle * _n,
						bool _try_parallelizing );
	virtual void FASTCALL deleteNotePluginData( notePlayHandle * _n );


	virtual void FASTCALL saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );

	virtual void FASTCALL setParameter( const QString & _param,
						const QString & _value );

	virtual QString nodeName( void ) const;


public slots:
	void openFile( void );
	void setFile( const QString & _patch_file, bool _rename = TRUE );


protected:
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );
	virtual void paintEvent( QPaintEvent * );


private:
	typedef struct
	{
		sampleBuffer::handleState * state;
		bool tuned;
		sampleBuffer * sample;
	} handle_data;

	QString m_patchFile;
	vvector<sampleBuffer *> m_patch_samples;

	pixmapButton * m_openFileButton;
	pixmapButton * m_loopButton;
	pixmapButton * m_tuneButton;
	QString m_display_filename;

	enum load_error
	{
		LOAD_OK,
		LOAD_OPEN,
		LOAD_NOT_GUS,
		LOAD_INSTRUMENTS,
		LOAD_LAYERS,
		LOAD_IO
	} ;

	load_error load_patch( const QString & _filename );
	void unload_current_patch( void );

	void select_sample( notePlayHandle * _n );

} ;


#endif
