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

#ifndef LMMS_GUI_SUBWINDOW_H
#define LMMS_GUI_SUBWINDOW_H

#include <QMdiSubWindow>
#include <QString>

#include "lmms_export.h"

class QGraphicsDropShadowEffect;
class QLabel;
class QPushButton;
class QWidget;

namespace lmms::gui
{



//! @brief The SubWindow class
//! 
//! Because of a bug in the QMdiSubWindow class to save the right position and size of a subwindow in a project and
//! because of the inability for customizing the title bar appearance, lmms implements its own subwindow class.
class LMMS_EXPORT SubWindow : public QMdiSubWindow
{
	Q_OBJECT
	Q_PROPERTY( QBrush activeColor READ activeColor WRITE setActiveColor )
	Q_PROPERTY( QColor textShadowColor READ textShadowColor WRITE setTextShadowColor )
	Q_PROPERTY( QColor borderColor READ borderColor WRITE setBorderColor )

public:
	SubWindow( QWidget *parent = nullptr, Qt::WindowFlags windowFlags = QFlag(0) );
	// same as QWidet::normalGeometry, but works properly under X11 (see https://bugreports.qt.io/browse/QTBUG-256)
	QRect getTrueNormalGeometry() const;
	QBrush activeColor() const;
	QColor textShadowColor() const;
	QColor borderColor() const;
	QMargins decorationMargins() const;
	void setActiveColor( const QBrush & b );
	void setTextShadowColor( const QColor &c );
	void setBorderColor( const QColor &c );
	int titleBarHeight() const;
	int frameWidth() const;
	bool isDetachable() const;
	void setDetachable(bool on);
	bool isDetached() const;
	void setDetached(bool on);

	// TODO Needed to update the title bar when replacing instruments.
	// Update works automatically if QMdiSubWindows are used.
	void updateTitleBar();

public slots:
	void detach();
	void attach();
	void setVisible(bool visible) override;

protected:
	// hook the QWidget move/resize events to update the tracked geometry

	//! Overrides the QMdiSubWindow::moveEvent() for saving the position of the subwindow into m_trackedNormalGeom. This
	//! position will be saved with the project because of an Qt bug which doesn't save the right position.
	//!
	//! @see [QTBUG-256](https://bugreports.qt.io/browse/QTBUG-256)
	// TODO: @param event
	void moveEvent(QMoveEvent* event) override;

	//! At first we give the event to QMdiSubWindow::resizeEvent() which handles the event on its behavior.
	//!
	//! On every resize event we have to adjust our title label.
	//!
	//! At last we store the current size into m_trackedNormalGeom. This size will be saved with the project because of an
	//! Qt bug which doesn't save the right size.
	//!
	//! @see [QTBUG-256](https://bugreports.qt.io/browse/QTBUG-256)
	// TODO: @param event
	void resizeEvent(QResizeEvent* event) override;

	//! @brief SubWindow::paintEvent
	//!
	//! This draws our new title bar with custom colors and draws a window icon on the left upper corner.
	void paintEvent(QPaintEvent* pe) override;

	//! @brief Triggers if the window title changes and calls adjustTitleBar().
	// TODO: @param event
	void changeEvent(QEvent* event) override;

	void showEvent(QShowEvent* e) override;

	//! @brief Override of QMdiSubWindow's event filter.
	//!
	//! This is not how regular eventFilters work, it is never installed explicitly. Instead, it is installed by Qt and
	//! conveniently installs itself onto the child widget. Despite relying on internal implementation details, as of
	//! writing this it seems to be the best way to do so as soon as the widget is set.
	bool eventFilter(QObject* obj, QEvent* event) override;

signals:
	void focusLost();

private:
	const QSize m_buttonSize;
	const int m_titleBarHeight;
	QPushButton * m_closeBtn;
	QPushButton * m_maximizeBtn;
	QPushButton * m_restoreBtn;
	QPushButton* m_detachBtn;
	QBrush m_activeColor;
	QColor m_textShadowColor;
	QColor m_borderColor;
	QPoint m_position;
	QRect m_trackedNormalGeom;
	QLabel * m_windowTitle;
	QGraphicsDropShadowEffect * m_shadow;
	bool m_hasFocus;
	bool m_isDetachable;

	//! @brief Stores the given text into the given label.
	//!
	//! Shorts the text if it's too big for the labels width and adds three dots (...)
	//!
	//! @param label Holds a pointer to the QLabel
	//! @param text The text which will be stored (and if needed broken down) into the QLabel.
	static void elideText( QLabel *label, QString text );

	//! Our title bar needs buttons for maximize/restore and close in the right upper corner.
	//! We check if the subwindow is maximizable and put the buttons on the right positions.
	//! At next we calculate the width of the title label and call elideText() for adding
	//! the window title to m_windowTitle (which is a QLabel)
	void adjustTitleBar();

private slots:
	void focusChanged( QMdiSubWindow * subWindow );
};



} // namespace lmms::gui

#endif // LMMS_GUI_SUBWINDOW_H
