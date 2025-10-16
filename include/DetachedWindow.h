/*
 * SubWindow.h - Implementation of QMdiSubWindow that correctly tracks
 *   the geometry that windows should be restored to.
 *   Workaround for https://bugreports.qt.io/browse/QTBUG-256
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
 * @brief The SubWindow class
 * 
 *  Because of a bug in the QMdiSubWindow class to save the right position and size
 *  of a subwindow in a project and because of the inability
 *  for cusomizing the title bar appearance, lmms implements its own subwindow
 *  class.
 */
class LMMS_EXPORT DetachedWindow : public QWidget
{
	Q_OBJECT
//	Q_PROPERTY( QBrush activeColor READ activeColor WRITE setActiveColor )
//	Q_PROPERTY( QColor textShadowColor READ textShadowColor WRITE setTextShadowColor )
//	Q_PROPERTY( QColor borderColor READ borderColor WRITE setBorderColor )

public:
	DetachedWindow(QWidget *child = nullptr, QWidget *parent = nullptr, Qt::WindowFlags windowFlags = QFlag(0));
	QWidget* widget() const;
	void setWidget(QWidget* widget);
	// same as QWidet::normalGeometry, but works properly under X11 (see https://bugreports.qt.io/browse/QTBUG-256)
	// TODO Needed to update the title bar when replacing instruments.
	// Update works automatically if QMdiSubWindows are used.

public slots:
	void detach();
	void attach();

protected:
	// hook the QWidget move/resize events to update the tracked geometry
	void closeEvent(QCloseEvent* e) override;

	bool isDetached() const;
	QVBoxLayout* m_layout;

//private:

//private slots:
//	void focusChanged( QMdiSubWindow * subWindow );
};



} // namespace lmms::gui

#endif // LMMS_GUI_SUBWINDOW_H
