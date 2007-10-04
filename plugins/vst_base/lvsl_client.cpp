/*
 * lvsl_client.cpp - client for LVSL Server
 *
 * Copyright (c) 2005-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtCore/QLocale>
#include <QtCore/QTime>
#include <QtCore/QDir>
#include <QtGui/QX11EmbedContainer>
#include <QtGui/QX11Info>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#include <cstdio>

#ifdef HAVE_SYS_IPC_H
#include <sys/ipc.h>
#endif

#ifdef HAVE_SYS_SHM_H
#include <sys/shm.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif


#include "templates.h"
#include "config_mgr.h"
#include "engine.h"
#include "main_window.h"
#include "lvsl_client.h"




remoteVSTPlugin::remoteVSTPlugin( const QString & _plugin ) :
	QObject(),
	m_failed( TRUE ),
	m_plugin( _plugin ),
	m_pluginWidget( NULL ),
	m_pluginXID( 0 ),
	m_pluginPID( -1 ),
	m_serverInFD( -1 ),
	m_serverOutFD( -1 ),
	m_serverMutex(),
	m_name( "" ),
	m_version( 0 ),
	m_vendorString( "" ),
	m_productString( "" ),
	m_inputCount( 0 ),
	m_outputCount( 0 ),
	m_shmID( -1 ),
	m_shm( NULL ),
	m_shmSize( 0 ),
	m_initialized( FALSE )
{
	pipe( m_pipes[0] );
	pipe( m_pipes[1] );

	if( ( m_pluginPID = fork() ) < 0 )
	{
		printf( "fork() failed!\n" );
		return;
	}
	else if( m_pluginPID == 0 )
	{
		dup2( m_pipes[0][0], 0 );
		dup2( m_pipes[1][1], 1 );
		QString lvsl_server_exec = configManager::inst()->pluginDir() +
							QDir::separator() +
								"lvsl_server";
		execlp( lvsl_server_exec.toAscii().constData(),
					lvsl_server_exec.toAscii().constData(),
								"", NULL );
		return;
	}
	m_serverInFD = m_pipes[1][0];
	m_serverOutFD = m_pipes[0][1];

	lock();

	writeValueS<Sint16>( VST_LANGUAGE );
	hostLanguages hlang = LanguageEnglish;
	switch( QLocale::system().language() )
	{
		case QLocale::German: hlang = LanguageGerman; break;
		case QLocale::French: hlang = LanguageFrench; break;
		case QLocale::Italian: hlang = LanguageItalian; break;
		case QLocale::Spanish: hlang = LanguageSpanish; break;
		case QLocale::Japanese: hlang = LanguageJapanese; break;
		default: break;
	}
	writeValueS<hostLanguages>( hlang );

	QString p = m_plugin;
	if( QFileInfo( p ).dir().isRelative() )
	{
		p = configManager::inst()->vstDir() + QDir::separator() + p;
	}

	writeValueS<Sint16>( VST_LOAD_PLUGIN );
	writeStringS( p.toAscii().constData() );
	unlock();

	while( 1 )
	{
		Sint16 cmd = VST_UNDEFINED_CMD;
		if( messagesLeft() == TRUE )
		{
			cmd = processNextMessage();
		}
		if( cmd == VST_INITIALIZATION_DONE )
		{
			m_failed = FALSE;
			break;
		}
		else if( cmd == VST_FAILED_LOADING_PLUGIN )
		{
			break;
		}
		QCoreApplication::processEvents( QEventLoop::AllEvents, 50 );
	}
}




remoteVSTPlugin::~remoteVSTPlugin()
{
	if( m_failed == FALSE )
	{
		setShmKeyAndSize( 0, 0 );
		// tell server to quit and wait for acknowledge
		writeValueS<Sint16>( VST_CLOSE_PLUGIN );
		QTime t;
		t.start();
		while( t.elapsed() < 1000 )
		{
			if( messagesLeft() == TRUE &&
					processNextMessage() == VST_QUIT_ACK )
			{
				//m_pluginPID = 0;
				break;
			}
		}
		if( m_pluginWidget != NULL )
		{
			m_pluginWidget->hide();
			delete m_pluginWidget;
		}
		// timeout?
/*		if( m_pluginPID != 0 )
		{*/
			kill( m_pluginPID, SIGTERM );
		//}

		// close all sides of our pipes
		close( m_pipes[0][0] );
		close( m_pipes[0][1] );
		close( m_pipes[1][0] );
		close( m_pipes[1][1] );
/*		close( m_serverInFD );
		close( m_serverOutFD );*/
	}
}




void remoteVSTPlugin::showEditor( void )
{
	if( m_pluginWidget != NULL )
	{
		m_pluginWidget->show();
		return;
	}

	if( m_pluginXID == 0 )
	{
		return;
	}

	m_pluginWidget = new QWidget( engine::getMainWindow()->workspace() );
	m_pluginWidget->setFixedSize( m_pluginGeometry );
	m_pluginWidget->setWindowTitle( name() );
	engine::getMainWindow()->workspace()->addWindow( m_pluginWidget );
	m_pluginWidget->show();

	QX11EmbedContainer * xe = new QX11EmbedContainer( m_pluginWidget );
	xe->embedClient( m_pluginXID );
	xe->setFixedSize( m_pluginGeometry );
	//xe->setAutoDelete( FALSE );
	xe->show();

	lock();
	writeValueS<Sint16>( VST_SHOW_EDITOR );
	unlock();
}




void remoteVSTPlugin::hideEditor( void )
{
	if( m_pluginWidget != NULL )
	{
		m_pluginWidget->hide();
	}
}




bool remoteVSTPlugin::process( const sampleFrame * _in_buf,
					sampleFrame * _out_buf, bool _wait )
{
	const fpp_t frames = engine::getMixer()->framesPerPeriod();

	if( m_shm == NULL )
	{
		// m_shm being zero means we didn't initialize everything so
		// far so process one message each time (and hope we get
		// information like SHM-key etc.) until we process messages
		// in a later stage of this procedure
		if( m_shmSize == 0 && messagesLeft() == TRUE )
		{
			(void) processNextMessage();
		}
		if( _out_buf != NULL )
		{
			engine::getMixer()->clearAudioBuffer( _out_buf,
								frames );
		}
		return( FALSE );
	}

	memset( m_shm, 0, m_shmSize );

	ch_cnt_t inputs = tMax<ch_cnt_t>( m_inputCount, DEFAULT_CHANNELS );

	if( _in_buf != NULL && inputs > 0 )
	{
		for( ch_cnt_t ch = 0; ch < inputs; ++ch )
		{
			for( fpp_t frame = 0; frame < frames; ++frame )
			{
				m_shm[ch * frames + frame] = _in_buf[frame][ch];
			}
		}
	}

	lock();
	writeValueS<Sint16>( VST_PROCESS );
	unlock();

	m_initialized = TRUE;
	if( _wait )
	{
		waitForProcessingFinished( _out_buf );
	}
	return( TRUE );
}




bool remoteVSTPlugin::waitForProcessingFinished( sampleFrame * _out_buf )
{
	if( !m_initialized || _out_buf == NULL || m_outputCount == 0 )
	{
		return( FALSE );
	}

	// wait until server signals that process()ing is done
	while( processNextMessage() != VST_PROCESS_DONE )
	{
		// hopefully scheduler gives process-time to plugin...
		usleep( 10 );
	}

	const fpp_t frames = engine::getMixer()->framesPerPeriod();
	const ch_cnt_t outputs = tMax<ch_cnt_t>( m_outputCount,
							DEFAULT_CHANNELS );
	if( outputs != DEFAULT_CHANNELS )
	{
		// clear buffer, if plugin didn't fill up both channels
		engine::getMixer()->clearAudioBuffer( _out_buf, frames );
	}

	for( ch_cnt_t ch = 0; ch < outputs; ++ch )
	{
		for( fpp_t frame = 0; frame < frames; ++frame )
		{
			_out_buf[frame][ch] = m_shm[( m_inputCount + ch ) *
								frames + frame];
		}
	}

	return( TRUE );
}




void remoteVSTPlugin::enqueueMidiEvent( const midiEvent & _event,
						const f_cnt_t _frames_ahead )
{
	lock();
	writeValueS<Sint16>( VST_ENQUEUE_MIDI_EVENT );

	writeValueS<midiEventTypes>( _event.m_type );
	writeValueS<Sint8>( _event.m_channel );
	writeValueS<Uint16>( _event.m_data.m_param[0] );
	writeValueS<Uint16>( _event.m_data.m_param[1] );

	writeValueS<f_cnt_t>( _frames_ahead );
	unlock();
}




void remoteVSTPlugin::loadSettings( const QDomElement & _this )
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




void remoteVSTPlugin::saveSettings( QDomDocument & _doc, QDomElement & _this )
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




void remoteVSTPlugin::setTempo( bpm_t _bpm )
{
	lock();
	writeValueS<Sint16>( VST_BPM );
	writeValueS<bpm_t>( _bpm );
	unlock();
}




void remoteVSTPlugin::updateSampleRate( void )
{
	lock();
	writeValueS<Sint16>( VST_SAMPLE_RATE );
	writeValueS<sample_rate_t>( engine::getMixer()->sampleRate() );
	unlock();
}




const QMap<QString, QString> & remoteVSTPlugin::parameterDump( void )
{
	writeValueS<Sint16>( VST_GET_PARAMETER_DUMP );

	while( processNextMessage() != VST_PARAMETER_DUMP )
	{
	}

	return( m_parameterDump );
}




void remoteVSTPlugin::setParameterDump( const QMap<QString, QString> & _pdump )
{
	writeValueS<Sint16>( VST_SET_PARAMETER_DUMP );
	writeValueS<Sint32>( _pdump.size() );
	for( QMap<QString, QString>::const_iterator it = _pdump.begin();
						it != _pdump.end(); ++it )
	{
		const vstParameterDumpItem dump_item =
		{
			( *it ).section( ':', 0, 0 ).toInt(),
			"",
			( *it ).section( ':', 1, 1 ).toFloat()
		} ;
		writeValueS<vstParameterDumpItem>( dump_item );
	}
}




void remoteVSTPlugin::setShmKeyAndSize( Uint16 _key, size_t _size )
{
	if( m_shm != NULL && m_shmSize > 0 )
	{
		shmdt( m_shm );
		m_shm = NULL;
		m_shmSize = 0;
	}

	// only called for detaching SHM?
	if( _size == 0 )
	{
		return;
	}

	int shm_id = shmget( _key, _size, 0 );
	if( shm_id == -1 )
	{
		printf( "failed getting shared memory\n" );
	}
	else
	{
		m_shm = (float *) shmat( shm_id, 0, 0 );
		// TODO: error-checking
	}
}




bool remoteVSTPlugin::messagesLeft( void ) const
{
	fd_set rfds;
	FD_ZERO( &rfds );
	FD_SET( m_serverInFD, &rfds );
	timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 1;	// can we use 0 here?
	return( select( m_serverInFD + 1, &rfds, NULL, NULL, &tv ) > 0 );
}




Sint16 remoteVSTPlugin::processNextMessage( void )
{
	fd_set rfds;
	FD_ZERO( &rfds );
	FD_SET( m_serverInFD, &rfds );
	if( select( m_serverInFD + 1, &rfds, NULL, NULL, NULL ) <= 0 )
	{
		return( VST_UNDEFINED_CMD );
	}

	lock();
	Sint16 cmd = readValueS<Sint16>();
	switch( cmd )
	{
		case VST_DEBUG_MSG:
			printf( "debug message from server: %s\n",
						readStringS().c_str() );
			break;

		case VST_GET_SAMPLE_RATE:
			writeValueS<Sint16>( VST_SAMPLE_RATE );
			// handle is the same
			writeValueS<sample_rate_t>(
					engine::getMixer()->sampleRate() );
			break;

		case VST_GET_BUFFER_SIZE:
			writeValueS<Sint16>( VST_BUFFER_SIZE );
			// handle is the same
			writeValueS<fpp_t>(
				engine::getMixer()->framesPerPeriod() );
			break;

		case VST_SHM_KEY_AND_SIZE:
		{
			Uint16 shm_key = readValueS<Uint16>();
			size_t shm_size = readValueS<Uint32>();
			setShmKeyAndSize( shm_key, shm_size );
			break;
		}

		case VST_INPUT_COUNT:
			m_inputCount = readValueS<ch_cnt_t>();
			break;

		case VST_OUTPUT_COUNT:
			m_outputCount = readValueS<ch_cnt_t>();
			break;

		case VST_PLUGIN_XID:
			m_pluginXID = readValueS<Sint32>();
			break;

		case VST_PLUGIN_EDITOR_GEOMETRY:
		{
			const Sint16 w = readValueS<Sint16>();
			const Sint16 h = readValueS<Sint16>();
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

		case VST_PLUGIN_NAME:
			m_name = readStringS().c_str();
			break;

		case VST_PLUGIN_VERSION:
			m_version = readValueS<Sint32>();
			break;

		case VST_PLUGIN_VENDOR_STRING:
			m_vendorString = readStringS().c_str();
			break;

		case VST_PLUGIN_PRODUCT_STRING:
			m_productString = readStringS().c_str();
			break;

		case VST_PARAMETER_DUMP:
		{
			m_parameterDump.clear();
			const Sint32 num_params = readValueS<Sint32>();
			for( Sint32 i = 0; i < num_params; ++i )
			{
				vstParameterDumpItem dump_item =
					readValueS<vstParameterDumpItem>();
	m_parameterDump["param" + QString::number( dump_item.index )] =
				QString::number( dump_item.index ) + ":" +
//					QString( dump_item.shortLabel ) + ":" +
					QString::number( dump_item.value );
			}
			break;
		}

		case VST_PROCESS_DONE:
		case VST_QUIT_ACK:
		case VST_UNDEFINED_CMD:
		default:
			break;
	}
	unlock();

	return( cmd );
}



#include "lvsl_client.moc"

