/*
 * ConfigManager.cpp - implementation of class ConfigManager
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

#include <QDomElement>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QApplication>
#include <QtCore/QTextStream>

#include "ConfigManager.h"
#include "MainWindow.h"
#include "ProjectVersion.h"
#include "GuiApplication.h"


static inline QString ensureTrailingSlash( const QString & s )
{
	if(s.at(s.length()-1) != '/')
	{
		return s + '/';
	}
	return s;
}


ConfigManager * ConfigManager::s_instanceOfMe = NULL;


ConfigManager::ConfigManager() :
	m_lmmsRcFile( QDir::home().absolutePath() +"/.lmmsrc.xml" ),
	m_workingDir( QDir::home().absolutePath() + "/lmms/"),
	m_dataDir( "data:/" ),
	m_artworkDir( defaultArtworkDir() ),
	m_vstDir( m_workingDir + "vst/" ),
	m_flDir( QDir::home().absolutePath() ),
	m_gigDir( m_workingDir + GIG_PATH ),
	m_sf2Dir( m_workingDir + SF2_PATH ),
	m_version( defaultVersion() )
{
	if (! qgetenv("LMMS_DATA_DIR").isEmpty())
		QDir::addSearchPath("data", QString::fromLocal8Bit(qgetenv("LMMS_DATA_DIR")));

	// If we're in development (lmms is not installed) let's get the source
	// directory by reading the CMake Cache
	QFile cmakeCache(qApp->applicationDirPath() + "/CMakeCache.txt");
	if (cmakeCache.exists()) {
		cmakeCache.open(QFile::ReadOnly);
		QTextStream stream(&cmakeCache);

		// Find the line containing something like lmms_SOURCE_DIR:static=<dir>
		while(! stream.atEnd())
		{
			QString line = stream.readLine();

			if (line.startsWith("lmms_SOURCE_DIR:")) {
				QString srcDir = line.section('=', -1).trimmed();
				QDir::addSearchPath("data", srcDir + "/data/");
				break;
			}
		}

		cmakeCache.close();
	}

#ifdef LMMS_BUILD_WIN32
	QDir::addSearchPath("data", qApp->applicationDirPath() + "/data/");
#else
	QDir::addSearchPath("data", qApp->applicationDirPath().section('/', 0, -2) + "/share/lmms/");
#endif


}




ConfigManager::~ConfigManager()
{
	saveConfigFile();
}


void ConfigManager::upgrade_1_1_90()
{
	// Remove trailing " (bad latency!)" string which was once saved with PulseAudio
	if( value( "mixer", "audiodev" ).startsWith( "PulseAudio (" ) )
	{
		setValue("mixer", "audiodev", "PulseAudio");
	}

	// MidiAlsaRaw used to store the device info as "Device" instead of "device"
	if ( value( "MidiAlsaRaw", "device" ).isNull() )
	{
		// copy "device" = "Device" and then delete the old "Device" (further down)
		QString oldDevice = value( "MidiAlsaRaw", "Device" );
		setValue("MidiAlsaRaw", "device", oldDevice);
	}
	if ( !value( "MidiAlsaRaw", "device" ).isNull() )
	{
		// delete the old "Device" in the case that we just copied it to "device"
		//   or if the user somehow set both the "Device" and "device" fields
		deleteValue("MidiAlsaRaw", "Device");
	}
}


void ConfigManager::upgrade()
{
	// Skip the upgrade if versions match
	if ( m_version == LMMS_VERSION )
	{
		return;
	}

	ProjectVersion createdWith = m_version;
	
	if ( createdWith.setCompareType(Build) < "1.1.90" )
	{
		upgrade_1_1_90();
	}
	
	// Don't use old themes as they break the UI (i.e. 0.4 != 1.0, etc)
	if ( createdWith.setCompareType(Minor) != LMMS_VERSION )
	{
		m_artworkDir = defaultArtworkDir();
	}

	// Bump the version, now that we are upgraded
	m_version = LMMS_VERSION;
}

bool ConfigManager::hasWorkingDir() const
{
	return QDir( m_workingDir ).exists();
}


void ConfigManager::setWorkingDir( const QString & _wd )
{
	m_workingDir = ensureTrailingSlash( _wd );
}




void ConfigManager::setVSTDir( const QString & _vd )
{
	m_vstDir = ensureTrailingSlash( _vd );
}




void ConfigManager::setArtworkDir( const QString & _ad )
{
	m_artworkDir = ensureTrailingSlash( _ad );
}




void ConfigManager::setFLDir( const QString & _fd )
{
	m_flDir = ensureTrailingSlash( _fd );
}




void ConfigManager::setLADSPADir( const QString & _fd )
{
	m_ladDir = _fd;
}




void ConfigManager::setSTKDir( const QString & _fd )
{
#ifdef LMMS_HAVE_STK
	m_stkDir = ensureTrailingSlash( _fd );
#endif
}




void ConfigManager::setDefaultSoundfont( const QString & _sf )
{
#ifdef LMMS_HAVE_FLUIDSYNTH
	m_defaultSoundfont = _sf;
#endif
}




void ConfigManager::setBackgroundArtwork( const QString & _ba )
{
#ifdef LMMS_HAVE_FLUIDSYNTH
	m_backgroundArtwork = _ba;
#endif
}

void ConfigManager::setGIGDir(const QString &gd)
{
	m_gigDir = gd;
}

void ConfigManager::setSF2Dir(const QString &sfd)
{
	m_sf2Dir = sfd;
}


void ConfigManager::createWorkingDir()
{
	QDir().mkpath( m_workingDir );

	QDir().mkpath( userProjectsDir() );
	QDir().mkpath( userTemplateDir() );
	QDir().mkpath( userSamplesDir() );
	QDir().mkpath( userPresetsDir() );
	QDir().mkpath( userGigDir() );
	QDir().mkpath( userSf2Dir() );
	QDir().mkpath( userVstDir() );
	QDir().mkpath( userLadspaDir() );
}



void ConfigManager::addRecentlyOpenedProject( const QString & file )
{
	QFileInfo recentFile( file );
	if( recentFile.suffix().toLower() == "mmp" ||
			recentFile.suffix().toLower() == "mmpz" )
	{
		m_recentlyOpenedProjects.removeAll( file );
		if( m_recentlyOpenedProjects.size() > 50 )
		{
			m_recentlyOpenedProjects.removeLast();
		}
		m_recentlyOpenedProjects.push_front( file );
		ConfigManager::inst()->saveConfigFile();
	}
}




const QString & ConfigManager::value( const QString & cls,
					const QString & attribute ) const
{
	if( m_settings.contains( cls ) )
	{
		for( stringPairVector::const_iterator it =
						m_settings[cls].begin();
					it != m_settings[cls].end(); ++it )
		{
			if( ( *it ).first == attribute )
			{
				return ( *it ).second ;
			}
		}
	}
	static QString empty;
	return empty;
}




void ConfigManager::setValue( const QString & cls,
				const QString & attribute,
				const QString & value )
{
	if( m_settings.contains( cls ) )
	{
		for( stringPairVector::iterator it = m_settings[cls].begin();
					it != m_settings[cls].end(); ++it )
		{
			if( ( *it ).first == attribute )
			{
				( *it ).second = value;
				return;
			}
		}
	}
	// not in map yet, so we have to add it...
	m_settings[cls].push_back( qMakePair( attribute, value ) );
}


void ConfigManager::deleteValue( const QString & cls, const QString & attribute)
{
	if( m_settings.contains( cls ) )
	{
		for( stringPairVector::iterator it = m_settings[cls].begin();
					it != m_settings[cls].end(); ++it )
		{
			if( ( *it ).first == attribute )
			{
				m_settings[cls].erase(it);
				return;
			}
		}
	}
}


void ConfigManager::loadConfigFile()
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

			// Cache the config version for upgrade()
			if ( !root.attribute( "version" ).isNull() ) {
				m_version = root.attribute( "version" );
			}

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

			if( value( "paths", "artwork" ) != "" )
			{
				m_artworkDir = value( "paths", "artwork" );
				if( !QDir( m_artworkDir ).exists() )
				{
					m_artworkDir = defaultArtworkDir();
				}
				m_artworkDir = ensureTrailingSlash(m_artworkDir);
			}
			setWorkingDir( value( "paths", "workingdir" ) );

			setGIGDir( value( "paths", "gigdir" ) == "" ? gigDir() : value( "paths", "gigdir" ) );
			setSF2Dir( value( "paths", "sf2dir" ) == "" ? sf2Dir() : value( "paths", "sf2dir" ) );
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
		else if( gui )
		{
			QMessageBox::warning( NULL, MainWindow::tr( "Configuration file" ),
									MainWindow::tr( "Error while parsing configuration file at line %1:%2: %3" ).
													arg( errorLine ).
													arg( errorCol ).
													arg( errorString ) );
		}
		cfg_file.close();
	}


	if( m_vstDir.isEmpty() || m_vstDir == QDir::separator() || m_vstDir == "/" ||
			!QDir( m_vstDir ).exists() )
	{
#ifdef LMMS_BUILD_WIN32
		QString programFiles = QString::fromLocal8Bit( getenv( "ProgramFiles" ) );
		m_vstDir =  programFiles + "/VstPlugins/";
#else
		m_vstDir =  m_workingDir + "plugins/vst/";
#endif
	}

	if( m_flDir.isEmpty() || m_flDir == QDir::separator() || m_flDir == "/")
	{
		m_flDir = ensureTrailingSlash( QDir::home().absolutePath() );
	}

	if( m_ladDir.isEmpty()  )
	{
		m_ladDir = userLadspaDir();
	}

#ifdef LMMS_HAVE_STK
	if( m_stkDir.isEmpty() || m_stkDir == QDir::separator() || m_stkDir == "/" ||
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

	QStringList searchPaths;
	if(! qgetenv("LMMS_THEME_PATH").isNull())
		searchPaths << qgetenv("LMMS_THEME_PATH");
	searchPaths << artworkDir() << defaultArtworkDir();
	QDir::setSearchPaths( "resources", searchPaths);

	// Create any missing subdirectories in the working dir, but only if the working dir exists
	if( hasWorkingDir() )
	{
		createWorkingDir();
	}

	upgrade();
}




void ConfigManager::saveConfigFile()
{
	setValue( "paths", "artwork", m_artworkDir );
	setValue( "paths", "workingdir", m_workingDir );
	setValue( "paths", "vstdir", m_vstDir );
	setValue( "paths", "fldir", m_flDir );
	setValue( "paths", "gigdir", m_gigDir );
	setValue( "paths", "sf2dir", m_sf2Dir );
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
	lmms_config.setAttribute( "version", m_version );
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


