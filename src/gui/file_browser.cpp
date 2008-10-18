/*
 * file_browser.cpp - implementation of the project-, preset- and
 *                    sample-file-browser
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtGui/QKeyEvent>
#include <QtGui/QMenu>
#include <QtGui/QPushButton>
#include <QtGui/QMdiArea>
#include <QtGui/QMdiSubWindow>

#include "file_browser.h"
#include "bb_track_container.h"
#include "config_mgr.h"
#include "debug.h"
#include "embed.h"
#include "engine.h"
#include "gui_templates.h"
#include "instrument.h"
#include "instrument_track.h"
#include "main_window.h"
#include "mmp.h"
#include "preset_preview_play_handle.h"
#include "sample_play_handle.h"
#include "song.h"
#include "string_pair_drag.h"
#include "text_float.h"

enum TreeWidgetItemTypes
{
	TypeFileItem = QTreeWidgetItem::UserType,
	TypeDirectoryItem
} ;


fileBrowser::fileBrowser( const QString & _directories, const QString & _filter,
			const QString & _title, const QPixmap & _pm,
			QWidget * _parent, bool _dirs_as_items ) :
	sideBarWidget( _title, _pm, _parent ),
	m_directories( _directories ),
	m_filter( _filter ),
	m_dirsAsItems( _dirs_as_items )
{
	setWindowTitle( tr( "Browser" ) );
	m_l = new listView( contentParent() );
	addContentWidget( m_l );

	QPushButton * reload_btn = new QPushButton( embed::getIconPixmap(
			"reload" ), tr( "Reload (F5)" ), contentParent() );
	addContentWidget( reload_btn );
	connect( reload_btn, SIGNAL( clicked() ), this, SLOT( reloadTree() ) );

	reloadTree();
	show();
}




fileBrowser::~fileBrowser()
{
}




void fileBrowser::reloadTree( void )
{
	m_l->clear();
	QStringList paths = m_directories.split( '*' );
	for( QStringList::iterator it = paths.begin(); it != paths.end(); ++it )
	{
		addItems( *it );
	}
}




void fileBrowser::addItems( const QString & _path )
{
	if( m_dirsAsItems )
	{
		m_l->addTopLevelItem( new directory( _path,
						QString::null, m_filter ) );
		return;
	}

	QDir cdir( _path );
	QStringList files = cdir.entryList( QDir::Dirs, QDir::Name );
	for( QStringList::const_iterator it = files.constBegin();
						it != files.constEnd(); ++it )
	{
		QString cur_file = *it;
		if( cur_file[0] != '.' )
		{
			bool orphan = true;
			for( int i = 0; i < m_l->topLevelItemCount(); ++i )
			{
				directory * d = dynamic_cast<directory *>(
						m_l->topLevelItem( i ) );
				if( d == NULL || cur_file < d->text( 0 ) )
				{
					m_l->insertTopLevelItem( i,
						new directory( cur_file, _path,
								m_filter ) );
					orphan = false;
					break;
				}
				else if( cur_file == d->text( 0 ) )
				{
					d->addDirectory( _path );
					orphan = false;
					break;
				}
			}
			if( orphan )
			{
				m_l->addTopLevelItem( new directory( cur_file,
							_path, m_filter ) );
			}
		}
	}

	files = cdir.entryList( QDir::Files, QDir::Name );
	for( QStringList::const_iterator it = files.constBegin();
						it != files.constEnd(); ++it )
	{
		QString cur_file = *it;
		if( cur_file[0] != '.' )
		{
			// TODO: don't insert instead of removing, order changed
			// remove existing file-items
			QList<QTreeWidgetItem *> existing = m_l->findItems(
					cur_file, Qt::MatchFixedString );
			if( !existing.empty() )
			{
				delete existing.front();
			}
			(void) new fileItem( m_l, cur_file, _path );
		}
	}
}




void fileBrowser::keyPressEvent( QKeyEvent * _ke )
{
	if( _ke->key() == Qt::Key_F5 )
	{
		reloadTree();
	}
	else
	{
		_ke->ignore();
	}
}








listView::listView( QWidget * _parent ) :
	QTreeWidget( _parent ),
	m_mousePressed( false ),
	m_pressPos(),
	m_previewPlayHandle( NULL ),
	m_pphMutex( QMutex::Recursive ),
	m_contextMenuItem( NULL )
{
	setColumnCount( 1 );
	setHeaderLabel( tr( "Files" ) );
	setSortingEnabled( false );

	setFont( pointSizeF( font(), 7.5f ) );

	connect( this, SIGNAL( itemDoubleClicked( QTreeWidgetItem *, int ) ),
			SLOT( activateListItem( QTreeWidgetItem *, int ) ) );
	connect( this, SIGNAL( itemCollapsed( QTreeWidgetItem * ) ),
				SLOT( updateDirectory( QTreeWidgetItem * ) ) );
	connect( this, SIGNAL( itemExpanded( QTreeWidgetItem * ) ),
				SLOT( updateDirectory( QTreeWidgetItem * ) ) );
}




listView::~listView()
{
}




void listView::contextMenuEvent( QContextMenuEvent * _e )
{
	fileItem * f = dynamic_cast<fileItem *>( itemAt( _e->pos() ) );
	if( f != NULL && ( f->handling() == fileItem::LoadAsPreset ||
				 f->handling() == fileItem::LoadByPlugin ) )
	{
		m_contextMenuItem = f;
		QMenu contextMenu( this );
		contextMenu.addAction( tr( "Send to active instrument-track" ),
						this,
					SLOT( sendToActiveInstrumentTrack() ) );
		contextMenu.addAction( tr( "Open in new instrument-track/"
								"Song-Editor" ),
						this,
					SLOT( openInNewInstrumentTrackSE() ) );
		contextMenu.addAction( tr( "Open in new instrument-track/"
								"B+B Editor" ),
						this,
					SLOT( openInNewInstrumentTrackBBE() ) );
		contextMenu.exec( _e->globalPos() );
		m_contextMenuItem = NULL;
	}
}




void listView::mousePressEvent( QMouseEvent * _me )
{
	QTreeWidget::mousePressEvent( _me );
	if( _me->button() != Qt::LeftButton )
	{
		return;
	}

	QTreeWidgetItem * i = itemAt( _me->pos() );
        if ( i )
	{
		// TODO: Restrict to visible selection
//		if ( _me->x() > header()->cellPos( header()->mapToActual( 0 ) )
//			+ treeStepSize() * ( i->depth() + ( rootIsDecorated() ?
//						1 : 0 ) ) + itemMargin() ||
//				_me->x() < header()->cellPos(
//						header()->mapToActual( 0 ) ) )
//		{
			m_pressPos = _me->pos();
			m_mousePressed = true;
//		}
	}

	fileItem * f = dynamic_cast<fileItem *>( i );
	if( f != NULL )
	{
		m_pphMutex.lock();
		if( m_previewPlayHandle != NULL )
		{
			engine::getMixer()->removePlayHandle(
							m_previewPlayHandle );
			m_previewPlayHandle = NULL;
		}

		// in special case of sample-files we do not care about
		// handling() rather than directly creating a samplePlayHandle
		if( f->type() == fileItem::SampleFile )
		{
			textFloat * tf = textFloat::displayMessage(
					tr( "Loading sample" ),
					tr( "Please wait, loading sample for "
								"preview..." ),
					embed::getIconPixmap( "sample_file",
								24, 24 ), 0 );
			qApp->processEvents(
					QEventLoop::ExcludeUserInputEvents );
			samplePlayHandle * s = new samplePlayHandle(
								f->fullName() );
			s->setDoneMayReturnTrue( false );
			m_previewPlayHandle = s;
			delete tf;
		}
		else if( f->handling() == fileItem::LoadAsPreset ||
				f->handling() == fileItem::LoadByPlugin )
		{
			m_previewPlayHandle =
				new presetPreviewPlayHandle( f->fullName(),
					f->handling() ==
						fileItem::LoadByPlugin );
		}
		if( m_previewPlayHandle != NULL )
		{
			if( !engine::getMixer()->addPlayHandle(
							m_previewPlayHandle ) )
			{
				m_previewPlayHandle = NULL;
			}
		}
		m_pphMutex.unlock();
	}
}




void listView::mouseMoveEvent( QMouseEvent * _me )
{
	if( m_mousePressed == true &&
		( m_pressPos - _me->pos() ).manhattanLength() >
					QApplication::startDragDistance() )
	{
		// make sure any playback is stopped
		mouseReleaseEvent( NULL );

		fileItem * f = dynamic_cast<fileItem *>( itemAt( m_pressPos ) );
		if( f != NULL )
		{
			switch( f->type() )
			{
				case fileItem::PresetFile:
					new stringPairDrag( "presetfile",
								f->fullName(),
							embed::getIconPixmap(
								"preset_file" ),
									this );
					break;

				case fileItem::SampleFile:
					new stringPairDrag( "samplefile",
								f->fullName(),
							embed::getIconPixmap(
								"sample_file" ),
									this );
					break;

				case fileItem::MidiFile:
					new stringPairDrag( "midifile",
								f->fullName(),
							embed::getIconPixmap(
								"midi_file" ),
									this );
					break;

				default:
					break;
			}
		}
	}
}




void listView::mouseReleaseEvent( QMouseEvent * _me )
{
	m_mousePressed = false;

	m_pphMutex.lock();
	if( m_previewPlayHandle != NULL )
	{
		// if there're samples shorter than 3 seconds, we don't
		// stop them if the user releases mouse-button...
		if( m_previewPlayHandle->type() ==
						playHandle::SamplePlayHandle )
		{
			samplePlayHandle * s = dynamic_cast<samplePlayHandle *>(
							m_previewPlayHandle );
			if( s && s->totalFrames() - s->framesDone() <=
				static_cast<f_cnt_t>( engine::getMixer()->
						processingSampleRate() * 3 ) )
			{
				s->setDoneMayReturnTrue( true );
				m_previewPlayHandle = NULL;
				m_pphMutex.unlock();
				return;
			}
		}
		engine::getMixer()->removePlayHandle( m_previewPlayHandle );
		m_previewPlayHandle = NULL;
	}
	m_pphMutex.unlock();
}





void listView::handleFile( fileItem * f, instrumentTrack * _it )
{
	engine::getMixer()->lock();
	switch( f->handling() )
	{
		case fileItem::LoadAsProject:
			if( engine::getMainWindow()->mayChangeProject() )
			{
				engine::getSong()->loadProject( f->fullName() );
			}
			break;

		case fileItem::LoadByPlugin:
		{
			const QString e = f->extension();
			instrument * i = _it->getInstrument();
			if( i == NULL ||
				!i->getDescriptor()->supportsFileType( e ) )
			{
				i = _it->loadInstrument(
					engine::pluginFileHandling()[e] );
			}
			i->loadFile( f->fullName() );
			break;
		}

		case fileItem::LoadAsPreset:
		{
			multimediaProject mmp( f->fullName() );
			instrumentTrack::removeMidiPortNode( mmp );
			_it->setSimpleSerializing();
			_it->loadSettings( mmp.content().toElement() );
		}

		case fileItem::NotSupported:
		default:
			break;
	}
	engine::getMixer()->unlock();
}




void listView::activateListItem( QTreeWidgetItem * _item, int _column )
{
	fileItem * f = dynamic_cast<fileItem *>( _item );
	if( f == NULL )
	{
		return;
	}

	if( f->handling() == fileItem::LoadAsProject )
	{
		handleFile( f, NULL );
	}
	else
	{
		engine::getMixer()->lock();
		instrumentTrack * it = dynamic_cast<instrumentTrack *>(
				track::create( track::InstrumentTrack,
					engine::getBBTrackContainer() ) );
		handleFile( f, it );
		engine::getMixer()->unlock();
	}
}




void listView::openInNewInstrumentTrack( trackContainer * _tc )
{
	if( m_contextMenuItem->handling() == fileItem::LoadAsPreset ||
		m_contextMenuItem->handling() == fileItem::LoadByPlugin )
	{
		engine::getMixer()->lock();
		instrumentTrack * it = dynamic_cast<instrumentTrack *>(
				track::create( track::InstrumentTrack, _tc ) );
		handleFile( m_contextMenuItem, it );
		engine::getMixer()->unlock();
	}
}




void listView::openInNewInstrumentTrackBBE( void )
{
	openInNewInstrumentTrack( engine::getBBTrackContainer() );
}




void listView::openInNewInstrumentTrackSE( void )
{
	openInNewInstrumentTrack( engine::getSong() );
}




void listView::sendToActiveInstrumentTrack( void )
{
	// get all windows opened in the workspace
	QList<QMdiSubWindow*> pl =
			engine::getMainWindow()->workspace()->
				subWindowList( QMdiArea::StackingOrder );
	QListIterator<QMdiSubWindow *> w( pl );
	w.toBack();
	// now we travel through the window-list until we find an
	// instrument-track
	while( w.hasPrevious() )
	{
		instrumentTrackWindow * itw =
			dynamic_cast<instrumentTrackWindow *>(
						w.previous()->widget() );
		if( itw != NULL && itw->isHidden() == false )
		{
			handleFile( m_contextMenuItem, itw->model() );
			break;
		}
	}
}




void listView::updateDirectory( QTreeWidgetItem * _item )
{
	directory * dir = dynamic_cast<directory *>( _item );
	if( dir != NULL )
	{
		dir->update();
	}
}






QPixmap * directory::s_folderPixmap = NULL;
QPixmap * directory::s_folderOpenedPixmap = NULL;
QPixmap * directory::s_folderLockedPixmap = NULL;


directory::directory( const QString & _name, const QString & _path,
						const QString & _filter ) :
	QTreeWidgetItem( QStringList( _name ), TypeDirectoryItem ),
	m_directories( _path ),
	m_filter( _filter )
{
	initPixmaps();

	setChildIndicatorPolicy( QTreeWidgetItem::ShowIndicator );

	if( !QDir( fullName() ).isReadable() )
	{
		setIcon( 0, *s_folderLockedPixmap );
	}
	else
	{
		setIcon( 0, *s_folderPixmap );
	}
}




void directory::initPixmaps( void )
{
	if( s_folderPixmap == NULL )
	{
		s_folderPixmap = new QPixmap(
					embed::getIconPixmap( "folder" ) );
	}

	if( s_folderOpenedPixmap == NULL )
	{
		s_folderOpenedPixmap = new QPixmap(
				embed::getIconPixmap( "folder_opened" ) );
	}

	if( s_folderLockedPixmap == NULL )
	{
		s_folderLockedPixmap = new QPixmap(
				embed::getIconPixmap( "folder_locked" ) );
	}
}




void directory::update( void )
{
	if( !isExpanded() )
	{
		setIcon( 0, *s_folderPixmap );
		return;
	}

	setIcon( 0, *s_folderOpenedPixmap );
	if( !childCount() )
	{
		for( QStringList::iterator it = m_directories.begin();
					it != m_directories.end(); ++it )
		{
			int top_index = childCount();
			if( addItems( fullName( *it ) ) &&
				( *it ).contains(
					configManager::inst()->dataDir() ) )
			{
				QTreeWidgetItem * sep = new QTreeWidgetItem;
				sep->setText( 0, listView::tr(
						"--- Factory files ---" ) );
				sep->setIcon( 0, embed::getIconPixmap(
							"factory_files" ) );
				insertChild( top_index, sep );
			}
		}
	}
}




bool directory::addItems( const QString & _path )
{
	QDir thisDir( _path );
	if( !thisDir.isReadable() )
	{
		return false;
	}

	treeWidget()->setUpdatesEnabled( false );

	bool added_something = false;

	QStringList files = thisDir.entryList( QDir::Dirs, QDir::Name );
	for( QStringList::const_iterator it = files.constBegin();
						it != files.constEnd(); ++it )
	{
		QString cur_file = *it;
		if( cur_file[0] != '.' )
		{
			bool orphan = true;
			for( int i = 0; i < childCount(); ++i )
			{
				directory * d = dynamic_cast<directory *>(
								child( i ) );
				if( d == NULL || cur_file < d->text( 0 ) )
				{
					insertChild( i, new directory( cur_file,
							_path, m_filter ) );
					orphan = false;
					break;
				}
				else if( cur_file == d->text( 0 ) )
				{
					d->addDirectory( _path );
					orphan = false;
					break;
				}
			}
			if( orphan )
			{
				addChild( new directory( cur_file, _path,
								m_filter ) );
			}

			added_something = true;
		}
	}

	QList<QTreeWidgetItem*> items;
	files = thisDir.entryList( QDir::Files, QDir::Name );
	for( QStringList::const_iterator it = files.constBegin();
						it != files.constEnd(); ++it )
	{
		QString cur_file = *it;
		if( cur_file[0] != '.' &&
				thisDir.match( m_filter, cur_file.toLower() ) )
		{
			items << new fileItem( cur_file, _path );
			added_something = true;
		}
	}
	addChildren( items );

	treeWidget()->setUpdatesEnabled( true );

	return added_something;
}




QPixmap * fileItem::s_projectFilePixmap = NULL;
QPixmap * fileItem::s_presetFilePixmap = NULL;
QPixmap * fileItem::s_sampleFilePixmap = NULL;
QPixmap * fileItem::s_midiFilePixmap = NULL;
QPixmap * fileItem::s_flpFilePixmap = NULL;
QPixmap * fileItem::s_unknownFilePixmap = NULL;


fileItem::fileItem( QTreeWidget * _parent, const QString & _name,
						const QString & _path ) :
	QTreeWidgetItem( _parent, QStringList( _name) , TypeFileItem ),
	m_path( _path )
{
	determineFileType();
	initPixmaps();
}




fileItem::fileItem( const QString & _name, const QString & _path ) :
	QTreeWidgetItem( QStringList( _name ), TypeFileItem ),
	m_path( _path )
{
	determineFileType();
	initPixmaps();
}




void fileItem::initPixmaps( void )
{
	if( s_projectFilePixmap == NULL )
	{
		s_projectFilePixmap = new QPixmap( embed::getIconPixmap(
						"project_file", 16, 16 ) );
	}

	if( s_presetFilePixmap == NULL )
	{
		s_presetFilePixmap = new QPixmap( embed::getIconPixmap(
						"preset_file", 16, 16 ) );
	}

	if( s_sampleFilePixmap == NULL )
	{
		s_sampleFilePixmap = new QPixmap( embed::getIconPixmap(
						"sample_file", 16, 16 ) );
	}

	if( s_midiFilePixmap == NULL )
	{
		s_midiFilePixmap = new QPixmap( embed::getIconPixmap(
							"midi_file", 16, 16 ) );
	}

	if( s_flpFilePixmap == NULL )
	{
		s_flpFilePixmap = new QPixmap( embed::getIconPixmap(
							"midi_file", 16, 16 ) );
	}

	if( s_unknownFilePixmap == NULL )
	{
		s_unknownFilePixmap = new QPixmap( embed::getIconPixmap(
							"unknown_file" ) );
	}

	switch( m_type )
	{
		case ProjectFile:
			setIcon( 0, *s_projectFilePixmap );
			break;
		case PresetFile:
			setIcon( 0, *s_presetFilePixmap );
			break;
		case SampleFile:
		case SoundFontFile:		// TODO
		case PatchFile:			// TODO
			setIcon( 0, *s_sampleFilePixmap );
			break;
		case MidiFile:
			setIcon( 0, *s_midiFilePixmap );
			break;
		case FlpFile:
			setIcon( 0, *s_flpFilePixmap );
			break;
		case UnknownFile:
		default:
			setIcon( 0, *s_unknownFilePixmap );
			break;
	}
}




void fileItem::determineFileType( void )
{
	m_handling = NotSupported;

	const QString ext = extension();
	if( ext == "mmp" || ext == "mpt" || ext == "mmpz" )
	{
		m_type = ProjectFile;
		m_handling = LoadAsProject;
	}
	else if( ext == "xpf" || ext == "xml" || ext == "xiz" )
	{
		m_type = PresetFile;
		m_handling = LoadAsPreset;
	}
	else if( ext == "sf2" )
	{
		m_type = SoundFontFile;
	}
	else if( ext == "pat" )
	{
		m_type = PatchFile;
	}
	else if( ext == "mid" )
	{
		m_type = MidiFile;
	}
	else if( ext == "flp" )
	{
		m_type = FlpFile;
	}
	else
	{
		m_type = UnknownFile;
	}

	if( !ext.isEmpty() && engine::pluginFileHandling().contains( ext ) )
	{
		m_handling = LoadByPlugin;
		// classify as sample if not classified by anything yet but can
		// be handled by a certain plugin
		if( m_type == UnknownFile )
		{
			m_type = SampleFile;
		}
	}
}




QString fileItem::extension( void )
{
	return extension( fullName() );
}




QString fileItem::extension( const QString & _file )
{
	return QFileInfo( _file ).suffix().toLower();
}




#include "moc_file_browser.cxx"

