/*
 * Patman.h - header for a GUS-compatible patch instrument plugin
 *
 * Copyright (c) 2007-2008 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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


#ifndef PATMAN_H
#define PATMAN_H

#include "Instrument.h"
#include "InstrumentView.h"
#include "SampleBuffer.h"
#include "AutomatableModel.h"
#include "MemoryManager.h"

namespace lmms
{

namespace gui
{
class PixmapButton;
class PatmanView;
} // namespace gui


#define MODES_16BIT	( 1 << 0 )
#define MODES_UNSIGNED	( 1 << 1 )
#define MODES_LOOPING	( 1 << 2 )
#define MODES_PINGPONG	( 1 << 3 )
#define MODES_REVERSE	( 1 << 4 )
#define MODES_SUSTAIN	( 1 << 5 )
#define MODES_ENVELOPE	( 1 << 6 )
#define MODES_CLAMPED	( 1 << 7 )


class PatmanInstrument : public Instrument
{
	Q_OBJECT
public:
	PatmanInstrument( InstrumentTrack * _track );
	~PatmanInstrument() override;

	void playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer ) override;
	void deleteNotePluginData( NotePlayHandle * _n ) override;


	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;

	void loadFile( const QString & _file ) override;

	QString nodeName() const override;

	f_cnt_t desiredReleaseFrames() const override
	{
		return( 128 );
	}

	gui::PluginView* instantiateView( QWidget * _parent ) override;


public slots:
	void setFile( const QString & _patch_file, bool _rename = true );


private:
	typedef struct
	{
		MM_OPERATORS
		SampleBuffer::handleState* state;
		bool tuned;
		SampleBuffer* sample;
	} handle_data;

	QString m_patchFile;
	QVector<SampleBuffer *> m_patchSamples;
	BoolModel m_loopedModel;
	BoolModel m_tunedModel;


	enum LoadErrors
	{
		LoadOK,
		LoadOpen,
		LoadNotGUS,
		LoadInstruments,
		LoadLayers,
		LoadIO
	} ;

	LoadErrors loadPatch( const QString & _filename );
	void unloadCurrentPatch();

	void selectSample( NotePlayHandle * _n );


	friend class gui::PatmanView;

signals:
	void fileChanged();

} ;


namespace gui
{


class PatmanView : public InstrumentViewFixedSize
{
	Q_OBJECT
public:
	PatmanView( Instrument * _instrument, QWidget * _parent );
	~PatmanView() override;


public slots:
	void openFile();
	void updateFilename();


protected:
	void dragEnterEvent( QDragEnterEvent * _dee ) override;
	void dropEvent( QDropEvent * _de ) override;
	void paintEvent( QPaintEvent * ) override;


private:
	void modelChanged() override;

	PatmanInstrument * m_pi;
	QString m_displayFilename;

	PixmapButton * m_openFileButton;
	PixmapButton * m_loopButton;
	PixmapButton * m_tuneButton;

} ;


} // namespace gui

} // namespace lmms

#endif // PATMAN_H
