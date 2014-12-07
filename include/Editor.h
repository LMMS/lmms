/*
 * Editor.h - declaration of Editor class
 *
 * Copyright (c) 2014 Lukas W <lukaswhl/at/gmail.com>
 *
 * This file is part of LMMS - http://lmms.io
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

#ifndef EDITOR_COMMON_H
#define EDITOR_COMMON_H

#include <QMainWindow>
#include <QToolBar>

#include "Timeline.h"
#include "ToolButton.h"

/// \brief Superclass for editors with a toolbar.
///
/// Those editors include the Song Editor, the Automation Editor,
/// B&B Editor, Piano Roll.
class Editor : public QMainWindow
{
	Q_OBJECT
public:
	void setPlaying(bool playing=true);

signals:

protected:
	/// \brief	Constructor.
	///
	/// \param	record	If set true, the editor's toolbar will contain record
	///					buttons in addition to the play and stop buttons.
	Editor(bool record = false);
	virtual ~Editor();


	QToolBar* m_toolBar;

	QAbstractButton* m_playButton;
	QAbstractButton* m_recordButton;
	QAbstractButton* m_recordAccompanyButton;
	QAbstractButton* m_stopButton;
private:
};


#endif
