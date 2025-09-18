/*
 * Vestige.h - instrument VeSTige for hosting VST-plugins
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


#ifndef _VESTIGE_H
#define _VESTIGE_H


#include <QMutex>

#include "Instrument.h"
#include "InstrumentView.h"


class QGridLayout;
class QMdiSubWindow;
class QPushButton;
class QScrollArea;

namespace lmms
{

class FloatModel;
class VstPlugin;

namespace gui
{
class PixmapButton;
class CustomTextKnob;
class VestigeInstrumentView;
class ManageVestigeInstrumentView;
} // namespace gui


class VestigeInstrument : public Instrument
{
	Q_OBJECT
public:
	VestigeInstrument( InstrumentTrack * _instrument_track );
	virtual ~VestigeInstrument();

	virtual void play( SampleFrame* _working_buffer );

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual QString nodeName() const;

	virtual void loadFile( const QString & _file );

	virtual bool handleMidiEvent( const MidiEvent& event, const TimePos& time, f_cnt_t offset = 0 );

	virtual gui::PluginView* instantiateView( QWidget * _parent );

protected slots:
	void setParameter( lmms::Model * action );
	void handleConfigChange( QString cls, QString attr, QString value );
	void reloadPlugin();

private:
	void closePlugin();


	VstPlugin * m_plugin;
	QMutex m_pluginMutex;

	QString m_pluginDLL;
	QMdiSubWindow * m_subWindow;
	QScrollArea * m_scrollArea;
	FloatModel ** knobFModel;
	QObject * p_subWindow;
	int paramCount;


	friend class gui::VestigeInstrumentView;
	friend class gui::ManageVestigeInstrumentView;

} ;


namespace gui
{

class ManageVestigeInstrumentView : public InstrumentViewFixedSize
{
	Q_OBJECT
public:
	ManageVestigeInstrumentView( Instrument * _instrument, QWidget * _parent, VestigeInstrument * m_vi2 );
	virtual ~ManageVestigeInstrumentView();


protected slots:
	void syncPlugin();
	void displayAutomatedOnly();
	void setParameter( lmms::Model * action );
	void syncParameterText();
	void closeWindow();


protected:
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );
	virtual void paintEvent( QPaintEvent * _pe );


private:
	VestigeInstrument * m_vi;

	QWidget *widget;
	QGridLayout * l;
	QPushButton * m_syncButton;
	QPushButton * m_displayAutomatedOnly;
	QPushButton * m_closeButton;
	CustomTextKnob ** vstKnobs;

} ;


class VestigeInstrumentView : public InstrumentViewFixedSize
{
	Q_OBJECT
public:
	VestigeInstrumentView( Instrument * _instrument, QWidget * _parent );
	virtual ~VestigeInstrumentView() = default;


protected slots:
	void updateMenu();
	void openPlugin();
	void managePlugin();
	void openPreset();
	void savePreset();
	void nextProgram();
	void previousProgram();
	void selPreset();
	void toggleGUI();
	void noteOffAll();


protected:
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );
	virtual void paintEvent( QPaintEvent * _pe );


private:
	virtual void modelChanged();


	VestigeInstrument * m_vi;

	int lastPosInMenu;

	PixmapButton * m_openPluginButton;
	PixmapButton * m_openPresetButton;
	PixmapButton * m_rolLPresetButton;
	PixmapButton * m_rolRPresetButton;
	QPushButton * m_selPresetButton;
	QPushButton * m_toggleGUIButton;
	PixmapButton * m_managePluginButton;
	PixmapButton * m_savePresetButton;

	Instrument * _instrument2;
	QWidget * _parent2;

} ;


} // namespace gui

} // namespace lmms

#endif
