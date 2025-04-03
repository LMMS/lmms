/*
 * UndoRedoMenu.cpp
 *
 * Copyright (c) 2004-2022 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2025 regulus79
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

#include "UndoRedoMenu.h"

#include <QContextMenuEvent>
#include <QFileInfo>
#include <QMessageBox>
#include <QTextStream>

#include "Engine.h"
#include "Song.h"

#include "embed.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "ProjectJournal.h"

namespace lmms::gui
{


UndoRedoMenu::UndoRedoMenu(QWidget *parent) :
	SideBarWidget(tr("Project History"), embed::getIconPixmap("edit_undo").transformed(QTransform().rotate(90)), parent)
{
	m_undoRedoTree = new UndoRedoTreeWidget(contentParent());
	addContentWidget(m_undoRedoTree);
	reloadTree();
	connect(Engine::projectJournal(), &ProjectJournal::checkPointAdded, this, &UndoRedoMenu::reloadTree);
	connect(Engine::projectJournal(), &ProjectJournal::undoTriggered, this, &UndoRedoMenu::reloadTree);
	connect(Engine::projectJournal(), &ProjectJournal::redoTriggered, this, &UndoRedoMenu::reloadTree);
}


void UndoRedoMenu::reloadTree()
{
	m_undoRedoTree->clear();

	auto& undoHistory = Engine::projectJournal()->undoCheckPoints();
	auto& redoHistory = Engine::projectJournal()->redoCheckPoints();

	int index = redoHistory.size() - 1;
	for (auto it = redoHistory.begin(); it != redoHistory.end(); ++it)
	{
		m_undoRedoTree->addTopLevelItem(new UndoRedoTreeWidgetItem(m_undoRedoTree, &(*it), index, false));
		--index;
	}

	QTreeWidgetItem* divider = new QTreeWidgetItem(m_undoRedoTree);
	divider->setText(0, tr("--- Current State ---"));
	m_undoRedoTree->addTopLevelItem(divider);

	index = 0;
	for (auto it = undoHistory.rbegin(); it != undoHistory.rend(); ++it)
	{
		m_undoRedoTree->addTopLevelItem(new UndoRedoTreeWidgetItem(m_undoRedoTree, &(*it), index, true));
		++index;
	}
}


UndoRedoTreeWidget::UndoRedoTreeWidget(QWidget* parent):
	QTreeWidget(parent)
{
	setColumnCount(1);
	headerItem()->setHidden(true);
	connect(this, &UndoRedoTreeWidget::itemDoubleClicked, this, &UndoRedoTreeWidget::applyUndoRedoEntry);
}


void UndoRedoTreeWidget::contextMenuEvent(QContextMenuEvent * e)
{
	if (itemAt(e->pos()) == nullptr) { return; }
	UndoRedoTreeWidgetItem* entry = dynamic_cast<UndoRedoTreeWidgetItem*>(itemAt(e->pos()));
	if (entry == nullptr) { return; }

	QMenu contextMenu(this);

	contextMenu.addAction(QIcon(embed::getIconPixmap("edit_undo")), tr(entry->m_isUndo ? "Undo" : "Redo"), [this, entry]{ applyUndoRedoEntry(entry, 0); });
	contextMenu.addAction(tr("View changes"),
		[entry] {
			QString saveState;
			QTextStream stream(&saveState);
			entry->m_checkpoint->data.save(stream, 2);
			auto saveStateWindow = new UndoRedoMenuDetailsWindow(QString("%1").arg(saveState));
			saveStateWindow->show();
			saveStateWindow->parentWidget()->update();
		}
	);
	contextMenu.exec(e->globalPos());
}

void UndoRedoTreeWidget::applyUndoRedoEntry(QTreeWidgetItem * item, int column)
{
	auto entry = dynamic_cast<UndoRedoTreeWidgetItem*>(item);
	if(entry == nullptr) { return; }

	bool isUndo = entry->m_isUndo;
	int numUndoRedos = entry->m_index + 1;
	for (int i = 0; i < numUndoRedos; ++i)
	{
		if (isUndo) { Engine::projectJournal()->undo(); }
		else { Engine::projectJournal()->redo(); }
	}
}


UndoRedoTreeWidgetItem::UndoRedoTreeWidgetItem(QTreeWidget* parent, ProjectJournal::CheckPoint* cp, int index, bool isUndo):
	QTreeWidgetItem(parent),
	m_index(index),
	m_isUndo(isUndo),
	m_checkpoint(cp)
{
	setText(0, m_checkpoint->description);
	setIcon(0, embed::getIconPixmap(m_isUndo ? "edit_undo" : "edit_redo"));
}


UndoRedoMenuDetailsWindow::UndoRedoMenuDetailsWindow(QString text):
	QTextEdit(nullptr)
{
#if (QT_VERSION < QT_VERSION_CHECK(5,12,0))
	// Bug workaround: https://codereview.qt-project.org/c/qt/qtbase/+/225348
	using ::operator|;
#endif
	setWindowTitle("Undo Checkpoint");
	setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
	getGUI()->mainWindow()->addWindowedWidget(this);
	parentWidget()->setWindowIcon(embed::getIconPixmap("edit_undo"));
	setPlainText(text);
	setMinimumSize(600,400);
}


} // namespace lmms::gui
