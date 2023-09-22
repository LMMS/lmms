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

namespace lmms::gui
{


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
	filterWidgetLayout->setContentsMargins(0, 0, 0, 0);
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

	auto searchWidget = new QWidget(contentParent());
	searchWidget->setFixedHeight( 24 );

	auto searchWidgetLayout = new QHBoxLayout(searchWidget);
	searchWidgetLayout->setContentsMargins(0, 0, 0, 0);
	searchWidgetLayout->setSpacing( 0 );

	m_filterEdit = new QLineEdit( searchWidget );
	m_filterEdit->setPlaceholderText( tr("Search") );
	m_filterEdit->setClearButtonEnabled( true );
	connect( m_filterEdit, SIGNAL( textEdited( const QString& ) ),
			this, SLOT( filterAndExpandItems( const QString& ) ) );

	auto reload_btn = new QPushButton(embed::getIconPixmap("reload"), QString(), searchWidget);
	reload_btn->setToolTip( tr( "Refresh list" ) );
	connect( reload_btn, SIGNAL(clicked()), this, SLOT(reloadTree()));

	searchWidgetLayout->addWidget( m_filterEdit );
	searchWidgetLayout->addSpacing( 5 );
	searchWidgetLayout->addWidget( reload_btn );

	addContentWidget( searchWidget );

	m_fileBrowserTreeWidget = new FileBrowserTreeWidget( contentParent() );
	addContentWidget( m_fileBrowserTreeWidget );

	// Whenever the FileBrowser has focus, Ctrl+F should direct focus to its filter box.
	auto filterFocusShortcut = new QShortcut(QKeySequence(QKeySequence::Find), this, SLOT(giveFocusToFilter()));
	filterFocusShortcut->setContext(Qt::WidgetWithChildrenShortcut);

	m_previousFilterValue = "";

	reloadTree();
	show();
}

void FileBrowser::saveDirectoriesStates()
{	
	m_savedExpandedDirs = m_fileBrowserTreeWidget->expandedDirs();
}
	
void FileBrowser::restoreDirectoriesStates()
{
	expandItems(nullptr, m_savedExpandedDirs);
}

bool FileBrowser::filterAndExpandItems(const QString & filter, QTreeWidgetItem * item)
{
	// Call with item = nullptr to filter the entire tree

	if (item == nullptr)
	{
		// First search character so need to save current expanded directories
		if (m_previousFilterValue.isEmpty())
		{
			saveDirectoriesStates();
		}

		m_previousFilterValue = filter;
	}

	if (filter.isEmpty())
	{
		// Restore previous expanded directories
		if (item == nullptr) 
		{
			restoreDirectoriesStates();
		}

		return false;
	}
	
	bool anyMatched = false;

	int numChildren = item ? item->childCount() : m_fileBrowserTreeWidget->topLevelItemCount();

	for (int i = 0; i < numChildren; ++i)
	{
		QTreeWidgetItem * it = item ? item->child( i ) : m_fileBrowserTreeWidget->topLevelItem(i);

		auto d = dynamic_cast<Directory*>(it);	
		if (d)
		{
			if (it->text(0).contains(filter, Qt::CaseInsensitive))
			{
				it->setHidden(false);
				it->setExpanded(true);
				filterAndExpandItems(QString(), it);
				anyMatched = true;
			}
			else
			{
				// Expanding is required when recursive to load in its contents, even if it's collapsed right afterward
				it->setExpanded(true);

				bool didMatch = filterAndExpandItems(filter, it);
				it->setHidden(!didMatch);
				it->setExpanded(didMatch);
				anyMatched = anyMatched || didMatch;
			}
		}

		else
		{
			auto f = dynamic_cast<FileItem*>(it);
			if (f)
			{
				// File
				bool didMatch = it->text(0).contains(filter, Qt::CaseInsensitive);
				it->setHidden(!didMatch);
				anyMatched = anyMatched || didMatch;
			}
			
			// A standard item (i.e. no file or directory item?)
			else
			{
				// Hide if there's any filter
				it->setHidden(!filter.isEmpty());
			}
		}
	}

	return anyMatched;
}


void FileBrowser::reloadTree()
{
	if (m_filterEdit->text().isEmpty())
	{
		saveDirectoriesStates();	
	}

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
		for (const auto& path : paths)
		{
			addItems(path);
		}
	}

	if (m_filterEdit->text().isEmpty())
	{
		restoreDirectoriesStates();
	}
	else
	{
		filterAndExpandItems(m_filterEdit->text());
	}
}



void FileBrowser::expandItems(QTreeWidgetItem* item, QList<QString> expandedDirs)
{
	int numChildren = item ? item->childCount() : m_fileBrowserTreeWidget->topLevelItemCount();
	for (int i = 0; i < numChildren; ++i)
	{
		auto it = item ? item->child(i) : m_fileBrowserTreeWidget->topLevelItem(i);
		auto d = dynamic_cast<Directory*>(it);
		if (d)
		{
			// Expanding is required when recursive to load in its contents, even if it's collapsed right afterward
			if (m_recurse) { d->setExpanded(true); }

			d->setExpanded(expandedDirs.contains(d->fullName()));

			if (m_recurse && it->childCount())
			{
				expandItems(it, expandedDirs);
			}
		}
		
		it->setHidden(false);		
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
				auto d = dynamic_cast<Directory*>(m_fileBrowserTreeWidget->topLevelItem(i));
				if( d == nullptr || cur_file < d->text( 0 ) )
				{
					// insert before item, we're done
					auto dd = new Directory(cur_file, path, m_filter);
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
				auto d = new Directory(cur_file, path, m_filter);
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
	m_previewPlayHandle( nullptr )
#if (QT_VERSION < QT_VERSION_CHECK(5,14,0))
	,m_pphMutex(QMutex::Recursive)
#endif
{
	setColumnCount( 1 );
	headerItem()->setHidden( true );
	setSortingEnabled( false );

	connect( this, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
			SLOT(activateListItem(QTreeWidgetItem*,int)));
	connect( this, SIGNAL(itemCollapsed(QTreeWidgetItem*)),
				SLOT(updateDirectory(QTreeWidgetItem*)));
	connect( this, SIGNAL(itemExpanded(QTreeWidgetItem*)),
				SLOT(updateDirectory(QTreeWidgetItem*)));

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
			auto d = static_cast<Directory*>(it);
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
	auto file = dynamic_cast<FileItem*>(currentItem());
	// If it's null (folder, separator, etc.), there's nothing left for us to do
	if (file == nullptr) { return; }

	// When moving to a new sound, preview it. Skip presets, they can play forever
	if (vertical && file->type() == FileItem::FileType::Sample)
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




void FileBrowserTreeWidget::focusOutEvent(QFocusEvent* fe)
{
	// Cancel previews when the user clicks outside the browser
	stopPreview();
	QTreeWidget::focusOutEvent(fe);
}




void FileBrowserTreeWidget::contextMenuEvent(QContextMenuEvent * e )
{
	auto file = dynamic_cast<FileItem*>(itemAt(e->pos()));
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

		auto songEditorHeader = new QAction(tr("Song Editor"), nullptr);
		songEditorHeader->setDisabled(true);
		contextMenu.addAction( songEditorHeader );
		contextMenu.addActions( getContextActions(file, true) );

		auto patternEditorHeader = new QAction(tr("Pattern Editor"), nullptr);
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
	const bool fileIsSample = file->type() == FileItem::FileType::Sample;

	QString instrumentAction = fileIsSample ?
		tr("Send to new AudioFileProcessor instance") :
		tr("Send to new instrument track");
	QString shortcutMod = songEditor ? "" : UI_CTRL_KEY + QString(" + ");

	auto toInstrument = new QAction(instrumentAction + tr(" (%2Enter)").arg(shortcutMod), nullptr);
	connect(toInstrument, &QAction::triggered,
		[=]{ openInNewInstrumentTrack(file, songEditor); });
	result.append(toInstrument);

	if (songEditor && fileIsSample)
	{
		auto toSampleTrack = new QAction(tr("Send to new sample track (Shift + Enter)"), nullptr);
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

	auto f = dynamic_cast<FileItem*>(i);
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
	if (file->type() == FileItem::FileType::Sample)
	{
		TextFloat * tf = TextFloat::displayMessage(
			tr("Loading sample"),
			tr("Please wait, loading sample for preview..."),
			embed::getIconPixmap("sample_file", 24, 24), 0);
		// TODO: this can be removed once we do this outside the event thread
		qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
		auto s = new SamplePlayHandle(fileName);
		s->setDoneMayReturnTrue(false);
		newPPH = s;
		delete tf;
	}
	else if (
		(ext == "xiz" || ext == "sf2" || ext == "sf3" ||
		 ext == "gig" || ext == "pat")
		&& !getPluginFactory()->pluginSupportingExtension(ext).isNull())
	{
		const bool isPlugin = file->handling() == FileItem::FileHandling::LoadByPlugin;
		newPPH = new PresetPreviewPlayHandle(fileName, isPlugin);
	}
	else if (file->type() != FileItem::FileType::VstPlugin && file->isTrack())
	{
		DataFile dataFile(fileName);
		if (dataFile.validate(ext))
		{
			const bool isPlugin = file->handling() == FileItem::FileHandling::LoadByPlugin;
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

		auto f = dynamic_cast<FileItem*>(itemAt(m_pressPos));
		if( f != nullptr )
		{
			switch( f->type() )
			{
				case FileItem::FileType::Preset:
					new StringPairDrag( f->handling() == FileItem::FileHandling::LoadAsPreset ?
							"presetfile" : "pluginpresetfile",
							f->fullName(),
							embed::getIconPixmap( "preset_file" ), this );
					break;

				case FileItem::FileType::Sample:
					new StringPairDrag( "samplefile", f->fullName(),
							embed::getIconPixmap( "sample_file" ), this );
					break;
				case FileItem::FileType::SoundFont:
					new StringPairDrag( "soundfontfile", f->fullName(),
							embed::getIconPixmap( "soundfont_file" ), this );
					break;
				case FileItem::FileType::Patch:
					new StringPairDrag( "patchfile", f->fullName(),
							embed::getIconPixmap( "sample_file" ), this );
					break;
				case FileItem::FileType::VstPlugin:
					new StringPairDrag( "vstpluginfile", f->fullName(),
							embed::getIconPixmap( "vst_plugin_file" ), this );
					break;
				case FileItem::FileType::Midi:
					new StringPairDrag( "importedproject", f->fullName(),
							embed::getIconPixmap( "midi_file" ), this );
					break;
				case FileItem::FileType::Project:
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
	bool isSample = m_previewPlayHandle->type() == PlayHandle::Type::SamplePlayHandle;
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
		case FileItem::FileHandling::LoadAsProject:
			if( getGUI()->mainWindow()->mayChangeProject(true) )
			{
				Engine::getSong()->loadProject( f->fullName() );
			}
			break;

		case FileItem::FileHandling::LoadByPlugin:
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

		case FileItem::FileHandling::LoadAsPreset: {
			DataFile dataFile(f->fullName());
			it->replaceInstrument(dataFile);
			break;
		}
		case FileItem::FileHandling::ImportAsProject:
			ImportFilter::import( f->fullName(),
							Engine::getSong() );
			break;

		case FileItem::FileHandling::NotSupported:
		default:
			break;

	}
	Engine::audioEngine()->doneChangeInModel();
}




void FileBrowserTreeWidget::activateListItem(QTreeWidgetItem * item,
								int column )
{
	auto f = dynamic_cast<FileItem*>(item);
	if( f == nullptr )
	{
		return;
	}

	if( f->handling() == FileItem::FileHandling::LoadAsProject ||
		f->handling() == FileItem::FileHandling::ImportAsProject )
	{
		handleFile( f, nullptr );
	}
	else if( f->handling() != FileItem::FileHandling::NotSupported )
	{
		auto it = dynamic_cast<InstrumentTrack*>(Track::create(Track::Type::Instrument, Engine::patternStore()));
		handleFile( f, it );
	}
}




void FileBrowserTreeWidget::openInNewInstrumentTrack(TrackContainer* tc, FileItem* item)
{
	if(item->isTrack())
	{
		auto it = dynamic_cast<InstrumentTrack*>(Track::create(Track::Type::Instrument, tc));
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
	if (item->type() != FileItem::FileType::Sample) { return false; }

	// Create a new sample track for this sample
	auto sampleTrack = static_cast<SampleTrack*>(Track::create(Track::Type::Sample, Engine::getSong()));

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
		auto itw = dynamic_cast<InstrumentTrackWindow*>(w.previous()->widget());
		if( itw != nullptr && itw->isHidden() == false )
		{
			handleFile( item, itw->model() );
			break;
		}
	}
}




void FileBrowserTreeWidget::updateDirectory(QTreeWidgetItem * item )
{
	auto dir = dynamic_cast<Directory*>(item);
	if( dir != nullptr )
	{
		dir->update();
	}
}

Directory::Directory(const QString & filename, const QString & path,
						const QString & filter ) :
	QTreeWidgetItem( QStringList( filename ), TypeDirectoryItem ),
	m_directories( path ),
	m_filter( filter ),
	m_dirCount( 0 )
{
	static auto s_folderPixmap = QPixmap{embed::getIconPixmap("folder")};
	static auto s_folderOpenedPixmap = QPixmap{embed::getIconPixmap("folder_opened")};
	static auto s_folderLockedPixmap = QPixmap{embed::getIconPixmap("folder_locked")};

	m_folderPixmap = &s_folderPixmap;
	m_folderOpenedPixmap = &s_folderOpenedPixmap;
	m_folderLockedPixmap = &s_folderLockedPixmap;

	setIcon(0, !QDir{fullName()}.isReadable() ? s_folderLockedPixmap : s_folderPixmap);
	setChildIndicatorPolicy( QTreeWidgetItem::ShowIndicator );
}

void Directory::update()
{
	if( !isExpanded() )
	{
		setIcon(0, *m_folderPixmap);
		return;
	}

	setIcon(0, *m_folderOpenedPixmap);
	if( !childCount() )
	{
		m_dirCount = 0;
		// for all paths leading here, add their items
		for (const auto& directory : m_directories)
		{
			int filesBeforeAdd = childCount() - m_dirCount;
			if(addItems(fullName(directory)) && directory.contains(ConfigManager::inst()->dataDir()))
			{
				// factory file directory is added
				// note: those are always added last
				int filesNow = childCount() - m_dirCount;
				if(filesNow > filesBeforeAdd) // any file appended?
				{
					auto sep = new QTreeWidgetItem;
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




bool Directory::addItems(const QString& path)
{
	QDir thisDir(path);
	if (!thisDir.isReadable()) { return false; }

	treeWidget()->setUpdatesEnabled(false);

	QFileInfoList entries = thisDir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::LocaleAware | QDir::DirsFirst | QDir::Name);
	for (auto& entry : entries)
	{
		QString fileName = entry.fileName();
		if (entry.isDir())
		{
			auto dir = new Directory(fileName, path, m_filter);
			addChild(dir);
			m_dirCount++;
		}
		else if (entry.isFile() && thisDir.match(m_filter, fileName.toLower()))
		{
			auto fileItem = new FileItem(fileName, path);
			addChild(fileItem);
		}
	}
	
	treeWidget()->setUpdatesEnabled(true);
	
	// return true if we added any child items
	return childCount() > 0;
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




void FileItem::initPixmaps()
{
	static auto s_projectFilePixmap = QPixmap{embed::getIconPixmap("project_file", 16, 16)};
	static auto s_presetFilePixmap = QPixmap{embed::getIconPixmap("preset_file", 16, 16)};
	static auto s_sampleFilePixmap = QPixmap{embed::getIconPixmap("sample_file", 16, 16)};
	static auto s_soundfontFilePixmap = QPixmap{embed::getIconPixmap("soundfont_file", 16, 16)};
	static auto s_vstPluginFilePixmap = QPixmap{embed::getIconPixmap("vst_plugin_file", 16, 16)};
	static auto s_midiFilePixmap = QPixmap{embed::getIconPixmap("midi_file", 16, 16)};
	static auto s_unknownFilePixmap = QPixmap{embed::getIconPixmap("unknown_file")};

	switch( m_type )
	{
		case FileType::Project:
			setIcon(0, s_projectFilePixmap);
			break;
		case FileType::Preset:
			setIcon(0, s_presetFilePixmap);
			break;
		case FileType::SoundFont:
			setIcon(0, s_soundfontFilePixmap);
			break;
		case FileType::VstPlugin:
			setIcon(0, s_vstPluginFilePixmap);
			break;
		case FileType::Sample:
		case FileType::Patch:			// TODO
			setIcon(0, s_sampleFilePixmap);
			break;
		case FileType::Midi:
			setIcon(0, s_midiFilePixmap);
			break;
		case FileType::Unknown:
		default:
			setIcon(0, s_unknownFilePixmap);
			break;
	}
}




void FileItem::determineFileType()
{
	m_handling = FileHandling::NotSupported;

	const QString ext = extension();
	if( ext == "mmp" || ext == "mpt" || ext == "mmpz" )
	{
		m_type = FileType::Project;
		m_handling = FileHandling::LoadAsProject;
	}
	else if( ext == "xpf" || ext == "xml" )
	{
		m_type = FileType::Preset;
		m_handling = FileHandling::LoadAsPreset;
	}
	else if( ext == "xiz" && ! getPluginFactory()->pluginSupportingExtension(ext).isNull() )
	{
		m_type = FileType::Preset;
		m_handling = FileHandling::LoadByPlugin;
	}
	else if( ext == "sf2" || ext == "sf3" )
	{
		m_type = FileType::SoundFont;
	}
	else if( ext == "pat" )
	{
		m_type = FileType::Patch;
	}
	else if( ext == "mid" || ext == "midi" || ext == "rmi" )
	{
		m_type = FileType::Midi;
		m_handling = FileHandling::ImportAsProject;
	}
	else if( ext == "dll"
#ifdef LMMS_BUILD_LINUX
		|| ext == "so" 
#endif
	)
	{
		m_type = FileType::VstPlugin;
		m_handling = FileHandling::LoadByPlugin;
	}
	else if ( ext == "lv2" )
	{
		m_type = FileType::Preset;
		m_handling = FileHandling::LoadByPlugin;
	}
	else
	{
		m_type = FileType::Unknown;
	}

	if( m_handling == FileHandling::NotSupported &&
		!ext.isEmpty() && ! getPluginFactory()->pluginSupportingExtension(ext).isNull() )
	{
		m_handling = FileHandling::LoadByPlugin;
		// classify as sample if not classified by anything yet but can
		// be handled by a certain plugin
		if( m_type == FileType::Unknown )
		{
			m_type = FileType::Sample;
		}
	}
}




QString FileItem::extension()
{
	return extension( fullName() );
}




QString FileItem::extension(const QString & file )
{
	return QFileInfo( file ).suffix().toLower();
}


} // namespace lmms::gui
