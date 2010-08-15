/*
 * VstPlugin.h - declaration of VstPlugin class
 *
 * Copyright (c) 2005-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _VST_PLUGIN_H
#define _VST_PLUGIN_H

#include <QtCore/QMutex>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtGui/QWidget>

#include "JournallingObject.h"
#include "communication.h"


class PLUGIN_EXPORT VstPlugin : public QObject, public JournallingObject,
								public RemotePlugin
{
	Q_OBJECT
public:
	VstPlugin( const QString & _plugin );
	virtual ~VstPlugin();

	virtual bool processMessage( const message & _m );

	inline bool hasEditor() const
	{
		return m_pluginWindowID != 0;
	}

	void showEditor( QWidget * _parent = NULL );
	void hideEditor();

	inline const QString & name() const
	{
		return m_name;
	}

	inline Sint32 version() const
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

	const QMap<QString, QString> & parameterDump();
	void setParameterDump( const QMap<QString, QString> & _pdump );


	inline QWidget * pluginWidget( bool _top_widget = true )
	{
		if( _top_widget && m_pluginWidget )
		{
			if( m_pluginWidget->parentWidget() )
			{
				return m_pluginWidget->parentWidget();
			}
		}
		return m_pluginWidget;
	}

	virtual void loadSettings( const QDomElement & _this );
	virtual void saveSettings( QDomDocument & _doc, QDomElement & _this );

	inline virtual QString nodeName() const
	{
		return "vstplugin";
	}


public slots:
	void setTempo( bpm_t _bpm );
	void updateSampleRate();


private:
	void loadChunk( const QByteArray & _chunk );
	QByteArray saveChunk();

	QString m_plugin;
	QPointer<QWidget> m_pluginWidget;
	int m_pluginWindowID;
	QSize m_pluginGeometry;

	QString m_name;
	Sint32 m_version;
	QString m_vendorString;
	QString m_productString;

	QMap<QString, QString> m_parameterDump;

} ;


#endif
