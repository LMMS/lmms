/*
 * VstPlugin.cpp - implementation of VstPlugin class
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "VstPlugin.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QLocale>
#include <QtCore/QTemporaryFile>
#include <QtGui/QCloseEvent>
#include <QtGui/QMdiArea>
#include <QtGui/QMdiSubWindow>
#ifdef LMMS_BUILD_LINUX
#include <QtGui/QX11EmbedContainer>
#include <QtGui/QX11Info>
#else
#include <QtGui/QLayout>
#endif
#include <QtXml/QDomDocument>

#ifdef LMMS_BUILD_WIN32
#include <windows.h>
#endif

#include "config_mgr.h"
#include "engine.h"
#include "MainWindow.h"
#include "song.h"
#include "templates.h"


class vstSubWin : public QMdiSubWindow
{
public:
	vstSubWin( QWidget * _parent ) :
		QMdiSubWindow( _parent )
	{
		setAttribute( Qt::WA_DeleteOnClose, false );
	}

	virtual ~vstSubWin()
	{
	}

	virtual void closeEvent( QCloseEvent * e )
	{
		// ignore close-events - for some reason otherwise the VST GUI
		// remains hidden when re-opening
		hide();
		e->ignore();
	}
} ;




VstPlugin::VstPlugin( const QString & _plugin ) :
	QObject(),
	JournallingObject(),
	RemotePlugin( "remote_vst_plugin", false ),
	m_plugin( _plugin ),
	m_pluginWidget( NULL ),
	m_pluginWindowID( 0 ),
	m_name(),
	m_version( 0 ),
	m_vendorString(),
	m_productString()
{
	setSplittedChannels( true );

	lock();
#ifdef LMMS_BUILD_WIN32
	QWidget * helper = new QWidget;
	QHBoxLayout * l = new QHBoxLayout( helper );
	QWidget * target = new QWidget( helper );
	l->setSpacing( 0 );
	l->setMargin( 0 );
	l->addWidget( target );

	static int k = 0;
	const QString t = QString( "vst%1%2" ).arg( GetCurrentProcessId()<<10 ).
								arg( ++k );
	helper->setWindowTitle( t );

	// we've to call that for making sure, Qt created the windows
	(void) helper->winId();
	(void) target->winId();

	sendMessage( message( IdVstPluginWindowInformation ).
					addString( QSTR_TO_STDSTR( t ) ) );
#endif

	VstHostLanguages hlang = LanguageEnglish;
	switch( QLocale::system().language() )
	{
		case QLocale::French: hlang = LanguageFrench; break;
		case QLocale::German: hlang = LanguageGerman; break;
		case QLocale::Italian: hlang = LanguageItalian; break;
		case QLocale::Japanese: hlang = LanguageJapanese; break;
		case QLocale::Korean: hlang = LanguageKorean; break;
		case QLocale::Spanish: hlang = LanguageSpanish; break;
		default: break;
	}
	sendMessage( message( IdVstSetLanguage ).addInt( hlang ) );


	QString p = m_plugin;
	if( QFileInfo( p ).dir().isRelative() )
	{
		p = configManager::inst()->vstDir() + QDir::separator() + p;
	}

	sendMessage( message( IdVstLoadPlugin ).addString( QSTR_TO_STDSTR( p ) ) );

	waitForInitDone();

	unlock();

#ifdef LMMS_BUILD_WIN32
	if( m_pluginWindowID )
	{
		target->setFixedSize( m_pluginGeometry );
		vstSubWin * sw = new vstSubWin(
					engine::mainWindow()->workspace() );
		sw->setWidget( helper );
		helper->setWindowTitle( name() );
		m_pluginWidget = helper;
	}
	else
	{
		delete helper;
	}
#endif

	setTempo( engine::getSong()->getTempo() );

	connect( engine::getSong(), SIGNAL( tempoChanged( bpm_t ) ),
			this, SLOT( setTempo( bpm_t ) ) );
	connect( engine::getMixer(), SIGNAL( sampleRateChanged() ),
				this, SLOT( updateSampleRate() ) );
}




VstPlugin::~VstPlugin()
{
}




void VstPlugin::showEditor( QWidget * _parent )
{
	QWidget * w = pluginWidget();
	if( w )
	{
		w->show();
		return;
	}

#ifdef LMMS_BUILD_LINUX
	if( m_pluginWindowID == 0 )
	{
		return;
	}

	m_pluginWidget = new QWidget( _parent );
	m_pluginWidget->setFixedSize( m_pluginGeometry );
	m_pluginWidget->setWindowTitle( name() );
	if( _parent == NULL )
	{
		vstSubWin * sw = new vstSubWin(
					engine::mainWindow()->workspace() );
		sw->setWidget( m_pluginWidget );
	}

	QX11EmbedContainer * xe = new QX11EmbedContainer( m_pluginWidget );
	xe->embedClient( m_pluginWindowID );
	xe->setFixedSize( m_pluginGeometry );
	xe->show();
#endif

	if( m_pluginWidget )
	{
		m_pluginWidget->show();
	}
}




void VstPlugin::hideEditor()
{
	QWidget * w = pluginWidget();
	if( w )
	{
		w->hide();
	}
}




void VstPlugin::loadSettings( const QDomElement & _this )
{
	if( pluginWidget() != NULL )
	{
		if( _this.attribute( "guivisible" ).toInt() )
		{
			showEditor();
		}
		else
		{
			hideEditor();
		}
	}

	const int num_params = _this.attribute( "numparams" ).toInt();
	// if it exists try to load settings chunk
	if( _this.hasAttribute( "chunk" ) )
	{
		loadChunk( QByteArray::fromBase64(
				_this.attribute( "chunk" ).toUtf8() ) );
	}
	else if( num_params > 0 )
	{
		// no chunk, restore individual parameters
		QMap<QString, QString> dump;
		for( int i = 0; i < num_params; ++i )
		{
			const QString key = "param" +
						QString::number( i );
			dump[key] = _this.attribute( key );
		}
		setParameterDump( dump );
	}

}




void VstPlugin::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	if( pluginWidget() != NULL )
	{
		_this.setAttribute( "guivisible", pluginWidget()->isVisible() );
	}

	// try to save all settings in a chunk
	QByteArray chunk = saveChunk();
	if( !chunk.isEmpty() )
	{
		_this.setAttribute( "chunk", QString( chunk.toBase64() ) );
	}
	else
	{
		// plugin doesn't seem to support chunks, therefore save
		// individual parameters
		const QMap<QString, QString> & dump = parameterDump();
		_this.setAttribute( "numparams", dump.size() );
		for( QMap<QString, QString>::const_iterator it = dump.begin();
							it != dump.end(); ++it )
		{
			_this.setAttribute( it.key(), it.value() );
		}
	}
}




void VstPlugin::setTempo( bpm_t _bpm )
{
	lock();
	sendMessage( message( IdVstSetTempo ).addInt( _bpm ) );
	unlock();
}




void VstPlugin::updateSampleRate()
{
	lock();
	sendMessage( message( IdSampleRateInformation ).
			addInt( engine::getMixer()->processingSampleRate() ) );
	unlock();
}




const QMap<QString, QString> & VstPlugin::parameterDump()
{
	lock();
	sendMessage( IdVstGetParameterDump );
	waitForMessage( IdVstParameterDump );
	unlock();

	return m_parameterDump;
}




void VstPlugin::setParameterDump( const QMap<QString, QString> & _pdump )
{
	message m( IdVstSetParameterDump );
	m.addInt( _pdump.size() );
	for( QMap<QString, QString>::ConstIterator it = _pdump.begin();
						it != _pdump.end(); ++it )
	{
		const VstParameterDumpItem item =
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




bool VstPlugin::processMessage( const message & _m )
{
	switch( _m.id )
	{
		case IdVstPluginWindowID:
			m_pluginWindowID = _m.getInt();
			break;

		case IdVstPluginEditorGeometry:
			m_pluginGeometry = QSize( _m.getInt( 0 ),
							_m.getInt( 1 ) );
			break;

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

		case IdVstPluginUniqueID:
			// TODO: display graphically in case of failure
			printf("unique ID: %s\n", _m.getString().c_str() );
			break;

		case IdVstParameterDump:
		{
			m_parameterDump.clear();
			const int num_params = _m.getInt();
			int p = 0;
			for( int i = 0; i < num_params; ++i )
			{
				VstParameterDumpItem item;
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
			return RemotePlugin::processMessage( _m );
	}
	return true;

}




void VstPlugin::loadChunk( const QByteArray & _chunk )
{
	QTemporaryFile tf;
	if( tf.open() )
	{
		tf.write( _chunk );
		lock();
		sendMessage( message( IdLoadSettingsFromFile ).
				addString(
					QSTR_TO_STDSTR(
						QDir::toNativeSeparators( tf.fileName() ) ) ).
				addInt( _chunk.size() ) );
		waitForMessage( IdLoadSettingsFromFile );
		unlock();
	}
}




QByteArray VstPlugin::saveChunk()
{
	QByteArray a;
	QTemporaryFile tf;
	if( tf.open() )
	{
		lock();
		sendMessage( message( IdSaveSettingsToFile ).
				addString(
					QSTR_TO_STDSTR(
						QDir::toNativeSeparators( tf.fileName() ) ) ) );
		waitForMessage( IdSaveSettingsToFile );
		unlock();
		a = tf.readAll();
	}

	return a;
}


#include "moc_VstPlugin.cxx"

