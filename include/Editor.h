/*
 * Editor.h - declaration of Editor class
 *
 * Copyright (c) 2014 Lukas W <lukaswhl/at/gmail.com>
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

#ifndef LMMS_GUI_EDITOR_H
#define LMMS_GUI_EDITOR_H

#include <QMainWindow>
#include <QToolBar>

class QAction;

namespace lmms::gui
{

static const int Quantizations[] = {
	1, 2, 4, 8, 16, 32, 64,
	3, 6, 12, 24, 48, 96, 192
};


class DropToolBar;

/// \brief Superclass for editors with a toolbar.
///
/// Those editors include the Song Editor, the Automation Editor, B&B Editor,
/// and the Piano Roll.
class Editor : public QMainWindow
{
	Q_OBJECT
public:
	void setPauseIcon(bool displayPauseIcon=true);
	QAction *playAction() const;
	static Editor* lastPlayedEditor() { return s_lastPlayedEditor; }
protected:
	DropToolBar * addDropToolBarToTop(QString const & windowTitle);
	DropToolBar * addDropToolBar(Qt::ToolBarArea whereToAdd, QString const & windowTitle);
	DropToolBar * addDropToolBar(QWidget * parent, Qt::ToolBarArea whereToAdd, QString const & windowTitle);

	void closeEvent(QCloseEvent * event) override;
	void keyPressEvent(QKeyEvent *ke) override;
public slots:
	//! Called by pressing the space key. Plays or stops.
	void togglePlayStop();
	//! Called by pressing shift+space. Toggles pause state.
	void togglePause();

protected slots:
	virtual void play() {}
	virtual void record() {}
	virtual void recordAccompany() {}
	virtual void toggleStepRecording() {}
	virtual void stop() {}

private slots:
	void toggleMaximize();
private:
	inline static Editor* s_lastPlayedEditor = nullptr;

signals:

protected:
	/// \brief	Constructor.
	///
	/// \param	record	If set true, the editor's toolbar will contain record
	///					buttons in addition to the play and stop buttons.
	Editor(bool record = false, bool record_step = false);
	~Editor() override = default;


	DropToolBar* m_toolBar;

	QAction* m_playAction;
	QAction* m_recordAction;
	QAction* m_recordAccompanyAction;
	QAction* m_toggleStepRecordingAction;
	QAction* m_stopAction;
};


/// Small helper class: A QToolBar that accepts and exposes drop events as signals
class DropToolBar : public QToolBar
{
	Q_OBJECT
public:
	DropToolBar(QWidget* parent=0);

signals:
	void dragEntered(QDragEnterEvent* event);
	void dropped(QDropEvent* event);

protected:
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dropEvent(QDropEvent* event) override;
};


} // namespace lmms::gui

#endif // LMMS_GUI_EDITOR_H
