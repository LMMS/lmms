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


/**
 * @brief The SubWindow class
 * 
 *  Because of a bug in the QMdiSubWindow class to save the right position and size
 *  of a subwindow in a project and because of the inability
 *  for cusomizing the title bar appearance, lmms implements its own subwindow
 *  class.
 */
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
	void setActiveColor( const QBrush & b );
	void setTextShadowColor( const QColor &c );
	void setBorderColor( const QColor &c );
	int titleBarHeight() const;
	int frameWidth() const;

	// TODO Needed to update the title bar when replacing instruments.
	// Update works automatically if QMdiSubWindows are used.
	void updateTitleBar();

protected:
	// hook the QWidget move/resize events to update the tracked geometry
	void moveEvent( QMoveEvent * event ) override;
	void resizeEvent( QResizeEvent * event ) override;
	void paintEvent( QPaintEvent * pe ) override;
	void changeEvent( QEvent * event ) override;

	QPushButton* addTitleButton(const std::string& iconName, const QString& toolTip);

signals:
	void focusLost();

private:
	const QSize m_buttonSize;
	const int m_titleBarHeight;
	QPushButton * m_closeBtn;
	QPushButton * m_maximizeBtn;
	QPushButton * m_restoreBtn;
	QBrush m_activeColor;
	QColor m_textShadowColor;
	QColor m_borderColor;
	QPoint m_position;
	QRect m_trackedNormalGeom;
	QLabel * m_windowTitle;
	QGraphicsDropShadowEffect * m_shadow;
	bool m_hasFocus;

	static void elideText( QLabel *label, QString text );
	void adjustTitleBar();

private slots:
	void focusChanged( QMdiSubWindow * subWindow );
};



} // namespace lmms::gui

#endif // LMMS_GUI_SUBWINDOW_H
