/*
 * InteractiveModelView.h - TODO
 *
 * Copyright (c) 2024 szeli1 <TODO/at/gmail/dot/com>
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

#ifndef LMMS_GUI_INTERACTIVE_MODEL_VIEW_H
#define LMMS_GUI_INTERACTIVE_MODEL_VIEW_H

#include <array>
#include <list>
#include <memory>
#include <utility>
#include <vector>

#include "Model.h"

namespace lmms::gui
{

class LMMS_EXPORT InteractiveModelView : public ModelView
{
public:
	InteractiveModelView(Model* model, QWidget* widget);
	~InteractiveModelView() override;

	//virtual void setModel( Model* model, bool isOldModelValid = true );
	//virtual void unsetModel();



	// highlighting
	// keyboard
	// display shortcuts -> mouse enter event


	static void startHighlighting(Clipboard::StringPairDataType dataType);
	static void stopHighlighting();

protected:
	struct ModelShortcut
	{
		Qt::Key m_key;
		Qt::KeyboardModifier m_modifier;
		unsigned int m_times;
		QString m_shortcutDescription;
	};

	void keyPressEvent(QKeyEvent* event) override;
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QLeaveEvent* event) override;
	
	virtual shortcutPressedEvent(size_t shortcutLocation, QKeyEvent* event) = 0;
	virtual bool canAcceptClipBoardData(Clipboard::StringPairDataType dataType);
	virtual const std::vector<ModelShortcut> getShortcuts() = 0;

	bool getIsHighlighted() const;
private:
	bool setIsHighlighted(bool isHighlighted);
	bool doesShortcutMatch(ModelShortcut& shortcut, QKeyEvent* event, unsigned int times);

	bool m_isHighlighted;
	
	ModelShortcut m_lastShortcut;
	unsigned int m_lastShortcutCounter;

	/*
	static const std::array<std::pair<Qt::Key, QString>, 2> s_keyNames = {
		std::make_pair<Qt::Key, QString>(Qt::Key_C, "c"),
		std::make_pair<Qt::Key, QString>(Qt::Key_V, "v")
	};
	*/
	
	static std::list<InteractiveModelView*> s_interactiveWidgets;
};

} // namespace lmms::gui

#endif // LMMS_GUI_INTERACTIVE_MODEL_VIEW_H
