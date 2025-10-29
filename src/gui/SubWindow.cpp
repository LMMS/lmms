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

#include "ConfigManager.h"
#include "embed.h"

// Only needed for error display for missing detach feature
#include "lmmsconfig.h"
#ifdef LMMS_BUILD_APPLE
#include "TextFloat.h"
#endif

namespace lmms::gui
{


SubWindow::SubWindow(QWidget *parent, Qt::WindowFlags windowFlags) :
	QMdiSubWindow(parent, windowFlags),
	m_buttonSize(17, 17),
	m_titleBarHeight(titleBarHeight()),
	m_hasFocus(false)
{
	// initialize the tracked geometry to whatever Qt thinks the normal geometry currently is.
	// this should always work, since QMdiSubWindows will not start as maximized
	m_trackedNormalGeom = normalGeometry();

	// inits the colors
	m_activeColor = Qt::SolidPattern;
	m_textShadowColor = Qt::black;
	m_borderColor = Qt::black;

	// close, maximize, restore, and detach buttons
	auto createButton = [this](const std::string& iconName, const QString& tooltip) -> QPushButton* {
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
	connect(m_maximizeBtn, &QPushButton::clicked, this, &QWidget::showMaximized);

	m_restoreBtn = createButton("restore", tr("Restore"));
	connect(m_restoreBtn, &QPushButton::clicked, this, &QWidget::showNormal);

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

	connect( mdiArea(), SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(focusChanged(QMdiSubWindow*)));
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
	if (isMaximized()) { return; }

	QPainter p( this );
	QRect rect( 0, 0, width(), m_titleBarHeight );

	const bool isActive = windowState() & Qt::WindowActive;

	p.fillRect( rect, isActive ? activeColor() : p.pen().brush() );

	// window border
	p.setPen( borderColor() );

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
	widget()->setVisible(visible);
	if (!isDetached()) { QMdiSubWindow::setVisible(visible); }
}




void SubWindow::showEvent(QShowEvent* e)
{
	widget()->show();
	if (isDetached())
	{
		widget()->setGeometry(m_childGeom);
		widget()->setWindowState((widget()->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
	}
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
 *  ans adds three dots (...)
 * 
 * @param label - holds a pointer to the QLabel
 * @param text  - the text which will be stored (and if needed breaked down) into the QLabel.
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




#ifdef LMMS_BUILD_APPLE
// FIXME: For some reason detaching on MacOS seems to never show the detached window.
void SubWindow::detach()
{
	TextFloat::displayMessage("Missing Feature",\
		tr("Sorry, detach is not yet available on this platform."), embed::getIconPixmap("error"), 2000);
}

#else
void SubWindow::detach()
{
#if QT_VERSION < 0x50C00
	// Workaround for a bug in Qt versions below 5.12,
	// where argument-dependent-lookup fails for QFlags operators
	// declared inside a namepsace.
	// This affects the Q_DECLARE_OPERATORS_FOR_FLAGS macro in Instrument.h
	// See also: https://codereview.qt-project.org/c/qt/qtbase/+/225348

	using ::operator|;
#endif

	if (isDetached()) { return; }

	const auto pos = mapToGlobal(widget()->pos());
	const bool shown = isVisible();

	auto flags = windowFlags();
	flags |= Qt::Window;
	flags &= ~Qt::Widget;
	flags |= Qt::WindowMinimizeButtonHint;

	hide();
	widget()->setWindowFlags(flags);
	m_childGeom = widget()->geometry(); // reset/init tracked detached geometry since it's only needed there

	if (shown) { widget()->show(); }

	widget()->move(pos);
}
#endif

void SubWindow::attach()
{
#if QT_VERSION < 0x50C00
	// Workaround for a bug in Qt versions below 5.12,
	// where argument-dependent-lookup fails for QFlags operators
	// declared inside a namepsace.
	// This affects the Q_DECLARE_OPERATORS_FOR_FLAGS macro in Instrument.h
	// See also: https://codereview.qt-project.org/c/qt/qtbase/+/225348

	using ::operator|;
#endif

	if (!isDetached()) { return; }

	const bool shown = widget()->isVisible();

	auto frame = widget()->windowHandle()->geometry();
	frame.moveTo(mdiArea()->mapFromGlobal(frame.topLeft()));
	frame += decorationMargins();

	// arbitrary values, require window to touch `mdiArea - margin`
	// TODO make this live configurble maybe?
	const auto margin = QMargins(40, 40, 40, 40);
	frame.moveTo(std::clamp(frame.left(),
	                        margin.left() - frame.width(),
	                        mdiArea()->rect().width() - margin.right()),
	             std::clamp(frame.top(),
	                        margin.top() - frame.height(),
	                        mdiArea()->rect().height() - margin.bottom()));

	auto flags = windowFlags();
	flags &= ~Qt::Window;
	flags |= Qt::Widget;
	flags &= ~Qt::WindowMinimizeButtonHint;
	widget()->setWindowFlags(flags);

	if (shown)
	{
		widget()->show();
		show();
	}

	if (QGuiApplication::platformName() == "wayland")
	{
		resize(frame.size());  // Workaround for wayland reporting position as 0-0, see https://doc.qt.io/qt-6.9/application-windows.html#wayland-peculiarities
	}
	else
	{
		setGeometry(frame);
	}
}



int SubWindow::titleBarHeight() const
{
	QStyleOptionTitleBar so;
	so.titleBarState = Qt::WindowActive; // kThemeStateActiv
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
	return QMargins(frameWidth(),     // left
	                titleBarHeight(), // top
	                frameWidth(),     // right
	                frameWidth());    // bottom
}




void SubWindow::updateTitleBar()
{
	adjustTitleBar();
}


/**
 * @brief SubWindow::moveEvent
 * 
 *  overides the QMdiSubWindow::moveEvent() for saving the position
 *  of the subwindow into m_trackedNormalGeom. This position
 *  will be saved with the project because of an Qt bug wich doesn't
 *  save the right position. look at: https://bugreports.qt.io/browse/QTBUG-256
 * @param event
 */
void SubWindow::moveEvent( QMoveEvent * event )
{
	QMdiSubWindow::moveEvent( event );
	// if the window was moved and ISN'T minimized/maximized/fullscreen,
	// then save the current position
	if( !isMaximized() && !isMinimized() && !isFullScreen() )
	{
		m_trackedNormalGeom.moveTopLeft( event->pos() );
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
	if (isMaximized())
	{
		m_closeBtn->hide();
		m_maximizeBtn->hide();
		m_restoreBtn->hide();
		m_windowTitle->hide();

		return;
	}

	// The sub window is not maximized, i.e. the title must be shown
	// as well as some buttons.

	// Title adjustments
	m_windowTitle->show();

	// button adjustments
	m_maximizeBtn->hide();
	m_restoreBtn->hide();
	m_closeBtn->show();

	const int rightSpace = 3;
	const int buttonGap = 1;
	const int menuButtonSpace = 24;

	QPoint buttonPos(width() - rightSpace - m_buttonSize.width(), 3);
	const QPoint buttonStep( m_buttonSize.width() + buttonGap, 0 );

	// the buttonBarWidth depends on the number of buttons.
	// we need it to calculate the width of window title label
	int buttonBarWidth = rightSpace + m_buttonSize.width();

	// set the buttons on their positions.
	// the close button is always needed and on the rightButtonPos
	m_closeBtn->move(buttonPos);
	buttonPos -= buttonStep;

	// here we ask: is the Subwindow maximizable and
	// then we set the buttons and show them if needed
	if( windowFlags() & Qt::WindowMaximizeButtonHint )
	{
		buttonBarWidth = buttonBarWidth + m_buttonSize.width() + buttonGap;
		m_maximizeBtn->move(buttonPos);
		m_restoreBtn->move(buttonPos);
		if (!isMaximized())
		{
			m_maximizeBtn->show();
			buttonPos -= buttonStep;
		}
	}

	// we're keeping the restore button around if we open projects
	// from older versions that have saved minimized windows
	if (isMinimized())
	{
		m_restoreBtn->show();
		buttonPos -= buttonStep;
	}

	m_detachBtn->move(buttonPos);
	m_detachBtn->show();
	buttonBarWidth = buttonBarWidth + m_buttonSize.width() + buttonGap;

	if( widget() )
	{
		// title QLabel adjustments
		m_windowTitle->setAlignment( Qt::AlignHCenter );
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
 *  will be saved with the project because of an Qt bug wich doesn't
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
	if( !isMaximized() && !isMinimized() && !isFullScreen() )
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
				attach();
				event->ignore();
				if (ConfigManager::inst()->value("ui", "hideondetachedclosed", "0").toInt())
				{
					hide();
				}
				else
				{
					return true;
				}
			}
			else
			{
				hide();
				event->ignore();
			}
			return QMdiSubWindow::eventFilter(obj, event);

		case QEvent::Move:
			if (isDetached()) { m_childGeom.moveTo(static_cast<QMoveEvent*>(event)->pos()); }
			return QMdiSubWindow::eventFilter(obj, event);

		case QEvent::Resize:
			if (isDetached()) { m_childGeom.setSize(static_cast<QResizeEvent*>(event)->size()); }
			return QMdiSubWindow::eventFilter(obj, event);

		default:
			return QMdiSubWindow::eventFilter(obj, event);
	}
}


} // namespace lmms::gui
