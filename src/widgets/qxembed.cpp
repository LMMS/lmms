/****************************************************************************
    Implementation of QXEmbed class

    Copyright (C) 1999-2002 Trolltech AS

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*****************************************************************************/


// L-000: About comments marked with Lxxxx.
// 
//    These comments represent an attempt to provide a more adequate
//    documentation to KDE developpers willing to modify QXEmbed.  Keep in
//    mind that these comments were written long after most of the code.
//    Please improve them if you spot something wrong or missing 
//    (Leon Bottou, 26-10-2003).
//
//    Relevant documents:
//    - QXEmbed developper documentation
//        (see comments in qxembed.h)
//    - Xlib Reference Manual  
//        (sections about focus, reparenting, window management)
//    - ICCCM Manual
//        (window management)
//    - XEMBED specification 
//        (http://www.freedesktop.org/Standards/xembed-spec)
//    - XPLAIN and XEMBED.
//        <http://lists.kde.org/?w=2&r=1&s=qxembed+variants&q=t>
//    - Accumulated community knowledge.
//        <http://lists.kde.org/?w=2&r=1&s=qxembed&q=t>
//        <http://lists.kde.org/?l=kde-devel&w=2&r=1&s=qxembed&q=b>
//        <http://lists.kde.org/?l=kfm-devel&w=2&r=1&s=qxembed&q=b>
// 

#include "qt3support.h"

#ifndef QT4


#include <qapplication.h>
#include <qptrlist.h>
#include <qptrdict.h>
#include <qguardedptr.h>
#include <qwhatsthis.h>
#include <qfocusdata.h>

// L0001: QXEmbed works only under X windows.
#ifdef Q_WS_X11

# include <X11/X.h>
# include <X11/Xlib.h>
# include <X11/Xutil.h>
# include <X11/Xatom.h>
# define XK_MISCELLANY
# define XK_LATIN1
# include <X11/keysymdef.h>
/*# include <kdebug.h>
# include <kxerrorhandler.h>*/

// L0002: Is file config.h KDE specific?
# include <config.h>
# ifdef HAVE_UNISTD_H
#  include <unistd.h>
#  ifdef HAVE_USLEEP
#   define USLEEP(x) usleep(x)
#  else
#   define USLEEP(x) sleep(0)
#  endif
# else
#  define USLEEP(x) sleep(0)
# endif

# include "qxembed.h"

// L0003: This keysym is used for focus navigation.
# ifndef XK_ISO_Left_Tab
#  define XK_ISO_Left_Tab 0xFE20
# endif

// L0004: Conflicts between X11 and Qt definitions.
const int XFocusOut = FocusOut;
const int XFocusIn = FocusIn;
const int XKeyPress = KeyPress;
const int XKeyRelease = KeyRelease;
# undef KeyRelease
# undef KeyPress
# undef FocusOut
# undef FocusIn

// L0005: Variables defined in qapplication_x11.cpp
extern Atom qt_wm_protocols;
extern Atom qt_wm_delete_window;
extern Atom qt_wm_take_focus;
extern Atom qt_wm_state;
extern Time qt_x_time;

// L0006: X11 atoms private to QXEmbed
static Atom xembed = 0;
static Atom context_help = 0;

// L0007: Xembed message codes (see XEmbed spec)
#define XEMBED_EMBEDDED_NOTIFY          0
#define XEMBED_WINDOW_ACTIVATE          1
#define XEMBED_WINDOW_DEACTIVATE        2
#define XEMBED_REQUEST_FOCUS            3
#define XEMBED_FOCUS_IN                 4
#define XEMBED_FOCUS_OUT                5
#define XEMBED_FOCUS_NEXT               6
#define XEMBED_FOCUS_PREV               7

// L0008: Xembed message details (see XEmbed spec)
// -- XEMBED_FOCUS_IN:
#define XEMBED_FOCUS_CURRENT            0
#define XEMBED_FOCUS_FIRST              1
#define XEMBED_FOCUS_LAST               2


// L0100: Private data held by the QXEmbed object.
//        This belongs to the embedder side.
class QXEmbedData
{
public:
    QXEmbedData(){ 
        autoDelete = true;
        xplain = false;
        xgrab = false;
        mapAfterRelease = false;
        lastPos = QPoint(0,0);
    }
    ~QXEmbedData(){};

    bool autoDelete;      // L0101: See L2600
    bool xplain;          // L0102: See L1100
    bool xgrab;           // L0103: See L2800
    bool mapAfterRelease;
    QWidget* focusProxy;  // L0104: See XEmbed spec
    QPoint lastPos;       // L0105: See L1390
};

namespace
{
    // L0200: This application wide event filter handles focus
    //        issues in the embedded client.
    class QXEmbedAppFilter : public QObject
    {
    public:
        QXEmbedAppFilter()  { qApp->installEventFilter( this ); } 
        ~QXEmbedAppFilter() { };
        bool eventFilter( QObject *, QEvent * );
    };
}

// L0201: See L0200, L0740
static QXEmbedAppFilter* filter = 0;
// L0202: See L0610, L0730
static QPtrDict<QGuardedPtr<QWidget> > *focusMap = 0;
// L0203: See L0660, L1400, L1450
static XKeyEvent last_key_event;

// L0300: This class gives access protected members of class QWidget.
//        Function focusData() is useful to reimplement tab focus management
//        (L0620) Function topData() returns a structure QTLWExtra containing
//        information unique to toplevel windows.  This structure contains two
//        members for the sole use of QXEmbed. Flag `embedded' indicates whether
//        the toplevel window is embedded using the XEMBED protocol (L0680). 
//        Handle `parentWinId' then records the id of the embedding window.

class QPublicWidget : public QWidget
{
public:
    QTLWExtra* topData() { return QWidget::topData(); }
    QFocusData *focusData(){ return QWidget::focusData(); }
    bool focusNextPrev(bool b) { return focusNextPrevChild(b); }
};

// L0400: This sets a very low level filter for X11 messages.
//        See qapplication_x11.cpp
typedef int (*QX11EventFilter) (XEvent*);
extern QX11EventFilter qt_set_x11_event_filter (QX11EventFilter filter);
static QX11EventFilter oldFilter = 0;


// L0500: Helper to send XEmbed messages.
static void sendXEmbedMessage( WId window, long message, long detail = 0,
                               long data1 = 0, long data2 = 0)
{
    if (!window) return;
    XEvent ev;
    memset(&ev, 0, sizeof(ev));
    ev.xclient.type = ClientMessage;
    ev.xclient.window = window;
    ev.xclient.message_type = xembed;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = qt_x_time;
    ev.xclient.data.l[1] = message;
    ev.xclient.data.l[2] = detail;
    ev.xclient.data.l[3] = data1;
    ev.xclient.data.l[4] = data2;
    XSendEvent(qt_xdisplay(), window, false, NoEventMask, &ev);
}

// L0501: Helper to send ICCCM Client messages. 
//        See X11 ICCCM Specification.
static void sendClientMessage(Window window, Atom a, long x)
{
  if (!window) return;
  XEvent ev;
  memset(&ev, 0, sizeof(ev));
  ev.xclient.type = ClientMessage;
  ev.xclient.window = window;
  ev.xclient.message_type = a;
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = x;
  ev.xclient.data.l[1] = qt_x_time;
  XSendEvent(qt_xdisplay(), window, false, NoEventMask, &ev);
}

// L0502: Helper to send fake X11 focus messages.
//        See X11 Reference Manual and Window Management stuff.
static void sendFocusMessage(Window window, int type, int mode, int detail)
{
  if (!window) return;
  XEvent ev;
  memset(&ev, 0, sizeof(ev));
  ev.xfocus.type = type;
  ev.xfocus.window = window;
  ev.xfocus.mode = mode;
  ev.xfocus.detail = detail;
  XSendEvent(qt_xdisplay(), window, false, FocusChangeMask, &ev);
}


// ------------------------------------------------------------
// L0600: MOST OF WHAT FOLLOWS CONCERNS THE CLIENT SIDE.
//        The following code mostly executes inside a Qt application swallowed
//        by QXEmbed widget.  It mostly consists of event filters that fight
//        the normal Qt mechanisms in order to implement the XEMBED protocol.
//        All this would be a lot simpler if it was implemented by Qt itself.



// L0610: This event filter receives all Qt events.  Its main purpose is to
//        capture the Qt focus events in the embedded client in order to
//        implement the XEMBED protocol. 
//
//        Let's start with a few reminders:
//
//        - X11 only has the concept of the "X11 focus window".  This window
//          basically receives all key events.  The ICCCM conventions define
//          how the window manager and the applications must cooperate to
//          choose the X11 focus window.
//
//        - Most toolkits, including Qt, maintain the concepts of 'active
//          widget' and 'Qt focus widget'.  A toplevel widget is active when
//          the X11 focus is set to one of its children.  By extension a
//          widget is active when its toplevel widget is active.  There is one
//          Qt focus widget for each toplevel widget.  When the toplevel
//          widget is active, all key events are sent to the Qt focus widget,
//          regardless of which descendant of the toplevel window has the X11
//          focus.  Widgets can adjust their appearance according to both 
//          their activation and focus states.  The Qt FocusIn and FocusOut 
//          events indicate when a widget simultaneously is active and has
//          the Qt focus. 
//
//        The XEMBED protocol defines ways to communicate abouth both
//        activation and focus. The embedded client is active as soon as the
//        embedding window is active (L0676, L0677).  A widget in the embedded
//        client receives key events when (1) it has the Qt focus in the
//        embedded application, and (2) the QXEmbed widget in the embedding
//        application is active and has the Qt focus.  The Qt library in the
//        embedded application is unaware of the focus status of the QXEmbed
//        widget.  We must make sure it does the right thing regarding the
//        sending of focus events and the visual appearance of the focussed 
//        widgets.  When the QXEmbed widget looses the Qt focus, we clear the 
//        focus in the embedded client (L1570, L0688). Conversely, when
//        the QXEmbed widget gains the Qt focus, we restore the Qt focus 
//        window in the embedded client (L1530, L0680, L0683). 
//        Variable focusMap is used to remember which was the Qt focus
//        widget in the embedded application.  All this would be a lot
//        simpler if it was implemented inside Qt...
//
//        The XPLAIN protocol is much less refined in this respect.
//        The activation status of the embedded client simply reflect
//        the focus status of the QXEmbed widget. This is achieved
//        by sending fake X11 focus message to the client (L1521, L1561).
//        A passive button grab (L2800) intercepts mouse activity in the
//        embedded client and sets the Qt focus to the QXEmbed widget
//        when this happens (L2060).  This can be achieved without
//        cooperation from the client.

bool QXEmbedAppFilter::eventFilter( QObject *o, QEvent * e)
{
    static bool obeyFocus = false;
    switch ( e->type() ) {
    case QEvent::MouseButtonPress:
        // L0612: This will become clear with L0614
        if ( !((QWidget*)o)->isActiveWindow() )
            obeyFocus = true;
        break;
    case QEvent::FocusIn:
        // L0613: FocusIn events either occur because the widget already was
        //        active and has just been given the Qt focus (L0614) or
        //        because the widget already had the Qt focus and just became
        //        active (L0615).
        if ( qApp->focusWidget() == o &&
             ((QPublicWidget*)qApp->focusWidget()->topLevelWidget())->topData()->embedded ) {
            QFocusEvent* fe = (QFocusEvent*) e;
            if ( obeyFocus || fe->reason() == QFocusEvent::Mouse ||
                 fe->reason() == QFocusEvent::Shortcut ) {
                // L0614: A widget in the embedded client was just given the Qt focus.
                //        Variable `obeyFocus' suggests that this is the result of mouse
                //        activity in the client.  The XEMBED_REQUEST_FOCUS message causes
                //        the embedding widget to take the Qt focus (L2085).
                WId window = ((QPublicWidget*)qApp->focusWidget()->topLevelWidget())->topData()->parentWinId;
                focusMap->remove( qApp->focusWidget()->topLevelWidget() );
                sendXEmbedMessage( window, XEMBED_REQUEST_FOCUS );
            } else if ( fe->reason() == QFocusEvent::ActiveWindow ) {
                // L0615: Both the embedder and the embedded client became active.
                //        But we do not know whether the QXEmbed widget has the Qt focus.
                //        So we clear the Qt focus for now.  If indeed the QXEmbed widget
                //        has the focus, it will receive a FocusIn message (L1530) and
                //        tell us to restore the focus (L0680, L0683).
                focusMap->remove( qApp->focusWidget()->topLevelWidget() );
                focusMap->insert( qApp->focusWidget()->topLevelWidget(),
                                  new QGuardedPtr<QWidget>(qApp->focusWidget()->topLevelWidget()->focusWidget() ) );
                // L0616: qApp->focusWidget() might belong to a modal dialog and not be 
                //        equal to qApp->focusWidget()->topLevelWidget()->focusWidget() !
                qApp->focusWidget()->clearFocus();
                // L0617: ??? [why not {obeyFocus=false; return true;} here?]
            }
            obeyFocus = false;
        }
        break;
    case QEvent::KeyPress: 
        if (qApp->focusWidget() == o &&
            ((QPublicWidget*)qApp->focusWidget()->topLevelWidget())->topData()->embedded ) {
            // L0620: The following code replaces the Qt code that 
            //        handles focus focus changes with the tab key. See the
            //        XEMBED specification for details.  The keypress event
            //        arrives here after an interesting itinerary. It is first
            //        saved in the embedding application (L0660). After being
            //        rejected for tab navigation in the embedding application
            //        (L1901), it gets forwarded to the embedded client
            //        (L1400) and arrives here.  Depending on the status of
            //        the tab chain in the embedded client, focus navigation
            //        messages are sent back to the embedding application
            //        (L0653, L0654) which then performs tab navigation
            //        (L2081).
            QKeyEvent *k = (QKeyEvent *)e;
            QWidget *w = qApp->focusWidget();
            // L0621: The following tests are copied from QWidget::event().
            bool res = false;
            bool tabForward = true;
            if ( !(k->state() & ControlButton || k->state() & AltButton) ) {
                if ( k->key() == Key_Backtab || (k->key() == Key_Tab && (k->state() & ShiftButton)) ) {
                    QFocusEvent::setReason( QFocusEvent::Backtab );
                    res = ((QPublicWidget*)w)->focusNextPrev( tabForward = false );
                    QFocusEvent::resetReason();
                } else if ( k->key() == Key_Tab ) {
                    QFocusEvent::setReason( QFocusEvent::Tab );
                    res = ((QPublicWidget*)w)->focusNextPrev( tabForward = true );
                    QFocusEvent::resetReason();
                }
            }
            if (res) {
                // L0625: We changed the focus because of tab/backtab key
                //        Now check whether we have been looping around.
                QFocusData *fd = ((QPublicWidget*)w)->focusData();
                WId window = ((QPublicWidget*)w->topLevelWidget())->topData()->parentWinId;
                QWidget *cw = 0;
                QWidget *fw = fd->home();
                if (tabForward && window) {
                    while (cw != w && cw != fw && cw != w->topLevelWidget()) 
                        cw = fd->prev();
                    if (cw != w)
                        sendXEmbedMessage( window, XEMBED_FOCUS_NEXT );
                } else if (window) {
                    while (cw != w && cw != fw && cw != w->topLevelWidget()) 
                        cw = fd->next();
                    if (cw != w)
                        sendXEmbedMessage( window, XEMBED_FOCUS_PREV );
                }
                // L0628: Qt should no longer process this event.
                return true;
            }
        }
        break;
    default:
        break;
    }
    // L0640: Application gets to see the events anyway.
    return false;
}

// L0650: This filter receives all XEvents in both the client and the embedder.  
//        Most of it involves the embedded client (except L0660, L0671).
static int qxembed_x11_event_filter( XEvent* e)
{
    switch ( e->type ) {
    case XKeyPress:
    case XKeyRelease: {
        // L0660: This is for the embedding side (L1450).
        last_key_event = e->xkey;
        break;
    }
    case ClientMessage:
        if ( e->xclient.message_type == xembed ) {
            // L0670: This is where the XEmbed messages are 
            //        processed on the client side.
            Time msgtime = (Time) e->xclient.data.l[0];
            long message = e->xclient.data.l[1];
            long detail = e->xclient.data.l[2];
            // L0671: Keep Qt message time up to date
            if ( msgtime > qt_x_time )
                qt_x_time = msgtime;
            QWidget* w = QWidget::find( e->xclient.window );
            if ( !w )
                break;
            switch ( message) {
            case XEMBED_EMBEDDED_NOTIFY: {
                // L0675: We just have been embedded into a XEMBED aware widget.
                QTLWExtra *extra = ((QPublicWidget*)w->topLevelWidget())->topData();
                extra->embedded = 1;
                extra->parentWinId = e->xclient.data.l[3];
                w->topLevelWidget()->show();
                break;
            }
            case XEMBED_WINDOW_ACTIVATE: {
                // L0676: Embedding window becomes active. Send a fake XFocusIn
                //        to convince Qt that we are active as well.  Qt will send
                //        us a focus notification (L0615) that we will intercept to
                //        ensure that we have no Qt focus widget yet.  The Qt focus
                //        widget might later be set in L0680.
                XEvent ev;
                memset(&ev, 0, sizeof(ev));
                ev.xfocus.display = qt_xdisplay();
                ev.xfocus.type = XFocusIn;
                ev.xfocus.window = w->topLevelWidget()->winId();
                ev.xfocus.mode = NotifyNormal;
                ev.xfocus.detail = NotifyAncestor;
                qApp->x11ProcessEvent( &ev );
            }
            break;
            case XEMBED_WINDOW_DEACTIVATE: {
                // L0677: Embedding window becomes inactive. Send a fake XFocusOut
                //        event to convince Qt that we no longer are active.  We will
                //        receive extra Qt FocusOut events but we do not care.
                XEvent ev;
                memset(&ev, 0, sizeof(ev));
                ev.xfocus.display = qt_xdisplay();
                ev.xfocus.type = XFocusOut;
                ev.xfocus.window = w->topLevelWidget()->winId();
                ev.xfocus.mode = NotifyNormal;
                ev.xfocus.detail = NotifyAncestor;
                qApp->x11ProcessEvent( &ev );
            }
            break;
            case XEMBED_FOCUS_IN:
                // L0680: Embedding application gives us the focus.
                {
                    // L0681: Search saved focus widget.
                    QWidget* focusCurrent = 0;
                    QGuardedPtr<QWidget>* fw = focusMap->find( w->topLevelWidget() );
                    if ( fw ) {
                        focusCurrent = *fw;
                        // L0682: Remove it from the map
                        focusMap->remove( w->topLevelWidget() );
                    }
                    switch ( detail ) {
                    case XEMBED_FOCUS_CURRENT:
                        // L0683: Set focus on saved focus widget
                        if ( focusCurrent )
                            focusCurrent->setFocus();
                        else if ( !w->topLevelWidget()->focusWidget() )
                            w->topLevelWidget()->setFocus();
                        break;
                    case XEMBED_FOCUS_FIRST:
                        {
                            // L0684: Search first widget in tab chain
                            QFocusEvent::setReason( QFocusEvent::Tab );
                            w->topLevelWidget()->setFocus();
                            ((QPublicWidget*)w->topLevelWidget())->focusNextPrev(true);
                            QFocusEvent::resetReason();
                        }
                        break;
                    case XEMBED_FOCUS_LAST:
                        {
                            // L0686: Search last widget in tab chain
                            QFocusEvent::setReason( QFocusEvent::Backtab );
                            w->topLevelWidget()->setFocus();
                            ((QPublicWidget*)w->topLevelWidget())->focusNextPrev(false);
                            QFocusEvent::resetReason();
                        }
                        break;
                    default:
                        break;
                    }
                }
                break;
            case XEMBED_FOCUS_OUT:
                // L0688: Embedding application takes the focus away
                //        We first record what the focus widget was
                //        and clear the Qt focus.
                if ( w->topLevelWidget()->focusWidget() ) {
                    focusMap->insert( w->topLevelWidget(),
                        new QGuardedPtr<QWidget>(w->topLevelWidget()->focusWidget() ) );
                    w->topLevelWidget()->focusWidget()->clearFocus();
                }
            break;
            default:
                break;
            }
        } else if ( e->xclient.format == 32 && e->xclient.message_type ) {
            if ( e->xclient.message_type == qt_wm_protocols ) {
                QWidget* w = QWidget::find( e->xclient.window );
                if ( !w )
                    break;
                // L0690: This is for the embedding side!
                //        See L0902 for more information about the focus proxy.
                //        Window manager may send WM_TAKE_FOCUS messages to the 
                //        embedding application to indicate that it becomes active. 
                //        But this also suggests that the window manager has
                //        changed the X11 focus. We want to make sure it goes
                //        to the focus proxy window eventually.
                Atom a = e->xclient.data.l[0];
                if ( a == qt_wm_take_focus ) {
                    // L0695: update Qt message time variable
                    if ( (ulong) e->xclient.data.l[1] > qt_x_time )
                        qt_x_time = e->xclient.data.l[1];
                    // L0696: There is no problem when the window is not active.
                    //        Qt will generate a WindowActivate event that will
                    //        do the job (L1310).  This does not happen if the
                    //        window is already active.  So we simulate it.
                    if ( w->isActiveWindow() ) {
                        QEvent e( QEvent::WindowActivate );
                        QApplication::sendEvent( w, &e );
                    }
                }
            }
        }
        break;
    default:
        break;
    }
    // L0698: The next x11 filter 
    if ( oldFilter )
        return oldFilter( e );
    // L0699: Otherwise process the event as usual.
    return false;
}



// L0700: Install the xembed filters in both client and embedder sides.
//        This function is called automatically when using
//        embedClientIntoWindow() or creating an instance of QXEmbed You may
//        have to call it manually for a client when using embedder-side
//        embedding, though.
void QXEmbed::initialize()
{
    static bool is_initialized = false;
    if ( is_initialized )
        return;

    // L0710: Atom used by the XEMBED protocol.
    xembed = XInternAtom( qt_xdisplay(), "_XEMBED", false );
    // L0720: Install low level filter for X11 events (L0650)
    oldFilter = qt_set_x11_event_filter( qxembed_x11_event_filter );
    // L0730: See L0610 for an explanation about focusMap.
    focusMap = new QPtrDict<QGuardedPtr<QWidget> >;
    focusMap->setAutoDelete( true );
    // L0740: Create client side application wide event filter (L0610)
    filter = new QXEmbedAppFilter;

    is_initialized = true;
}





// ------------------------------------------------------------
// L0800: MOST OF WHAT FOLLOWS CONCERNS THE EMBEDDER SIDE.
//        Things that happen inside a Qt application that contain
//        a QXEmbed widget for embedding other applications.
//        This applies to both the XEMBED and XPLAIN protocols.
//        Deviations are commented below.



// L0810: Class QXEmbed.
//        A QXEmbed widget serves as an embedder that can manage one single
//        embedded X-window. These so-called client windows can be arbitrary
//        Qt or non Qt applications.  There are two different ways of using
//        QXEmbed, from the client side or from the embedder's side.


// L0900: Constructs a xembed widget.
QXEmbed::QXEmbed(QWidget *parent, const char *name, WFlags f)
  : QWidget(parent, name, f)
{
    // L0901: Create private data. See L0100.
    d = new QXEmbedData;
    // L0902: Create focus proxy widget. See XEmbed specification.
    //        Each QXEmbed widget has a focus proxy window. Every single
    //        QXEmbed widget tries to force its focus proxy window onto the
    //        whole embedding application. They compete between themselves and
    //        against Qt (L0690, L0914, L1040, L1310, L1510, L1580). 
    //        This would be much simpler if implemented within Qt.
    d->focusProxy = new QWidget( topLevelWidget(), "xembed_focus" );
    d->focusProxy->setGeometry( -1, -1, 1, 1 );
    d->focusProxy->show();
    // make sure it's shown - for XSetInputFocus
    QApplication::sendPostedEvents( d->focusProxy, 0 );
    // L0903: Install the client side event filters
    //        because they also provide services for the embedder side
    //        See L0660, L0671, L0685.
    initialize();
    window = 0;
    setFocusPolicy(StrongFocus);
    setKeyCompression( false );

    // L0910: Trick Qt to create extraData();
    (void) topData();

    // L0912: We are mostly interested in SubstructureNotify
    //        This is sent when something happens to the children of
    //        the X11 window associated with the QXEmbed widget.
    XSelectInput(qt_xdisplay(), winId(),
                 KeyPressMask | KeyReleaseMask |
                 ButtonPressMask | ButtonReleaseMask |
                 KeymapStateMask |
                 ButtonMotionMask |
                 PointerMotionMask | // may need this, too
                 EnterWindowMask | LeaveWindowMask |
                 FocusChangeMask |
                 ExposureMask |
                 StructureNotifyMask |
                 SubstructureRedirectMask |
                 SubstructureNotifyMask
                 );
    // L0913: all application events pass through eventFilter().
    //        This is mostly used to force the X11 focus on the 
    //        proxy focus window. See L1300.
    topLevelWidget()->installEventFilter( this );
    qApp->installEventFilter( this );

    // L0914: Start moving the X11 focus on the focus proxy window.
    //        See L1581 to know why we do not use isActiveWindow().
    if ( qApp->activeWindow() == topLevelWidget() )
        if ( !((QPublicWidget*) topLevelWidget())->topData()->embedded )
            XSetInputFocus( qt_xdisplay(), d->focusProxy->winId(), 
                            RevertToParent, qt_x_time );
    // L0915: ??? [drag&drop?]
    setAcceptDrops( true );
}

// L1000: Destructor must dispose of the embedded client window.
QXEmbed::~QXEmbed()
{
    // L1010: Make sure no pointer grab is left.
    if ( d && d->xgrab)
        XUngrabButton( qt_xdisplay(), AnyButton, AnyModifier, winId() );
    if ( window && ( autoDelete() || !d->xplain ))
        {
            // L1021: Hide the window and safely reparent it into the root,
            //        otherwise it would be destroyed by X11 together 
            //        with this QXEmbed's window.
#if 0
// TODO: The proper XEmbed way would be to unmap the window, and the embedded
// app would detect the embedding has ended, and do whatever it finds appropriate.
// However, QXEmbed currently doesn't provide support for this detection,
// so for the time being, it's better to leave the window mapped as toplevel window.
// This will be ever more complicated with the systray windows, as the simple API
// for them (KWin::setSystemTrayWindowFor()) doesn't make it possible to detect
// themselves they have been released from systray, but KWin requires them
// to be visible to allow next Kicker instance to swallow them.
// See also below the L1022 comment.
//            XUnmapWindow( qt_xdisplay(), window );
#else
            if( autoDelete())
                XUnmapWindow( qt_xdisplay(), window );
#endif
            XReparentWindow(qt_xdisplay(), window, qt_xrootwin(), 0, 0);
            if( !d->xplain )
                XRemoveFromSaveSet( qt_xdisplay(), window );
            if( d->mapAfterRelease )
                XMapWindow( qt_xdisplay(), window );
            XSync(qt_xdisplay(), false);
            // L1022: Send the WM_DELETE_WINDOW message
            if( autoDelete() /*&& d->xplain*/ ) 
                // This sendDelete should only apply to XPLAIN.
                // XEMBED apps are supposed to detect when the embedding ends.
                // ??? [We do not do this detection yet! 
                //      So we sendDelete() instead.]
                sendDelete();
      }
    window = 0;
    // L01040: Our focus proxy window will be destroyed as well.
    //         Make sure that the X11 focus is not lost in the process.
    Window focus;
    int revert;
    XGetInputFocus( qt_xdisplay(), &focus, &revert );
    if( focus == d->focusProxy->winId())
        XSetInputFocus( qt_xdisplay(), topLevelWidget()->winId(), RevertToParent, qt_x_time );
    // L01045: Delete our private data.
    delete d;
}


// L1050: Sends a WM_DELETE_WINDOW message to the embedded window.  This is
//        what typically happens when you click on the close button of a 
//        window manager decoration.
void QXEmbed::sendDelete( void )
{
  if (window)
    {
      sendClientMessage(window, qt_wm_protocols, qt_wm_delete_window);
      XFlush( qt_xdisplay() );
    }
}

// L1100: Sets the protocol used for embedding windows.
//        This function must be called before embedding a window.
//        Protocol XEMBED provides maximal functionality (focus, tabs, etc)
//        but requires explicit cooperation from the embedded window.  
//        Protocol XPLAIN provides maximal compatibility with 
//        embedded applications that do not support the XEMBED protocol.
//        The default is XEMBED.  
void QXEmbed::setProtocol( Protocol proto )
{
    if (!window) {
        d->xplain = false;
        if (proto == XPLAIN)
            d->xplain = true;
    }
}

// L1150: Returns the protocol used for embedding the current window.
QXEmbed::Protocol QXEmbed::protocol()
{
  if (d->xplain)
    return XPLAIN;
  return XEMBED;
}


// L1200: QXEmbed widget size changes: resize embedded window.
void QXEmbed::resizeEvent(QResizeEvent*)
{
    if (window)
        XResizeWindow(qt_xdisplay(), window, width(), height());
}

// L1250: QXEmbed widget is shown: make sure embedded window is visible.
void QXEmbed::showEvent(QShowEvent*)
{
    if (window)
        XMapRaised(qt_xdisplay(), window);
}


// L1300: This event filter sees all application events (L0913).
bool QXEmbed::eventFilter( QObject *o, QEvent * e)
{

    switch ( e->type() ) {
    case QEvent::WindowActivate:
        if ( o == topLevelWidget() ) {
            // L1310: Qt thinks the application window has just been activated.
            //        Make sure the X11 focus is on the focus proxy window. See L0686.
            if ( !((QPublicWidget*) topLevelWidget())->topData()->embedded )
                if (! hasFocus() )
                    XSetInputFocus( qt_xdisplay(), d->focusProxy->winId(), 
                                    RevertToParent, qt_x_time );
            if (d->xplain)
                // L1311: Activation has changed. Grab state might change. See L2800.
                checkGrab();
            else
                // L1312: Let the client know that we just became active
                sendXEmbedMessage( window, XEMBED_WINDOW_ACTIVATE );
        }
        break;
    case QEvent::WindowDeactivate:
        if ( o == topLevelWidget() ) {
            if (d->xplain)
                // L1321: Activation has changed. Grab state might change. See L2800.
                checkGrab();
            else
                // L1322: Let the client know that we are no longer active
                sendXEmbedMessage( window, XEMBED_WINDOW_DEACTIVATE );
        }
        break;
    case QEvent::Move:
        {
            QWidget* pos = this;
            while( pos != o && pos != topLevelWidget())
                pos = pos->parentWidget();
            if( pos == o ) {
                // L1390: Send fake configure notify events whenever the
                //        global position of the client changes. See L2900.
                QPoint globalPos = mapToGlobal(QPoint(0,0));
                if (globalPos != d->lastPos) {
                    d->lastPos = globalPos;
                    sendSyntheticConfigureNotifyEvent();
                }
            }
        }                    
        break;
    default:
        break;
   }
   return false;
}

// L1350: ??? [why this?]
bool  QXEmbed::event( QEvent * e)
{
    return QWidget::event( e );
}

// L1400: Forward keypress event to the client
//        Receiving a Qt key event indicates that
//        the QXEmbed object has the Qt focus.
//        The X11 event that caused the Qt key event
//        must be forwarded to the client.
//        See L0660.
void QXEmbed::keyPressEvent( QKeyEvent *)
{
    if (!window)
        return;
    last_key_event.window = window;
    XSendEvent(qt_xdisplay(), window, false, KeyPressMask, (XEvent*)&last_key_event);

}

// L1450: Forward keyrelease event to the client.
//        See comment L1400.
void QXEmbed::keyReleaseEvent( QKeyEvent *)
{
    if (!window)
        return;
    last_key_event.window = window;
    XSendEvent(qt_xdisplay(), window, false, KeyReleaseMask, (XEvent*)&last_key_event);
}

// L1500: Handle Qt focus in event.
void QXEmbed::focusInEvent( QFocusEvent * e ){
    if (!window)
        return;
    // L1510: This is a good time to set the X11 focus on the focus proxy window.
    //        Except if the the embedding application itself is embedded into another.
    if ( !((QPublicWidget*) topLevelWidget())->topData()->embedded )
      if ( qApp->activeWindow() == topLevelWidget() )
          // L1511: Alter X focus only when window is active. 
          //        This is dual safety here because FocusIn implies this.
          //        But see L1581 for an example where this really matters.
          XSetInputFocus( qt_xdisplay(), d->focusProxy->winId(), 
                          RevertToParent, qt_x_time );
    if (d->xplain) {
        // L1520: Qt focus has changed. Grab state might change. See L2800.
        checkGrab();
        // L1521: Window managers activate applications by setting the X11 focus.
        //        We cannot do this (see L1510) but we can send a fake focus event
        //        and forward the X11 key events ourselves (see L1400, L1450).
        sendFocusMessage(window, XFocusIn, NotifyNormal, NotifyPointer );
    } else {
        // L1530: No need for fake events with XEMBED.
        //        Just inform the client. It knows what to do.
        int detail = XEMBED_FOCUS_CURRENT;
        // L1531: When the focus change is caused by the tab key,
        //        the client must select the first (or last) widget of
        //        its own tab chain.
        if ( e->reason() == QFocusEvent::Tab )
            detail = XEMBED_FOCUS_FIRST;
        else if ( e->reason() == QFocusEvent::Backtab )
            detail = XEMBED_FOCUS_LAST;
        sendXEmbedMessage( window, XEMBED_FOCUS_IN, detail);
    }
}

// L1550: Handle Qt focus out event.
void QXEmbed::focusOutEvent( QFocusEvent * ){
    if (!window)
        return;
    if (d->xplain) {
        // L1560: Qt focus has changed. Grab state might change. See L2800.
        checkGrab();
        // L1561: Send fake focus out message. See L1521.
        sendFocusMessage(window, XFocusOut, NotifyNormal, NotifyPointer );
    } else {
        // L1570: Send XEMBED focus out message. See L1531.
        sendXEmbedMessage( window, XEMBED_FOCUS_OUT );
    }
    // L1580: The QXEmbed object might loose the focus because its
    //        toplevel window looses the X11 focus and is no longer active, 
    //        or simply because the Qt focus has been moved to another widget.
    //        In the latter case only, we want to make sure that the X11 focus
    //        is properly set to the X11 focus widget.  We do this because
    //        the client application might have moved the X11 focus after
    //        receiving the fake focus messages.
    if ( !((QPublicWidget*) topLevelWidget())->topData()->embedded )
        if ( qApp->activeWindow() == topLevelWidget() )
            // L1581: Alter X focus only when window is active.
            //        The test above is not the same as isActiveWindow().
            //        Function isActiveWindow() also returns true when a modal
            //        dialog child of this window is active.
            XSetInputFocus( qt_xdisplay(), d->focusProxy->winId(), 
                            RevertToParent, qt_x_time );
}


// L1600: Helper for QXEmbed::embed()
//        Check whether a window is in withdrawn state.
static bool wstate_withdrawn( WId winid )
{
    Atom type;
    int format;
    unsigned long length, after;
    unsigned char *data;
    int r = XGetWindowProperty( qt_xdisplay(), winid, qt_wm_state, 0, 2,
                                false, AnyPropertyType, &type, &format,
                                &length, &after, &data );
    bool withdrawn = true;
    // L1610: Non managed windows have no WM_STATE property.
    //        Returning true ensures that the loop L1711 stops.
    if ( r == Success && data && format == 32 ) {
        Q_UINT32 *wstate = (Q_UINT32*)data;
        withdrawn  = (*wstate == WithdrawnState );
        XFree( (char *)data );
    }
    return withdrawn;
}

// L1650: Helper for QXEmbed::embed()
//        Get the X11 id of the parent window.
static int get_parent(WId winid, Window *out_parent)
{
    Window root, *children=0;
    unsigned int nchildren;
    int st = XQueryTree(qt_xdisplay(), winid, &root, out_parent, &children, &nchildren);
    if (st && children) 
        XFree(children);
    return st;
}

// L1700: Embeds the window w into this QXEmbed widget.
//        See doc in qxembed.h.
void QXEmbed::embed(WId w)
{
/*    kdDebug() << "*** Embed " << w << " into " << winId() << ". window=" << window << endl;*/
    if (!w)
        return;
    // L1701: The has_window variable prevents embedding a same window twice.
    //        ??? [what happens if one embed two windows into the same QXEmbed?]
    bool has_window =  (w == window);
    window = w;
    if ( !has_window ) {
        //KXErrorHandler errhandler; // make X BadWindow errors silent
        // L1710: Try hard to withdraw the window.
        //        This makes sure that the window manager will
        //        no longer try to manage this window.
        if ( !wstate_withdrawn(window) ) {
            XWithdrawWindow(qt_xdisplay(), window, qt_xscreen());
            QApplication::flushX();
            // L1711: See L1610
            while (!wstate_withdrawn(window))
                USLEEP(1000);
        }
        // L1710: It would be sufficient in principle to reparent
        //        window w into winId(). Everything else happens in L2020.
        //        The following code might be useful when the X11 server takes 
        //        time to create the embedded application main window.
        Window parent = 0;
        get_parent(w, &parent);
/*        kdDebug() << QString("> before reparent: parent=0x%1").arg(parent,0,16) << endl;*/
        for (int i = 0; i < 50; i++) {
            // this is done once more when finishing embedding, but it's done also here
            // just in case we crash before reaching that place
            if( !d->xplain )
                XAddToSaveSet( qt_xdisplay(), w );
            XReparentWindow(qt_xdisplay(), w, winId(), 0, 0);
            if (get_parent(w, &parent) && parent == winId()) {
/*               kdDebug() << QString("> Loop %1: ").arg(i)
                         << QString("> reparent of 0x%1").arg(w,0,16)
                         << QString(" into 0x%1").arg(winId(),0,16)
                         << QString(" successful") << endl;*/
                break;
            }
/*            kdDebug() << QString("> Loop %1: ").arg(i)
                      << QString("> reparent of 0x%1").arg(w,0,16)
                      << QString(" into 0x%1").arg(winId(),0,16)
                      << QString(" failed") << endl;*/
            USLEEP(1000);
        }
        if( parent != winId()) // failed
            window = 0;
    }
}


// L1800: Returns the window identifier of the embedded window
WId QXEmbed::embeddedWinId() const
{
    return window;
}


// L1900: Control Qt tab focus management.
//        See Qt documentation.
bool QXEmbed::focusNextPrevChild( bool next )
{
    if ( window )
        // L1901: Return false when there is an embedded window
        //        When the user presses TAB, Qt will not change 
        //        the focus and pass the TAB key events to the QXEmbed widget.
        //        These key events will be forwarded to the client (L1400, L1450)
        //        who eventually will manage the tab focus (L0620) and possible
        //        instruct us to call QWidget::focusNextPrevChild (L2081).
        return false;
    else
        // L1920: Default behavior otherwise.
        return QWidget::focusNextPrevChild( next );
}


// L2000: Filter for X11 events sent to the QXEmbed window.
bool QXEmbed::x11Event( XEvent* e)
{
    switch ( e->type ) {
    case DestroyNotify:
        if ( e->xdestroywindow.window == window ) {
            // L2005: Client window is being destroyed.
            window = 0;
            windowChanged( window );
            emit embeddedWindowDestroyed();
        }
        break;
    case ReparentNotify:
        if ( e->xreparent.window == d->focusProxy->winId() )
            break; // ignore proxy
        if ( window && e->xreparent.window == window &&
             e->xreparent.parent != winId() ) {
            // L2010: We lost the window
            window = 0;
            windowChanged( window );
            emit embeddedWindowDestroyed();
            // L2011: Remove window from save set
            //        ??? [not sure it is good to touch this window since
            //             someone else has taken control of it already.]
            if( !d->xplain )
                XRemoveFromSaveSet( qt_xdisplay(), window );
        } else if ( e->xreparent.parent == winId()){
            // L2020: We got a window. Complete the embedding process.
            window = e->xreparent.window;
            // only XEMBED apps can survive crash,
            // see http://lists.kde.org/?l=kfm-devel&m=106752026501968&w=2
            if( !d->xplain )
                XAddToSaveSet( qt_xdisplay(), window );
            XResizeWindow(qt_xdisplay(), window, width(), height());
            XMapRaised(qt_xdisplay(), window);
            // L2024: see L2900.
            sendSyntheticConfigureNotifyEvent();
            // L2025: ??? [any idea about drag&drop?] 
            extraData()->xDndProxy = window;
            if ( parent() ) {
                // L2030: embedded window might have new size requirements.
                //        see L2500, L2520, L2550.
                QEvent * layoutHint = new QEvent( QEvent::LayoutHint );
                QApplication::postEvent( parent(), layoutHint );
            }
            windowChanged( window );
            if (d->xplain) {
                // L2040: Activation has changed. Grab state might change. See L2800.
                checkGrab();
                if ( hasFocus() )
                    // L2041: Send fake focus message to inform the client. See L1521.
                    sendFocusMessage(window, XFocusIn, NotifyNormal, NotifyPointer );
            } else {
                // L2050: Send XEMBED messages (see L0670, L1312, L1322, L1530)
                sendXEmbedMessage( window, XEMBED_EMBEDDED_NOTIFY, 0, (long) winId() );
                if (isActiveWindow())
                    sendXEmbedMessage( window, XEMBED_WINDOW_ACTIVATE);
                else
                    sendXEmbedMessage( window, XEMBED_WINDOW_DEACTIVATE);
                if ( hasFocus() )
                    sendXEmbedMessage( window, XEMBED_FOCUS_IN, XEMBED_FOCUS_CURRENT );
            }
        }
        break;
    case ButtonPress:
        if (d->xplain && d->xgrab) {
            // L2060: The passive grab has intercepted a mouse click
            //        in the embedded client window. Take the focus.
            QFocusEvent::setReason( QFocusEvent::Mouse );
            setFocus();
            QFocusEvent::resetReason();
            // L2064: Resume X11 event processing.
            XAllowEvents(qt_xdisplay(), ReplayPointer, CurrentTime);
            // L2065: Qt should not know about this.
            return true;
        }
        break;
    case ButtonRelease:
        if (d->xplain && d->xgrab) {
            // L2064: Resume X11 event processing after passive grab (see L2060)
            XAllowEvents(qt_xdisplay(), SyncPointer, CurrentTime);
            return true;
        }
        break;
    case MapRequest:
        // L2070: Behave like a window manager.
        if ( window && e->xmaprequest.window == window )
            XMapRaised(qt_xdisplay(), window );
        break;
    case ClientMessage:
        // L2080: This is where the QXEmbed object receives XEMBED 
        //        messaged from the client application.
        if ( e->xclient.format == 32 && e->xclient.message_type == xembed ) {
            long message = e->xclient.data.l[1];
            switch ( message ) {
                // L2081: Tab focus management. It is very important to call the 
                //        focusNextPrevChild() defined by QWidget (not QXEmbed). 
                //        See L1901.
            case XEMBED_FOCUS_NEXT:
                QWidget::focusNextPrevChild( true );
                break;
            case XEMBED_FOCUS_PREV:
                QWidget::focusNextPrevChild( false );
                break;
                // L2085: The client asks for the focus.
            case XEMBED_REQUEST_FOCUS:
                if( ((QPublicWidget*)topLevelWidget())->topData()->embedded ) {
                    WId window = ((QPublicWidget*)topLevelWidget())->topData()->parentWinId;
                    sendXEmbedMessage( window, XEMBED_REQUEST_FOCUS );
                } else {
                    QFocusEvent::setReason( QFocusEvent::Mouse );
                    setFocus();
                    QFocusEvent::resetReason();
                }
                break;
            default:
                break;
            }
        }
	break;

    case ConfigureRequest:
        // L2090: Client wants to change its geometry. 
        //        Just inform it that nothing has changed.
        if (e->xconfigurerequest.window == window) 
        {
             sendSyntheticConfigureNotifyEvent();
        }
        break;
    case MotionNotify: 
	// fall through, workaround for Qt 3.0 < 3.0.3
    case EnterNotify:
        // L2095: See L2200.
        if ( QWhatsThis::inWhatsThisMode() )
            enterWhatsThisMode();
        break;
    default:
        break;
    }
    return false;
}


// L2200: Try to handle Qt's "what's this" mode.  Broken.
//        "temporary, fix in Qt (Matthias, Mon Jul 17 15:20:55 CEST 2000"
void QXEmbed::enterWhatsThisMode()
{
    // L2210: When the what-s-this pointer enters the embedded window (L2095)
    //        cancel what-s-this mode, and use a non stantard _NET_WM_ message
    //        to instruct the embedded client to enter the "what's this" mode.
    //        This works only one way...
    QWhatsThis::leaveWhatsThisMode();
    if ( !context_help )
        context_help = XInternAtom( x11Display(), "_NET_WM_CONTEXT_HELP", false );
    sendClientMessage(window , qt_wm_protocols, context_help );
}


// L2300: indicates that the embedded window has been changed.
void QXEmbed::windowChanged( WId )
{
}


// L2400: Utility function for clients that embed themselves.
//        This is client side code.
bool QXEmbed::processClientCmdline( QWidget* client, int& argc, char ** argv )
{
    int myargc = argc;
    WId window = 0;
    int i, j;

    j = 1;
    for ( i=1; i<myargc; i++ ) {
        if ( argv[i] && *argv[i] != '-' ) {
            argv[j++] = argv[i];
            continue;
        }
        QCString arg = argv[i];
        if ( !strcmp(arg,"-embed") && i < myargc-1 ) {
            QCString s = argv[++i];
            window = s.toInt();
        } else
            argv[j++] = argv[i];
    }
    argc = j;

    if ( window ) {
        embedClientIntoWindow( client, window );
        return true;
    }

    return false;
}


// L2450: Utility function for clients that embed themselves.
//        This is client side code.
void QXEmbed::embedClientIntoWindow(QWidget* client, WId window)
{
    initialize();
    XReparentWindow(qt_xdisplay(), client->winId(), window, 0, 0);
    // L2451: These two lines are redundant. See L0680.
    ((QXEmbed*)client)->topData()->embedded = true;
    ((QXEmbed*)client)->topData()->parentWinId = window;
    // L2452: This seems redundant because L2020 maps the window.
    //        But calling show() might also set Qt internal flags.
    client->show();
}



// L2500: Specifies that this widget can use additional space,
//        and that it can survive on less than sizeHint().
QSizePolicy QXEmbed::sizePolicy() const
{
    return QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
}


// L2520: Returns a size sufficient for the embedded window
QSize QXEmbed::sizeHint() const
{
    return minimumSizeHint();
}

// L2550: Returns the minimum size specified by the embedded window.
QSize QXEmbed::minimumSizeHint() const
{
    int minw = 0;
    int minh = 0;
    if ( window ) {
        XSizeHints size;
        long msize;
        if (XGetWMNormalHints(qt_xdisplay(), window, &size, &msize)
            && ( size.flags & PMinSize) ) {
            minw = size.min_width;
            minh = size.min_height;
        }
    }

    return QSize( minw, minh );
}

// L2600: Tells what shoud be done with the embedded window when
//        the embedding window is destroyed. 
void QXEmbed::setAutoDelete( bool b)
{
    d->autoDelete = b;
}

// L2650: See L2600.
bool QXEmbed::autoDelete() const
{
    return d->autoDelete;
}

// L2700: See L2200.
bool QXEmbed::customWhatsThis() const
{
    return true;
}

// L2800: When using the XPLAIN protocol, this function maintains
//        a passive button grab when (1) the application is active
//        and (2) the Qt focus is not on the QXEmbed.  This passive
//        grab intercepts button clicks in the client window and
//        give us chance to request the Qt focus (L2060).
void QXEmbed::checkGrab() 
{
    if (d->xplain && isActiveWindow() && !hasFocus()) {
        if (! d->xgrab)
            XGrabButton(qt_xdisplay(), AnyButton, AnyModifier, winId(),
                        false, ButtonPressMask, GrabModeSync, GrabModeAsync,
                        None, None );
        d->xgrab = true;
    } else {
        if (d->xgrab)
            XUngrabButton( qt_xdisplay(), AnyButton, AnyModifier, winId() );
        d->xgrab = false;
    }
}

// L2900: This sends fake configure notify events to inform
//        the client about its window geometry. See L1390, L2024 and L2090.
void QXEmbed::sendSyntheticConfigureNotifyEvent() 
{
    // L2910: It seems that the x and y coordinates are global.
    //        But this is what ICCCM section 4.1.5 wants.
    //        See http://lists.kde.org/?l=kfm-devel&m=107090222032378
    QPoint globalPos = mapToGlobal(QPoint(0,0));
    if (window) {
        XConfigureEvent c;
        memset(&c, 0, sizeof(c));
        c.type = ConfigureNotify;
        c.display = qt_xdisplay();
        c.send_event = True;
        c.event = window;
        c.window = winId();
        c.x = globalPos.x();
        c.y = globalPos.y();
        c.width = width();
        c.height = height();
        c.border_width = 0;
        c.above = None;
        c.override_redirect = 0;
        XSendEvent( qt_xdisplay(), c.event, true, StructureNotifyMask, (XEvent*)&c );
    }
}

// L3000: One should not call QWidget::reparent after embedding a window.
void QXEmbed::reparent( QWidget * parent, WFlags f, const QPoint & p, bool showIt )
{
    // QWidget::reparent() destroys the old X Window for the widget, and
    // creates a new one, thus QXEmbed after reparenting is no longer the
    // parent of the embedded window.  I think reparenting of QXEmbed can be
    // done only by a mistake, so just complain.
    Q_ASSERT( !window );
    QWidget::reparent( parent, f, p, showIt );
}

// for KDE
#include "qxembed.moc"
#endif
#endif // Q_WS_X11
