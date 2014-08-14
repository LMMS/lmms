/*
 * file_browser.cpp - implementation of the project-, preset- and
 *                    sample-file-browser
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QMdiArea>
#include <QMdiSubWindow>

#include "file_browser.h"
#include "bb_track_container.h"
#include "config_mgr.h"
#include "debug.h"
#include "embed.h"
#include "engine.h"
#include "gui_templates.h"
#include "ImportFilter.h"
#include "Instrument.h"
#include "InstrumentTrack.h"
#include "MainWindow.h"
#include "DataFile.h"
#include "PresetPreviewPlayHandle.h"
#include "SamplePlayHandle.h"
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
	SideBarWidget( _title, _pm, _parent ),
	m_directories( _directories ),
	m_filter( _filter ),
	m_dirsAsItems( _dirs_as_items )
{
	setWindowTitle( tr( "Browser" ) );
	m_l = new fileBrowserTreeWidget( contentParent() );
	addContentWidget( m_l );

	QWidget * ops = new QWidget( contentParent() );
	ops->setFixedHeight( 24 );

	QHBoxLayout * opl = new QHBoxLayout( ops );
	opl->setMargin( 0 );
	opl->setSpacing( 0 );

	m_filterEdit = new QLineEdit( ops );
	connect( m_filterEdit, SIGNAL( textEdited( const QString & ) ),
			this, SLOT( filterItems( const QString & ) ) );

	QPushButton * reload_btn = new QPushButton(
				embed::getIconPixmap( "reload" ),
						QString::null, ops );
	connect( reload_btn, SIGNAL( clicked() ), this, SLOT( reloadTree() ) );

	opl->addWidget( m_filterEdit );
	opl->addSpacing( 5 );
	opl->addWidget( reload_btn );

	addContentWidget( ops );

	reloadTree();
	show();
}




fileBrowser::~fileBrowser()
{
}




void fileBrowser::filterItems( const QString & _filter )
{
	const bool show_all = _filter.isEmpty();

	for( int i = 0; i < m_l->topLevelItemCount(); ++i )
	{
		QTreeWidgetItem * it = m_l->topLevelItem( i );
		// show all items if filter is empty
		if( show_all )
		{
			it->setHidden( false );
			if( it->childCount() )
			{
				filterItems( it, _filter );
			}
		}
		// is directory?
		else if( it->childCount() )
		{
			// matches filter?
			if( it->text( 0 ).
				contains( _filter, Qt::CaseInsensitive ) )
			{
				// yes, then show everything below
				it->setHidden( false );
				filterItems( it, QString::null );
			}
			else
			{
				// only show if item below matches filter
				it->setHidden( !filterItems( it, _filter ) );
			}
		}
		// a standard item (i.e. no file or directory item?)
		else if( it->type() == QTreeWidgetItem::Type )
		{
			// hide in every case when filtering
			it->setHidden( true );
		}
		else
		{
			// file matches filter?
			it->setHidden( !it->text( 0 ).
				contains( _filter, Qt::CaseInsensitive ) );
		}
	
	}
}




bool fileBrowser::filterItems( QTreeWidgetItem * _item, const QString & _filter )
{
	const bool show_all = _filter.isEmpty();
	bool matched = false;

	for( int i = 0; i < _item->childCount(); ++i )
	{
		QTreeWidgetItem * it = _item->child( i );
		bool cm = false;	// whether current item matched
		// show all items if filter is empty
		if( show_all )
		{
			it->setHidden( false );
			if( it->childCount() )
			{
				filterItems( it, _filter );
			}
		}
		// is directory?
		else if( it->childCount() )
		{
			// matches filter?
			if( it->text( 0 ).
				contains( _filter, Qt::CaseInsensitive ) )
			{
				// yes, then show everything below
				it->setHidden( false );
				filterItems( it, QString::null );
				cm = true;
			}
			else
			{
				// only show if item below matches filter
				cm = filterItems( it, _filter );
				it->setHidden( !cm );
			}
		}
		// a standard item (i.e. no file or directory item?)
		else if( it->type() == QTreeWidgetItem::Type )
		{
			// hide in every case when filtering
			it->setHidden( true );
		}
		else
		{
			// file matches filter?
			cm = it->text( 0 ).
				contains( _filter, Qt::CaseInsensitive );
			it->setHidden( !cm );
		}
	
		if( cm )
		{
			matched = true;
		}
	}

	return matched;
}




void fileBrowser::reloadTree( void )
{
	const QString text = m_filterEdit->text();
	m_filterEdit->clear();
	m_l->clear();
	QStringList paths = m_directories.split( '*' );
	for( QStringList::iterator it = paths.begin(); it != paths.end(); ++it )
	{
		addItems( *it );
	}
	m_filterEdit->setText( text );
	filterItems( text );
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








fileBrowserTreeWidget::fileBrowserTreeWidget( QWidget * _parent ) :
	QTreeWidget( _parent ),
	m_mousePressed( false ),
	m_pressPos(),
	m_previewPlayHandle( NULL ),
	m_pphMutex( QMutex::Recursive ),
	m_contextMenuItem( NULL )
{
	setColumnCount( 1 );
	headerItem()->setHidden( true );
	setSortingEnabled( false );

	setFont( pointSizeF( font(), 7.5f ) );

	connect( this, SIGNAL( itemDoubleClicked( QTreeWidgetItem *, int ) ),
			SLOT( activateListItem( QTreeWidgetItem *, int ) ) );
	connect( this, SIGNAL( itemCollapsed( QTreeWidgetItem * ) ),
				SLOT( updateDirectory( QTreeWidgetItem * ) ) );
	connect( this, SIGNAL( itemExpanded( QTreeWidgetItem * ) ),
				SLOT( updateDirectory( QTreeWidgetItem * ) ) );
}




fileBrowserTreeWidget::~fileBrowserTreeWidget()
{
}




void fileBrowserTreeWidget::contextMenuEvent( QContextMenuEvent * _e )
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




void fileBrowserTreeWidget::mousePressEvent( QMouseEvent * _me )
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
			engine::mixer()->removePlayHandle(
							m_previewPlayHandle );
			m_previewPlayHandle = NULL;
		}

		// in special case of sample-files we do not care about
		// handling() rather than directly creating a SamplePlayHandle
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
			SamplePlayHandle * s = new SamplePlayHandle(
								f->fullName() );
			s->setDoneMayReturnTrue( false );
			m_previewPlayHandle = s;
			delete tf;
		}
		else if( f->type() != fileItem::VstPluginFile &&
				( f->handling() == fileItem::LoadAsPreset ||
				f->handling() == fileItem::LoadByPlugin ) )
		{
			m_previewPlayHandle = new PresetPreviewPlayHandle( f->fullName(), f->handling() == fileItem::LoadByPlugin );
		}
		if( m_previewPlayHandle != NULL )
		{
			if( !engine::mixer()->addPlayHandle(
							m_previewPlayHandle ) )
			{
				m_previewPlayHandle = NULL;
			}
		}
		m_pphMutex.unlock();
	}
}




void fileBrowserTreeWidget::mouseMoveEvent( QMouseEvent * _me )
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
	new stringPairDrag( f->handling() == fileItem::LoadAsPreset ?
					"presetfile" : "pluginpresetfile",
				f->fullName(),
				embed::getIconPixmap( "preset_file" ), this );
					break;

				case fileItem::SampleFile:
	new stringPairDrag( "samplefile", f->fullName(),
				embed::getIconPixmap( "sample_file" ), this );
					break;
				case fileItem::SoundFontFile:
 	new stringPairDrag( "soundfontfile", f->fullName(),
 				embed::getIconPixmap( "soundfont_file" ), this );
 					break;
				case fileItem::VstPluginFile:
	new stringPairDrag( "vstpluginfile", f->fullName(),
				embed::getIconPixmap( "vst_plugin_file" ), this );
					break;
				case fileItem::MidiFile:
// don't allow dragging FLP-files as FLP import filter clears project
// without asking
//				case fileItem::FlpFile:
	new stringPairDrag( "importedproject", f->fullName(),
				embed::getIconPixmap( "midi_file" ), this );
					break;

				default:
					break;
			}
		}
	}
}




void fileBrowserTreeWidget::mouseReleaseEvent( QMouseEvent * _me )
{
	m_mousePressed = false;

	m_pphMutex.lock();
	if( m_previewPlayHandle != NULL )
	{
		// if there're samples shorter than 3 seconds, we don't
		// stop them if the user releases mouse-button...
		if( m_previewPlayHandle->type() == PlayHandle::TypeSamplePlayHandle )
		{
			SamplePlayHandle * s = dynamic_cast<SamplePlayHandle *>(
							m_previewPlayHandle );
			if( s && s->totalFrames() - s->framesDone() <=
				static_cast<f_cnt_t>( engine::mixer()->
						processingSampleRate() * 3 ) )
			{
				s->setDoneMayReturnTrue( true );
				m_previewPlayHandle = NULL;
				m_pphMutex.unlock();
				return;
			}
		}
		engine::mixer()->removePlayHandle( m_previewPlayHandle );
		m_previewPlayHandle = NULL;
	}
	m_pphMutex.unlock();
}





void fileBrowserTreeWidget::handleFile( fileItem * f, InstrumentTrack * _it )
{
	engine::mixer()->lock();
	switch( f->handling() )
	{
		case fileItem::LoadAsProject:
			if( engine::mainWindow()->mayChangeProject() )
			{
				engine::getSong()->loadProject( f->fullName() );
			}
			break;

		case fileItem::LoadByPlugin:
		{
			const QString e = f->extension();
			Instrument * i = _it->instrument();
			if( i == NULL ||
				!i->descriptor()->supportsFileType( e ) )
			{
				i = _it->loadInstrument(
					engine::pluginFileHandling()[e] );
			}
			i->loadFile( f->fullName() );
			break;
		}

		case fileItem::LoadAsPreset:
		{
			DataFile dataFile( f->fullName() );
			InstrumentTrack::removeMidiPortNode( dataFile );
			_it->setSimpleSerializing();
			_it->loadSettings( dataFile.content().toElement() );
			break;
		}

		case fileItem::ImportAsProject:
			if( f->type() == fileItem::FlpFile &&
				!engine::mainWindow()->mayChangeProject() )
			{
				break;
			}
			ImportFilter::import( f->fullName(),
							engine::getSong() );
			break;

		case fileItem::NotSupported:
		default:
			break;

	}
	engine::mixer()->unlock();
}




void fileBrowserTreeWidget::activateListItem( QTreeWidgetItem * _item,
								int _column )
{
	fileItem * f = dynamic_cast<fileItem *>( _item );
	if( f == NULL )
	{
		return;
	}

	if( f->handling() == fileItem::LoadAsProject ||
		f->handling() == fileItem::ImportAsProject )
	{
		handleFile( f, NULL );
	}
	else if( f->handling() != fileItem::NotSupported )
	{
//		engine::mixer()->lock();
		InstrumentTrack * it = dynamic_cast<InstrumentTrack *>(
				track::create( track::InstrumentTrack,
					engine::getBBTrackContainer() ) );
		handleFile( f, it );
//		engine::mixer()->unlock();
	}
}




void fileBrowserTreeWidget::openInNewInstrumentTrack( TrackContainer* tc )
{
	if( m_contextMenuItem->handling() == fileItem::LoadAsPreset ||
		m_contextMenuItem->handling() == fileItem::LoadByPlugin )
	{
//		engine::mixer()->lock();
		InstrumentTrack * it = dynamic_cast<InstrumentTrack *>(
				track::create( track::InstrumentTrack, tc ) );
		handleFile( m_contextMenuItem, it );
//		engine::mixer()->unlock();
	}
}




void fileBrowserTreeWidget::openInNewInstrumentTrackBBE( void )
{
	openInNewInstrumentTrack( engine::getBBTrackContainer() );
}




void fileBrowserTreeWidget::openInNewInstrumentTrackSE( void )
{
	openInNewInstrumentTrack( engine::getSong() );
}




void fileBrowserTreeWidget::sendToActiveInstrumentTrack( void )
{
	// get all windows opened in the workspace
	QList<QMdiSubWindow*> pl =
			engine::mainWindow()->workspace()->
				subWindowList( QMdiArea::StackingOrder );
	QListIterator<QMdiSubWindow *> w( pl );
	w.toBack();
	// now we travel through the window-list until we find an
	// instrument-track
	while( w.hasPrevious() )
	{
		InstrumentTrackWindow * itw =
			dynamic_cast<InstrumentTrackWindow *>(
						w.previous()->widget() );
		if( itw != NULL && itw->isHidden() == false )
		{
			handleFile( m_contextMenuItem, itw->model() );
			break;
		}
	}
}




void fileBrowserTreeWidget::updateDirectory( QTreeWidgetItem * _item )
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
				sep->setText( 0,
					fileBrowserTreeWidget::tr(
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
QPixmap * fileItem::s_soundfontFilePixmap = NULL;
QPixmap * fileItem::s_vstPluginFilePixmap = NULL;
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

	if ( s_soundfontFilePixmap == NULL )
	{
		s_soundfontFilePixmap = new QPixmap( embed::getIconPixmap(
						"soundfont_file", 16, 16 ) );
	}
	
	if ( s_vstPluginFilePixmap == NULL ) 
	{
		s_vstPluginFilePixmap = new QPixmap( embed::getIconPixmap(
						"vst_plugin_file", 16, 16 ) );
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
		case SoundFontFile:
			setIcon( 0, *s_soundfontFilePixmap );
			break;
		case VstPluginFile:
			setIcon( 0, *s_vstPluginFilePixmap );
			break;
		case SampleFile:
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
	else if( ext == "xpf" || ext == "xml" )
	{
		m_type = PresetFile;
		m_handling = LoadAsPreset;
	}
	else if( ext == "xiz" && engine::pluginFileHandling().contains( ext ) )
	{
		m_type = PresetFile;
		m_handling = LoadByPlugin;
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
		m_handling = ImportAsProject;
	}
	else if( ext == "flp" )
	{
		m_type = FlpFile;
		m_handling = ImportAsProject;
	}
	else if( ext == "dll" )
	{
		m_type = VstPluginFile;
		m_handling = LoadByPlugin;
	}
	else
	{
		m_type = UnknownFile;
	}

	if( m_handling == NotSupported &&
		!ext.isEmpty() && engine::pluginFileHandling().contains( ext ) )
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






