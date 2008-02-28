/*
 * sf2_player.h - a soundfont2 player using fluidSynth
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
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


#ifndef _SF2_PLAYER_H
#define _SF2_PLAYER_H

#include "instrument.h"
#include "pixmap_button.h"
#include "instrument_view.h"
#include "knob.h"
#include "lcd_spinbox.h"
#include "fluidsynth.h"

class sf2InstrumentView;
class notePlayHandle;

class patchesDialog;
class QLabel;

class sf2Instrument : public instrument
{
	Q_OBJECT
public:
	sf2Instrument( instrumentTrack * _instrument_track );
	virtual ~sf2Instrument();

	virtual void play( bool _try_parallelizing );

	virtual void FASTCALL playNote( notePlayHandle * _n,
						bool _try_parallelizing );
	virtual void FASTCALL deleteNotePluginData( notePlayHandle * _n );


	virtual void FASTCALL saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );

	virtual QString nodeName( void ) const;

	virtual f_cnt_t desiredReleaseFrames( void ) const
	{
		return( 0 );
	}

	virtual bool notePlayHandleBased( void ) const
	{
		return( FALSE );
	}


	virtual bool supportsParallelizing( void ) const
	{
		return( FALSE );
	}

	virtual void waitForWorkerThread( void );


	virtual pluginView * instantiateView( QWidget * _parent );

public slots:
	void openFile( const QString & _sf2File );
	void updatePatch( void );


private:
	fluid_settings_t* m_settings;

	fluid_synth_t* m_synth;

	fluid_audio_driver_t* m_adriver;

	int m_fontId;



	QString m_filename;

	lcdSpinBoxModel m_bankNum;
	lcdSpinBoxModel m_patchNum;

	friend class sf2InstrumentView;

signals:
	void fileChanged( void );
	void patchChanged( void );

} ;



class sf2InstrumentView : public instrumentView
{
	Q_OBJECT
public:
	sf2InstrumentView( instrument * _instrument,
					QWidget * _parent );
	virtual ~sf2InstrumentView();

private:
	virtual void modelChanged( void );

	pixmapButton * m_fileDialogButton;
	pixmapButton * m_patchDialogButton;

	lcdSpinBox * m_bankNumLcd;
	lcdSpinBox * m_patchNumLcd;

	QLabel * m_filenameLabel;

	static patchesDialog * s_patchDialog;

protected slots:
	void showFileDialog( void );
	void showPatchDialog( void );
	void updateFilename( void );
} ;



#endif
