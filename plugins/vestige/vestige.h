/*
 * vestige.h - instrument VeSTige for hosting VST-plugins
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of LMMS - http://lmms.io
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


#ifndef _VESTIGE_H
#define _VESTIGE_H


#include <QtCore/QMutex>
#include <QtGui/QLayout>
#include <QtGui/QMdiSubWindow>
#include <QtGui/QScrollArea>

#include "Instrument.h"
#include "InstrumentView.h"
#include "note.h"
#include "knob.h"

#include "AutomatableModel.h"


class QPixmap;
class QPushButton;

class pixmapButton;
class VstPlugin;


class vestigeInstrument : public Instrument
{
	Q_OBJECT
public:
	vestigeInstrument( InstrumentTrack * _instrument_track );
	virtual ~vestigeInstrument();

	virtual void play( sampleFrame * _working_buffer );

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual QString nodeName( void ) const;

	virtual void loadFile( const QString & _file );

	virtual Flags flags() const
	{
		return IsSingleStreamed | IsMidiBased;
	}

	virtual bool handleMidiEvent( const MidiEvent& event, const MidiTime& time, f_cnt_t offset = 0 );

	virtual PluginView * instantiateView( QWidget * _parent );

protected slots:
	void setParameter( void );

private:
	void closePlugin( void );


	VstPlugin * m_plugin;
	QMutex m_pluginMutex;

	QString m_pluginDLL;
	QMdiSubWindow * m_subWindow;
	QScrollArea * m_scrollArea;
	knob ** vstKnobs;
	FloatModel ** knobFModel;
	QObject * p_subWindow;
	int paramCount;


	friend class VestigeInstrumentView;
	friend class manageVestigeInstrumentView;

} ;


class manageVestigeInstrumentView : public InstrumentView
{
	Q_OBJECT
public:
	manageVestigeInstrumentView( Instrument * _instrument, QWidget * _parent, vestigeInstrument * m_vi2 );
	virtual ~manageVestigeInstrumentView();


protected slots:
	void syncPlugin( void );
	void displayAutomatedOnly( void );
	void setParameter( void );
	void closeWindow();


protected:
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );
	virtual void paintEvent( QPaintEvent * _pe );


private:
	static QPixmap * s_artwork;

	vestigeInstrument * m_vi;

	QWidget *widget;
	QGridLayout * l;
	QPushButton * m_syncButton;
	QPushButton * m_displayAutomatedOnly;
	QPushButton * m_closeButton;

} ;


class VestigeInstrumentView : public InstrumentView
{
	Q_OBJECT
public:
	VestigeInstrumentView( Instrument * _instrument, QWidget * _parent );
	virtual ~VestigeInstrumentView();


protected slots:
	void updateMenu( void );
	void openPlugin( void );
	void managePlugin( void );
	void openPreset( void );
	void savePreset( void );
	void nextProgram();
	void previousProgram();
	void selPreset( void );
	void toggleGUI( void );
	void noteOffAll( void );


protected:
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );
	virtual void paintEvent( QPaintEvent * _pe );


private:
	virtual void modelChanged( void );

	static QPixmap * s_artwork;

	vestigeInstrument * m_vi;

	int lastPosInMenu;

	pixmapButton * m_openPluginButton;
	pixmapButton * m_openPresetButton;
	pixmapButton * m_rolLPresetButton;
	pixmapButton * m_rolRPresetButton;
	QPushButton * m_selPresetButton;
	QPushButton * m_toggleGUIButton;
	pixmapButton * m_managePluginButton;
	pixmapButton * m_savePresetButton;

	Instrument * _instrument2;
	QWidget * _parent2;

} ;



#endif
