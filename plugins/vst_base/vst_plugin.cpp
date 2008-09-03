/*
 * vst_plugin.cpp - implementation of vstPlugin class
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


#include "vst_plugin.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QLocale>
#include <QtGui/QMdiArea>
#include <QtGui/QMdiSubWindow>
#include <QtGui/QX11EmbedContainer>
#include <QtGui/QX11Info>
#include <QtXml/QDomDocument>


#include "config_mgr.h"
#include "engine.h"
#include "main_window.h"
#include "templates.h"




vstPlugin::vstPlugin( const QString & _plugin ) :
	QObject(),
	journallingObject(),
	remotePlugin( "remote_vst_plugin", false ),
	m_plugin( _plugin ),
	m_pluginWidget( NULL ),
	m_pluginWindowID( 0 ),
	m_name( "" ),
	m_version( 0 ),
	m_vendorString( "" ),
	m_productString( "" )
{
	setSplittedChannels( true );

	lock();

	VstHostLanguages hlang = LanguageEnglish;
	switch( QLocale::system().language() )
	{
		case QLocale::German: hlang = LanguageGerman; break;
		case QLocale::French: hlang = LanguageFrench; break;
		case QLocale::Italian: hlang = LanguageItalian; break;
		case QLocale::Spanish: hlang = LanguageSpanish; break;
		case QLocale::Japanese: hlang = LanguageJapanese; break;
		default: break;
	}
	sendMessage( message( IdVstSetLanguage ).addInt( hlang ) );


	QString p = m_plugin;
	if( QFileInfo( p ).dir().isRelative() )
	{
		p = configManager::inst()->vstDir() + QDir::separator() + p;
	}

	sendMessage( message( IdVstLoadPlugin ).addString( p.toStdString() ) );

	waitForInitDone();

	unlock();
}




vstPlugin::~vstPlugin()
{
	if( m_pluginWidget != NULL &&
		m_pluginWidget->parentWidget() != NULL &&
		dynamic_cast<QMdiSubWindow *>(
			m_pluginWidget->parentWidget() ) != NULL )
	{
		delete m_pluginWidget->parentWidget();
	}
}




QWidget * vstPlugin::showEditor( QWidget * _parent )
{
	if( m_pluginWidget != NULL )
	{
		if( m_pluginWidget->parentWidget() )
		{
			m_pluginWidget->parentWidget()->show();
		}
		return( m_pluginWidget );
	}

	if( m_pluginWindowID == 0 )
	{
		return( NULL );
	}

	m_pluginWidget = new QWidget( _parent );
	m_pluginWidget->setFixedSize( m_pluginGeometry );
	m_pluginWidget->setWindowTitle( name() );
	if( _parent == NULL )
	{
		engine::getMainWindow()->workspace()->addSubWindow(
							m_pluginWidget )
				->setAttribute( Qt::WA_DeleteOnClose, FALSE );
	}

#ifdef LMMS_BUILD_LINUX
	QX11EmbedContainer * xe = new QX11EmbedContainer( m_pluginWidget );
	xe->embedClient( m_pluginWindowID );
	xe->setFixedSize( m_pluginGeometry );
	//xe->setAutoDelete( FALSE );
	xe->show();
#endif

	m_pluginWidget->show();

	showUI();

	return( m_pluginWidget );
}




void vstPlugin::hideEditor( void )
{
	if( m_pluginWidget != NULL && m_pluginWidget->parentWidget() )
	{
		m_pluginWidget->parentWidget()->hide();
	}
}




void vstPlugin::loadSettings( const QDomElement & _this )
{
	if( pluginWidget() != NULL )
	{
		if( _this.attribute( "guivisible" ).toInt() )
		{
			pluginWidget()->show();
		}
		else
		{
			pluginWidget()->hide();
		}
	}
	const Sint32 num_params = _this.attribute( "numparams" ).toInt();
	if( num_params > 0 )
	{
		QMap<QString, QString> dump;
		for( Sint32 i = 0; i < num_params; ++i )
		{
			const QString key = "param" +
						QString::number( i );
			dump[key] = _this.attribute( key );
		}
		setParameterDump( dump );
	}
}




void vstPlugin::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	if( pluginWidget() != NULL )
	{
		_this.setAttribute( "guivisible", pluginWidget()->isVisible() );
	}
	const QMap<QString, QString> & dump = parameterDump();
	_this.setAttribute( "numparams", dump.size() );
	for( QMap<QString, QString>::const_iterator it = dump.begin();
							it != dump.end(); ++it )
	{
		_this.setAttribute( it.key(), it.value() );
	}
}




void vstPlugin::setTempo( bpm_t _bpm )
{
	lock();
	sendMessage( message( IdVstSetTempo ).addInt( _bpm ) );
	unlock();
}




void vstPlugin::updateSampleRate( void )
{
	lock();
	sendMessage( message( IdSampleRateInformation ).
			addInt( engine::getMixer()->processingSampleRate() ) );
	unlock();
}




const QMap<QString, QString> & vstPlugin::parameterDump( void )
{
	sendMessage( IdVstGetParameterDump );
	waitForMessage( IdVstParameterDump );

	return( m_parameterDump );
}




void vstPlugin::setParameterDump( const QMap<QString, QString> & _pdump )
{
	message m( IdVstSetParameterDump );
	m.addInt( _pdump.size() );
	for( QMap<QString, QString>::const_iterator it = _pdump.begin();
						it != _pdump.end(); ++it )
	{
		const vstParameterDumpItem item =
		{
			( *it ).section( ':', 0, 0 ).toInt(),
			"",
			( *it ).section( ':', 1, 1 ).toFloat()
		} ;
		m.addInt( item.index );
		m.addString( item.shortLabel );
		m.addFloat( item.value );
	}
	lock();
	sendMessage( m );
	unlock();
}




bool vstPlugin::processMessage( const message & _m )
{
	switch( _m.id )
	{
		case IdVstPluginWindowID:
			m_pluginWindowID = _m.getInt();
			break;

		case IdVstPluginEditorGeometry:
		{
			const int w = _m.getInt( 0 );
			const int h = _m.getInt( 1 );
			m_pluginGeometry = QSize( w, h );
			if( m_pluginWidget != NULL )
			{
				m_pluginWidget->setFixedSize(
							m_pluginGeometry );
				if( m_pluginWidget->childAt( 0, 0 ) != NULL )
				{
					m_pluginWidget->childAt( 0, 0
						)->setFixedSize(
							m_pluginGeometry );
				}
			}
			break;
		}

		case IdVstPluginName:
			m_name = _m.getQString();
			break;

		case IdVstPluginVersion:
			m_version = _m.getInt();
			break;

		case IdVstPluginVendorString:
			m_vendorString = _m.getQString();
			break;

		case IdVstPluginProductString:
			m_productString = _m.getQString();
			break;

		case IdVstParameterDump:
		{
			m_parameterDump.clear();
			const int num_params = _m.getInt();
			int p = 0;
			for( int i = 0; i < num_params; ++i )
			{
				vstParameterDumpItem item;
				item.index = _m.getInt( ++p );
				item.shortLabel = _m.getString( ++p );
				item.value = _m.getFloat( ++p );
	m_parameterDump["param" + QString::number( item.index )] =
				QString::number( item.index ) + ":" +
//					QString( item.shortLabel ) + ":" +
					QString::number( item.value );
			}
			break;
		}
		default:
			return remotePlugin::processMessage( _m );
	}
	return true;

}



#include "moc_vst_plugin.cxx"

