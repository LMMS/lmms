/*
 * VstPlugin.h - declaration of VstPlugin class
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

#ifndef _VST_PLUGIN_H
#define _VST_PLUGIN_H

#include <QMap>
#include <QPointer>
#include <QSize>
#include <QString>
#include <QTimer>

#include "JournallingObject.h"
#include "RemotePlugin.h"

#include "vstbase_export.h"

namespace lmms
{

class RemotePluginAudioPortController;

class VSTBASE_EXPORT VstPlugin
	: public RemotePlugin
	, public JournallingObject
{
	Q_OBJECT
public:
	VstPlugin(const QString& plugin, RemotePluginAudioPortController& audioPort);
	~VstPlugin() override;

	void tryLoad( const QString &remoteVstPluginExecutable );

	bool processMessage( const message & _m ) override;

	inline bool hasEditor() const
	{
		return m_pluginWindowID != 0;
	}

	/// Same as pluginWidget(), but can be overwritten in sub-classes to modify
	/// behavior the UI. This is used in VstInstrumentPlugin to wrap the VST UI
	/// in a QMdiSubWindow
	virtual QWidget* editor();

	inline const QString & name() const
	{
		return m_name;
	}

	inline int version() const
	{
		return m_version;
	}
	
	inline const QString & vendorString() const
	{
		return m_vendorString;
	}

	inline const QString & productString() const
	{
		return m_productString;
	}

	inline const QString& currentProgramName() const
	{
		return m_currentProgramName;
	}

	inline const QString& allProgramNames() const
	{
		return m_allProgramNames;
	}

	inline const QString& allParameterLabels() const
	{
		return m_allParameterLabels;
	}

	inline const QString& allParameterDisplays() const
	{
		return m_allParameterDisplays;
	}

	int currentProgram();

	const QMap<QString, QString> & parameterDump();
	void setParameterDump( const QMap<QString, QString> & _pdump );


	QWidget * pluginWidget();

	void loadSettings( const QDomElement & _this ) override;
	void saveSettings( QDomDocument & _doc, QDomElement & _this ) override;

	QString nodeName() const override
	{
		return "vstplugin";
	}


	virtual void createUI(QWidget *parent);
	bool eventFilter(QObject *obj, QEvent *event) override;

	QString embedMethod() const;

public slots:
	void setTempo( lmms::bpm_t _bpm );
	void updateSampleRate();
	void openPreset();
	void setProgram( int index );
	void rotateProgram( int offset );
	void loadProgramNames();
	void loadParameterLabels();
	void loadParameterDisplays();
	void savePreset();
	void setParam( int i, float f );
	void idleUpdate();

	void showUI() override;
	void hideUI() override;
	void toggleUI() override;

	void handleClientEmbed();

private:
	void loadChunk( const QByteArray & _chunk );
	QByteArray saveChunk();

	void toggleEditorVisibility(int visible = -1);

	QString m_plugin;
	QPointer<QWidget> m_pluginWidget;
	int m_pluginWindowID;
	QSize m_pluginGeometry;
	const QString m_embedMethod;

	QString m_name;
	int m_version;
	QString m_vendorString;
	QString m_productString;
	QString m_currentProgramName;
	QString m_allProgramNames;
	QString m_allParameterLabels;
	QString m_allParameterDisplays;

	QString p_name;

	QMap<QString, QString> m_parameterDump;

	int m_currentProgram;

	QTimer m_idleTimer;
};


} // namespace lmms

#endif
