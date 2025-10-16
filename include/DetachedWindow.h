/*
 * DetachedWindow.h - Substitute for SubWindow class to be used when window is detached.
 *
 * Copyright (c) 2015 Colin Wallace <wallace.colin.a@gmail.com>
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

#ifndef LMMS_GUI_DETACHEDWINDOW
#define LMMS_GUI_DETACHEDWINDOW

#include <QWidget>
#include <QString>
#include <QVBoxLayout>

#include "lmms_export.h"

namespace lmms::gui
{


/**
 * @brief The DetachedWindow class
 * 
 *  Substitute for SubWindow class to be used when window is detached.
 *  This handles window-specific features like icon, title and close event.
 *  Could include additional decorations, common to all detached windows.
 */
class LMMS_EXPORT DetachedWindow : public QWidget
{
	Q_OBJECT

public:
	DetachedWindow(QWidget *child = nullptr, QWidget *parent = nullptr, Qt::WindowFlags windowFlags = QFlag(0));
	QWidget* widget() const;
	void setWidget(QWidget* widget);

public slots:
	void detach();
	void attach();

protected:
	void closeEvent(QCloseEvent* e) override;

	bool isDetached() const;
	QVBoxLayout* m_layout;
};



} // namespace lmms::gui

#endif // LMMS_GUI_DETACHEDWINDOW
