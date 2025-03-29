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

#include <QFileInfo>

#include "Engine.h"
#include "Song.h"

#include "embed.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "ProjectJournal.h"

namespace lmms::gui
{


UndoRedoMenu::UndoRedoMenu(QWidget *parent, bool isUndo) :
	QMenu(tr(isUndo? "Undo" : "Redo"), parent),
	m_isUndo(isUndo)
{
	setIcon(embed::getIconPixmap(m_isUndo ? "edit_undo" : "edit_redo"));

	connect(this, &UndoRedoMenu::aboutToShow, this, &UndoRedoMenu::fillMenu);
}


void UndoRedoMenu::fillMenu()
{
	clear();
	ProjectJournal::CheckPointStack history = m_isUndo
		? Engine::projectJournal()->undoCheckPoints()
		: Engine::projectJournal()->redoCheckPoints();

	int index = 0;
	for (auto it = history.rbegin(); it != history.rend(); ++it)
	{
		auto cp = *it;
		addAction(cp.description, this,
			[this, cp, index]()
			{
				for (int i = 0; i < index + 1; ++i)
				{
					if (m_isUndo) { Engine::projectJournal()->undo(); }
					else { Engine::projectJournal()->redo(); }
				}
			}
		);
		++index;
	}
}

} // namespace lmms::gui
