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

#include "communication.h"

#include <QtCore/QtEndian>
#include <QtCore/QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLocale>
#include <QTemporaryFile>
#include <QCloseEvent>
#include <QMdiArea>
#include <QMdiSubWindow>

#ifdef LMMS_BUILD_LINUX
#	include <QX11Info>
#	include "X11EmbedContainer.h"
#endif

#include <QWindow>

#include <QDomDocument>

#ifdef LMMS_BUILD_WIN32
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif

#	include <windows.h>
#	include <QLayout>
#endif

#include "ConfigManager.h"
#include "GuiApplication.h"
#include "LocaleHelper.h"
#include "MainWindow.h"
#include "Mixer.h"
#include "Song.h"
#include "FileDialog.h"

#ifdef LMMS_BUILD_LINUX
#	include <X11/Xlib.h>
#endif

namespace PE
{
// Utilities for reading PE file machine type
// See specification at https://msdn.microsoft.com/library/windows/desktop/ms680547(v=vs.85).aspx

// Work around name conflict
#ifdef i386
#	undef i386
#endif

enum class MachineType : uint16_t
{
	unknown = 0x0,
	amd64 = 0x8664,
	i386 = 0x14c,
};

class FileInfo
{
public:
	FileInfo(QString filePath)
		: m_file(filePath)
	{
		m_file.open(QFile::ReadOnly);
		m_map = m_file.map(0, m_file.size());
		if (m_map == nullptr) {
			throw std::runtime_error("Cannot map file");
		}
	}
	~FileInfo()
	{
		m_file.unmap(m_map);
	}

	MachineType machineType()
	{
		int32_t peOffset = qFromLittleEndian(* reinterpret_cast<int32_t*>(m_map + 0x3C));
		uchar* peSignature = m_map + peOffset;
		if (memcmp(peSignature, "PE\0\0", 4)) {
			throw std::runtime_error("Invalid PE file");
		}
		uchar * coffHeader = peSignature + 4;
		uint16_t machineType = qFromLittleEndian(* reinterpret_cast<uint16_t*>(coffHeader));
		return static_cast<MachineType>(machineType);
	}

private:
	QFile m_file;
	uchar* m_map;
};

}


VstPlugin::VstPlugin( const QString & _plugin ) :
	m_plugin( _plugin ),
	m_pluginWindowID( 0 ),
	m_embedMethod( gui
			? ConfigManager::inst()->vstEmbedMethod()
			: "headless" ),
	m_version( 0 ),
	m_currentProgram()
{
	if( QDir::isRelativePath( m_plugin ) )
	{
		m_plugin = ConfigManager::inst()->vstDir()  + m_plugin;
	}

	setSplittedChannels( true );

	PE::MachineType machineType;
	try {
		PE::FileInfo peInfo(m_plugin);
		machineType = peInfo.machineType();
	} catch (std::runtime_error& e) {
		qCritical() << "Error while determining PE file's machine type: " << e.what();
		machineType = PE::MachineType::unknown;
	}

	switch(machineType)
	{
	case PE::MachineType::amd64:
		tryLoad( REMOTE_VST_PLUGIN_FILEPATH_64 ); // Default: RemoteVstPlugin64
		break;
	case PE::MachineType::i386:
		tryLoad( REMOTE_VST_PLUGIN_FILEPATH_32 ); // Default: 32/RemoteVstPlugin32
		break;
	default:
		m_failed = true;
		return;
	}

	setTempo( Engine::getSong()->getTempo() );

	connect( Engine::getSong(), SIGNAL( tempoChanged( bpm_t ) ),
			this, SLOT( setTempo( bpm_t ) ), Qt::DirectConnection );
	connect( Engine::mixer(), SIGNAL( sampleRateChanged() ),
				this, SLOT( updateSampleRate() ) );

	// update once per second
	m_idleTimer.start( 1000 );
	connect( &m_idleTimer, SIGNAL( timeout() ),
				this, SLOT( idleUpdate() ) );
}




VstPlugin::~VstPlugin()
{
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
	sendMessage( message( IdVstLoadPlugin ).addString( QSTR_TO_STDSTR( m_plugin ) ) );

	waitForInitDone();

	unlock();
}




void VstPlugin::loadSettings( const QDomElement & _this )
{
	if( _this.hasAttribute( "program" ) )
	{
		setProgram( _this.attribute( "program" ).toInt() );
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
		toggleEditorVisibility();
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
	waitForMessage( IdInformationUpdated, true );
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
			LocaleHelper::toFloat((*it).section(':', 2, -1))
		} ;
		m.addInt( item.index );
		m.addString( item.shortLabel );
		m.addFloat( item.value );
	}
	lock();
	sendMessage( m );
	unlock();
}

QWidget *VstPlugin::pluginWidget()
{
	return m_pluginWidget;
}




bool VstPlugin::processMessage( const message & _m )
{
	switch( _m.id )
	{
	case IdVstPluginWindowID:
		m_pluginWindowID = _m.getInt();
		if( m_embedMethod == "none"
			&& ConfigManager::inst()->value(
				"ui", "vstalwaysontop" ).toInt() )
		{
#ifdef LMMS_BUILD_WIN32
			// We're changing the owner, not the parent,
			// so this is legal despite MSDN's warning
			SetWindowLongPtr( (HWND)(intptr_t) m_pluginWindowID,
					GWLP_HWNDPARENT,
					(LONG_PTR) gui->mainWindow()->winId() );
#endif

#ifdef LMMS_BUILD_LINUX
			XSetTransientForHint( QX11Info::display(),
					m_pluginWindowID,
					gui->mainWindow()->winId() );
#endif
		}
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


QWidget *VstPlugin::editor()
{
	return m_pluginWidget;
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
	else if ( m_embedMethod != "headless" )
	{
		if (! editor()) {
			qWarning() << "VstPlugin::showUI called before VstPlugin::createUI";
		}
		toggleEditorVisibility( true );
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
		toggleEditorVisibility( false );
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

void VstPlugin::toggleEditorVisibility( int visible )
{
	QWidget* w = editor();
	if ( ! w ) {
		return;
	}

	if ( visible < 0 ) {
		visible = ! w->isVisible();
	}
	w->setVisible( visible );
}

void VstPlugin::createUI( QWidget * parent )
{
	if ( m_pluginWidget ) {
		qWarning() << "VstPlugin::createUI called twice";
		m_pluginWidget->setParent( parent );
		return;
	}

	if( m_pluginWindowID == 0 )
	{
		return;
	}

	QWidget* container = nullptr;

#if QT_VERSION >= 0x050100
	if (m_embedMethod == "qt" )
	{
		QWindow* vw = QWindow::fromWinId(m_pluginWindowID);
		container = QWidget::createWindowContainer(vw, parent );
		container->installEventFilter(this);
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
		if (parent)
		{
			parent->setAttribute(Qt::WA_NativeWindow);
		}
		QX11EmbedContainer * embedContainer = new QX11EmbedContainer( parent );
		connect(embedContainer, SIGNAL(clientIsEmbedded()), this, SLOT(handleClientEmbed()));
		embedContainer->embedClient( m_pluginWindowID );
		container = embedContainer;
	} else
#endif
	{
		qCritical() << "Unknown embed method" << m_embedMethod;
		return;
	}

	container->setFixedSize( m_pluginGeometry );
	container->setWindowTitle( name() );

	m_pluginWidget = container;
}

bool VstPlugin::eventFilter(QObject *obj, QEvent *event)
{
#if QT_VERSION >= 0x050100
	if (embedMethod() == "qt" && obj == m_pluginWidget)
	{
		if (event->type() == QEvent::Show) {
			RemotePlugin::showUI();
		}
		qDebug() << obj << event;
	}
#endif
	return false;
}

QString VstPlugin::embedMethod() const
{
	return m_embedMethod;
}




