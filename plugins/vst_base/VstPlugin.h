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
#include <QMutex>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <QWidget>

#include "JournallingObject.h"
#include "RemotePlugin.h"

#include "vstbase_export.h"

class vstSubWin;


class VSTBASE_EXPORT VstPlugin : public RemotePlugin, public JournallingObject
{
	Q_OBJECT
public:
	VstPlugin( const QString & _plugin );
	virtual ~VstPlugin();

	void tryLoad( const QString &remoteVstPluginExecutable );

	virtual bool processMessage( const message & _m );

	inline bool hasEditor() const
	{
		return m_pluginWindowID != 0;
	}

	void hideEditor();
	void toggleEditor();

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

	int currentProgram();

	const QMap<QString, QString> & parameterDump();
	void setParameterDump( const QMap<QString, QString> & _pdump );


	QWidget * pluginWidget( bool _top_widget = true );

	virtual void loadSettings( const QDomElement & _this );
	virtual void saveSettings( QDomDocument & _doc, QDomElement & _this );

	inline virtual QString nodeName() const
	{
		return "vstplugin";
	}

	void toggleUI() override;

	void createUI( QWidget *parent, bool isEffect );
	bool eventFilter(QObject *obj, QEvent *event);

	QString embedMethod() const;

public slots:
	void setTempo( bpm_t _bpm );
	void updateSampleRate();
	void openPreset( void );
	void setProgram( int index );
	void rotateProgram( int offset );
	void loadProgramNames();
	void savePreset( void );
	void setParam( int i, float f );
	void idleUpdate();

	void showUI() override;
	void hideUI() override;

	void handleClientEmbed();

private:
	void loadChunk( const QByteArray & _chunk );
	QByteArray saveChunk();

	QString m_plugin;
	QPointer<QWidget> m_pluginWidget;
	QPointer<vstSubWin> m_pluginSubWindow;
	int m_pluginWindowID;
	QSize m_pluginGeometry;
	const QString m_embedMethod;

	bool m_badDllFormat;

	QString m_name;
	int m_version;
	QString m_vendorString;
	QString m_productString;
	QString m_currentProgramName;
	QString m_allProgramNames;

	QString p_name;

	QMap<QString, QString> m_parameterDump;

	int m_currentProgram;

	QTimer m_idleTimer;

} ;


#endif
