/*
 * UndoRedoMenu.h
 *
 * Copyright (c) 2004-2022 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_GUI_UNDO_REDO_MENU_H
#define LMMS_GUI_UNDO_REDO_MENU_H

#include <QMenu>
#include <QTreeWidget>
#include <QTextEdit>

#include "SideBarWidget.h"
#include "ProjectJournal.h"

namespace lmms::gui
{

class UndoRedoTreeWidget;

class UndoRedoMenu : public SideBarWidget
{
	Q_OBJECT
public:
	UndoRedoMenu(QWidget *parent = nullptr);
private slots:
	void reloadTree();
private:
	UndoRedoTreeWidget* m_undoRedoTree;
};

class UndoRedoTreeWidget : public QTreeWidget
{
	Q_OBJECT
public:
	UndoRedoTreeWidget(QWidget * parent);
	~UndoRedoTreeWidget() override = default;
protected:
	void contextMenuEvent(QContextMenuEvent * e) override;
private:
	void applyUndoRedoEntry(QTreeWidgetItem * item, int column);
};

class UndoRedoTreeWidgetItem : public QTreeWidgetItem
{
public:
	UndoRedoTreeWidgetItem(QTreeWidget* parent, ProjectJournal::CheckPoint* cp, int index, bool isUndo);
private:
	int m_index;
	bool m_isUndo;
	ProjectJournal::CheckPoint* m_checkpoint;
	friend class UndoRedoTreeWidget;
};

class UndoRedoMenuDetailsWindow : public QTextEdit
{
	Q_OBJECT
public:
	UndoRedoMenuDetailsWindow(QString text);
};


} // namespace lmms::gui

#endif // LMMS_GUI_UNDO_REDO_MENU_H
