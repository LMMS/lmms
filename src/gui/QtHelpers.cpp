/*
 * QtHelpers.cpp - Qt helper functions
 *
 * Copyright (c) 2025 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
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

#include <QWidget>

#include "QtHelpers.h"

namespace lmms::gui
{

void setMinMaxFromChild(QWidget& parent, const QWidget& child, QSize space)
{
	// If the child has a maximum, we cannot grow larger
	if(parent.maximumHeight() > child.maximumHeight() + space.height())
	{
		parent.setMaximumHeight(child.maximumHeight() + space.height());
	}
	if(parent.maximumWidth() > child.maximumWidth() + space.width())
	{
		parent.setMaximumWidth(child.maximumWidth() + space.width());
	}
	// If the child has a minimum, we cannot shrink smaller
	if(parent.minimumHeight() < child.minimumHeight() + space.height())
	{
		parent.setMinimumHeight(child.minimumHeight() + space.height());
	}
	if(parent.minimumWidth() < child.minimumWidth() + space.width())
	{
		parent.setMinimumWidth(child.minimumWidth() + space.width());
	}
}

} // namespace lmms::gui

