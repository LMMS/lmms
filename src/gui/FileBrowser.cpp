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


#include <QDesktopServices>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QShortcut>
#include <QStringList>

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
#include "SampleTrack.h"
#include "Song.h"
#include "StringPairDrag.h"
#include "TextFloat.h"

enum TreeWidgetItemTypes
{
	TypeFileItem = QTreeWidgetItem::UserType,
	TypeDirectoryItem
} ;



void FileBrowser::addContentCheckBox()
{
	auto filterWidget = new QWidget(contentParent());
	filterWidget->setFixedHeight(15);
	auto filterWidgetLayout = new QHBoxLayout(filterWidget);
	filterWidgetLayout->setMargin(0);
	filterWidgetLayout->setSpacing(0);

	auto configCheckBox = [this, &filterWidgetLayout](QCheckBox* box)
	{
		box->setCheckState(Qt::Checked);
		connect(box, SIGNAL(stateChanged(int)), this, SLOT(reloadTree()));
		filterWidgetLayout->addWidget(box);
	};

	m_showUserContent = new QCheckBox(tr("User content"));
	configCheckBox(m_showUserContent);
	m_showFactoryContent = new QCheckBox(tr("Factory content"));
	configCheckBox(m_showFactoryContent);

	addContentWidget(filterWidget);
};


FileBrowser::FileBrowser(const QString & directories, const QString & filter,
			const QString & title, const QPixmap & pm,
			QWidget * parent, bool dirs_as_items, bool recurse,
			const QString& userDir,
			const QString& factoryDir):
	SideBarWidget( title, pm, parent ),
	m_directories( directories ),
	m_filter( filter ),
	m_dirsAsItems( dirs_as_items ),
	m_recurse( recurse ),
	m_userDir(userDir),
	m_factoryDir(factoryDir)
{
	setWindowTitle( tr( "Browser" ) );

	if (!userDir.isEmpty() && !factoryDir.isEmpty())
	{
		addContentCheckBox();
	}

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
	QStringList paths = m_directories.split('*');
	if (m_showUserContent && !m_showUserContent->isChecked())
	{
		paths.removeAll(m_userDir);
	}
	if (m_showFactoryContent && !m_showFactoryContent->isChecked())
	{
		paths.removeAll(m_factoryDir);
	}

	if (!paths.isEmpty())
	{
		for (QStringList::iterator it = paths.begin(); it != paths.end(); ++it)
		{
			addItems(*it);
		}
	}
	expandItems(nullptr, expandedDirs);
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

	// try to add all directories from file system alphabetically into the tree
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
				if( d == nullptr || cur_file < d->text( 0 ) )
				{
					// insert before item, we're done
					Directory *dd = new Directory( cur_file, path,
												   m_filter );
					m_fileBrowserTreeWidget->insertTopLevelItem( i,dd );
					dd->update(); // add files to the directory
					orphan = false;
					break;
				}
				else if( cur_file == d->text( 0 ) )
				{
					// imagine we have subdirs named "TripleOscillator/xyz" in
					// two directories from m_directories
					// then only add one tree widget for both
					// so we don't add a new Directory - we just
					// add the path to the current directory
					d->addDirectory( path );
					d->update();
					orphan = false;
					break;
				}
			}
			if( orphan )
			{
				// it has not yet been added yet, so it's (lexically)
				// larger than all other dirs => append it at the bottom
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
	switch( ke->key() ){
		case Qt::Key_F5:
			reloadTree();
			break;
		default:
			ke->ignore();
	}
}








FileBrowserTreeWidget::FileBrowserTreeWidget(QWidget * parent ) :
	QTreeWidget( parent ),
	m_mousePressed( false ),
	m_pressPos(),
	m_previewPlayHandle( nullptr ),
	m_pphMutex( QMutex::Recursive )
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

#if QT_VERSION < QT_VERSION_CHECK(5, 12, 2) && defined LMMS_BUILD_WIN32
	// Set the font for the QTreeWidget to the Windows System font to make sure that
	// truncated (elided) items use the same font as non-truncated items.
	// This is a workaround for this qt bug, fixed in 5.12.2: https://bugreports.qt.io/browse/QTBUG-29232
	// TODO: remove this when all builds use a recent enough version of qt.
	setFont( GuiApplication::getWin32SystemFont() );
#endif
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




void FileBrowserTreeWidget::keyPressEvent(QKeyEvent * ke )
{
	// Shorter names for some commonly used properties of the event
	const auto key = ke->key();
	const bool vertical   = (key == Qt::Key_Up    || key == Qt::Key_Down);
	const bool horizontal = (key == Qt::Key_Left  || key == Qt::Key_Right);
	const bool insert     = (key == Qt::Key_Enter || key == Qt::Key_Return);
	const bool preview    = (key == Qt::Key_Space);

	// First of all, forward all keypresses
	QTreeWidget::keyPressEvent(ke);
	// Then, ignore all autorepeats (they would spam new tracks or previews)
	if (ke->isAutoRepeat()) { return; }
	// We should stop any running previews before we do anything new
	else if (vertical || horizontal || preview || insert) { stopPreview(); }

	// Try to get the currently selected item as a FileItem
	FileItem * file = dynamic_cast<FileItem *>(currentItem());
	// If it's null (folder, separator, etc.), there's nothing left for us to do
	if (file == nullptr) { return; }

	// When moving to a new sound, preview it. Skip presets, they can play forever
	if (vertical && file->type() == FileItem::SampleFile)
	{
		previewFileItem(file);
	}

	// When enter is pressed, add the selected item...
	if (insert)
	{
		// ...to the song editor by default, or to the BB editor if ctrl is held
		bool songEditor = !(ke->modifiers() & Qt::ControlModifier);
		// If shift is held, we send the item to a new sample track...
		bool sampleTrack = ke->modifiers() & Qt::ShiftModifier;
		// ...but only in the song editor. So, ctrl+shift enter does nothing
		if (sampleTrack && songEditor){ openInNewSampleTrack(file); }
		// Otherwise we send the item as a new instrument track
		else if (!sampleTrack){ openInNewInstrumentTrack(file, songEditor); }
	}

	// When space is pressed, start a preview of the selected item
	if (preview) { previewFileItem(file); }
}




void FileBrowserTreeWidget::keyReleaseEvent(QKeyEvent* ke)
{
	// Cancel previews when the space key is released
	if (ke->key() == Qt::Key_Space && !ke->isAutoRepeat()) { stopPreview(); }
}




void FileBrowserTreeWidget::hideEvent(QHideEvent* he)
{
	// Cancel previews when the user switches tabs or hides the sidebar
	stopPreview();
	QTreeWidget::hideEvent(he);
}




void FileBrowserTreeWidget::focusOutEvent(QFocusEvent* fe)
{
	// Cancel previews when the user clicks outside the browser
	stopPreview();
	QTreeWidget::focusOutEvent(fe);
}




void FileBrowserTreeWidget::contextMenuEvent(QContextMenuEvent * e )
{
	FileItem * file = dynamic_cast<FileItem *>( itemAt( e->pos() ) );
	if( file != nullptr && file->isTrack() )
	{
		QMenu contextMenu( this );

		contextMenu.addAction(
			tr( "Send to active instrument-track" ),
			[=]{ sendToActiveInstrumentTrack(file); }
		);

		contextMenu.addSeparator();

		contextMenu.addAction(
			QIcon(embed::getIconPixmap("folder")),
			tr("Open containing folder"),
			[=]{ openContainingFolder(file); }
		);

		QAction* songEditorHeader = new QAction( tr("Song Editor"), nullptr );
		songEditorHeader->setDisabled(true);
		contextMenu.addAction( songEditorHeader );
		contextMenu.addActions( getContextActions(file, true) );

		QAction* bbEditorHeader = new QAction( tr("BB Editor"), nullptr );
		bbEditorHeader->setDisabled(true);
		contextMenu.addAction( bbEditorHeader );
		contextMenu.addActions( getContextActions(file, false) );

		// We should only show the menu if it contains items
		if (!contextMenu.isEmpty()) { contextMenu.exec( e->globalPos() ); }
	}
}




QList<QAction*> FileBrowserTreeWidget::getContextActions(FileItem* file, bool songEditor)
{
	QList<QAction*> result = QList<QAction*>();
	const bool fileIsSample = file->type() == FileItem::SampleFile;

	QString instrumentAction = fileIsSample ?
		tr("Send to new AudioFileProcessor instance") :
		tr("Send to new instrument track");
	QString shortcutMod = songEditor ? "" : UI_CTRL_KEY + QString(" + ");

	QAction* toInstrument = new QAction(
		instrumentAction + tr(" (%2Enter)").arg(shortcutMod),
		nullptr
	);
	connect(toInstrument, &QAction::triggered,
		[=]{ openInNewInstrumentTrack(file, songEditor); });
	result.append(toInstrument);

	if (songEditor && fileIsSample)
	{
		QAction* toSampleTrack = new QAction(
			tr("Send to new sample track (Shift + Enter)"),
			nullptr
		);
		connect(toSampleTrack, &QAction::triggered,
			[=]{ openInNewSampleTrack(file); });
		result.append(toSampleTrack);
	}

	return result;
}




void FileBrowserTreeWidget::mousePressEvent(QMouseEvent * me )
{
	// Forward the event
	QTreeWidget::mousePressEvent(me);
	// QTreeWidget handles right clicks for us, so we only care about left clicks
	if(me->button() != Qt::LeftButton) { return; }

	QTreeWidgetItem * i = itemAt(me->pos());
	if (i)
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

	FileItem * f = dynamic_cast<FileItem *>(i);
	if(f != nullptr) { previewFileItem(f); }
}




void FileBrowserTreeWidget::previewFileItem(FileItem* file)
{	// TODO: We should do this work outside the event thread
	// Lock the preview mutex
	QMutexLocker previewLocker(&m_pphMutex);
	// If something is already playing, stop it before we continue
	stopPreview();

	PlayHandle* newPPH = nullptr;
	const QString fileName = file->fullName();
	const QString ext = file->extension();

	// In special case of sample-files we do not care about
	// handling() rather than directly creating a SamplePlayHandle
	if (file->type() == FileItem::SampleFile)
	{
		TextFloat * tf = TextFloat::displayMessage(
			tr("Loading sample"),
			tr("Please wait, loading sample for preview..."),
			embed::getIconPixmap("sample_file", 24, 24), 0);
		// TODO: this can be removed once we do this outside the event thread
		qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
		SamplePlayHandle* s = new SamplePlayHandle(fileName);
		s->setDoneMayReturnTrue(false);
		newPPH = s;
		delete tf;
	}
	else if (
		(ext == "xiz" || ext == "sf2" || ext == "sf3" ||
		 ext == "gig" || ext == "pat")
		&& !pluginFactory->pluginSupportingExtension(ext).isNull())
	{
		const bool isPlugin = file->handling() == FileItem::LoadByPlugin;
		newPPH = new PresetPreviewPlayHandle(fileName, isPlugin);
	}
	else if (file->type() != FileItem::VstPluginFile && file->isTrack())
	{
		DataFile dataFile(fileName);
		if (dataFile.validate(ext))
		{
			const bool isPlugin = file->handling() == FileItem::LoadByPlugin;
			newPPH = new PresetPreviewPlayHandle(fileName, isPlugin, &dataFile);
		}
		else
		{
			QMessageBox::warning(0, tr ("Error"),
				tr("%1 does not appear to be a valid %2 file")
				.arg(fileName, ext),
				QMessageBox::Ok, QMessageBox::NoButton);
		}
	}

	if (newPPH != nullptr)
	{
		if (Engine::mixer()->addPlayHandle(newPPH))
		{
			m_previewPlayHandle = newPPH;
		}
		else { m_previewPlayHandle = nullptr; }
	}
}




void FileBrowserTreeWidget::stopPreview()
{
	QMutexLocker previewLocker(&m_pphMutex);
	if (m_previewPlayHandle != nullptr)
	{
		Engine::mixer()->removePlayHandle(m_previewPlayHandle);
		m_previewPlayHandle = nullptr;
	}
}




void FileBrowserTreeWidget::mouseMoveEvent( QMouseEvent * me )
{
	if( m_mousePressed == true &&
		( m_pressPos - me->pos() ).manhattanLength() >
					QApplication::startDragDistance() )
	{
		// make sure any playback is stopped
		mouseReleaseEvent( nullptr );

		FileItem * f = dynamic_cast<FileItem *>( itemAt( m_pressPos ) );
		if( f != nullptr )
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

	// If a preview is running, we may need to stop it. Otherwise, we're done
	QMutexLocker previewLocker(&m_pphMutex);
	if (m_previewPlayHandle == nullptr) { return; }

	// Only sample previews may continue after mouse up. Is this a sample preview?
	bool isSample = m_previewPlayHandle->type() == PlayHandle::TypeSamplePlayHandle;
	// Even sample previews should only continue if the user wants them to. Do they?
	bool shouldContinue = ConfigManager::inst()->value("ui", "letpreviewsfinish").toInt();
	// If both are true the preview may continue, otherwise we stop it
	if (!(isSample && shouldContinue)) { stopPreview(); }
}




void FileBrowserTreeWidget::handleFile(FileItem * f, InstrumentTrack * it)
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
			if( i == nullptr ||
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
	if( f == nullptr )
	{
		return;
	}

	if( f->handling() == FileItem::LoadAsProject ||
		f->handling() == FileItem::ImportAsProject )
	{
		handleFile( f, nullptr );
	}
	else if( f->handling() != FileItem::NotSupported )
	{
		InstrumentTrack * it = dynamic_cast<InstrumentTrack *>(
				Track::create( Track::InstrumentTrack,
					Engine::getBBTrackContainer() ) );
		handleFile( f, it );
	}
}




void FileBrowserTreeWidget::openInNewInstrumentTrack(TrackContainer* tc, FileItem* item)
{
	if(item->isTrack())
	{
		InstrumentTrack * it = dynamic_cast<InstrumentTrack *>(
				Track::create(Track::InstrumentTrack, tc));
		handleFile(item, it);
	}
}




void FileBrowserTreeWidget::openInNewInstrumentTrack(FileItem* item, bool songEditor)
{
	// Get the correct TrackContainer. Ternary doesn't compile here
	TrackContainer* tc = Engine::getSong();
	if (!songEditor) { tc = Engine::getBBTrackContainer(); }
	openInNewInstrumentTrack(tc, item);
}




bool FileBrowserTreeWidget::openInNewSampleTrack(FileItem* item)
{
	// Can't add non-samples to a sample track
	if (item->type() != FileItem::SampleFile) { return false; }

	// Create a new sample track for this sample
	SampleTrack* sampleTrack = static_cast<SampleTrack*>(
		Track::create(Track::SampleTrack, Engine::getSong()));

	// Add the sample clip to the track
	Engine::mixer()->requestChangeInModel();
	SampleTCO* clip = static_cast<SampleTCO*>(sampleTrack->createTCO(0));
	clip->setSampleFile(item->fullName());
	Engine::mixer()->doneChangeInModel();
	return true;
}




void FileBrowserTreeWidget::openContainingFolder(FileItem* item)
{
	// Delegate to QDesktopServices::openUrl with the directory of the selected file. Please note that
	// this will only open the directory but not select the file as this is much more complicated due
	// to different implementations that are needed for different platforms (Linux/Windows/MacOS).

	// Using QDesktopServices::openUrl seems to be the most simple cross platform way which uses
	// functionality that's already available in Qt.
	QFileInfo fileInfo(item->fullName());
	QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.dir().path()));
}




void FileBrowserTreeWidget::sendToActiveInstrumentTrack( FileItem* item )
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
		if( itw != nullptr && itw->isHidden() == false )
		{
			handleFile( item, itw->model() );
			break;
		}
	}
}




void FileBrowserTreeWidget::updateDirectory(QTreeWidgetItem * item )
{
	Directory * dir = dynamic_cast<Directory *>( item );
	if( dir != nullptr )
	{
		dir->update();
	}
}






QPixmap * Directory::s_folderPixmap = nullptr;
QPixmap * Directory::s_folderOpenedPixmap = nullptr;
QPixmap * Directory::s_folderLockedPixmap = nullptr;


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
	if( s_folderPixmap == nullptr )
	{
		s_folderPixmap = new QPixmap(
					embed::getIconPixmap( "folder" ) );
	}

	if( s_folderOpenedPixmap == nullptr )
	{
		s_folderOpenedPixmap = new QPixmap(
				embed::getIconPixmap( "folder_opened" ) );
	}

	if( s_folderLockedPixmap == nullptr )
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
		// for all paths leading here, add their items
		for( QStringList::iterator it = m_directories.begin();
					it != m_directories.end(); ++it )
		{
			int filesBeforeAdd = childCount() - m_dirCount;
			if( addItems( fullName( *it ) ) &&
				( *it ).contains(
					ConfigManager::inst()->dataDir() ) )
			{
				// factory file directory is added
				// note: those are always added last
				int filesNow = childCount() - m_dirCount;
				if(filesNow > filesBeforeAdd) // any file appended?
				{
					QTreeWidgetItem * sep = new QTreeWidgetItem;
					sep->setText( 0,
						FileBrowserTreeWidget::tr(
							"--- Factory files ---" ) );
					sep->setIcon( 0, embed::getIconPixmap(
								"factory_files" ) );
					// add delimeter after last file before appending our files
					insertChild( filesBeforeAdd + m_dirCount, sep );
				}
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

	// try to add all directories from file system alphabetically into the tree
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
				if( d == nullptr || cur_file < d->text( 0 ) )
				{
					// insert before item, we're done
					insertChild( i, new Directory( cur_file,
							path, m_filter ) );
					orphan = false;
					m_dirCount++;
					break;
				}
				else if( cur_file == d->text( 0 ) )
				{
					// imagine we have top-level subdirs named "TripleOscillator" in
					// two directories from FileBrowser::m_directories
					// and imagine both have a sub folder named "xyz"
					// then only add one tree widget for both
					// so we don't add a new Directory - we just
					// add the path to the current directory
					d->addDirectory( path );
					orphan = false;
					break;
				}
			}
			if( orphan )
			{
				// it has not yet been added yet, so it's (lexically)
				// larger than all other dirs => append it at the bottom
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




QPixmap * FileItem::s_projectFilePixmap = nullptr;
QPixmap * FileItem::s_presetFilePixmap = nullptr;
QPixmap * FileItem::s_sampleFilePixmap = nullptr;
QPixmap * FileItem::s_soundfontFilePixmap = nullptr;
QPixmap * FileItem::s_vstPluginFilePixmap = nullptr;
QPixmap * FileItem::s_midiFilePixmap = nullptr;
QPixmap * FileItem::s_unknownFilePixmap = nullptr;


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
	if( s_projectFilePixmap == nullptr )
	{
		s_projectFilePixmap = new QPixmap( embed::getIconPixmap(
						"project_file", 16, 16 ) );
	}

	if( s_presetFilePixmap == nullptr )
	{
		s_presetFilePixmap = new QPixmap( embed::getIconPixmap(
						"preset_file", 16, 16 ) );
	}

	if( s_sampleFilePixmap == nullptr )
	{
		s_sampleFilePixmap = new QPixmap( embed::getIconPixmap(
						"sample_file", 16, 16 ) );
	}

	if ( s_soundfontFilePixmap == nullptr )
	{
		s_soundfontFilePixmap = new QPixmap( embed::getIconPixmap(
						"soundfont_file", 16, 16 ) );
	}

	if ( s_vstPluginFilePixmap == nullptr )
	{
		s_vstPluginFilePixmap = new QPixmap( embed::getIconPixmap(
						"vst_plugin_file", 16, 16 ) );
	}

	if( s_midiFilePixmap == nullptr )
	{
		s_midiFilePixmap = new QPixmap( embed::getIconPixmap(
							"midi_file", 16, 16 ) );
	}

	if( s_unknownFilePixmap == nullptr )
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
	else if ( ext == "lv2" )
	{
		m_type = PresetFile;
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
