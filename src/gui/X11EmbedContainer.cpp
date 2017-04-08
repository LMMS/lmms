/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2017 Lukas W <lukaswhl/at/gmail.com>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "X11EmbedContainer.h"

#include <QAbstractNativeEventFilter>
#include <qapplication.h>
#include <qevent.h>
#include <qpainter.h>
#include <qlayout.h>
#include <qstyle.h>
#include <qstyleoption.h>
#include <qelapsedtimer.h>
#include <qpointer.h>
#include <qdebug.h>
#include <QtX11Extras/QX11Info>
#include <QThread>

#include <QtCore/QMutex>

#include <QtCore/private/qobject_p.h>
#include <QtWidgets/private/qwidget_p.h>
#include <QtGui/private/qguiapplication_p.h>

#include <QWindow>
#include <QGuiApplication>
#include <qpa/qplatformintegration.h>
//#include <private/qt_x11_p.h>

#include <queue>
#include <cstring>

#define XK_MISCELLANY
#define XK_LATIN1
#define None 0
#include <X11/Xlib-xcb.h>
#include <X11/Xutil.h>
#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_util.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>

#ifndef XK_ISO_Left_Tab
#define XK_ISO_Left_Tab 0xFE20
#endif

//#define QX11EMBED_DEBUG
#ifdef QX11EMBED_DEBUG
#include <qdebug.h>
#endif

#undef KeyPress
#undef KeyRelease
#undef FocusIn
#undef FocusOut

static const int XRevertToParent = RevertToParent;
#undef RevertToParent

QT_BEGIN_NAMESPACE

enum ATOM_ID : int {
	_XEMBED
	,_XEMBED_INFO
	,WM_PROTOCOLS
	,WM_DELETE_WINDOW
	,WM_STATE
};

static const std::vector<std::pair<int, std::string>> atom_list({
	{_XEMBED, "_XEMBED"},
	{_XEMBED_INFO, "_XEMBED_INFO"},
	{WM_PROTOCOLS, "WM_PROTOCOLS"},
	{WM_DELETE_WINDOW, "WM_DELETE_WINDOW"},
	{WM_STATE, "WM_STATE"},
});

static QMap<int, xcb_atom_t> atoms;
static QMutex atoms_lock;

void initAtoms()
{
	QMutexLocker locker(&atoms_lock); Q_UNUSED(locker);

	std::queue<xcb_intern_atom_cookie_t> cookies;

	for (const auto& pair : atom_list)
	{
		cookies.push(xcb_intern_atom(QX11Info::connection(), false, pair.second.length(), pair.second.data()));
	}

	for (const auto& pair : atom_list)
	{
		auto cookie = cookies.front();
		cookies.pop();

		auto reply = xcb_intern_atom_reply(QX11Info::connection(), cookie, nullptr);
		atoms[pair.first] = reply->atom;

		Q_ASSERT(pair.second == XGetAtomName(QX11Info::display(), reply->atom));

#ifdef QX11EMBED_DEBUG
		qDebug() << "atom" << QString::fromStdString(pair.second)
				 << XGetAtomName(QX11Info::display(), reply->atom) << reply->atom;
#endif
		free(reply);
	}
}

xcb_atom_t ATOM(int atomID)
{
	return atoms.value(atomID);
}


struct xembed_info
{
	uint32_t version;
	uint32_t flags;
};

xembed_info* get_xembed_info(xcb_window_t window)
{
	auto cookie = xcb_get_property(QX11Info::connection(), 0, window, ATOM(_XEMBED_INFO), ATOM(_XEMBED_INFO), 0, 2);
	if (auto reply = xcb_get_property_reply(QX11Info::connection(), cookie, nullptr)) {
		auto val_len = xcb_get_property_value_length(reply);
		if (val_len < 2) {
#ifdef QX11EMBED_DEBUG
			qDebug() << "Client has malformed _XEMBED_INFO property, len is" << val_len;
#endif
			free(reply);
			return nullptr;
		}

		void* result = malloc(sizeof(xembed_info));
		memcpy(result, xcb_get_property_value(reply), sizeof(xembed_info));
		return reinterpret_cast<xembed_info*>(result);
	}

	return nullptr;
}

// This is a hack to move topData() out from QWidgetPrivate to public.  We
// need to to inspect window()'s embedded state.
class QHackWidget : public QWidget
{
	Q_DECLARE_PRIVATE(QWidget)
public:
	QTLWExtra* topData() { return d_func()->topData(); }
};

static unsigned int XEMBED_VERSION = 0;

enum QX11EmbedMessageType {
	XEMBED_EMBEDDED_NOTIFY = 0,
	XEMBED_WINDOW_ACTIVATE = 1,
	XEMBED_WINDOW_DEACTIVATE = 2,
	XEMBED_REQUEST_FOCUS = 3,
	XEMBED_FOCUS_IN = 4,
	XEMBED_FOCUS_OUT = 5,
	XEMBED_FOCUS_NEXT = 6,
	XEMBED_FOCUS_PREV = 7,
	XEMBED_MODALITY_ON = 10,
	XEMBED_MODALITY_OFF = 11,
	XEMBED_REGISTER_ACCELERATOR = 12,
	XEMBED_UNREGISTER_ACCELERATOR = 13,
	XEMBED_ACTIVATE_ACCELERATOR = 14
};

enum QX11EmbedFocusInDetail {
	XEMBED_FOCUS_CURRENT = 0,
	XEMBED_FOCUS_FIRST = 1,
	XEMBED_FOCUS_LAST = 2
};

enum QX11EmbedFocusInFlags {
	XEMBED_FOCUS_OTHER = (0 << 0),
	XEMBED_FOCUS_WRAPAROUND = (1 << 0)
};

enum QX11EmbedInfoFlags {
	XEMBED_MAPPED = (1 << 0)
};

enum QX11EmbedAccelModifiers {
	XEMBED_MODIFIER_SHIFT = (1 << 0),
	XEMBED_MODIFIER_CONTROL = (1 << 1),
	XEMBED_MODIFIER_ALT = (1 << 2),
	XEMBED_MODIFIER_SUPER = (1 << 3),
	XEMBED_MODIFIER_HYPER = (1 << 4)
};

enum QX11EmbedAccelFlags {
	XEMBED_ACCELERATOR_OVERLOADED = (1 << 0)
};

// Silence the default X11 error handler.
/*static int x11ErrorHandler(Display *, xcb_generic_error_t *)
{
	return 0;
}*/

// Returns the X11 timestamp. Maintained mainly by qapplication
// internals, but also updated by the XEmbed widgets.
static xcb_timestamp_t x11Time()
{
	return QX11Info::getTimestamp();
}

// Gives the version and flags of the supported XEmbed protocol.
static unsigned int XEmbedVersion()
{
	return 0;
}


// Sends an XEmbed message.
static void sendXEmbedMessage(WId window, long message,
							  long detail = 0, long data1 = 0, long data2 = 0)
{
	auto display = QX11Info::display();

	XClientMessageEvent c;
	memset(&c, 0, sizeof(c));
	c.type = ClientMessage;
	c.message_type = ATOM(_XEMBED);
	c.format = 32;
	c.display = display;
	c.window = window;

	c.data.l[0] = x11Time();
	c.data.l[1] = message;
	c.data.l[2] = detail;
	c.data.l[3] = data1;
	c.data.l[4] = data2;

	XSendEvent(display, window, false, NoEventMask, (XEvent *) &c);
}

// From qapplication_x11.cpp
static xcb_key_press_event_t lastKeyEvent;

// The purpose of this global x11 filter is for one to capture the key
// events in their original state, but most importantly this is the
// only way to get the WM_TAKE_FOCUS message from WM_PROTOCOLS.
class X11EventFilter : public QAbstractNativeEventFilter
{
public:
	bool nativeEventFilter(const QByteArray &eventType, void *message, long *result)
	{
		if (eventType != "xcb_generic_event_t") {
			return false;
		}

		xcb_generic_event_t *event = reinterpret_cast<xcb_generic_event_t *>(message);
		if (event->response_type == XCB_KEY_PRESS || event->response_type == XCB_KEY_RELEASE) {
			lastKeyEvent = *reinterpret_cast<xcb_key_press_event_t*>(message);
		}

		return false;
	}
} static x11EventFilter;


class QX11EmbedContainerPrivate : public QWidgetPrivate
{
	Q_DECLARE_PUBLIC(QX11EmbedContainer)
public:
	inline QX11EmbedContainerPrivate()
	{
		lastError = QX11EmbedContainer::Unknown;
		client = 0;
		focusProxy = 0;
		clientIsXEmbed = false;
		xgrab = false;
	}

	bool isEmbedded() const;
	void moveInputToProxy();

	void acceptClient(WId window);
	void rejectClient(WId window);

	void checkGrab();
	void checkXembedInfo();

	WId topLevelParentWinId() const;

	void emitError(QX11EmbedContainer::Error error) {
		Q_Q(QX11EmbedContainer);
		lastError = error;
		emit q->error(error);
	}

	WId client;
	QWidget *focusProxy;
	bool clientIsXEmbed;
	bool xgrab;
	QRect clientOriginalRect;
	QSize wmMinimumSizeHint;

	QX11EmbedContainer::Error lastError;

	static QX11EmbedContainer *activeContainer;
};

QX11EmbedContainer *QX11EmbedContainerPrivate::activeContainer = 0;

/*!
	Creates a QX11EmbedContainer object with the given \a parent.
*/
QX11EmbedContainer::QX11EmbedContainer(QWidget *parent)
	: QWidget(*new QX11EmbedContainerPrivate, parent, 0)
{
	initAtoms();
	Q_D(QX11EmbedContainer);
	//XSetErrorHandler(x11ErrorHandler);

	setAttribute(Qt::WA_NativeWindow);
	setAttribute(Qt::WA_DontCreateNativeAncestors);
	createWinId();

	setFocusPolicy(Qt::StrongFocus);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	// ### PORT setKeyCompression(false);
	setAcceptDrops(true);
	setEnabled(false);

	// Everybody gets a focus proxy, but only one toplevel container's
	// focus proxy is actually in use.
	d->focusProxy = new QWidget(this);
	d->focusProxy->setAttribute(Qt::WA_NativeWindow);
	d->focusProxy->setAttribute(Qt::WA_DontCreateNativeAncestors);
	d->focusProxy->createWinId();
	d->focusProxy->winId();
	d->focusProxy->setGeometry(-1, -1, 1, 1);

	// We need events from the window (activation status) and
	// from qApp (keypress/release).
	qApp->installEventFilter(this);

	// Install X11 event filter.
	QCoreApplication::instance()->installNativeEventFilter(&x11EventFilter);
	QCoreApplication::instance()->installNativeEventFilter(this);

	XSelectInput(QX11Info::display(), internalWinId(),
				 KeyPressMask | KeyReleaseMask
				 | ButtonPressMask | ButtonReleaseMask | ButtonMotionMask
				 | KeymapStateMask
				 | PointerMotionMask
				 | EnterWindowMask | LeaveWindowMask
				 | FocusChangeMask
				 | ExposureMask
				 | StructureNotifyMask
				 | SubstructureNotifyMask);

	// Make sure our new event mask takes effect as soon as possible.
	XFlush(QX11Info::display());

	// Move input to our focusProxy if this widget is active, and not
	// shaded by a modal dialog (in which case isActiveWindow() would
	// still return true, but where we must not move input focus).

	if (qApp->activeWindow() == window() && !d->isEmbedded())
		d->moveInputToProxy();

#ifdef QX11EMBED_DEBUG
	qDebug() << "QX11EmbedContainer::QX11EmbedContainer: constructed container"
			 << (void *)this << "with winId" << winId();
#endif
}

/*!
	Destructs a QX11EmbedContainer.
*/
QX11EmbedContainer::~QX11EmbedContainer()
{
	Q_D(QX11EmbedContainer);
	if (d->client) {
		XUnmapWindow(QX11Info::display(), d->client);
		XReparentWindow(QX11Info::display(), d->client, QX11Info::appRootWindow(QX11Info::appScreen()), 0, 0);
	}

	if (d->xgrab)
		XUngrabButton(QX11Info::display(), AnyButton, AnyModifier, internalWinId());
}


QX11EmbedContainer::Error QX11EmbedContainer::error() const {
	return d_func()->lastError;
}

bool QX11EmbedContainer::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
	if (eventType == "xcb_generic_event_t") {
		return x11Event(message, result);
	} else {
		return false;
	}
}



/*! \reimp
*/
void QX11EmbedContainer::paintEvent(QPaintEvent *)
{
}

/*! \internal

	Returns whether or not the windows' embedded flag is set.
*/
bool QX11EmbedContainerPrivate::isEmbedded() const
{
	Q_Q(const QX11EmbedContainer);
	return ((QHackWidget *)q->window())->topData()->embedded == 1;
}

/*! \internal

	Returns the parentWinId of the window.
*/
WId QX11EmbedContainerPrivate::topLevelParentWinId() const
{
	Q_Q(const QX11EmbedContainer);
	return q->window()->effectiveWinId();
	//TODO
	//return ((QHackWidget *)q->window())->topData()->parentWinId;
}

/*!
	If the container has an embedded widget, this function returns
	the X11 window ID of the client; otherwise it returns 0.
*/
WId QX11EmbedContainer::clientWinId() const
{
	Q_D(const QX11EmbedContainer);
	return d->client;
}

/*!
	Instructs the container to embed the X11 window with window ID \a
	id. The client widget will then move on top of the container
	window and be resized to fit into the container.

	The \a id should be the ID of a window controlled by an XEmbed
	enabled application, but this is not mandatory. If \a id does not
	belong to an XEmbed client widget, then focus handling,
	activation, accelerators and other features will not work
	properly.
*/
void QX11EmbedContainer::embedClient(WId id)
{
	Q_D(QX11EmbedContainer);

	if (id == 0) {
		d->emitError(InvalidWindowID);
		return;
	}

	// Walk up the tree of parent windows to prevent embedding of ancestors.
	WId thisId = internalWinId();
	xcb_window_t rootReturn;
	xcb_window_t parentReturn;
	do {
		auto cookie = xcb_query_tree(QX11Info::connection(), thisId);
		xcb_generic_error_t* error = nullptr;
		auto reply = xcb_query_tree_reply(QX11Info::connection(), cookie, &error);

		if (error) {
			d->emitError(InvalidWindowID);
			return;
		}

		rootReturn = reply->root;
		parentReturn = reply->parent;

		thisId = parentReturn;
		if (id == thisId) {
			d->emitError(InvalidWindowID);
			return;
		}
	} while (thisId != rootReturn);

	switch (XReparentWindow(QX11Info::display(), id, internalWinId(), 0, 0)) {
	case BadWindow:
	case BadMatch:
		d->emitError(InvalidWindowID);
		break;
	default:
		break;
	}

#ifdef QX11EMBED_DEBUG
	qDebug() << "reparented client" << id << "into" << winId();
#endif
}

/*! \internal

	Handles key, activation and focus events for the container.
*/
bool QX11EmbedContainer::eventFilter(QObject *o, QEvent *event)
{
	Q_D(QX11EmbedContainer);
	switch (event->type()) {
	case QEvent::KeyPress:
		// Forward any keypresses to our client.
		if (o == this && d->client) {
			lastKeyEvent.event = d->client;
			xcb_send_event(QX11Info::connection(), false, d->client, KeyPressMask, (char*) &lastKeyEvent);
			return true;
		}
		break;
	case QEvent::KeyRelease:
		// Forward any keyreleases to our client.
		if (o == this && d->client) {
			lastKeyEvent.event = d->client;
			xcb_send_event(QX11Info::connection(), false, d->client, KeyReleaseMask, (char*) &lastKeyEvent);
			return true;
		}
		break;

	case QEvent::WindowActivate:
		// When our container window is activated, we pass the
		// activation message on to our client. Note that X input
		// focus is set to our focus proxy. We want to intercept all
		// keypresses.
		if (o == window() && d->client) {
			if (d->clientIsXEmbed) {
				sendXEmbedMessage(d->client, XEMBED_WINDOW_ACTIVATE);
			} else {
				d->checkGrab();
				if (hasFocus())
					XSetInputFocus(QX11Info::display(), d->client, XRevertToParent, x11Time());
			}
			if (!d->isEmbedded())
				d->moveInputToProxy();
		}
		break;
	case QEvent::WindowDeactivate:
		// When our container window is deactivated, we pass the
		// deactivation message to our client.
		if (o == window() && d->client) {
			if (d->clientIsXEmbed)
				sendXEmbedMessage(d->client, XEMBED_WINDOW_DEACTIVATE);
			else
				d->checkGrab();
		}
		break;
	case QEvent::FocusIn:
		// When receiving FocusIn events generated by Tab or Backtab,
		// we pass focus on to our client. Any mouse activity is sent
		// directly to the client, and it will ask us for focus with
		// XEMBED_REQUEST_FOCUS.
		if (o == this && d->client) {
			if (!d->isEmbedded())
				d->activeContainer = this;

			if (d->clientIsXEmbed) {
				if (!d->isEmbedded())
					d->moveInputToProxy();

				QFocusEvent *fe = (QFocusEvent *)event;
				switch (fe->reason()) {
				case Qt::TabFocusReason:
					sendXEmbedMessage(d->client, XEMBED_FOCUS_IN, XEMBED_FOCUS_FIRST);
					break;
				case Qt::BacktabFocusReason:
					sendXEmbedMessage(d->client, XEMBED_FOCUS_IN, XEMBED_FOCUS_LAST);
					break;
				default:
					sendXEmbedMessage(d->client, XEMBED_FOCUS_IN, XEMBED_FOCUS_CURRENT);
					break;
				}
			} else {
				d->checkGrab();
				XSetInputFocus(QX11Info::display(), d->client, XRevertToParent, x11Time());
			}
		}

		break;
	case QEvent::FocusOut: {
		// When receiving a FocusOut, we ask our client to remove its
		// focus.
		if (o == this && d->client) {
			if (!d->isEmbedded()) {
				d->activeContainer = 0;
				if (isActiveWindow())
					d->moveInputToProxy();
			}

			if (d->clientIsXEmbed) {
				QFocusEvent *fe = (QFocusEvent *)event;
				if (o == this && d->client && fe->reason() != Qt::ActiveWindowFocusReason)
					sendXEmbedMessage(d->client, XEMBED_FOCUS_OUT);
			} else {
				d->checkGrab();
			}
		}
	}
		break;

	case QEvent::Close: {
		if (o == this && d->client) {
			// Unmap the client and reparent it to the root window.
			// Wait until the messages have been processed. Then ask
			// the window manager to delete the window.
			XUnmapWindow(QX11Info::display(), d->client);
			XReparentWindow(QX11Info::display(), d->client, QX11Info::appRootWindow(QX11Info::appScreen()), 0, 0);
			XSync(QX11Info::display(), false);

			XEvent ev;
			memset(&ev, 0, sizeof(ev));
			ev.xclient.type = ClientMessage;
			ev.xclient.window = d->client;
			ev.xclient.message_type = ATOM(WM_PROTOCOLS);
			ev.xclient.format = 32;
			ev.xclient.data.s[0] = ATOM(WM_DELETE_WINDOW);
			XSendEvent(QX11Info::display(), d->client, false, NoEventMask, &ev);

			XFlush(QX11Info::display());
			d->client = 0;
			d->clientIsXEmbed = false;
			d->wmMinimumSizeHint = QSize();
			updateGeometry();
			setEnabled(false);
			update();

			emit clientClosed();
		}
	}
	default:
		break;
	}

	return QWidget::eventFilter(o, event);
}

/*! \internal

	Handles X11 events for the container.
*/
bool QX11EmbedContainer::x11Event(void *message, long*)
{
	xcb_generic_event_t* e = reinterpret_cast<xcb_generic_event_t*>(message);
	Q_D(QX11EmbedContainer);

	switch (e->response_type & ~0x80) {
	case XCB_CREATE_NOTIFY:
#ifdef QX11EMBED_DEBUG
		qDebug() << "client created" << reinterpret_cast<xcb_create_notify_event_t*>(e)->window;
#endif
		// The client created an embedded window.
		if (d->client)
			d->rejectClient(reinterpret_cast<xcb_create_notify_event_t*>(e)->window);
		else
			d->acceptClient(reinterpret_cast<xcb_create_notify_event_t*>(e)->window);
		break;
	case XCB_DESTROY_NOTIFY:
		if (reinterpret_cast<xcb_destroy_notify_event_t*>(e)->window == d->client) {
#ifdef QX11EMBED_DEBUG
			qDebug() << "client died";
#endif
			// The client died.
			d->client = 0;
			d->clientIsXEmbed = false;
			d->wmMinimumSizeHint = QSize();
			updateGeometry();
			update();
			setEnabled(false);
			emit clientClosed();
		}
		break;
	case XCB_REPARENT_NOTIFY:
		// The client sends us this if it reparents itself out of our
		// widget.
	{
		auto* event = reinterpret_cast<xcb_reparent_notify_event_t*>(e);
		if (event->window == d->client && event->parent != internalWinId()) {
			d->client = 0;
			d->clientIsXEmbed = false;
			d->wmMinimumSizeHint = QSize();
			updateGeometry();
			update();
			setEnabled(false);
			emit clientClosed();
		} else if (event->parent == internalWinId()) {
			// The client reparented itself into this window.
			if (d->client)
				d->rejectClient(event->window);
			else
				d->acceptClient(event->window);
		}
		break;
	}
	case XCB_CLIENT_MESSAGE: {
		auto* event = reinterpret_cast<xcb_client_message_event_t*>(e);
		if (event->type == ATOM(_XEMBED)) {
			// Ignore XEMBED messages not to ourselves
			if (event->window != internalWinId())
				break;

			// Receiving an XEmbed message means the client
			// is an XEmbed client.
			d->clientIsXEmbed = true;

			//TODO: Port to Qt5, if needed
			//Time msgtime = (Time) event->data.data32[0];
			//if (msgtime > X11->time)
			//X11->time = msgtime;

			switch (event->data.data32[1]) {
			case XEMBED_REQUEST_FOCUS: {
				// This typically happens when the client gets focus
				// because of a mouse click.
				if (!hasFocus())
					setFocus(Qt::OtherFocusReason);

				// The message is passed along to the topmost container
				// that eventually responds with a XEMBED_FOCUS_IN
				// message. The focus in message is passed all the way
				// back until it reaches the original focus
				// requestor. In the end, not only the original client
				// has focus, but also all its ancestor containers.
				if (d->isEmbedded()) {
					// If our window's embedded flag is set, then
					// that suggests that we are part of a client. The
					// parentWinId will then point to an container to whom
					// we must pass this message.
					sendXEmbedMessage(d->topLevelParentWinId(), XEMBED_REQUEST_FOCUS);
				} else {
					// Our window's embedded flag is not set,
					// so we are the topmost container. We respond to
					// the focus request message with a focus in
					// message. This message will pass on from client
					// to container to client until it reaches the
					// originator of the XEMBED_REQUEST_FOCUS message.
					sendXEmbedMessage(d->client, XEMBED_FOCUS_IN, XEMBED_FOCUS_CURRENT);
				}

				break;
			}
			case XEMBED_FOCUS_NEXT:
				// Client sends this event when it received a tab
				// forward and was at the end of its focus chain. If
				// we are the only widget in the focus chain, we send
				// ourselves a FocusIn event.
				if (d->focus_next != this) {
					focusNextPrevChild(true);
				} else {
					QFocusEvent event(QEvent::FocusIn, Qt::TabFocusReason);
					qApp->sendEvent(this, &event);
				}

				break;
			case XEMBED_FOCUS_PREV:
				// Client sends this event when it received a backtab
				// and was at the start of its focus chain. If we are
				// the only widget in the focus chain, we send
				// ourselves a FocusIn event.
				if (d->focus_next != this) {
					focusNextPrevChild(false);
				} else {
					QFocusEvent event(QEvent::FocusIn, Qt::BacktabFocusReason);
					qApp->sendEvent(this, &event);
				}

				break;
			default:
				break;
			}
		}
	}
		break;
	case XCB_BUTTON_PRESS:
	{
		auto event = reinterpret_cast<xcb_key_press_event_t*>(e);
		if (event->child == d->client && !d->clientIsXEmbed) {
			setFocus(Qt::MouseFocusReason);
			XAllowEvents(QX11Info::display(), ReplayPointer, CurrentTime);
			return true;
		}
	}
		break;
	case XCB_BUTTON_RELEASE:
		if (!d->clientIsXEmbed)
			XAllowEvents(QX11Info::display(), SyncPointer, CurrentTime);
		break;
	case XCB_PROPERTY_NOTIFY:
	{
		auto event = reinterpret_cast<xcb_property_notify_event_t*>(e);

		if (event->atom == ATOM(_XEMBED_INFO) && event->window == d->client) {
			if (auto info = get_xembed_info(d->client)) {
				if (info->flags & XEMBED_MAPPED) {
#ifdef QX11EMBED_DEBUG
					qDebug() << "mapping client per _xembed_info";
#endif
					XMapWindow(QX11Info::display(), d->client);
					XRaiseWindow(QX11Info::display(), d->client);
				} else {
#ifdef QX11EMBED_DEBUG
					qDebug() << "unmapping client per _xembed_info";
#endif
					XUnmapWindow(QX11Info::display(), d->client);
				}

				free(info);
			}
		}
		break;
	}
	case XCB_CONFIGURE_NOTIFY:
		return true;
	default:
		break;
	}

	return false;
}

/*! \internal

	Whenever the container is resized, we need to resize our client.
*/
void QX11EmbedContainer::resizeEvent(QResizeEvent *)
{
	Q_D(QX11EmbedContainer);
	if (d->client)
		XResizeWindow(QX11Info::display(), d->client, width(), height());
}

/*!
	\reimp
*/
bool QX11EmbedContainer::event(QEvent *event)
{
	if (event->type() == QEvent::ParentChange) {
		XSelectInput(QX11Info::display(), internalWinId(),
					 KeyPressMask | KeyReleaseMask
					 | ButtonPressMask | ButtonReleaseMask | ButtonMotionMask
					 | KeymapStateMask
					 | PointerMotionMask
					 | EnterWindowMask | LeaveWindowMask
					 | FocusChangeMask
					 | ExposureMask
					 | StructureNotifyMask
					 | SubstructureNotifyMask);
	}
	return QWidget::event(event);
}

/*! \internal

	Rejects a client window by reparenting it to the root window.  The
	client will receive a reparentnotify, and will most likely assume
	that the container has shut down. The XEmbed protocol does not
	define any way to reject a client window, but this is a clean way
	to do it.
*/
void QX11EmbedContainerPrivate::rejectClient(WId window)
{
	Q_Q(QX11EmbedContainer);
	q->setEnabled(false);
	XRemoveFromSaveSet(QX11Info::display(), client);
	XReparentWindow(QX11Info::display(), window, QX11Info::appRootWindow(QX11Info::appScreen()), 0, 0);
}

/*! \internal

	Accepts a client by mapping it, resizing it and optionally
	activating and giving it logical focusing through XEMBED messages.
*/
void QX11EmbedContainerPrivate::acceptClient(WId window)
{
	Q_Q(QX11EmbedContainer);
	client = window;
	q->setEnabled(true);

	XSelectInput(QX11Info::display(), client, PropertyChangeMask);

	// This tells Qt that we wish to forward DnD messages to
	// our client.
	if (!extra)
		createExtra();
	//TODO
	//extraData()->xDndProxy = client;

	unsigned int version = XEmbedVersion();
	unsigned int clientversion = 0;

	// Add this client to our saveset, so if we crash, the client window
	// doesn't get destroyed. This is useful for containers that restart
	// automatically after a crash, because it can simply reembed its clients
	// without having to restart them (KDE panel).
	XAddToSaveSet(QX11Info::display(), client);

	// XEmbed clients have an _XEMBED_INFO property in which we can
	// fetch the version
	if (auto info = get_xembed_info(client)) {
		clientIsXEmbed = true;
		clientversion = info->version;
		free(info);
	}

	// Store client window's original size and placement.
	Window root;
	int x_return, y_return;
	unsigned int width_return, height_return, border_width_return, depth_return;
	XGetGeometry(QX11Info::display(), client, &root, &x_return, &y_return,
				 &width_return, &height_return, &border_width_return, &depth_return);
	clientOriginalRect.setCoords(x_return, y_return,
								 x_return + width_return - 1,
								 y_return + height_return - 1);

	// Ask the client for its minimum size.
	XSizeHints size;
	long msize;
	if (XGetWMNormalHints(QX11Info::display(), client, &size, &msize) && (size.flags & PMinSize)) {
		wmMinimumSizeHint = QSize(size.min_width, size.min_height);
		q->updateGeometry();
	}

	// The container should set the data2 field to the lowest of its
	// supported version number and that of the client (from
	// _XEMBED_INFO property).
	unsigned int minversion = version > clientversion ? clientversion : version;
	sendXEmbedMessage(client, XEMBED_EMBEDDED_NOTIFY, 0, q->internalWinId(), minversion);

	// Resize it, but no smaller than its minimum size hint.
	XResizeWindow(QX11Info::display(),
				  client,
				  qMax(q->width(), wmMinimumSizeHint.width()),
				  qMax(q->height(), wmMinimumSizeHint.height()));
	q->update();

	// Not mentioned in the protocol is that if the container
	// is already active, the client must be activated to work
	// properly.
	if (q->window()->isActiveWindow())
		sendXEmbedMessage(client, XEMBED_WINDOW_ACTIVATE);

	// Also, if the container already has focus, then it must
	// send a focus in message to its new client; otherwise we ask
	// it to remove focus.
	if (q->focusWidget() == q && q->hasFocus())
		sendXEmbedMessage(client, XEMBED_FOCUS_IN, XEMBED_FOCUS_FIRST);
	else
		sendXEmbedMessage(client, XEMBED_FOCUS_OUT);

	if (!clientIsXEmbed) {
		checkGrab();
		if (q->hasFocus()) {
			XSetInputFocus(QX11Info::display(), client, XRevertToParent, x11Time());
		}
	} else {
		if (!isEmbedded())
			moveInputToProxy();
	}

	emit q->clientIsEmbedded();
}

/*! \internal

	Moves X11 keyboard input focus to the focusProxy, unless the focus
	is there already. When X11 keyboard input focus is on the
	focusProxy, which is a child of the container and a sibling of the
	client, X11 keypresses and keyreleases will always go to the proxy
	and not to the client.
*/
void QX11EmbedContainerPrivate::moveInputToProxy()
{
	Q_Q(QX11EmbedContainer);
	// Following Owen Taylor's advice from the XEmbed specification to
	// always use CurrentTime when no explicit user action is involved.
	XSetInputFocus(QX11Info::display(), focusProxy->internalWinId(), XRevertToParent, CurrentTime);
}

/*! \internal

	Ask the window manager to give us a default minimum size.
*/
QSize QX11EmbedContainer::minimumSizeHint() const
{
	Q_D(const QX11EmbedContainer);
	if (!d->client || !d->wmMinimumSizeHint.isValid())
		return QWidget::minimumSizeHint();
	return d->wmMinimumSizeHint;
}

/*! \internal

*/
void QX11EmbedContainerPrivate::checkGrab()
{
	Q_Q(QX11EmbedContainer);
	if (!clientIsXEmbed && q->isActiveWindow() && !q->hasFocus()) {
		if (!xgrab) {
			XGrabButton(QX11Info::display(), AnyButton, AnyModifier, q->internalWinId(),
						true, ButtonPressMask, GrabModeSync, GrabModeAsync,
						None, None);
		}
		xgrab = true;
	} else {
		if (xgrab)
			XUngrabButton(QX11Info::display(), AnyButton, AnyModifier, q->internalWinId());
		xgrab = false;
	}
}

/*!
	Detaches the client from the embedder. The client will appear as a
	standalone window on the desktop.
*/
void QX11EmbedContainer::discardClient()
{
	Q_D(QX11EmbedContainer);
	if (d->client) {
		XResizeWindow(QX11Info::display(), d->client, d->clientOriginalRect.width(),
					  d->clientOriginalRect.height());

		d->rejectClient(d->client);
	}
}

QT_END_NAMESPACE
