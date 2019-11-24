/*
 * FileBrowser.cpp - implementation of the project-, preset- and
 *                    sample-file-browser
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QShortcut>

#include "FileBrowser.h"
#include "BBTrackContainer.h"
#include "ConfigManager.h"
#include "embed.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "gui_templates.h"
#include "ImportFilter.h"
#include "Instrument.h"
#include "InstrumentTrack.h"
#include "MainWindow.h"
#include "Mixer.h"
#include "PluginFactory.h"
#include "PresetPreviewPlayHandle.h"
#include "SamplePlayHandle.h"
#include "Song.h"
#include "StringPairDrag.h"
#include "TextFloat.h"



enum TreeWidgetItemTypes
{
	TypeFileItem = QTreeWidgetItem::UserType,
	TypeDirectoryItem
} ;



FileBrowser::FileBrowser(const QString & directories, const QString & filter,
			const QString & title, const QPixmap & pm,
			QWidget * parent, bool dirs_as_items, bool recurse ) :
	SideBarWidget( title, pm, parent ),
	m_directories( directories ),
	m_filter( filter ),
	m_dirsAsItems( dirs_as_items ),
	m_recurse( recurse )
{
	setWindowTitle( tr( "Browser" ) );

	QWidget * searchWidget = new QWidget( contentParent() );
	searchWidget->setFixedHeight( 24 );

	QHBoxLayout * searchWidgetLayout = new QHBoxLayout( searchWidget );
	searchWidgetLayout->setMargin( 0 );
	searchWidgetLayout->setSpacing( 0 );

	m_filterEdit = new QLineEdit( searchWidget );
	m_filterEdit->setPlaceholderText( tr("Search") );
	m_filterEdit->setClearButtonEnabled( true );
	connect( m_filterEdit, SIGNAL( textEdited( const QString & ) ),
			this, SLOT( filterItems( const QString & ) ) );

	QPushButton * reload_btn = new QPushButton(
				embed::getIconPixmap( "reload" ),
						QString(), searchWidget );
	reload_btn->setToolTip( tr( "Refresh list" ) );
	connect( reload_btn, SIGNAL( clicked() ), this, SLOT( reloadTree() ) );

	searchWidgetLayout->addWidget( m_filterEdit );
	searchWidgetLayout->addSpacing( 5 );
	searchWidgetLayout->addWidget( reload_btn );

	addContentWidget( searchWidget );

	m_fileBrowserTreeWidget = new FileBrowserTreeWidget( contentParent() );
	addContentWidget( m_fileBrowserTreeWidget );

	// Whenever the FileBrowser has focus, Ctrl+F should direct focus to its filter box.
	QShortcut *filterFocusShortcut = new QShortcut( QKeySequence( QKeySequence::Find ), this, SLOT(giveFocusToFilter()) );
	filterFocusShortcut->setContext(Qt::WidgetWithChildrenShortcut);

	reloadTree();
	show();
}

bool FileBrowser::filterItems( const QString & filter, QTreeWidgetItem * item )
{
	// call with item=NULL to filter the entire tree
	bool anyMatched = false;

	int numChildren = item ? item->childCount() : m_fileBrowserTreeWidget->topLevelItemCount();
	for( int i = 0; i < numChildren; ++i )
	{
		QTreeWidgetItem * it = item ? item->child( i ) : m_fileBrowserTreeWidget->topLevelItem(i);

		// is directory?
		if( it->childCount() )
		{
			// matches filter?
			if( it->text( 0 ).
				contains( filter, Qt::CaseInsensitive ) )
			{
				// yes, then show everything below
				it->setHidden( false );
				filterItems( QString(), it );
				anyMatched = true;
			}
			else
			{
				// only show if item below matches filter
				bool didMatch = filterItems( filter, it );
				it->setHidden( !didMatch );
				anyMatched = anyMatched || didMatch;
			}
		}
		// a standard item (i.e. no file or directory item?)
		else if( it->type() == QTreeWidgetItem::Type )
		{
			// hide if there's any filter
			it->setHidden( !filter.isEmpty() );
		}
		else
		{
			// file matches filter?
			bool didMatch = it->text( 0 ).
				contains( filter, Qt::CaseInsensitive );
			it->setHidden( !didMatch );
			anyMatched = anyMatched || didMatch;
		}
	}

	return anyMatched;
}



void FileBrowser::reloadTree( void )
{
	QList<QString> expandedDirs = m_fileBrowserTreeWidget->expandedDirs();
	const QString text = m_filterEdit->text();
	m_filterEdit->clear();
	m_fileBrowserTreeWidget->clear();
	QStringList paths = m_directories.split( '*' );
	for( QStringList::iterator it = paths.begin(); it != paths.end(); ++it )
	{
		addItems( *it );
	}
	expandItems(NULL, expandedDirs);
	m_filterEdit->setText( text );
	filterItems( text );
}



void FileBrowser::expandItems( QTreeWidgetItem * item, QList<QString> expandedDirs )
{
	int numChildren = item ? item->childCount() : m_fileBrowserTreeWidget->topLevelItemCount();
	for (int i = 0; i < numChildren; ++i)
	{
		QTreeWidgetItem * it = item ? item->child( i ) : m_fileBrowserTreeWidget->topLevelItem(i);
		if ( m_recurse )
		{
			it->setExpanded( true );
		}
		Directory *d = dynamic_cast<Directory *> ( it );
		if (d)
		{
			d->update();
			bool expand = expandedDirs.contains( d->fullName() );
			d->setExpanded( expand );
		}
		if (m_recurse && it->childCount())
		{
			expandItems(it, expandedDirs);
		}
	}
}



void FileBrowser::giveFocusToFilter()
{
	if (!m_filterEdit->hasFocus())
	{
		// give focus to filter text box and highlight its text for quick editing if not previously focused
		m_filterEdit->setFocus();
		m_filterEdit->selectAll();
	}
}



void FileBrowser::addItems(const QString & path )
{
	if( m_dirsAsItems )
	{
		m_fileBrowserTreeWidget->addTopLevelItem( new Directory( path, QString(), m_filter ) );
		return;
	}

	QDir cdir( path );
	QStringList files = cdir.entryList( QDir::Dirs, QDir::Name );
	for( QStringList::const_iterator it = files.constBegin();
						it != files.constEnd(); ++it )
	{
		QString cur_file = *it;
		if( cur_file[0] != '.' )
		{
			bool orphan = true;
			for( int i = 0; i < m_fileBrowserTreeWidget->topLevelItemCount(); ++i )
			{
				Directory * d = dynamic_cast<Directory *>(
						m_fileBrowserTreeWidget->topLevelItem( i ) );
				if( d == NULL || cur_file < d->text( 0 ) )
				{
					Directory *dd = new Directory( cur_file, path,
												   m_filter );
					m_fileBrowserTreeWidget->insertTopLevelItem( i,dd );
					dd->update(); // add files to the directory
					orphan = false;
					break;
				}
				else if( cur_file == d->text( 0 ) )
				{
					d->addDirectory( path );
					d->update();
					orphan = false;
					break;
				}
			}
			if( orphan )
			{
				Directory *d = new Directory( cur_file,
											  path, m_filter );
				d->update();
				m_fileBrowserTreeWidget->addTopLevelItem( d );
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
			QList<QTreeWidgetItem *> existing = m_fileBrowserTreeWidget->findItems(
					cur_file, Qt::MatchFixedString );
			if( !existing.empty() )
			{
				delete existing.front();
			}
			(void) new FileItem( m_fileBrowserTreeWidget, cur_file, path );
		}
	}
}




void FileBrowser::keyPressEvent(QKeyEvent * ke )
{
	if( ke->key() == Qt::Key_F5 )
	{
		reloadTree();
	}
	else
	{
		ke->ignore();
	}
}








FileBrowserTreeWidget::FileBrowserTreeWidget(QWidget * parent ) :
	QTreeWidget( parent ),
	m_mousePressed( false ),
	m_pressPos(),
	m_previewPlayHandle( NULL ),
	m_pphMutex( QMutex::Recursive ),
	m_contextMenuItem( NULL )
{
	setColumnCount( 1 );
	headerItem()->setHidden( true );
	setSortingEnabled( false );

	connect( this, SIGNAL( itemDoubleClicked( QTreeWidgetItem *, int ) ),
			SLOT( activateListItem( QTreeWidgetItem *, int ) ) );
	connect( this, SIGNAL( itemCollapsed( QTreeWidgetItem * ) ),
				SLOT( updateDirectory( QTreeWidgetItem * ) ) );
	connect( this, SIGNAL( itemExpanded( QTreeWidgetItem * ) ),
				SLOT( updateDirectory( QTreeWidgetItem * ) ) );

}

QList<QString> FileBrowserTreeWidget::expandedDirs( QTreeWidgetItem * item ) const
{
	int numChildren = item ? item->childCount() : topLevelItemCount();
	QList<QString> dirs;
	for (int i = 0; i < numChildren; ++i)
	{
		QTreeWidgetItem * it  = item ? item->child(i) : topLevelItem(i);

		// Add expanded top level directories.
		if (it->isExpanded() && (it->type() == TypeDirectoryItem))
		{
			Directory *d = static_cast<Directory *> ( it );
			dirs.append( d->fullName() );
		}

		// Add expanded child directories (recurse).
		if (it->childCount())
		{
			dirs.append( expandedDirs( it ) );
		}
	}
	return dirs;
}

void FileBrowserTreeWidget::contextMenuEvent(QContextMenuEvent * e )
{
	FileItem * f = dynamic_cast<FileItem *>( itemAt( e->pos() ) );
	if( f != NULL && ( f->handling() == FileItem::LoadAsPreset ||
				 f->handling() == FileItem::LoadByPlugin ) )
	{
		m_contextMenuItem = f;
		QMenu contextMenu( this );
		contextMenu.addAction( tr( "Send to active instrument-track" ),
						this,
					SLOT( sendToActiveInstrumentTrack() ) );
		contextMenu.addAction( tr( "Open in new instrument-track/"
								"Song Editor" ),
						this,
					SLOT( openInNewInstrumentTrackSE() ) );
		contextMenu.addAction( tr( "Open in new instrument-track/"
								"B+B Editor" ),
						this,
					SLOT( openInNewInstrumentTrackBBE() ) );
		contextMenu.exec( e->globalPos() );
		m_contextMenuItem = NULL;
	}
}




void FileBrowserTreeWidget::mousePressEvent(QMouseEvent * me )
{
	QTreeWidget::mousePressEvent( me );
	if( me->button() != Qt::LeftButton )
	{
		return;
	}

	QTreeWidgetItem * i = itemAt( me->pos() );
	if ( i )
	{
		// TODO: Restrict to visible selection
//		if ( _me->x() > header()->cellPos( header()->mapToActual( 0 ) )
//			+ treeStepSize() * ( i->depth() + ( rootIsDecorated() ?
//						1 : 0 ) ) + itemMargin() ||
//				_me->x() < header()->cellPos(
//						header()->mapToActual( 0 ) ) )
//		{
			m_pressPos = me->pos();
			m_mousePressed = true;
//		}
	}

	FileItem * f = dynamic_cast<FileItem *>( i );
	if( f != NULL )
	{
		m_pphMutex.lock();
		if( m_previewPlayHandle != NULL )
		{
			Engine::mixer()->removePlayHandle(
							m_previewPlayHandle );
			m_previewPlayHandle = NULL;
		}

		// in special case of sample-files we do not care about
		// handling() rather than directly creating a SamplePlayHandle
		if( f->type() == FileItem::SampleFile )
		{
			TextFloat * tf = TextFloat::displayMessage(
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
		else if( ( f->extension ()== "xiz" || f->extension() == "sf2" || f->extension() == "sf3" || f->extension() == "gig" || f->extension() == "pat" ) &&
			! pluginFactory->pluginSupportingExtension(f->extension()).info.isNull() )
		{
			m_previewPlayHandle = new PresetPreviewPlayHandle( f->fullName(), f->handling() == FileItem::LoadByPlugin );
		}
		else if( f->type() != FileItem::VstPluginFile &&
				( f->handling() == FileItem::LoadAsPreset ||
				f->handling() == FileItem::LoadByPlugin ) )
		{
			DataFile dataFile( f->fullName() );
			if( !dataFile.validate( f->extension() ) )
			{
				QMessageBox::warning( 0, tr ( "Error" ),
					tr( "%1 does not appear to be a valid %2 file" ).arg( f->fullName(), f->extension() ),
					QMessageBox::Ok, QMessageBox::NoButton );
				m_pphMutex.unlock();
				return;
			}
			m_previewPlayHandle = new PresetPreviewPlayHandle( f->fullName(), f->handling() == FileItem::LoadByPlugin, &dataFile );
		}
		if( m_previewPlayHandle != NULL )
		{
			if( !Engine::mixer()->addPlayHandle(
							m_previewPlayHandle ) )
			{
				m_previewPlayHandle = NULL;
			}
		}
		m_pphMutex.unlock();
	}
}




void FileBrowserTreeWidget::mouseMoveEvent( QMouseEvent * me )
{
	if( m_mousePressed == true &&
		( m_pressPos - me->pos() ).manhattanLength() >
					QApplication::startDragDistance() )
	{
		// make sure any playback is stopped
		mouseReleaseEvent( NULL );

		FileItem * f = dynamic_cast<FileItem *>( itemAt( m_pressPos ) );
		if( f != NULL )
		{
			switch( f->type() )
			{
				case FileItem::PresetFile:
					new StringPairDrag( f->handling() == FileItem::LoadAsPreset ?
							"presetfile" : "pluginpresetfile",
							f->fullName(),
							embed::getIconPixmap( "preset_file" ), this );
					break;

				case FileItem::SampleFile:
					new StringPairDrag( "samplefile", f->fullName(),
							embed::getIconPixmap( "sample_file" ), this );
					break;
				case FileItem::SoundFontFile:
					new StringPairDrag( "soundfontfile", f->fullName(),
							embed::getIconPixmap( "soundfont_file" ), this );
					break;
				case FileItem::PatchFile:
					new StringPairDrag( "patchfile", f->fullName(),
							embed::getIconPixmap( "sample_file" ), this );
					break;
				case FileItem::VstPluginFile:
					new StringPairDrag( "vstpluginfile", f->fullName(),
							embed::getIconPixmap( "vst_plugin_file" ), this );
					break;
				case FileItem::MidiFile:
					new StringPairDrag( "importedproject", f->fullName(),
							embed::getIconPixmap( "midi_file" ), this );
					break;
				case FileItem::ProjectFile:
					new StringPairDrag( "projectfile", f->fullName(),
							embed::getIconPixmap( "project_file" ), this );
					break;

				default:
					break;
			}
		}
	}
}




void FileBrowserTreeWidget::mouseReleaseEvent(QMouseEvent * me )
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
				static_cast<f_cnt_t>( Engine::mixer()->
						processingSampleRate() * 3 ) )
			{
				s->setDoneMayReturnTrue( true );
				m_previewPlayHandle = NULL;
				m_pphMutex.unlock();
				return;
			}
		}
		Engine::mixer()->removePlayHandle( m_previewPlayHandle );
		m_previewPlayHandle = NULL;
	}
	m_pphMutex.unlock();
}





void FileBrowserTreeWidget::handleFile(FileItem * f, InstrumentTrack * it )
{
	Engine::mixer()->requestChangeInModel();
	switch( f->handling() )
	{
		case FileItem::LoadAsProject:
			if( gui->mainWindow()->mayChangeProject(true) )
			{
				Engine::getSong()->loadProject( f->fullName() );
			}
			break;

		case FileItem::LoadByPlugin:
		{
			const QString e = f->extension();
			Instrument * i = it->instrument();
			if( i == NULL ||
				!i->descriptor()->supportsFileType( e ) )
			{
				PluginFactory::PluginInfoAndKey piakn =
					pluginFactory->pluginSupportingExtension(e);
				i = it->loadInstrument(piakn.info.name(), &piakn.key);
			}
			i->loadFile( f->fullName() );
			break;
		}

		case FileItem::LoadAsPreset:
		{
			DataFile dataFile( f->fullName() );
			InstrumentTrack::removeMidiPortNode( dataFile );
			it->setSimpleSerializing();
			it->loadSettings( dataFile.content().toElement() );
			break;
		}

		case FileItem::ImportAsProject:
			ImportFilter::import( f->fullName(),
							Engine::getSong() );
			break;

		case FileItem::NotSupported:
		default:
			break;

	}
	Engine::mixer()->doneChangeInModel();
}




void FileBrowserTreeWidget::activateListItem(QTreeWidgetItem * item,
								int column )
{
	FileItem * f = dynamic_cast<FileItem *>( item );
	if( f == NULL )
	{
		return;
	}

	if( f->handling() == FileItem::LoadAsProject ||
		f->handling() == FileItem::ImportAsProject )
	{
		handleFile( f, NULL );
	}
	else if( f->handling() != FileItem::NotSupported )
	{
		InstrumentTrack * it = dynamic_cast<InstrumentTrack *>(
				Track::create( Track::InstrumentTrack,
					Engine::getBBTrackContainer() ) );
		handleFile( f, it );
	}
}




void FileBrowserTreeWidget::openInNewInstrumentTrack( TrackContainer* tc )
{
	if( m_contextMenuItem->handling() == FileItem::LoadAsPreset ||
		m_contextMenuItem->handling() == FileItem::LoadByPlugin )
	{
		InstrumentTrack * it = dynamic_cast<InstrumentTrack *>(
				Track::create( Track::InstrumentTrack, tc ) );
		handleFile( m_contextMenuItem, it );
	}
}




void FileBrowserTreeWidget::openInNewInstrumentTrackBBE( void )
{
	openInNewInstrumentTrack( Engine::getBBTrackContainer() );
}




void FileBrowserTreeWidget::openInNewInstrumentTrackSE( void )
{
	openInNewInstrumentTrack( Engine::getSong() );
}




void FileBrowserTreeWidget::sendToActiveInstrumentTrack( void )
{
	// get all windows opened in the workspace
	QList<QMdiSubWindow*> pl =
			gui->mainWindow()->workspace()->
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




void FileBrowserTreeWidget::updateDirectory(QTreeWidgetItem * item )
{
	Directory * dir = dynamic_cast<Directory *>( item );
	if( dir != NULL )
	{
		dir->update();
	}
}






QPixmap * Directory::s_folderPixmap = NULL;
QPixmap * Directory::s_folderOpenedPixmap = NULL;
QPixmap * Directory::s_folderLockedPixmap = NULL;


Directory::Directory(const QString & filename, const QString & path,
						const QString & filter ) :
	QTreeWidgetItem( QStringList( filename ), TypeDirectoryItem ),
	m_directories( path ),
	m_filter( filter ),
	m_dirCount( 0 )
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




void Directory::initPixmaps( void )
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




void Directory::update( void )
{
	if( !isExpanded() )
	{
		setIcon( 0, *s_folderPixmap );
		return;
	}

	setIcon( 0, *s_folderOpenedPixmap );
	if( !childCount() )
	{
		m_dirCount = 0;
		for( QStringList::iterator it = m_directories.begin();
					it != m_directories.end(); ++it )
		{
			int top_index = childCount();
			if( addItems( fullName( *it ) ) &&
				( *it ).contains(
					ConfigManager::inst()->dataDir() ) )
			{
				QTreeWidgetItem * sep = new QTreeWidgetItem;
				sep->setText( 0,
					FileBrowserTreeWidget::tr(
						"--- Factory files ---" ) );
				sep->setIcon( 0, embed::getIconPixmap(
							"factory_files" ) );
				insertChild(  m_dirCount + top_index, sep );
			}
		}
	}
}




bool Directory::addItems(const QString & path )
{
	QDir thisDir( path );
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
				Directory * d = dynamic_cast<Directory *>(
								child( i ) );
				if( d == NULL || cur_file < d->text( 0 ) )
				{
					insertChild( i, new Directory( cur_file,
							path, m_filter ) );
					orphan = false;
					m_dirCount++;
					break;
				}
				else if( cur_file == d->text( 0 ) )
				{
					d->addDirectory( path );
					orphan = false;
					break;
				}
			}
			if( orphan )
			{
				addChild( new Directory( cur_file, path,
								m_filter ) );
				m_dirCount++;
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
			items << new FileItem( cur_file, path );
			added_something = true;
		}
	}
	addChildren( items );

	treeWidget()->setUpdatesEnabled( true );

	return added_something;
}




QPixmap * FileItem::s_projectFilePixmap = NULL;
QPixmap * FileItem::s_presetFilePixmap = NULL;
QPixmap * FileItem::s_sampleFilePixmap = NULL;
QPixmap * FileItem::s_soundfontFilePixmap = NULL;
QPixmap * FileItem::s_vstPluginFilePixmap = NULL;
QPixmap * FileItem::s_midiFilePixmap = NULL;
QPixmap * FileItem::s_unknownFilePixmap = NULL;


FileItem::FileItem(QTreeWidget * parent, const QString & name,
						const QString & path ) :
	QTreeWidgetItem( parent, QStringList( name) , TypeFileItem ),
	m_path( path )
{
	determineFileType();
	initPixmaps();
}




FileItem::FileItem(const QString & name, const QString & path ) :
	QTreeWidgetItem( QStringList( name ), TypeFileItem ),
	m_path( path )
{
	determineFileType();
	initPixmaps();
}




void FileItem::initPixmaps( void )
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
		case UnknownFile:
		default:
			setIcon( 0, *s_unknownFilePixmap );
			break;
	}
}




void FileItem::determineFileType( void )
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
	else if( ext == "xiz" && ! pluginFactory->pluginSupportingExtension(ext).isNull() )
	{
		m_type = PresetFile;
		m_handling = LoadByPlugin;
	}
	else if( ext == "sf2" || ext == "sf3" )
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
		!ext.isEmpty() && ! pluginFactory->pluginSupportingExtension(ext).isNull() )
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




QString FileItem::extension( void )
{
	return extension( fullName() );
}




QString FileItem::extension(const QString & file )
{
	return QFileInfo( file ).suffix().toLower();
}




