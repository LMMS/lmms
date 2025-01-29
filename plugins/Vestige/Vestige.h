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


#include <QMdiSubWindow>
#include <QMutex>

#include "AudioPlugin.h"
#include "InstrumentView.h"
#include "RemotePluginAudioPort.h"


class QPixmap;
class QPushButton;
class QScrollArea;
class QGridLayout;

namespace lmms
{

class FloatModel;
class VstPlugin;

namespace gui
{
class CustomTextKnob;
class ManageVestigeInstrumentView;
class PixmapButton;
class PluginPinConnectorView;
class VestigeInstrumentView;
} // namespace gui


constexpr auto VestigeConfig = AudioPluginConfig {
	.kind = AudioDataKind::F32,
	.interleaved = false
};

class VestigeInstrument
	: public AudioPlugin<Instrument, VestigeConfig, RemotePluginAudioPort<VestigeConfig>>
{
	Q_OBJECT
public:
	VestigeInstrument( InstrumentTrack * _instrument_track );
	~VestigeInstrument() override;

	void processImpl() override;

	void saveSettings(QDomDocument& _doc, QDomElement& _parent) override;
	void loadSettings(const QDomElement& _this) override;

	QString nodeName() const override;

	void loadFile(const QString& _file) override;

	bool handleMidiEvent(const MidiEvent& event, const TimePos& time, f_cnt_t offset = 0) override;

	gui::PluginView* instantiateView(QWidget* _parent) override;

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
	ManageVestigeInstrumentView( VestigeInstrument * _instrument, QWidget * _parent, VestigeInstrument * _vi2 );
	virtual ~ManageVestigeInstrumentView();


protected slots:
	void syncPlugin();
	void displayAutomatedOnly();
	void setParameter( lmms::Model * action );
	void syncParameterText();
	void togglePinConnector();
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
	QPushButton* m_pinConnectorButton;
	PluginPinConnectorView* m_pinConnector = nullptr;
	CustomTextKnob ** vstKnobs;

} ;


class VestigeInstrumentView : public InstrumentViewFixedSize
{
	Q_OBJECT
public:
	VestigeInstrumentView( VestigeInstrument * _instrument, QWidget * _parent );
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

	VestigeInstrument* m_instrument2;
	QWidget * _parent2;

} ;


} // namespace gui

} // namespace lmms

#endif
