/*
 * vst_plugin.h - declaration of vstPlugin class
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtCore/QString>
#include <QtCore/QMutex>
#include <QtGui/QWidget>

#include "mixer.h"
#include "journalling_object.h"
#include "communication.h"
#include "remote_plugin.h"



class PLUGIN_EXPORT vstPlugin : public QObject, public journallingObject,
							public remotePlugin
{
	Q_OBJECT
public:
	vstPlugin( const QString & _plugin );
	virtual ~vstPlugin();

	virtual bool processMessage( const message & _m );

	QWidget * showEditor( QWidget * _parent = NULL );
	void hideEditor( void );

	inline const QString & name( void ) const
	{
		return( m_name );
	}

	inline Sint32 version( void ) const
	{
		return( m_version );
	}
	
	inline const QString & vendorString( void ) const
	{
		return( m_vendorString );
	}

	inline const QString & productString( void ) const
	{
		return( m_productString );
	}

	const QMap<QString, QString> & parameterDump( void );
	void setParameterDump( const QMap<QString, QString> & _pdump );


	inline QWidget * pluginWidget( void )
	{
		return( m_pluginWidget != NULL ?
					m_pluginWidget->parentWidget() : NULL );
	}

	virtual void loadSettings( const QDomElement & _this );
	virtual void saveSettings( QDomDocument & _doc, QDomElement & _this );

	inline virtual QString nodeName( void ) const
	{
		return( "vstplugin" );
	}


public slots:
	void setTempo( bpm_t _bpm );
	void updateSampleRate( void );


private:
	QString m_plugin;
	QWidget * m_pluginWidget;
	int m_pluginWindowID;
	QSize m_pluginGeometry;

	QString m_name;
	Sint32 m_version;
	QString m_vendorString;
	QString m_productString;

	QMap<QString, QString> m_parameterDump;

} ;


#endif
