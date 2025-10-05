/*
 * PluginView.h - declaration of class PluginView
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_GUI_PLUGIN_VIEW_H
#define LMMS_GUI_PLUGIN_VIEW_H

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QWidget>

#include "Clipboard.h"
#include "ModelView.h"
#include "Plugin.h"

namespace lmms::gui {

class LMMS_EXPORT PluginView : public QWidget, public ModelView
{
public:
	PluginView(Plugin* _plugin, QWidget* _parent)
		: QWidget(_parent)
		, ModelView(_plugin, this)
	{
	}

	virtual bool isResizable() const { return false; }

protected:
	void dragEnterEvent(QDragEnterEvent* dee) override
	{
		const auto [path, ext] = DragAndDrop::getFileAndExt(dee);

		if (castModel<Plugin>()->descriptor()->supportsFileType(ext))
		{
			dee->acceptProposedAction();
		}
	}

	void dropEvent(QDropEvent* de) override
	{
		const auto [path, ext] = DragAndDrop::getFileAndExt(de);

		if (castModel<Plugin>()->descriptor()->supportsFileType(ext))
		{
			castModel<Plugin>()->loadFile(path);
			de->accept();
		}
	}
};

} // namespace lmms::gui

#endif // LMMS_GUI_PLUGIN_VIEW_H
