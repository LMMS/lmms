/*
 * FileBrowser.h - include file for FileBrowser
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

#ifndef LMMS_GUI_FILE_BROWSER_H
#define LMMS_GUI_FILE_BROWSER_H

#include <QDir>
#include <QMutex>
#include <memory>

#include "embed.h"

#include <QTreeWidget>

#include "SideBarWidget.h"
#include "lmmsconfig.h"

class QCheckBox;
class QLineEdit;
class QProgressBar;

namespace lmms
{

class FileSearch;
class InstrumentTrack;
class PlayHandle;
class TrackContainer;

namespace gui
{

class FileBrowserTreeWidget;
class FileItem;

class FileBrowser : public SideBarWidget
{
	Q_OBJECT
public:
	enum class Type
	{
		Normal,
		Favorites
	};

	/**
			Create a file browser side bar widget
			@param directories '*'-separated list of directories to search for.
				If a directory of factory files should be in the list it
				must be the last one (for the factory files delimiter to work)
			@param filter Filter as used in QDir::match
			@param recurse *to be documented*
		*/
	FileBrowser(Type type, const QString& directories, const QString& filter, const QString& title, const QPixmap& pm,
		QWidget* parent, bool dirs_as_items = false, const QString& userDir = "", const QString& factoryDir = "");

	~FileBrowser() override = default;

	static QStringList excludedPaths()
	{
		static auto s_excludedPaths = QStringList{
#ifdef LMMS_BUILD_LINUX
			"/bin", "/boot", "/dev", "/etc", "/proc", "/run", "/sbin",
			"/sys"
#endif
#ifdef LMMS_BUILD_WIN32
			"C:\\Windows"
#endif
		};
		return s_excludedPaths;
	}

	static QDir::Filters dirFilters() { return QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden; }
	static QDir::SortFlags sortFlags() { return QDir::LocaleAware | QDir::DirsFirst | QDir::Name | QDir::IgnoreCase; }

private slots:
	void reloadTree();
	void expandItems(const QList<QString>& expandedDirs, QTreeWidgetItem* item = nullptr);
	void giveFocusToFilter();

private:
	void keyPressEvent( QKeyEvent * ke ) override;

	void addItems(const QString & path);

	void saveDirectoriesStates();
	void restoreDirectoriesStates();

	void foundSearchMatch(FileSearch* search, const QString& match);
	void searchCompleted(FileSearch* search);
	void onSearch(const QString& filter);
	void displaySearch(bool on);

	void addContentCheckBox();

	FileBrowserTreeWidget * m_fileBrowserTreeWidget;
	FileBrowserTreeWidget * m_searchTreeWidget;

	QLineEdit * m_filterEdit;
	Type m_type;

	std::shared_ptr<FileSearch> m_currentSearch;
	QProgressBar* m_searchIndicator = nullptr;

	QString m_directories; //!< Directories to search, split with '*'
	QString m_filter; //!< Filter as used in QDir::match()

	bool m_dirsAsItems;

	QCheckBox* m_showUserContent = nullptr;
	QCheckBox* m_showFactoryContent = nullptr;
	QCheckBox* m_showHiddenContent = nullptr;

	QBoxLayout *filterWidgetLayout = nullptr;
	QBoxLayout *hiddenWidgetLayout = nullptr;
	QBoxLayout *outerLayout = nullptr;
	QString m_userDir;
	QString m_factoryDir;
	QList<QString> m_savedExpandedDirs;
	QString m_previousFilterValue;
} ;




class FileBrowserTreeWidget : public QTreeWidget
{
	Q_OBJECT
public:
	FileBrowserTreeWidget( QWidget * parent );
	~FileBrowserTreeWidget() override = default;

	//! This method returns a QList with paths (QString's) of all directories
	//! that are expanded in the tree.
	QList<QString> expandedDirs( QTreeWidgetItem * item = nullptr ) const;

protected:
	void contextMenuEvent( QContextMenuEvent * e ) override;
	void mousePressEvent( QMouseEvent * me ) override;
	void mouseMoveEvent( QMouseEvent * me ) override;
	void mouseReleaseEvent( QMouseEvent * me ) override;
	void keyPressEvent( QKeyEvent * ke ) override;
	void keyReleaseEvent( QKeyEvent * ke ) override;
	void hideEvent( QHideEvent * he ) override;
	void focusOutEvent( QFocusEvent * fe ) override;


private:
	//! Start a preview of a file item
	void previewFileItem(FileItem* file);
	//! If a preview is playing, stop it.
	void stopPreview();

	void handleFile( FileItem * fi, InstrumentTrack * it );
	void openInNewInstrumentTrack( TrackContainer* tc, FileItem* item );


	bool m_mousePressed;
	QPoint m_pressPos;

	//! This should only be accessed or modified when m_pphMutex is held
	PlayHandle* m_previewPlayHandle;
	
#if (QT_VERSION >= QT_VERSION_CHECK(5,14,0))
	QRecursiveMutex m_pphMutex;
#else
	QMutex m_pphMutex;
#endif

	QList<QAction*> getContextActions(FileItem* item, bool songEditor);


private slots:
	void activateListItem( QTreeWidgetItem * item, int column );
	void openInNewInstrumentTrack( lmms::gui::FileItem* item, bool songEditor );
	bool openInNewSampleTrack( lmms::gui::FileItem* item );
	void sendToActiveInstrumentTrack( lmms::gui::FileItem* item );
	void updateDirectory( QTreeWidgetItem * item );
} ;



class Directory : public QTreeWidgetItem
{
public:
	Directory(const QString& filename, const QString& path, const QString& filter, bool disableEntryPopulation = false);

	void update();

	inline QString fullName( QString path = QString() )
	{
		if( path.isEmpty() )
		{
			path = m_directories[0];
		}
		if( ! path.isEmpty() )
		{
			path += QDir::separator();
		}
		return QDir::cleanPath(path + text(0));
	}

	inline void addDirectory( const QString & dir )
	{
		m_directories.push_back( dir );
	}


private:
	bool addItems( const QString & path );


	QPixmap m_folderPixmap = embed::getIconPixmap("folder");
	QPixmap m_folderOpenedPixmap = embed::getIconPixmap("folder_opened");
	QPixmap m_folderLockedPixmap = embed::getIconPixmap("folder_locked");

	//! Directories that lead here
	//! Initially, this is just set to the current path of a directory
	//! If, however, you have e.g. 'TripleOscillator/xyz' in two of the
	//! file browser's search directories 'a' and 'b', this will have two
	//! entries 'a/TripleOscillator' and 'b/TripleOscillator'
	//! and 'xyz' in the tree widget
	QStringList m_directories;
	//! Filter as used in QDir::match()
	QString m_filter;

	int m_dirCount;
	bool m_disableEntryPopulation = false;
} ;




class FileItem : public QTreeWidgetItem
{
public:
	enum class FileType
	{
		Project,
		Preset,
		Sample,
		SoundFont,
		Patch,
		Midi,
		VstPlugin,
		Unknown
	} ;

	enum class FileHandling
	{
		NotSupported,
		LoadAsProject,
		LoadAsPreset,
		LoadByPlugin,
		ImportAsProject
	} ;


	FileItem( QTreeWidget * parent, const QString & name,
							const QString & path );
	FileItem( const QString & name, const QString & path );

	QString fullName() const
	{
		return QFileInfo(m_path, text(0)).absoluteFilePath();
	}

	inline FileType type() const
	{
		return( m_type );
	}

	inline FileHandling handling() const
	{
		return( m_handling );
	}

	inline bool isTrack() const
	{
		return m_handling == FileHandling::LoadAsPreset || m_handling == FileHandling::LoadByPlugin;
	}

	QString extension();
	static QString extension( const QString & file );
	static QString defaultFilters();


private:
	void initPixmaps();
	void determineFileType();

	QString m_path;
	FileType m_type;
	FileHandling m_handling;

} ;


} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_FILE_BROWSER_H
