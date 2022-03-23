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


#include <QApplication>
#include <QCheckBox>
#include <QCollator>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QToolButton>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QShortcut>
#include <QStringList>

#include "FileBrowser.h"
#include "AudioEngine.h"
#include "ConfigManager.h"
#include "DataFile.h"
#include "embed.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "ImportFilter.h"
#include "Instrument.h"
#include "InstrumentTrack.h"
#include "InstrumentTrackWindow.h"
#include "MainWindow.h"
#include "PatternStore.h"
#include "PluginFactory.h"
#include "PresetPreviewPlayHandle.h"
#include "SampleClip.h"
#include "SamplePlayHandle.h"
#include "SampleTrack.h"
#include "Song.h"
#include "StringPairDrag.h"
#include "TextFloat.h"




FileBrowser::FileBrowser(const QString& title, const QPixmap& pm, QWidget* parent):
	SideBarWidget(title, pm, parent)
{
	setWindowTitle( tr( "Browser" ) );

	// Search box

	m_searchBox = new QLineEdit(contentParent());
	m_searchBox->setPlaceholderText( tr("Search") );
	m_searchBox->setClearButtonEnabled( true );
	connect(m_searchBox, SIGNAL(textEdited(const QString&)), this, SLOT(onSearch(const QString&)));


	// File browser

	m_tree = new FileBrowserTreeWidget(this);

	// Whenever the FileBrowser has focus, Ctrl+F should direct focus to its search box.
	QShortcut *filterFocusShortcut = new QShortcut( QKeySequence( QKeySequence::Find ), this, SLOT(giveFocusToFilter()) );
	filterFocusShortcut->setContext(Qt::WidgetWithChildrenShortcut);

	// Used to load directory content when expanded
	connect(m_tree, SIGNAL(itemExpanded(QTreeWidgetItem*)), this, SLOT(onItemExpand(QTreeWidgetItem*)));
	// Used to change icon of directories from open to closed
	connect(m_tree, SIGNAL(itemCollapsed(QTreeWidgetItem*)), this, SLOT(onItemExpand(QTreeWidgetItem*)));


	// Check boxes

	auto checkBoxWidget = new QWidget(contentParent());
	checkBoxWidget->setMaximumHeight(24);
	auto checkBoxLayout = new QHBoxLayout(checkBoxWidget);
	checkBoxLayout->setMargin(0);
	checkBoxLayout->setSpacing(0);

	auto createCheckBox = [&](const QString& iconName, const QString& title, bool checked)
	{
		auto box = new QCheckBox(checkBoxWidget);
		box->setIcon(embed::getIconPixmap(iconName, 16, 16));
		box->setToolTip(title);
		box->setChecked(checked);
		box->hide();
		checkBoxLayout->addWidget(box);
		connect(box, SIGNAL(stateChanged(int)), this, SLOT(filterItems()));
		return box;
	};

	m_userCheckBox = createCheckBox("user_files", tr("User content"), true);
	m_factoryCheckBox = createCheckBox("factory_files", tr("Factory content"), true);
	m_backupCheckBox = createCheckBox("backup_files", tr("Backups"), true);
	m_unknownCheckBox = createCheckBox("unknown_file", tr("Unknown files"), false);
	m_hiddenCheckBox = createCheckBox("hidden_files", tr("Hidden files"), false);

	checkBoxLayout->addStretch();

	QToolButton* reload_btn = new QToolButton(contentParent());
	reload_btn->setIcon(embed::getIconPixmap("reload"));
	reload_btn->setToolTip( tr( "Refresh list" ) );
	connect( reload_btn, SIGNAL( clicked() ), this, SLOT( reloadTree() ) );
	checkBoxLayout->addWidget(reload_btn);


	addContentWidget(m_searchBox);
	addContentWidget(m_tree);
	addContentWidget(checkBoxWidget);
}




/*! \brief Filter the tree items
 *
 *  \param parentDir - item to start filtering from
 *  \param search - if items should be filtered by the string in the search box
 *  \return bool - true if any items matched the search string (only if search is true)
 */
bool FileBrowser::filterItems(TreeItem* parentDir, bool search)
{
	bool searchResultFound = false;

	int numChildren = parentDir ? parentDir->childCount() : m_tree->topLevelItemCount();
	bool showUser = m_userCheckBox->isChecked();
	bool showFactory = m_factoryCheckBox->isChecked();
	QStringList searchTerms = m_searchBox->text().split(" ", QString::KeepEmptyParts);
	QString previousFileName;

	// Lambda to check if a string contains all words from a list
	auto containsAll = [](const QString& string, const QStringList& words){
		for (const QString& word: words)
		{
			if (!string.contains(word, Qt::CaseInsensitive))
			{
				return false;
			}
		}
		return true;
	};

	for( int i = 0; i < numChildren; ++i )
	{
		auto item = static_cast<TreeItem*>(parentDir ? parentDir->child(i) : m_tree->topLevelItem(i));
		item->setHidden(true);

		// User/factory filter
		if (!(item->isUser() && showUser)	&& !(item->isFactory() && showFactory)) { continue; }

		// Hidden dir/file filter
		if (!m_hiddenCheckBox->isChecked() && item->isHidden()) { continue; }

		FileItem* file = dynamic_cast<FileItem*>(item);
		if (file)
		{
			// There may be two items in the list with the same name (user/factory)
			// In that case we only show the first item (user - they are always sorted that way)
			if (file->text(0) == previousFileName) { continue; }
			previousFileName = file->text(0);
			// Backup filter
			if (!m_backupCheckBox->isChecked() && file->type() == FileItem::ProjectBackupFile) { continue; }
			// Unsupported file filter
			if (!m_unknownCheckBox->isChecked() && file->type() == FileItem::UnknownFile) { continue; }
		}

		// Whether child items will be filtered through the search term
		bool searchChildren = search;

		// Search filter
		if (search && !searchTerms.isEmpty())
		{
			if (containsAll(item->text(0), searchTerms))
			{
				// If the name matches, show the item and all of it's children
				item->setHidden(false);
				searchChildren = false;
				searchResultFound = true;
			}
			// Do first-time loading of directory content if recursive search is enabled
			if (m_recursiveSearch && item->isDirectory() && item->childCount() == 0)
			{
				dynamic_cast<Directory*>(item)->addDirectoryContent();
			}
		}
		else
		{
			// We are not searching and all tests passed, so item should be visible
			item->setHidden(false);
		}

		// Continue filtering children recursively
		if (item->childCount() != 0)
		{
			if (filterItems(item, searchChildren))
			{
				// If we are searching and one of the children matched, we show the parent
				item->setHidden(false);
				item->setExpanded(true);
				searchResultFound = true;
			}
		}
	}

	return searchResultFound;
}


void FileBrowser::reloadTree( void )
{
	QList<QString> expandedDirs = m_tree->expandedDirs();
	m_tree->clear();

	for (const QString& dir: m_toplevelDirectories)
	{
		m_tree->addTopLevelItem(new Directory(dir, dir, ""));
	}

	for (TreeItem* item: Directory("", m_userDir, m_factoryDir).getDirectoryContent())
	{
		m_tree->addTopLevelItem(item);
	}
	expandItems(nullptr, expandedDirs);
	filterItems();
}



void FileBrowser::expandItems( QTreeWidgetItem * item, QList<QString> expandedDirs )
{
	if (item == nullptr) { m_tree->collapseAll(); }

	int numChildren = item ? item->childCount() : m_tree->topLevelItemCount();
	for (int i = 0; i < numChildren; ++i)
	{
		Directory* child = dynamic_cast<Directory*>(item ? item->child(i) : m_tree->topLevelItem(i));
		if (!child) { break; } // we can break because directories are always sorted before files
		if (!expandedDirs.contains(child->path())) { continue; }

		// (this will populate directories on the first run)
		child->setExpanded(true);

		// Continue expanding recursively
		if (child->childCount() != 0)
		{
			expandItems(child, expandedDirs);
		}
	}
}



void FileBrowser::giveFocusToFilter()
{
	if (!m_searchBox->hasFocus())
	{
		// give focus to filter text box and highlight its text for quick editing if not previously focused
		m_searchBox->setFocus();
		m_searchBox->selectAll();
	}
}



/* TODO remove
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
	files.sort(Qt::CaseInsensitive);
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
*/




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




void FileBrowser::onItemExpand(QTreeWidgetItem * item)
{
	// Only works on directories
	Directory* dir = dynamic_cast<Directory*>(item);
	if (!dir) { return; }

	// Populate directory when it is expanded for the first time
	if (dir->isExpanded() && dir->childCount() == 0)
	{
		dir->addDirectoryContent();
		// During search, only directories that match the search term are shown (as collapsed)
		// If one of those directories gets expanded we want to show all of its content
		filterItems(dir, /*enableSearchFilter = */ false);
	}

	dir->updateIcon(m_userCheckBox->isChecked(), m_factoryCheckBox->isChecked());
}




void FileBrowser::onSearch(const QString &filter)
{
	if (!m_isSearching && !filter.isEmpty())
	{
		m_expandedDirsPriorToSearch = m_tree->expandedDirs();
		m_tree->collapseAll();
	}
	else if (m_isSearching && filter.isEmpty())
	{
		expandItems(nullptr, m_expandedDirsPriorToSearch);
		m_expandedDirsPriorToSearch.clear();
	}
	m_isSearching = !m_searchBox->text().isEmpty();

	filterItems();
}



void FileBrowser::enableBackupFilter()
{
	m_backupCheckBox->setVisible(true);
	// By default backups are visibile, but when given a choice we default to hidden
	m_backupCheckBox->setChecked(false);
}




void FileBrowser::setDirectory(const QString& dir)
{
	m_userDir = dir;
	m_factoryDir.clear();
	m_toplevelDirectories.clear();
}




void FileBrowser::setDirectories(const QFileInfoList& dirList)
{
	m_userDir.clear();
	m_factoryDir.clear();
	m_toplevelDirectories.clear();
	for (auto const& dir: dirList)
	{
		m_toplevelDirectories += dir.absolutePath();
	}
}




void FileBrowser::setUserFactoryDir(const QString& userDir, const QString& factoryDir)
{
	m_userDir = userDir;
	m_factoryDir = factoryDir;
	m_toplevelDirectories.clear();
	m_userCheckBox->setVisible(true);
	m_factoryCheckBox->setVisible(true);
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

		if (it->isExpanded())
		{
			Directory *d = static_cast<Directory *> ( it );
			dirs.append(d->path());
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
		// ...to the song editor by default, or to the pattern editor if ctrl is held
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




void FileBrowser::showEvent(QShowEvent* se)
{
	// Load the tree the first time it becomes visible
	if (m_tree->topLevelItemCount() == 0) { reloadTree(); }
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

		QAction* patternEditorHeader = new QAction(tr("Pattern Editor"), nullptr);
		patternEditorHeader->setDisabled(true);
		contextMenu.addAction(patternEditorHeader);
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
	QTreeWidgetItem * i = itemAt(me->pos());
	QTreeWidget::mousePressEvent(me);
	// QTreeWidget handles right clicks for us, so we only care about left clicks
	if(me->button() != Qt::LeftButton) { return; }

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
		&& !getPluginFactory()->pluginSupportingExtension(ext).isNull())
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
		if (Engine::audioEngine()->addPlayHandle(newPPH))
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
		Engine::audioEngine()->removePlayHandle(m_previewPlayHandle);
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
	Engine::audioEngine()->requestChangeInModel();
	switch( f->handling() )
	{
		case FileItem::LoadAsProject:
			if( getGUI()->mainWindow()->mayChangeProject(true) )
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
					getPluginFactory()->pluginSupportingExtension(e);
				i = it->loadInstrument(piakn.info.name(), &piakn.key);
			}
			i->loadFile( f->fullName() );
			break;
		}

		case FileItem::LoadAsPreset: {
			DataFile dataFile(f->fullName());
			it->replaceInstrument(dataFile);
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
	Engine::audioEngine()->doneChangeInModel();
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
			Track::create(Track::InstrumentTrack, Engine::patternStore())
		);
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
	if (!songEditor) { tc = Engine::patternStore(); }
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
	Engine::audioEngine()->requestChangeInModel();
	SampleClip* clip = static_cast<SampleClip*>(sampleTrack->createClip(0));
	clip->setSampleFile(item->fullName());
	Engine::audioEngine()->doneChangeInModel();
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
			getGUI()->mainWindow()->workspace()->
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








bool TreeItem::lessThan(const TreeItem* first, const TreeItem* second)
{
	static QCollator collator = QCollator();
	collator.setNumericMode(true);

	// 1. Sort directories before files
	if (first->isDirectory() != second->isDirectory())
	{
		return first->isDirectory() > second->isDirectory();
	}
	// 2. Sort by name
	if (first->text(0) != second->text(0))
	{
		return collator.compare(first->text(0), second->text(0)) < 0;
	}
	// 3. Sort user files before factory files (this is necessary for filtering to work correctly)
	return first->isFactory() < second->isFactory();
}






QPixmap * Directory::s_folderPixmap = nullptr;
QPixmap * Directory::s_folderOpenedPixmap = nullptr;
QPixmap * Directory::s_folderLockedPixmap = nullptr;


Directory::Directory(const QString& name, const QString& userPath, const QString& factoryPath) :
	TreeItem(name),
	m_userPath(userPath),
	m_factoryPath(factoryPath)
{
	initPixmaps();

	setChildIndicatorPolicy( QTreeWidgetItem::ShowIndicator );

	updateIcon();
	m_hidden = QFileInfo(path()).isHidden();
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




void Directory::updateIcon(bool user, bool factory)
{
	if (QFileInfo(path(user, factory)).isReadable())
	{
		setIcon(0, isExpanded() ? *s_folderOpenedPixmap : *s_folderPixmap);
	}
	else
	{
		setIcon(0, *s_folderLockedPixmap);
	}
}




std::vector<TreeItem*> Directory::getDirectoryContent()
{
	std::vector<TreeItem*> items;

	// Map of directory names and their full user/factory path (if it exists)
	// we use this to later create a single Directory object of the both paths
	QMap<QString, std::pair<QString, QString>> dirs;

	// Lambda used to read a directory and store the paths in the appropriate lists
	auto parseDir = [&](const QString& path, bool isFactory)
	{
		if (path.isEmpty()) { return; }
		static auto filter = QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden;
		for (const QFileInfo& item: QDir(path).entryInfoList(filter))
		{
			if (item.isDir())
			{
				if (!isFactory)
				{
					dirs[item.fileName()].first = item.absoluteFilePath();
				}
				else
				{
					dirs[item.fileName()].second = item.absoluteFilePath();
				}
			}
			else
			{
				items.push_back(new FileItem(item.fileName(), item.absoluteFilePath(), isFactory));
			}
		}
	};

	parseDir(m_userPath, /* fromFactory = */ false);
	parseDir(m_factoryPath, /* fromFactory = */ true);

	// Create DirItems from the pair of paths we collected earlier
	for (auto it = dirs.cbegin(); it != dirs.cend(); it++)
	{
		items.push_back(new Directory(it.key(), it.value().first, it.value().second));
	}

	std::sort(items.begin(), items.end(), TreeItem::lessThan);

	return items;
}




void Directory::addDirectoryContent()
{
	for (TreeItem* item: getDirectoryContent())
	{
		addChild(item);
	}
}




QString Directory::path(bool user, bool factory) const
{
	if (user && !m_userPath.isEmpty()) { return m_userPath; }
	if (factory && !m_factoryPath.isEmpty()) { return m_factoryPath; }
	return QString();
}




QPixmap * FileItem::s_projectFilePixmap = nullptr;
QPixmap * FileItem::s_presetFilePixmap = nullptr;
QPixmap * FileItem::s_sampleFilePixmap = nullptr;
QPixmap * FileItem::s_soundfontFilePixmap = nullptr;
QPixmap * FileItem::s_vstPluginFilePixmap = nullptr;
QPixmap * FileItem::s_midiFilePixmap = nullptr;
QPixmap * FileItem::s_unknownFilePixmap = nullptr;


FileItem::FileItem(const QString& name, const QString& path, bool fromFactory) :
	TreeItem(name),
	m_factory(fromFactory),
	m_path( path )
{
	determineFileType();
	initPixmaps();
	m_hidden = QFileInfo(path).isHidden();
}








void FileItem::initPixmaps( void )
{
	static const auto backupPixmap = new QPixmap(embed::getIconPixmap("backup_file", 16, 16));

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
		case ProjectBackupFile:
			setIcon(0, *backupPixmap);
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
	else if (text(0).endsWith(".mmp.bak", Qt::CaseInsensitive) || text(0).endsWith(".mmpz.bak", Qt::CaseInsensitive))
	{
		m_type = ProjectBackupFile;
		m_handling = LoadAsProject;
	}
	else if( ext == "xpf" || ext == "xml" )
	{
		m_type = PresetFile;
		m_handling = LoadAsPreset;
	}
	else if( ext == "xiz" && ! getPluginFactory()->pluginSupportingExtension(ext).isNull() )
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
	else if( ext == "dll"
#ifdef LMMS_BUILD_LINUX
		|| ext == "so" 
#endif
	)
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
		!ext.isEmpty() && ! getPluginFactory()->pluginSupportingExtension(ext).isNull() )
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
	return extension(text(0));
}




QString FileItem::extension(const QString & file )
{
	return QFileInfo( file ).suffix().toLower();
}
