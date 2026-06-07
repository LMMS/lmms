/*
* SubWindow.cpp - Implementation of QMdiSubWindow that correctly tracks
*   the geometry that windows should be restored to.
*   Workaround for https://bugreports.qt.io/browse/QTBUG-256
*   This implementation adds a custom themed title bar to
*   the subwindow.
*
* Copyright (c) 2015 Colin Wallace <wallace.colin.a@gmail.com>
* Copyright (c) 2016 Steffen Baranowsky <baramgb@freenet.de>
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

#include "SubWindow.h"

#include <algorithm>

#include <QGraphicsDropShadowEffect>
#include <QGuiApplication>
#include <QLabel>
#include <QLayout>
#include <QMdiArea>
#include <QMetaMethod>
#include <QMoveEvent>
#include <QPainter>
#include <QPushButton>
#include <QStyleOption>
#include <QStyleOptionTitleBar>
#include <QWindow>
#include <QMenu>
#include <QAction>
#include "Editor.h"

#include "ConfigManager.h"
#include "embed.h"


namespace lmms::gui
{




void splitRequested(Qt::ArrowType direction)
{
	// This signal is emitted when the user right clicks the maximize button and chooses a split direction.
}





void SubWindow::showSplitMenu(const QPoint &pos)
{
    QMenu menu;

    QAction *splitUp    = menu.addAction("Split up");
    QAction *splitDown  = menu.addAction("Split down");
    QAction *splitLeft  = menu.addAction("Split left");
    QAction *splitRight = menu.addAction("Split right");

    QAction *chosen = menu.exec(m_maximizeBtn->mapToGlobal(pos));
    if (!chosen)
        return;

    if (chosen == splitUp) {
        emit splitRequested(Qt::UpArrow);
    } else if (chosen == splitDown) {
        emit splitRequested(Qt::DownArrow);
    } else if (chosen == splitLeft) {
        emit splitRequested(Qt::LeftArrow);
    } else if (chosen == splitRight) {
        emit splitRequested(Qt::RightArrow);
    }
}




void SubWindow::showMaximized()
{
    auto area = mdiArea();
    if (!area)
        return;

    m_trackedNormalGeom = geometry();

    m_isFakeMaximized = true;

    QRect full = area->rect();
    full.adjust(0, 0, -1, -1);

    setGeometry(full);

    adjustTitleBar();
}
    // Mark as "fake maximized"
    // setWindowState(Qt::WindowMaximized);
// }




void SubWindow::showNormal()
{
    setGeometry(m_trackedNormalGeom);
    m_isFakeMaximized = false;
    adjustTitleBar();
}
    // setWindowState(Qt::WindowNoState);
// }





SubWindow::SubWindow(QWidget* parent, Qt::WindowFlags windowFlags)
	: QMdiSubWindow{parent, windowFlags}
	, m_buttonSize{17, 17}
	, m_titleBarHeight{titleBarHeight()}
	, m_hasFocus{false}
	, m_isDetachable{true}
{
	// initialize the tracked geometry to whatever Qt thinks the normal geometry currently is.
	// this should always work, since QMdiSubWindows will not start as maximized
	m_trackedNormalGeom = normalGeometry();

	// inits the colors
	m_activeColor = Qt::SolidPattern;
	m_textShadowColor = Qt::black;
	m_borderColor = Qt::black;

	// close, maximize, restore, and detach buttons
	auto createButton = [this](std::string_view iconName, const QString& tooltip) -> QPushButton* {
		auto button = new QPushButton{embed::getIconPixmap(iconName), QString{}, this};
		button->resize(m_buttonSize);
		button->setFocusPolicy(Qt::NoFocus);
		button->setCursor(Qt::ArrowCursor);
		button->setAttribute(Qt::WA_NoMousePropagation);
		button->setToolTip(tooltip);
		return button;
	};
	m_closeBtn = createButton("close", tr("Close"));
	connect(m_closeBtn, &QPushButton::clicked, this, &QWidget::close);

	m_maximizeBtn = createButton("maximize", tr("Maximize"));
	setWindowFlags((this->windowFlags()) & ~Qt::WindowMaximizeButtonHint);
	connect(m_maximizeBtn, &QPushButton::clicked, this, &SubWindow::showMaximized);

	m_restoreBtn = createButton("restore", tr("Restore"));
	connect(m_restoreBtn, &QPushButton::clicked, this, &SubWindow::showNormal);
	// right click on the restore button should show the split menu
	m_restoreBtn->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(
		m_restoreBtn,
		&QWidget::customContextMenuRequested,
		this,
		[this](const QPoint &pos) {
			showSplitMenu(pos);
		}
	);

	m_detachBtn = createButton("detach", tr("Detach"));
	connect(m_detachBtn, &QPushButton::clicked, this, &SubWindow::detach);

	// QLabel for the window title and the shadow effect
	m_shadow = new QGraphicsDropShadowEffect();
	m_shadow->setColor( m_textShadowColor );
	m_shadow->setXOffset( 1 );
	m_shadow->setYOffset( 1 );
	
	m_windowTitle = new QLabel( this );
	m_windowTitle->setFocusPolicy( Qt::NoFocus );
	m_windowTitle->setAttribute( Qt::WA_TransparentForMouseEvents, true );
	m_windowTitle->setGraphicsEffect( m_shadow );

	layout()->setSizeConstraint(QLayout::SetMinAndMaxSize);

	// Disable the minimize button and make sure that the custom window hint is set
	setWindowFlags((this->windowFlags() & ~Qt::WindowMinimizeButtonHint) | Qt::CustomizeWindowHint);

	connect(mdiArea(), &QMdiArea::subWindowActivated, this, &SubWindow::focusChanged);
}




/**
 * @brief SubWindow::paintEvent
 * 
 *  This draws our new title bar with custom colors
 *  and draws a window icon on the left upper corner.
 */
void SubWindow::paintEvent( QPaintEvent * )
{
	// Don't paint any of the other stuff if the sub window is maximized
	// so that only its child content is painted.
	// if (isMaximized()) { return; }

	QPainter p( this );
	QRect rect( 0, 0, width(), m_titleBarHeight );

	const bool isActive = windowState() & Qt::WindowActive;

	p.fillRect( rect, isActive ? activeColor() : p.pen().brush() );
	
	// window border
	p.setPen( borderColor() );

	// top line
	p.drawLine(0, 0, width(), 0);

	// bottom, left, and right lines
	p.drawLine( 0, height() - 1, width(), height() - 1 );
	p.drawLine( 0, m_titleBarHeight, 0, height() - 1 );
	p.drawLine( width() - 1, m_titleBarHeight, width() - 1, height() - 1 );

	// window icon
	if( widget() )
	{
		QPixmap winicon( widget()->windowIcon().pixmap( m_buttonSize ) );
		p.drawPixmap( 3, 3, m_buttonSize.width(), m_buttonSize.height(), winicon );
	}
}




/**
 * @brief SubWindow::changeEvent
 * 
 * Triggers if the window title changes and calls adjustTitleBar().
 * @param event
 */
void SubWindow::changeEvent( QEvent *event )
{
	QMdiSubWindow::changeEvent( event );

	if( event->type() == QEvent::WindowTitleChange )
	{
		adjustTitleBar();
	}
}




void SubWindow::setVisible(bool visible)
{
	if (isDetached())
	{
		// When detached, top-level window is the child widget itself. (This is janky and is best changed at some point.)
		// For that reason we forward show/hide to the child widget, and don't touch the hidden attached window frame.
		widget()->setVisible(visible);
	}
	else
	{
		// When attached, visibility of the actual window is controlled by SubWindow.
		// SubWindow::hide() when it's already hidden still causes a size recalculation,
		// if the child widget is hidden it does not get included into the layout, and maximum size is set to 0x0.
		// There shouldn't be a reason to keep the child widget hidden when the subwindow is attached.
		// see bug #8292
		widget()->show();
		QMdiSubWindow::setVisible(visible);
	}
}




void SubWindow::showEvent(QShowEvent* e)
{
	if (ConfigManager::inst()->value("ui", "detachbehavior", "show") == "detached") { detach(); }
	if (isDetached())
	{
		widget()->setWindowState((widget()->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
	}
}




bool SubWindow::isDetachable() const
{
	return m_isDetachable;
}




void SubWindow::setDetachable(bool on)
{
	m_isDetachable = on;
}




bool SubWindow::isDetached() const
{
	return widget()->windowFlags().testFlag(Qt::Window);
}




void SubWindow::setDetached(bool on)
{
	if (on) { detach(); }
	else { attach(); }
}




/**
 * @brief SubWindow::elideText
 * 
 *  Stores the given text into the given label.
 *  Shorts the text if it's too big for the labels width
 *  and adds three dots (...)
 * 
 * @param label - holds a pointer to the QLabel
 * @param text  - the text which will be stored (and if needed broken down) into the QLabel.
 */
void SubWindow::elideText( QLabel *label, QString text )
{
	QFontMetrics metrix( label->font() );
	int width = label->width() - 2;
	QString clippedText = metrix.elidedText( text, Qt::ElideRight, width );
	label->setText( clippedText );
}




/**
 * @brief SubWindow::getTrueNormalGeometry
 * 
 *  same as QWidet::normalGeometry, but works properly under X11
 *  see https://bugreports.qt.io/browse/QTBUG-256
 */
QRect SubWindow::getTrueNormalGeometry() const
{
	return m_trackedNormalGeom;
}




QBrush SubWindow::activeColor() const
{
	return m_activeColor;
}




QColor SubWindow::textShadowColor() const
{
	return m_textShadowColor;
}




QColor SubWindow::borderColor() const
{
	return m_borderColor;
}




void SubWindow::setActiveColor( const QBrush & b )
{
	m_activeColor = b;
}




void SubWindow::setTextShadowColor( const QColor & c )
{
	m_textShadowColor = c;
}




void SubWindow::setBorderColor( const QColor &c )
{
	m_borderColor = c;
}




void SubWindow::detach()
{
	if (!isDetachable() || isDetached()) { return; }

	const auto pos = mapToGlobal(widget()->pos());
	const bool shown = isVisible();

	auto flags = windowFlags();
	flags |= Qt::Window;
	flags &= ~Qt::SubWindow;
	flags |= Qt::WindowMinimizeButtonHint;

	hide();
	widget()->setWindowFlags(flags);

	if (shown) { widget()->show(); }

	widget()->move(pos);
}




void SubWindow::attach()
{
	if (!isDetached()) { return; }

	const bool shown = widget()->isVisible();

	auto frame = widget()->geometry();
	frame.moveTo(mdiArea()->mapFromGlobal(frame.topLeft()));
	frame += decorationMargins();

	// Make sure the window fully fits on screen
	frame.setSize({
		std::min(frame.width(), mdiArea()->width()),
		std::min(frame.height(), mdiArea()->height())
	});

	frame.moveTo(
		std::clamp(frame.left(), 0, mdiArea()->rect().width() - frame.width()),
		std::clamp(frame.top(), 0, mdiArea()->rect().height() - frame.height())
	);

	auto flags = windowFlags();
	flags &= ~Qt::Window;
	flags |= Qt::SubWindow;
	flags &= ~Qt::WindowMinimizeButtonHint;
	widget()->setWindowFlags(flags);

	if (shown)
	{
		widget()->show();
		show();
	}

	if (QGuiApplication::platformName() == "wayland")
	{
		// Workaround for wayland reporting position as 0-0
		// See https://doc.qt.io/qt-6.9/application-windows.html#wayland-peculiarities
		resize(frame.size());
	}
	else
	{
		setGeometry(frame);
	}
}




int SubWindow::titleBarHeight() const
{
	QStyleOptionTitleBar so;
	so.titleBarState = Qt::WindowActive; // kThemeStateActive
	so.titleBarFlags = Qt::Window;
	return style()->pixelMetric(QStyle::PM_TitleBarHeight, &so, this);
}




int SubWindow::frameWidth() const
{
	QStyleOptionFrame so;
	return style()->pixelMetric(QStyle::PM_MdiSubWindowFrameWidth, &so, this);
}




QMargins SubWindow::decorationMargins() const
{
	return {
		frameWidth(),     // left
		titleBarHeight(), // top
		frameWidth(),     // right
		frameWidth()      // bottom
	};
}




void SubWindow::updateTitleBar()
{
	adjustTitleBar();
}




QList<SubWindow*> SubWindow::otherWindows() const
{
    QList<SubWindow*> list;
    for (auto *w : mdiArea()->subWindowList())
    {
        if (w != this)
            list.append(static_cast<SubWindow*>(w));
    }
    return list;
}

SubWindow* SubWindow::windowAt(const QPoint &pos) const
{
    for (auto *w : mdiArea()->subWindowList())
    {
        if (w != this && w->geometry().contains(pos))
            return static_cast<SubWindow*>(w);
    }
    return nullptr;
}




QRect SubWindow::computeSnapTarget(const QPoint &globalPos)
{
    auto *area = mdiArea();
    if (!area)
        return {};

    QRect areaRect = area->rect();
    const int snap = 20;

    int w = areaRect.width();
    int h = areaRect.height();

    bool left   = globalPos.x() <= snap;
    bool right  = globalPos.x() >= w - snap;
    bool top    = globalPos.y() <= snap;
    bool bottom = globalPos.y() >= h - snap;

    QRect leftZone(0, 0, w/2, h);
    QRect rightZone(w/2, 0, w/2, h);
    QRect topZone(0, 0, w, h/2);
    QRect bottomZone(0, h/2, w, h/2);

    if (left && top)    return QRect(0, 0, w/2, h/2);
    if (right && top)   return QRect(w/2, 0, w/2, h/2);
    if (left && bottom) return QRect(0, h/2, w/2, h/2);
    if (right && bottom)return QRect(w/2, h/2, w/2, h/2);

    QRect target;
    if (left)  target = leftZone;
    if (right) target = rightZone;
    if (top)   target = topZone;
    if (bottom)target = bottomZone;

    if (target.isNull())
        return {};

    SubWindow *other = windowAt(globalPos);
    if (!other)
        return target;

    QRect g = other->geometry();

    bool otherLeft   = g.left() == 0;
    bool otherRight  = g.right() == w - 1;
    bool otherTop    = g.top() == 0;
    bool otherBottom = g.bottom() == h - 1;

	// double click to enlarge?
	// bug with movement still
	// if (otherTop || otherBottom)
	// {
	// 	int midX = g.left() + g.width() / 2;

	// 	QRect leftRect(g.left(), g.top(), g.width(), g.height() / 2);
	// 	QRect rightRect(g.left(), g.top() + g.height() / 2,
	// 					g.width(), g.height() - g.height() / 2);

	// 	if (globalPos.x() < midX)
	// 	{
	// 		m_pendingOther = other;
	// 		m_pendingOtherRect = rightRect;
	// 		return leftRect;
	// 	}
	// 	else
	// 	{
	// 		m_pendingOther = other;
	// 		m_pendingOtherRect = leftRect;
	// 		return rightRect;
	// 	}
	// }

	// if (otherLeft || otherRight)
	// {
	// 	int midY = g.top() + g.height() / 2;

	// 	QRect topRect(g.left(), g.top(), g.width() / 2, g.height());
	// 	QRect bottomRect(g.left() + g.width() / 2, g.top(),
	// 					g.width() - g.width() / 2, g.height());

	// 	if (globalPos.y() < midY)
	// 	{
	// 		m_pendingOther = other;
	// 		m_pendingOtherRect = bottomRect;
	// 		return topRect;
	// 	}
	// 	else
	// 	{
	// 		m_pendingOther = other;
	// 		m_pendingOtherRect = topRect;
	// 		return bottomRect;
	// 	}
	// }

    return target;
}






// to stop movement of the titlebar
// bool SubWindow::isInTitleBar(const QPoint &pos) const
// {
//     QWidget *child = childAt(pos);

//     if (child == m_closeBtn ||
//         child == m_maximizeBtn ||
//         child == m_restoreBtn ||
//         child == m_detachBtn)
//     {
//         return false;
//     }

//     if (child == m_windowTitle)
//         return false;

//     return pos.y() < m_titleBarHeight;
// }




bool SubWindow::event(QEvent *event)
{
    if (m_isFakeMaximized)
    {
        if (event->type() == QEvent::ContextMenu)
        {
            event->accept();
            return true;
        }
    }

    return QMdiSubWindow::event(event);
}




void SubWindow::mousePressEvent(QMouseEvent *event)
{
    if (m_isFakeMaximized) // && isInTitleBar(event->pos()))
    {
        m_blockNextMove = true;
        m_snapTarget = {};
		event->accept();
        return;
    }

    m_blockNextMove = false;
	m_snapTarget = {};

    QMdiSubWindow::mousePressEvent(event);
}




void SubWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_blockNextMove)
    {
        event->accept();
        return;
    }

    auto *area = mdiArea();
    if (area)
    {
        // Convert mouse position to MDI-area coordinates
        QPoint areaPos = area->mapFromGlobal(mapToGlobal(event->pos()));
		m_snapTarget = computeSnapTarget(areaPos);
    }

    // Let the window move normally
    QMdiSubWindow::mouseMoveEvent(event);
}




void SubWindow::mouseReleaseEvent(QMouseEvent *event)
{
	m_blockNextMove = false;

    if (!m_snapTarget.isNull())
    {
        // Apply snap to this window
        setGeometry(m_snapTarget);

        // Apply paired snap to the other window
        if (m_pendingOther)
            m_pendingOther->setGeometry(m_pendingOtherRect);

        // Clear pending state
        m_pendingOther = nullptr;
        m_pendingOtherRect = QRect();
        m_snapTarget = QRect();
    }

    QMdiSubWindow::mouseReleaseEvent(event);
}





void SubWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (m_blockNextMove)
    {
        event->accept();
        return;
    }

    QMdiSubWindow::mouseDoubleClickEvent(event);
}




/**
 * @brief SubWindow::moveEvent
 * 
 *  overrides the QMdiSubWindow::moveEvent() for saving the position
 *  of the subwindow into m_trackedNormalGeom. This position
 *  will be saved with the project because of an Qt bug which doesn't
 *  save the right position. look at: https://bugreports.qt.io/browse/QTBUG-256
 * @param event
 */
void SubWindow::moveEvent(QMoveEvent *event)
{
    // Normal behavior
    QMdiSubWindow::moveEvent(event);

	if (!m_isFakeMaximized && !isMinimized() && !isFullScreen())
	{
		m_trackedNormalGeom.moveTopLeft(event->pos());
	}
}




/**
 * @brief SubWindow::adjustTitleBar
 * 
 *  Our title bar needs buttons for maximize/restore and close in the right upper corner.
 *  We check if the subwindow is maximizable and put the buttons on the right positions.
 *  At next we calculate the width of the title label and call elideText() for adding
 *  the window title to m_windowTitle (which is a QLabel)
 */
void SubWindow::adjustTitleBar()
{
	// Don't show the title or any button if the sub window is maximized. Otherwise they
	// might show up behind the actual maximized content of the child widget.
	// if (isMaximized())
	// {
		// m_closeBtn->hide();
		// m_maximizeBtn->hide();
		// m_restoreBtn->hide();
		// m_windowTitle->hide();
	// }

	// The sub window is not maximized, i.e. the title must be shown
	// as well as some buttons.

	// Title adjustments
	m_windowTitle->show();

	// button adjustments
	m_maximizeBtn->hide();
	m_restoreBtn->hide();
	m_detachBtn->hide();
	m_closeBtn->show();

	const int rightSpace = 3;
	const int buttonGap = 1;
	const int menuButtonSpace = 24;

	auto buttonPos = QPoint{width() - rightSpace - m_buttonSize.width(), 3};
	const auto buttonStep = QPoint{m_buttonSize.width() + buttonGap, 0};

	// the buttonBarWidth depends on the number of buttons.
	// we need it to calculate the width of window title label
	int buttonBarWidth = rightSpace + m_buttonSize.width();

	// set the buttons on their positions.
	// the close button is always needed and on the rightButtonPos
	m_closeBtn->move(buttonPos);
	buttonPos -= buttonStep;

	// here we ask: is the Subwindow maximizable and
	// then we set the buttons and show them if needed
	// if( windowFlags() & Qt::WindowMaximizeButtonHint )
	// {
		// Always show maximize/restore support (custom maximize)
		buttonBarWidth += m_buttonSize.width() + buttonGap;

		m_maximizeBtn->move(buttonPos);
		m_restoreBtn->move(buttonPos);

		if (m_isFakeMaximized)
		{
			m_maximizeBtn->hide();
			m_restoreBtn->show();
		}
		else
		{
			m_maximizeBtn->show();
			m_restoreBtn->hide();
		}

		buttonPos -= buttonStep;
	//}

	// we're keeping the restore button around if we open projects
	// from older versions that have saved minimized windows
	if (isMinimized())
	{
		m_restoreBtn->show();
		buttonPos -= buttonStep;
	}

	if (isDetachable())
	{
		m_detachBtn->move(buttonPos);
		m_detachBtn->show();
		buttonBarWidth = buttonBarWidth + m_buttonSize.width() + buttonGap;
	}

	if( widget() )
	{
		// title QLabel adjustments
		m_windowTitle->setAlignment( Qt::AlignLeft );

		// make title bold
		QFont f = m_windowTitle->font();
    	f.setBold(true);
    	m_windowTitle->setFont(f);

		m_windowTitle->setFixedWidth( widget()->width() - ( menuButtonSpace + buttonBarWidth ) );
		m_windowTitle->move( menuButtonSpace,
			( m_titleBarHeight / 2 ) - ( m_windowTitle->sizeHint().height() / 2 ) - 1 );

		// if minimized we can't use widget()->width(). We have to hard code the width,
		// as the width of all minimized windows is the same.
		if( isMinimized() )
		{
			m_windowTitle->setFixedWidth( 120 );
		}

		// truncate the label string if the window is to small. Adds "..."
		elideText( m_windowTitle, widget()->windowTitle() );
		m_windowTitle->setTextInteractionFlags( Qt::NoTextInteraction );
		m_windowTitle->adjustSize();
	}

	QSet<QToolBar*> seen;
	auto *editor = qobject_cast<Editor*>(widget());
	if (editor)
	{
		QFontMetrics fm(m_windowTitle->font());
		int titleWidth = fm.horizontalAdvance(widget()->windowTitle());

		int iconWidth = m_buttonSize.width(); // LMMS uses button size for icon size
		int iconPadding = 6; // LMMS draws icon at (3,3)

		int x = iconPadding + iconWidth + 8 + titleWidth + 10;
		int y = (m_titleBarHeight - 24) / 2;

		for (auto *bar : editor->allToolBars())
		{
			if (seen.contains(bar))
				continue;

			seen.insert(bar);

			bar->setParent(this);
			bar->raise();
			bar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
			bar->adjustSize();
			bar->move(x, y);
			bar->show();

			x += bar->width() + 10;
		}
	}
}



void SubWindow::focusChanged( QMdiSubWindow *subWindow )
{
	if( m_hasFocus && subWindow != this )
	{
		m_hasFocus = false;
		emit focusLost();
	}
	else if( subWindow == this )
	{
		m_hasFocus = true;
	}
}




/**
 * @brief SubWindow::resizeEvent
 * 
 *  At first we give the event to QMdiSubWindow::resizeEvent() which handles
 *  the event on its behavior.
 *
 *  On every resize event we have to adjust our title label.
 * 
 *  At last we store the current size into m_trackedNormalGeom. This size
 *  will be saved with the project because of an Qt bug which doesn't
 *  save the right size. look at: https://bugreports.qt.io/browse/QTBUG-256
 * 
 * @param event
 */
void SubWindow::resizeEvent( QResizeEvent * event )
{
	// When the parent QMdiArea gets resized, maximized subwindows also gets resized, if any.
	// In that case, we should call QMdiSubWindow::resizeEvent first
	// to ensure we get the correct window state.
	QMdiSubWindow::resizeEvent( event );
	adjustTitleBar();

	// if the window was resized and ISN'T minimized/maximized/fullscreen,
	// then save the current size
	if( !m_isFakeMaximized && !isMinimized() && !isFullScreen() )
	{
		m_trackedNormalGeom.setSize( event->size() );
	}
}




/**
 * @brief SubWindow::eventFilter
 *
 * Override of QMdiSubWindow's event filter.
 * This is not how regular eventFilters work, it is never installed explicitly.
 * Instead, it is installed by Qt and conveniently installs itself
 * onto the child widget. Despite relying on internal implementation details,
 * as of writing this it seems to be the best way to do so as soon as the widget is set.
 */
bool SubWindow::eventFilter(QObject* obj, QEvent* event)
{
	if (obj != static_cast<QObject*>(widget()))
	{
		return QMdiSubWindow::eventFilter(obj, event);
	}

	switch (event->type())
	{
		case QEvent::WindowStateChange:
			event->accept();
			return true;

		case QEvent::Close:
			if (isDetached())
			{
				QString detachBehavior = ConfigManager::inst()->value("ui", "detachbehavior", "show");
				if (detachBehavior == "show")
				{
					attach();
					event->ignore();
					return true;
				}
				else if (detachBehavior == "hide")
				{
					attach();
					hide();
					event->ignore();
					return QMdiSubWindow::eventFilter(obj, event);
				}
				else if (detachBehavior == "detached")
				{
					event->accept();
					return QMdiSubWindow::eventFilter(obj, event);
				}
			}
			else
			{
				hide();
			}
			return QMdiSubWindow::eventFilter(obj, event);

		default:
			return QMdiSubWindow::eventFilter(obj, event);
	}
}


} // namespace lmms::gui
