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

#include <list>
#include <memory>
#include <vector>

#include <QWidget>

#include "Clipboard.h"
#include "lmms_export.h"
#include "ModelView.h"

namespace lmms::gui
{



class LMMS_EXPORT InteractiveModelView : public QWidget
{
Q_OBJECT
public:
	InteractiveModelView(QWidget* widget);
	~InteractiveModelView() override;

	//virtual void setModel( Model* model, bool isOldModelValid = true );
	//virtual void unsetModel();



	// highlighting
	// keyboard
	// display shortcuts -> mouse enter event


	static void startHighlighting(Clipboard::StringPairDataType dataType);
	static void stopHighlighting();

protected:
	class ModelShortcut
	{
	public:
	
		inline bool operator==(ModelShortcut& rhs)
		{
			return m_key == rhs.m_key &&
				m_modifier == rhs.m_modifier &&
				m_times == rhs.m_times;
		}
		
		/*
		inline bool operator==(const ModelShortcut& firstShortcut, const ModelShortcut& secondShortcut)
		{
    		return firstShortcut.m_key == secondShortcut.m_key &&
    			firstShortcut.m_modifier == secondShortcut.m_modifier &&
    			firstShortcut.m_times == secondShortcut.m_times;
		}
		*/
		Qt::Key m_key;
		Qt::KeyboardModifier m_modifier;
		unsigned int m_times;
		QString m_shortcutDescription;
	};

	void keyPressEvent(QKeyEvent* event) override;
	void enterEvent(QEvent* event) override;
	void leaveEvent(QEvent* event) override;
	
	virtual void shortcutPressedEvent(size_t shortcutLocation, QKeyEvent* event) = 0;
	virtual bool canAcceptClipBoardData(Clipboard::StringPairDataType dataType);
	virtual std::vector<ModelShortcut> getShortcuts() = 0;

	bool getIsHighlighted() const;
private:
	void setIsHighlighted(bool isHighlighted);
	bool doesShortcutMatch(const ModelShortcut* shortcut, QKeyEvent* event, unsigned int times) const;

	bool m_isHighlighted;
	
	ModelShortcut m_lastShortcut;
	unsigned int m_lastShortcutCounter;
	
	static std::list<InteractiveModelView*> s_interactiveWidgets;
};

} // namespace lmms::gui

#endif // LMMS_GUI_INTERACTIVE_MODEL_VIEW_H
