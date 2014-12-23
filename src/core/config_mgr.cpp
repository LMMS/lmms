/*
 * config_mgr.cpp - implementation of class configManager
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

#include <QtXml/QDomElement>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtGui/QMessageBox>
#include <QtGui/QApplication>

#include "lmmsversion.h"
#include "config_mgr.h"
#include "MainWindow.h"


static inline QString ensureTrailingSlash( const QString & _s )
{
	if( _s.right( 1 ) != QDir::separator() )
	{
		return( _s + QDir::separator() );
	}
	return( _s );
}


configManager * configManager::s_instanceOfMe = NULL;


configManager::configManager() :
	m_lmmsRcFile( QDir::home().absolutePath() + QDir::separator() +
								".lmmsrc.xml" ),
	m_workingDir( QDir::home().absolutePath() + QDir::separator() +
						"lmms" + QDir::separator() ),
	m_dataDir( qApp->applicationDirPath()
#ifdef LMMS_BUILD_WIN32
			+ QDir::separator() + "data" + QDir::separator()
#else
				.section( '/', 0, -2 ) + "/share/lmms/"
#endif
									),
	m_artworkDir( defaultArtworkDir() ),
#ifdef LMMS_BUILD_WIN32
	m_pluginDir( qApp->applicationDirPath()
			+ QDir::separator() + "plugins" + QDir::separator() ),
#else
	m_pluginDir( qApp->applicationDirPath() + '/' + PLUGIN_DIR ),
#endif
	m_vstDir( m_workingDir + "vst" + QDir::separator() ),
	m_flDir( QDir::home().absolutePath() )
{
}




configManager::~configManager()
{
	saveConfigFile();
}




void configManager::setWorkingDir( const QString & _wd )
{
	m_workingDir = ensureTrailingSlash( _wd );
}




void configManager::setVSTDir( const QString & _vd )
{
	m_vstDir = ensureTrailingSlash( _vd );
}




void configManager::setArtworkDir( const QString & _ad )
{
	m_artworkDir = ensureTrailingSlash( _ad );
}




void configManager::setFLDir( const QString & _fd )
{
	m_flDir = ensureTrailingSlash( _fd );
}




void configManager::setLADSPADir( const QString & _fd )
{
	m_ladDir = _fd;
}




void configManager::setSTKDir( const QString & _fd )
{
#ifdef LMMS_HAVE_STK
	m_stkDir = ensureTrailingSlash( _fd );
#endif
}




void configManager::setDefaultSoundfont( const QString & _sf )
{
#ifdef LMMS_HAVE_FLUIDSYNTH
	m_defaultSoundfont = _sf;
#endif
}




void configManager::setBackgroundArtwork( const QString & _ba )
{
#ifdef LMMS_HAVE_FLUIDSYNTH
	m_backgroundArtwork = _ba;
#endif
}




void configManager::addRecentlyOpenedProject( const QString & _file )
{
	m_recentlyOpenedProjects.removeAll( _file );
	if( m_recentlyOpenedProjects.size() > 15 )
	{
		m_recentlyOpenedProjects.removeLast();
	}
	m_recentlyOpenedProjects.push_front( _file );
	configManager::inst()->saveConfigFile();
}




const QString & configManager::value( const QString & _class,
					const QString & _attribute ) const
{
	if( m_settings.contains( _class ) )
	{
		for( stringPairVector::const_iterator it =
						m_settings[_class].begin();
					it != m_settings[_class].end(); ++it )
		{
			if( ( *it ).first == _attribute )
			{
				return( ( *it ).second );
			}
		}
	}
	static QString empty;
	return( empty );
}




void configManager::setValue( const QString & _class,
				const QString & _attribute,
				const QString & _value )
{
	if( m_settings.contains( _class ) )
	{
		for( stringPairVector::iterator it = m_settings[_class].begin();
					it != m_settings[_class].end(); ++it )
		{
			if( ( *it ).first == _attribute )
			{
				( *it ).second = _value;
				return;
			}
		}
	}
	// not in map yet, so we have to add it...
	m_settings[_class].push_back( qMakePair( _attribute, _value ) );
}



#ifdef LMMS_BUILD_WIN32
#include <QtCore/QLibrary>
#include <shlobj.h>

// taken from qt-win-opensource-src-4.2.2/src/corelib/io/qsettings.cpp
static QString windowsConfigPath( int _type )
{
	QString result;

	QLibrary library( "shell32" );
	typedef BOOL( WINAPI* GetSpecialFolderPath )( HWND, char *, int, BOOL );
	GetSpecialFolderPath SHGetSpecialFolderPath = (GetSpecialFolderPath)
	library.resolve( "SHGetSpecialFolderPathA" );
	if( SHGetSpecialFolderPath )
	{
		char path[MAX_PATH];
		SHGetSpecialFolderPath( 0, path, _type, false );
		result = QString::fromLocal8Bit( path );
	}
	return result;
}
#endif



void configManager::loadConfigFile()
{
	// read the XML file and create DOM tree
	QFile cfg_file( m_lmmsRcFile );
	QDomDocument dom_tree;

	if( cfg_file.open( QIODevice::ReadOnly ) )
	{
		QString errorString;
		int errorLine, errorCol;
		if( dom_tree.setContent( &cfg_file, false, &errorString, &errorLine, &errorCol ) )
		{
			// get the head information from the DOM
			QDomElement root = dom_tree.documentElement();

			QDomNode node = root.firstChild();

			// create the settings-map out of the DOM
			while( !node.isNull() )
			{
				if( node.isElement() &&
					node.toElement().hasAttributes () )
				{
					stringPairVector attr;
					QDomNamedNodeMap node_attr =
						node.toElement().attributes();
					for( int i = 0; i < node_attr.count();
									++i )
					{
		QDomNode n = node_attr.item( i );
		if( n.isAttr() )
		{
			attr.push_back( qMakePair( n.toAttr().name(),
							n.toAttr().value() ) );
		}
					}
					m_settings[node.nodeName()] = attr;
				}
				else if( node.nodeName() == "recentfiles" )
				{
					m_recentlyOpenedProjects.clear();
					QDomNode n = node.firstChild();
					while( !n.isNull() )
					{
		if( n.isElement() && n.toElement().hasAttributes() )
		{
			m_recentlyOpenedProjects <<
					n.toElement().attribute( "path" );
		}
		n = n.nextSibling();
					}
				}
				node = node.nextSibling();
			}

			// don't use dated theme folders as they break the UI (i.e. 0.4 != 1.0, etc)
			bool use_artwork_path = 
				root.attribute( "version" ).startsWith( 
					QString::number( LMMS_VERSION_MAJOR ) + "." + 
					QString::number( LMMS_VERSION_MINOR ) );

			if( use_artwork_path && value( "paths", "artwork" ) != "" )
			{
				m_artworkDir = value( "paths", "artwork" );
				if( !QDir( m_artworkDir ).exists() )
				{
					m_artworkDir = defaultArtworkDir();
				}
				if( m_artworkDir.right( 1 ) !=
							QDir::separator() )
				{
					m_artworkDir += QDir::separator();
				}
			}
			setWorkingDir( value( "paths", "workingdir" ) );
			setVSTDir( value( "paths", "vstdir" ) );
			setFLDir( value( "paths", "fldir" ) );
			setLADSPADir( value( "paths", "laddir" ) );
		#ifdef LMMS_HAVE_STK
			setSTKDir( value( "paths", "stkdir" ) );
		#endif
		#ifdef LMMS_HAVE_FLUIDSYNTH
			setDefaultSoundfont( value( "paths", "defaultsf2" ) );
		#endif
			setBackgroundArtwork( value( "paths", "backgroundartwork" ) );
		}
		else if( QApplication::type() == QApplication::GuiClient )
		{
			QMessageBox::warning( NULL, MainWindow::tr( "Configuration file" ),
									MainWindow::tr( "Error while parsing configuration file at line %1:%2: %3" ).
													arg( errorLine ).
													arg( errorCol ).
													arg( errorString ) );
		}
		cfg_file.close();
	}


	if( m_vstDir.isEmpty() || m_vstDir == QDir::separator() ||
			!QDir( m_vstDir ).exists() )
	{
#ifdef LMMS_BUILD_WIN32
		m_vstDir = windowsConfigPath( CSIDL_PROGRAM_FILES ) +
											QDir::separator() + "VstPlugins";
#else
		m_vstDir = ensureTrailingSlash( QDir::home().absolutePath() );
#endif
	}

	if( m_flDir.isEmpty() || m_flDir == QDir::separator() )
	{
		m_flDir = ensureTrailingSlash( QDir::home().absolutePath() );
	}

	if( m_ladDir.isEmpty() || m_ladDir == QDir::separator() ||
			( !m_ladDir.contains( ':' ) && !QDir( m_ladDir ).exists() ) )
	{
#if defined(LMMS_BUILD_WIN32)
		m_ladDir = m_pluginDir + "ladspa" + QDir::separator();
#elif defined(LMMS_BUILD_APPLE)
		m_ladDir = qApp->applicationDirPath() + "/../lib/lmms/ladspa/";
#else
		m_ladDir = qApp->applicationDirPath() + '/' + LIB_DIR + "/ladspa/";
#endif
	}

#ifdef LMMS_HAVE_STK
	if( m_stkDir.isEmpty() || m_stkDir == QDir::separator() ||
			!QDir( m_stkDir ).exists() )
	{
#if defined(LMMS_BUILD_WIN32)
		m_stkDir = m_dataDir + "stk/rawwaves/";
#elif defined(LMMS_BUILD_APPLE)
		m_stkDir = qApp->applicationDirPath() + "/../share/stk/rawwaves/";
#else
		m_stkDir = "/usr/share/stk/rawwaves/";
#endif
	}
#endif


	QDir::setSearchPaths( "resources", QStringList() << artworkDir()
						<< defaultArtworkDir() );

	if( !QDir( m_workingDir ).exists() &&
		QApplication::type() == QApplication::GuiClient &&
		QMessageBox::question( 0,
			MainWindow::tr( "Working directory" ),
			MainWindow::tr( "The LMMS working directory %1 does not "
				"exist. Create it now? You can change the directory "
				"later via Edit -> Settings." ).arg( m_workingDir ),
					QMessageBox::Yes, QMessageBox::No ) == QMessageBox::Yes )
	{
		QDir().mkpath( m_workingDir );
	}

	if( QDir( m_workingDir ).exists() )
	{
		QDir().mkpath( userProjectsDir() );
		QDir().mkpath( userSamplesDir() );
		QDir().mkpath( userPresetsDir() );
	}
}




void configManager::saveConfigFile()
{
	setValue( "paths", "artwork", m_artworkDir );
	setValue( "paths", "workingdir", m_workingDir );
	setValue( "paths", "vstdir", m_vstDir );
	setValue( "paths", "fldir", m_flDir );
	setValue( "paths", "laddir", m_ladDir );
#ifdef LMMS_HAVE_STK
	setValue( "paths", "stkdir", m_stkDir );
#endif
#ifdef LMMS_HAVE_FLUIDSYNTH
	setValue( "paths", "defaultsf2", m_defaultSoundfont );
#endif
	setValue( "paths", "backgroundartwork", m_backgroundArtwork );

	QDomDocument doc( "lmms-config-file" );

	QDomElement lmms_config = doc.createElement( "lmms" );
	lmms_config.setAttribute( "version", LMMS_VERSION );
	doc.appendChild( lmms_config );

	for( settingsMap::iterator it = m_settings.begin();
						it != m_settings.end(); ++it )
	{
		QDomElement n = doc.createElement( it.key() );
		for( stringPairVector::iterator it2 = ( *it ).begin();
						it2 != ( *it ).end(); ++it2 )
		{
			n.setAttribute( ( *it2 ).first, ( *it2 ).second );
		}
		lmms_config.appendChild( n );
	}

	QDomElement recent_files = doc.createElement( "recentfiles" );

	for( QStringList::iterator it = m_recentlyOpenedProjects.begin();
				it != m_recentlyOpenedProjects.end(); ++it )
	{
		QDomElement n = doc.createElement( "file" );
		n.setAttribute( "path", *it );
		recent_files.appendChild( n );
	}
	lmms_config.appendChild( recent_files );

	QString xml = "<?xml version=\"1.0\"?>\n" + doc.toString( 2 );

	QFile outfile( m_lmmsRcFile );
	if( !outfile.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
	{
		QMessageBox::critical( NULL,
			MainWindow::tr( "Could not save config-file" ),
			MainWindow::tr( "Could not save configuration file %1. "
					"You're probably not permitted to "
					"write to this file.\n"
					"Please make sure you have write-"
					"access to the file and try again." ).
							arg( m_lmmsRcFile  ) );
		return;
	}

	outfile.write( xml.toUtf8() );
	outfile.close();
}


