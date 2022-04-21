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


#ifndef FILE_BROWSER_H
#define FILE_BROWSER_H

#include <QCheckBox>
#include <QDir>
#include <QMutex>
#include <QTreeWidget>


#include "SideBarWidget.h"


class QLineEdit;

class TreeItem;
class FileItem;
class InstrumentTrack;
class FileBrowserTreeWidget;
class PlayHandle;
class TrackContainer;



class FileBrowser : public SideBarWidget
{
	Q_OBJECT
public:
	/**
		Create a file browser side bar widget
		@param directories '*'-separated list of directories to search for.
			If a directory of factory files should be in the list it
			must be the last one (for the factory files delimiter to work)
		@param filter Filter as used in QDir::match
		@param recurse *to be documented*
	*/
	FileBrowser(const QString& title, const QPixmap& pm, QWidget* parent);
	virtual ~FileBrowser() = default;

	void enableBackupFilter();
	void enableHiddenFiles() { m_hiddenCheckBox->setVisible(true); }
	void enableRecursiveSearch() { m_recursiveSearch = true; }
	void enableUnknownFiles() { m_unknownCheckBox->setVisible(true); }

	void setDirectory(const QString& dir);
	void setDirectories(const QFileInfoList& dirList);
	void setUserFactoryDir(const QString& userDir, const QString& factoryDir);

private slots:
	void reloadTree( void );
	void expandItems( QTreeWidgetItem * item=nullptr, QList<QString> expandedDirs = QList<QString>() );
	bool filterItems(TreeItem* parentDir = nullptr, bool search = true);
	void giveFocusToFilter();
	void onItemExpand(QTreeWidgetItem* item);
	void onSearch(const QString& filter);

private:
	void keyPressEvent( QKeyEvent * ke ) override;
	void showEvent(QShowEvent* se) override;

	FileBrowserTreeWidget* m_tree;

	QLineEdit* m_searchBox;
	QCheckBox* m_userCheckBox;
	QCheckBox* m_factoryCheckBox;
	QCheckBox* m_backupCheckBox;
	QCheckBox* m_hiddenCheckBox;
	QCheckBox* m_unknownCheckBox;

	//! Directories to display as toplevel items ("My computer" style)
	QStringList m_toplevelDirectories;
	//! Base path to user data
	QString m_userDir;
	//! Base path to factory data
	QString m_factoryDir;

	QList<QString> m_expandedDirsPriorToSearch;
	bool m_isSearching = false;
	bool m_recursiveSearch = false;
} ;




class FileBrowserTreeWidget : public QTreeWidget
{
	Q_OBJECT
public:
	FileBrowserTreeWidget( QWidget * parent );
	virtual ~FileBrowserTreeWidget() = default;

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
	QMutex m_pphMutex;

	QList<QAction*> getContextActions(FileItem* item, bool songEditor);


private slots:
	void activateListItem( QTreeWidgetItem * item, int column );
	void openInNewInstrumentTrack( FileItem* item, bool songEditor );
	bool openInNewSampleTrack( FileItem* item );
	void sendToActiveInstrumentTrack( FileItem* item );
	void openContainingFolder( FileItem* item );

} ;



//! \brief Base class for tree items that can be sorted
class TreeItem : public QTreeWidgetItem
{
public:
	TreeItem(const QString& name) : QTreeWidgetItem(QStringList(name), UserType) {}

	virtual bool isDirectory() const { return false; }
	virtual bool isFactory() const { return false; }
	virtual bool isHidden() const { return false; } //!< If hidden on the filesystem
	virtual bool isUser() const { return false; }

	static bool lessThan(const TreeItem* first, const TreeItem* second);
};




class Directory : public TreeItem
{
public:
	Directory(const QString& name, const QFileInfo& userPath, const QFileInfo& factoryPath);
	Directory(const QString& path) : Directory(path, QFileInfo(path), {}) {}

	//! Read directory and add its content as child items
	void addDirectoryContent();

	//! Read directory and return a list of TreeItems
	std::vector<TreeItem*> getDirectoryContent();

	//! If the directory content has been read
	bool initialized() const { return m_initialized; }

	bool isDirectory() const override { return true; }
	bool isFactory() const override { return !m_factoryPath.filePath().isEmpty(); }
	bool isUser() const override { return !m_userPath.filePath().isEmpty(); }
	bool isHidden() const override
	{
		// Hidden if both user and factory is hidden (lazy logic)
		return (m_userPath.isHidden() || !isUser()) && (m_factoryPath.isHidden() || !isFactory());
	}

	//! Return best path (user path takes precidence)
	QString path(bool user = true, bool factory = true) const;

	void updateIcon(bool user = true, bool factory = true);

private:
	void initPixmaps( void );

	static QPixmap * s_folderPixmap;
	static QPixmap * s_folderOpenedPixmap;
	static QPixmap * s_folderLockedPixmap;

	QFileInfo m_userPath;
	QFileInfo m_factoryPath;
	bool m_initialized = false;
};




class FileItem : public TreeItem
{
public:
	enum FileTypes
	{
		ProjectFile,
		ProjectBackupFile,
		PresetFile,
		SampleFile,
		SoundFontFile,
		PatchFile,
		MidiFile,
		VstPluginFile,
		UnknownFile,
		NumFileTypes
	} ;

	enum FileHandling
	{
		NotSupported,
		LoadAsProject,
		LoadAsPreset,
		LoadByPlugin,
		ImportAsProject
	} ;


	FileItem(const QString& name, const QFileInfo& path, bool fromFactory = false);
	FileItem(const QFileInfo& path) : FileItem(path.fileName(), path) {}

	// TODO rename to path()
	QString fullName() const
	{
		return m_path.filePath();
	}

	inline FileTypes type( void ) const
	{
		return( m_type );
	}

	inline FileHandling handling( void ) const
	{
		return( m_handling );
	}

	bool isUser() const override { return !m_factory; }
	bool isFactory() const override { return m_factory; }
	bool isHidden() const override { return m_path.isHidden(); }

	//! True if file may be loaded as a track
	inline bool isTrack( void ) const
	{
		return m_handling == LoadAsPreset || m_handling == LoadByPlugin;
	}

	QString extension( void );
	static QString extension( const QString & file );


private:
	void initPixmaps( void );
	void determineFileType( void );

	static QPixmap * s_projectFilePixmap;
	static QPixmap * s_presetFilePixmap;
	static QPixmap * s_sampleFilePixmap;
	static QPixmap * s_soundfontFilePixmap;
	static QPixmap * s_vstPluginFilePixmap;
	static QPixmap * s_midiFilePixmap;
	static QPixmap * s_unknownFilePixmap;

	bool m_factory;
	QFileInfo m_path;
	FileTypes m_type;
	FileHandling m_handling;

} ;


#endif
