/*
 * VstPlugin.cpp - implementation of VstPlugin class
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

#include "VstPlugin.h"

#include <QtCore/QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLocale>
#include <QTemporaryFile>
#include <QCloseEvent>
#include <QMdiArea>
#include <QMdiSubWindow>

#ifdef LMMS_BUILD_LINUX
#	if QT_VERSION < 0x050000
#		include <QX11EmbedContainer>
#		include <QX11Info>
#	else
#		include "X11EmbedContainer.h"
#		include <QWindow>
#	endif
#endif

#if QT_VERSION >= 0x050000
#	include <QWindow>
#endif

#include <QDomDocument>

#ifdef LMMS_BUILD_WIN32
#	include <windows.h>
#	include <QLayout>
#endif

#include "ConfigManager.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "Mixer.h"
#include "Song.h"
#include "templates.h"
#include "FileDialog.h"

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
	m_plugin( _plugin ),
	m_pluginWindowID( 0 ),
	m_embedMethod( ConfigManager::inst()->vstEmbedMethod() ),
	m_badDllFormat( false ),
	m_version( 0 ),
	m_currentProgram()
{
	setSplittedChannels( true );

	tryLoad( REMOTE_VST_PLUGIN_FILEPATH );
#ifdef LMMS_BUILD_WIN64
	if( m_badDllFormat )
	{
		m_badDllFormat = false;
		tryLoad( "32/RemoteVstPlugin32" );
	}
#endif

	setTempo( Engine::getSong()->getTempo() );

	connect( Engine::getSong(), SIGNAL( tempoChanged( bpm_t ) ),
			this, SLOT( setTempo( bpm_t ) ) );
	connect( Engine::mixer(), SIGNAL( sampleRateChanged() ),
				this, SLOT( updateSampleRate() ) );

	// update once per second
	m_idleTimer.start( 1000 );
	connect( &m_idleTimer, SIGNAL( timeout() ),
				this, SLOT( idleUpdate() ) );
}




VstPlugin::~VstPlugin()
{
	delete m_pluginSubWindow;
	delete m_pluginWidget;
}




void VstPlugin::tryLoad( const QString &remoteVstPluginExecutable )
{
	init( remoteVstPluginExecutable, false, {m_embedMethod} );

	waitForHostInfoGotten();
	if( failed() )
	{
		return;
	}

	lock();

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
			p = ConfigManager::inst()->vstDir()  + p;
		}


	sendMessage( message( IdVstLoadPlugin ).addString( QSTR_TO_STDSTR( p ) ) );

	waitForInitDone();

	unlock();
}




void VstPlugin::hideEditor()
{
	QWidget * w = pluginWidget();
	if( w )
	{
		w->hide();
	}
}




void VstPlugin::toggleEditor()
{
	QWidget * w = pluginWidget();
	if( w )
	{
		w->setVisible( !w->isVisible() );
	}
}




void VstPlugin::loadSettings( const QDomElement & _this )
{
	if( _this.attribute( "guivisible" ).toInt() )
	{
		showUI();
	}
	else
	{
		hideUI();
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

	if( _this.hasAttribute( "program" ) )
	{
		setProgram( _this.attribute( "program" ).toInt() );
	}
}




void VstPlugin::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	if ( m_embedMethod != "none" )
	{
		if( pluginWidget() != NULL )
		{
			_this.setAttribute( "guivisible", pluginWidget()->isVisible() );
		}
	}
	else
	{
		int visible = isUIVisible();
		if ( visible != -1 )
		{
			_this.setAttribute( "guivisible", visible );
		}
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

	_this.setAttribute( "program", currentProgram() );
}

void VstPlugin::toggleUI()
{
	if ( m_embedMethod == "none" )
	{
		RemotePlugin::toggleUI();
	}
	else if (pluginWidget())
	{
		toggleEditor();
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
			addInt( Engine::mixer()->processingSampleRate() ) );
	unlock();
}




int VstPlugin::currentProgram()
{
	lock();
	sendMessage( message( IdVstCurrentProgram ) );
	waitForMessage( IdVstCurrentProgram, true );
	unlock();

	return m_currentProgram;
}



const QMap<QString, QString> & VstPlugin::parameterDump()
{
	lock();
	sendMessage( IdVstGetParameterDump );
	waitForMessage( IdVstParameterDump, true );
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
			( *it ).section( ':', 2, -1 ).toFloat()
		} ;
		m.addInt( item.index );
		m.addString( item.shortLabel );
		m.addFloat( item.value );
	}
	lock();
	sendMessage( m );
	unlock();
}

QWidget *VstPlugin::pluginWidget(bool _top_widget)
{
	if ( m_embedMethod != "none" )
	{
		if( _top_widget && m_pluginWidget )
		{
			if( m_pluginWidget->parentWidget() )
			{
				return m_pluginWidget->parentWidget();
			}
		}
	}
	return m_pluginWidget;
}




bool VstPlugin::processMessage( const message & _m )
{
	switch( _m.id )
	{
	case IdVstBadDllFormat:
		m_badDllFormat = true;
		break;

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

		case IdVstCurrentProgram:
			m_currentProgram = _m.getInt();
			break;

		case IdVstCurrentProgramName:
			m_currentProgramName = _m.getQString();
			break;

		case IdVstProgramNames:
			m_allProgramNames = _m.getQString();
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
/*uncomented*/				/*QString( item.shortLabel )*/ QString::fromStdString(item.shortLabel) + ":" +
					QString::number( item.value );
			}
			break;
		}
		default:
			return RemotePlugin::processMessage( _m );
	}
	return true;

}




void VstPlugin::openPreset( )
{

	FileDialog ofd( NULL, tr( "Open Preset" ), "",
		tr( "Vst Plugin Preset (*.fxp *.fxb)" ) );
	ofd.setFileMode( FileDialog::ExistingFiles );
	if( ofd.exec () == QDialog::Accepted &&
					!ofd.selectedFiles().isEmpty() )
	{
		lock();
		sendMessage( message( IdLoadPresetFile ).
			addString(
				QSTR_TO_STDSTR(
					QDir::toNativeSeparators( ofd.selectedFiles()[0] ) ) )
			);
		waitForMessage( IdLoadPresetFile, true );
		unlock();
	}
}




void VstPlugin::setProgram( int index )
{
	lock();
	sendMessage( message( IdVstSetProgram ).addInt( index ) );
	waitForMessage( IdVstSetProgram, true );
	unlock();
}




void VstPlugin::rotateProgram( int offset )
{
	lock();
	sendMessage( message( IdVstRotateProgram ).addInt( offset ) );
	waitForMessage( IdVstRotateProgram, true );
	unlock();
}




void VstPlugin::loadProgramNames()
{
	lock();
	sendMessage( message( IdVstProgramNames ) );
	waitForMessage( IdVstProgramNames, true );
	unlock();
}




void VstPlugin::savePreset( )
{
	QString presName = currentProgramName().isEmpty() ? tr(": default") : currentProgramName();
	presName.replace(tr("\""), tr("'")); // QFileDialog unable to handle double quotes properly

	FileDialog sfd( NULL, tr( "Save Preset" ), presName.section(": ", 1, 1) + tr(".fxp"),
		tr( "Vst Plugin Preset (*.fxp *.fxb)" ) );

	if( p_name != "" ) // remember last directory
	{
		sfd.setDirectory( QFileInfo( p_name ).absolutePath() );
	}

	sfd.setAcceptMode( FileDialog::AcceptSave );
	sfd.setFileMode( FileDialog::AnyFile );
	if( sfd.exec () == QDialog::Accepted &&
				!sfd.selectedFiles().isEmpty() && sfd.selectedFiles()[0] != "" )
	{
		QString fns = sfd.selectedFiles()[0];
		p_name = fns;

		if ((fns.toUpper().indexOf(tr(".FXP")) == -1) && (fns.toUpper().indexOf(tr(".FXB")) == -1))
			fns = fns + tr(".fxb");
		else fns = fns.left(fns.length() - 4) + (fns.right( 4 )).toLower();
		lock();
		sendMessage( message( IdSavePresetFile ).
			addString(
				QSTR_TO_STDSTR(
					QDir::toNativeSeparators( fns ) ) )
			);
		waitForMessage( IdSavePresetFile, true );
		unlock();
	}
}




void VstPlugin::setParam( int i, float f )
{
	lock();
	sendMessage( message( IdVstSetParameter ).addInt( i ).addFloat( f ) );
	//waitForMessage( IdVstSetParameter, true );
	unlock();
}



void VstPlugin::idleUpdate()
{
	lock();
	sendMessage( message( IdVstIdleUpdate ) );
	unlock();
}

void VstPlugin::showUI()
{
	if ( m_embedMethod == "none" )
	{
		RemotePlugin::showUI();
	}
	else
	{
		if (! pluginWidget()) {
			createUI( NULL, false );
		}

		QWidget * w = pluginWidget();
		if( w )
		{
			w->show();
		}
	}
}

void VstPlugin::hideUI()
{
	if ( m_embedMethod == "none" )
	{
		RemotePlugin::hideUI();
	}
	else if ( pluginWidget() != nullptr )
	{
		hideEditor();
	}
}

// X11Embed only
void VstPlugin::handleClientEmbed()
{
	lock();
	sendMessage( IdShowUI );
	unlock();
}



void VstPlugin::loadChunk( const QByteArray & _chunk )
{
	QTemporaryFile tf;
	if( tf.open() )
	{
		tf.write( _chunk );
		tf.flush();

		lock();
		sendMessage( message( IdLoadSettingsFromFile ).
				addString(
					QSTR_TO_STDSTR(
						QDir::toNativeSeparators( tf.fileName() ) ) ).
				addInt( _chunk.size() ) );
		waitForMessage( IdLoadSettingsFromFile, true );
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
		waitForMessage( IdSaveSettingsToFile, true );
		unlock();
		a = tf.readAll();
	}

	return a;
}

void VstPlugin::createUI( QWidget * parent, bool isEffect )
{
	if( m_pluginWindowID == 0 )
	{
		return;
	}

	QWidget* container = nullptr;
	m_pluginSubWindow = new vstSubWin( gui->mainWindow()->workspace() );
	auto sw = m_pluginSubWindow.data();

#if QT_VERSION >= 0x050100
	if (m_embedMethod == "qt" )
	{
		QWindow* vw = QWindow::fromWinId(m_pluginWindowID);
		container = QWidget::createWindowContainer(vw, sw );
		RemotePlugin::showUI();
		// TODO: Synchronize show
		// Tell remote that it is embedded
		// Wait for remote reply
	} else
#endif

#ifdef LMMS_BUILD_WIN32
	if (m_embedMethod == "win32" )
	{
		QWidget * helper = new QWidget;
		QHBoxLayout * l = new QHBoxLayout( helper );
		QWidget * target = new QWidget( helper );
		l->setSpacing( 0 );
		l->setMargin( 0 );
		l->addWidget( target );

		// we've to call that for making sure, Qt created the windows
		helper->winId();
		HWND targetHandle = (HWND)target->winId();
		HWND pluginHandle = (HWND)(intptr_t)m_pluginWindowID;

		DWORD style = GetWindowLong(pluginHandle, GWL_STYLE);
		style = style & ~(WS_POPUP);
		style = style | WS_CHILD;
		SetWindowLong(pluginHandle, GWL_STYLE, style);
		SetParent(pluginHandle, targetHandle);

		DWORD threadId = GetWindowThreadProcessId(pluginHandle, NULL);
		DWORD currentThreadId = GetCurrentThreadId();
		AttachThreadInput(currentThreadId, threadId, true);

		container = helper;
		RemotePlugin::showUI();

	} else
#endif

#ifdef LMMS_BUILD_LINUX
	if (m_embedMethod == "xembed" )
	{
		QX11EmbedContainer * embedContainer = new QX11EmbedContainer( sw );
		connect(embedContainer, SIGNAL(clientIsEmbedded()), this, SLOT(handleClientEmbed()));
		embedContainer->embedClient( m_pluginWindowID );
		container = embedContainer;
	} else
#endif
	{
		qCritical() << "Unknown embed method" << m_embedMethod;
		delete m_pluginSubWindow;
		return;
	}

	container->setFixedSize( m_pluginGeometry );
	container->setWindowTitle( name() );

	if( parent == NULL )
	{
		m_pluginWidget = container;

		sw->setWidget(container);

		if( isEffect )
		{
			sw->setAttribute( Qt::WA_TranslucentBackground );
			sw->setWindowFlags( Qt::FramelessWindowHint );
		}
		else
		{
			sw->setWindowFlags( Qt::WindowCloseButtonHint );
		}
	};

	container->setFixedSize( m_pluginGeometry );
}

QString VstPlugin::embedMethod() const
{
	return m_embedMethod;
}




