#include "SubprocessWrapper.h"
#include <QMdiArea>
#include <QtCore/QProcess>
#include <iostream>

#ifdef __linux__
#include <QX11EmbedContainer>
#include "GuiApplication.h"
#include "MainWindow.h"

#include <chrono>
#include <thread>

//from wmctrl/main.c by Tomas Styblo
//https://stackoverflow.com/questions/12324302/is-it-ok-to-put-a-standard-pure-c-header-include-directive-inside-a-namespace?noredirect=1&lq=1
namespace xlib {
#include <X11/Xlib.h>
#include <X11/Xatom.h>
//#include <X11/Xmu/WinUtil.h>
//#include <X11/cursorfont.h>
}
// https://bugs.freedesktop.org/show_bug.cgi?id=48117
#undef Status
#undef Unsorted
#undef None
#undef CursorShape
#undef KeyPress
#undef Bool
#undef GrayScale

#undef ScreenOfDisplay
#undef DefaultScreen
#define DefaultScreen(dpy) 	(((xlib::_XPrivDisplay)(dpy))->default_screen)
#define DefaultRootWindow(dpy) 	(ScreenOfDisplay(dpy,DefaultScreen(dpy))->root)
#define ScreenOfDisplay(dpy, scr)(&((xlib::_XPrivDisplay)(dpy))->screens[scr])

//using xlib::Display;
using xlib::Window;
using xlib::Atom;
using xlib::XOpenDisplay;
using xlib::XFree;
using xlib::XDefaultRootWindow;
using xlib::XUnmapWindow;
using xlib::XInternAtom;

//#include <glib.h>
typedef char gchar;
typedef bool gboolean;

#define g_malloc malloc
#define g_free free
#define TRUE true
#define FALSE false
#define _NET_WM_STATE_REMOVE        0    /* remove/unset property */
#define _NET_WM_STATE_ADD           1    /* add/set property */
#define _NET_WM_STATE_TOGGLE        2    /* toggle property  */

#define MAX_PROPERTY_VALUE_LEN 4096
#define SELECT_WINDOW_MAGIC ":SELECT:"
#define ACTIVE_WINDOW_MAGIC ":ACTIVE:"

static gchar* get_property(xlib::Display *disp, Window win, Atom xa_prop_type, std::string prop_name, unsigned long *size) {
	Atom xa_prop_name;
	Atom xa_ret_type;
	int ret_format;
	unsigned long ret_nitems;
	unsigned long ret_bytes_after;
	unsigned long tmp_size;
	unsigned char *ret_prop;
	gchar *ret;
	
	xa_prop_name = XInternAtom(disp, prop_name.c_str(), False);
	
	/* MAX_PROPERTY_VALUE_LEN / 4 explanation (XGetWindowProperty manpage):
	 *
	 * long_length = Specifies the length in 32-bit multiples of the
	 *               data to be retrieved.
	 *
	 * NOTE:  see 
	 * http://mail.gnome.org/archives/wm-spec-list/2003-March/msg00067.html
	 * In particular:
	 *
	 * 	When the X window system was ported to 64-bit architectures, a
	 * rather peculiar design decision was made. 32-bit quantities such
	 * as Window IDs, atoms, etc, were kept as longs in the client side
	 * APIs, even when long was changed to 64 bits.
	 *
	 */
	if (XGetWindowProperty(disp, win, xa_prop_name, 0, MAX_PROPERTY_VALUE_LEN / 4, False,
			xa_prop_type, &xa_ret_type, &ret_format,     
			&ret_nitems, &ret_bytes_after, &ret_prop) != Success) {
		std::cout << "Cannot get property: " << prop_name << std::endl;
		return NULL;
	}
  
	if (xa_ret_type != xa_prop_type) {
		std::cout << "Invalid type of property: " << prop_name << std::endl;
		XFree(ret_prop);
		return NULL;
	}

	/* null terminate the result to make string handling easier */
	tmp_size = (ret_format / 8) * ret_nitems;
	/* Correct 64 Architecture implementation of 32 bit data */
	if(ret_format==32) tmp_size *= sizeof(long)/4;
	ret = (gchar*)g_malloc(tmp_size + 1);
	memcpy(ret, ret_prop, tmp_size);
	ret[tmp_size] = '\0';

	if (size) {
		*size = tmp_size;
	}
	
	XFree(ret_prop);
	return ret;
}

static Window *get_client_list(xlib::Display *disp, unsigned long *size) {
	Window *client_list;
	if ((client_list = (Window *)get_property(disp, DefaultRootWindow(disp), 
					XA_WINDOW, "_NET_CLIENT_LIST", size)) == NULL) {
		if ((client_list = (Window *)get_property(disp, DefaultRootWindow(disp), 
						XA_CARDINAL, "_WIN_CLIENT_LIST", size)) == NULL) {
			fputs("Cannot get client list properties. \n"
				  "(_NET_CLIENT_LIST or _WIN_CLIENT_LIST)"
				  "\n", stderr);
			return NULL;
		}
	}
	return client_list;
}

static gchar *get_window_title(xlib::Display *disp, Window win) {
	gchar *title_utf8;
	gchar *wm_name;
	gchar *net_wm_name;

	wm_name = get_property(disp, win, XA_STRING, "WM_NAME", NULL);
	net_wm_name = get_property(
			disp, win, 
			XInternAtom(disp, "UTF8_STRING", False), "_NET_WM_NAME", NULL);

	if (net_wm_name) {
		title_utf8 = strdup(net_wm_name);
	} else {
		if (wm_name) {
			//title_utf8 = g_locale_to_utf8(wm_name, -1, NULL, NULL, NULL);
			title_utf8 = strdup(wm_name);
		} else {
			title_utf8 = NULL;
		}
	}

	g_free(wm_name);
	g_free(net_wm_name);
	return title_utf8;
}

static unsigned long list_windows(xlib::Display *disp, unsigned long match_pid) {
	std::cout << "checking for pid: " << match_pid << std::endl;
	Window *client_list;
	unsigned long client_list_size;
	int i;
	//int max_client_machine_len = 0;
	unsigned long xid = 0;  // Note: X Window is an unsigned long
	
	if ((client_list = get_client_list(disp, &client_list_size)) == NULL) {
		std::cout << "SubprocessWrapper static list_windows failed to get client_list" << std::endl;
		return 0;
	}
	
	/* find the longest client_machine name */
	//for (i = 0; i < client_list_size / sizeof(Window); i++) {
	//	gchar *client_machine;
	//	if ((client_machine = get_property(disp, client_list[i],
	//			XA_STRING, "WM_CLIENT_MACHINE", NULL))) {
	//		//max_client_machine_len = strlen(client_machine);    
	//	}
	//	g_free(client_machine);
	//}
	
	/* print the list */
	for (i = 0; i < client_list_size / sizeof(Window); i++) {
		std::cout << "checking window xid=" << client_list[i] << std::endl;

		gchar *title_utf8 = get_window_title(disp, client_list[i]); /* UTF8 */
		//gchar *title_out = get_output_str(title_utf8, TRUE);
		gchar *client_machine;
		//gchar *class_out = get_window_class(disp, client_list[i]); /* UTF8 */
		unsigned long *pid;
		unsigned long *desktop;
		int x, y, junkx, junky;
		unsigned int wwidth, wheight, bw, depth;
		Window junkroot;

		/* desktop ID */
		if ((desktop = (unsigned long *)get_property(disp, client_list[i],
				XA_CARDINAL, "_NET_WM_DESKTOP", NULL)) == NULL) {
			desktop = (unsigned long *)get_property(disp, client_list[i],
					XA_CARDINAL, "_WIN_WORKSPACE", NULL);
		}

		/* client machine */
		client_machine = get_property(disp, client_list[i],
				XA_STRING, "WM_CLIENT_MACHINE", NULL);
	   
		/* pid */
		pid = (unsigned long *)get_property(disp, client_list[i],
				XA_CARDINAL, "_NET_WM_PID", NULL);

		std::cout << "	window pid: " << *pid << std::endl;

		/* geometry */
		XGetGeometry (disp, client_list[i], &junkroot, &junkx, &junky,
						  &wwidth, &wheight, &bw, &depth);
		XTranslateCoordinates (disp, client_list[i], junkroot, junkx, junky,
							   &x, &y, &junkroot);
	  
		/* special desktop ID -1 means "all desktops", so we 
		   have to convert the desktop value to signed long */
		//printf("0x%.8lx %2ld", client_list[i], 
		//		desktop ? (signed long)*desktop : 0);

		if (match_pid == *pid) {
			std::cout << "	GOT PID MATCH: " << *pid << std::endl;
			xid = client_list[i];
		}
		//if (options.match_by_pid) {
		//   printf(" %-6lu", pid ? *pid : 0);
		//}
		//if (options.show_geometry) {
		//   printf(" %-4d %-4d %-4d %-4d", x, y, wwidth, wheight);
		//}
		//if (options.show_class) {
		//   printf(" %-20s ", class_out ? class_out : "N/A");
		//}

		//printf(" %*s %s\n",
		//	  max_client_machine_len,
		//	  client_machine ? client_machine : "N/A",
		//	  title_out ? title_out : "N/A"
		//);
		g_free(title_utf8);
		//g_free(title_out);
		g_free(desktop);
		g_free(client_machine);
		//g_free(class_out);
		g_free(pid);
	}
	g_free(client_list);
   
	return xid;
}

#endif

#ifdef __linux__
		void SubprocessWrapper::embedClient(unsigned long windowID) {
			connect(embedContainer, SIGNAL(clientIsEmbedded()), this, SLOT(handleClientEmbed()));
			this->embedContainer->embedClient( windowID );
		};
		unsigned long SubprocessWrapper::getWindowID() {
			return this->xid;
		}
		void SubprocessWrapper::handleClientEmbed() {
			//lock();
			//sendMessage( IdShowUI );
			//unlock();
			std::cout << "XEMBED DONE?" << std::endl;
		}
		void SubprocessWrapper::captureWindow(){
			connect(embedContainer, SIGNAL(clientIsEmbedded()), this, SLOT(handleClientEmbed()));
			this->pid = this->subprocess->pid();
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			xlib::Display *disp;
			if (! (disp = XOpenDisplay(NULL))) { return; }
			auto xxid = list_windows( disp, this->pid );
			std::cout << "got XID of window " << xxid << std::endl;
			if (xxid) {
				this->embedContainer->embedClient( xxid );
				//XUnmapWindow(disp, xxid);
				//QWindow* vw = QWindow::fromWinId(xxid);
				//vw->setFlags(Qt::FramelessWindowHint);
				//https://stackoverflow.com/questions/45061803/cannot-get-qwindowfromwinid-to-work-properly
				//vw->show();
				//vw->hide();
				//vw->setAttribute(Qt::WA_NativeWindow);
				//auto container = QWidget::createWindowContainer(vw);
				//gui->mainWindow()->workspace()->addSubWindow(container);
				//vw->show();
			} else {
				std::this_thread::sleep_for(std::chrono::milliseconds(2000));
				xxid = list_windows( disp, this->pid );
				std::cout << "(second try) got XID of window " << xxid << std::endl;
				if (xxid) {
					this->embedContainer->embedClient( xxid );
				} else {
					std::cout << "ERROR: can not get XID of window" << std::endl;
				}
			}
		}
#endif

SubprocessWrapper::SubprocessWrapper(QString exe, QStringList args, bool capture, QWidget* parent, int width, int height) {

#ifdef __linux__
	// a parent widget also breaks mplayer
	if (parent) {
		parent->setAttribute(Qt::WA_NativeWindow);
	}
	embedContainer = new QX11EmbedContainer(parent);
	embedContainer->setMinimumSize(width, height);
	gui->mainWindow()->workspace()->addSubWindow(embedContainer);
	embedContainer->show();  // this also sets the xid
	this->xid = embedContainer->winId();
	this->subprocess = new QProcess( embedContainer );
	if (capture) {
		//std::cout << "SubprocessWrapper starting window capture..." << std::endl;
		//connect(this->subprocess, SIGNAL(started()), this, SLOT(captureWindow()));
		//connect(this->subprocess, SIGNAL(started()), this, SIGNAL(captureWindow()));  // TODO fix runtime slot connect failure
		//captureWindow();
	} else {
		// assume in this case that the last arg is the xid, 
		// and the sub-application is able to embed itself into the given xid. 
		args << QString::number(xid);
	}
	subprocess->start(exe, args);

	if (capture) {  // workaround for runtime slot failure
		std::cout << "SubprocessWrapper starting window capture..." << std::endl;
		captureWindow();
	}
#else

	this->subprocess = new QProcess();
	subprocess->start(exe, args);

#endif
};

//Q_DECLARE_METATYPE(SubprocessWrapper)
Q_DECLARE_METATYPE( SubprocessWrapper* )