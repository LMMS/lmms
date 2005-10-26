/*
 * qt3support.h - layer for supporting Qt3
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _QT3SUPPORT_H
#define _QT3SUPPORT_H

// don't know why following line is neccessary, it's a bug in qt-headers -
// otherwise LMMS sometimes fails to compile
#include <qmap.h>

#include <qglobal.h>
//#include <qpair.h>

#if QT_VERSION >= 0x040000
#ifndef QT4
#define QT4
#endif
#endif


#ifdef QT4


class QColorGroup;
#include <Qt3Support/Q3ListView>


typedef int csize;
#define QListViewItem Q3ListViewItem
#define vvector QVector
#define vlist QList 


#else


#define vvector QValueVector
#define vlist QValueList 

#define QMenu QPopupMenu
#define QAbstractButton QButton
#define QScrollArea QScrollView

// QWidget
#define setWindowTitle setCaption
#define setWindowIcon setIcon
#define isExplicitlyHidden isHidden
#define accessibleName name
#define ensurePolished constPolish


// QMenu/QPopupMenu
#define addAction insertItem
//#define addSeparator insertSeparator


// QFile/QIODevice
#define seek at


// QFileDialog
#define setFileMode setMode
#define setDirectory setDir
#define selectFile setSelection


// QThread
#define isRunning running


// QScrollView/QScrollArea
#define setHorizontalScrollBarPolicy setHScrollBarMode


// QScrollBar
#define setMaximum setMaxValue
#define setMinimum setMinValue


// QAbstractButton/QButton
#define setCheckable setToggleButton


// QInputEvent/QKeyEvent
#define modifiers state


// QButtonGroup
#define addButton insert


// QProgressBar
#define setTextVisible setPercentageVisible


// QFileInfo
//#define completeSuffix extension
//#define suffix() extension( FALSE )


// QComboBox
#define addItem insertItem
//#define currentIndex currentItem
//#define setCurrentIndex setCurrentItem


// QString
#define toLower lower


// QTextEdit
#define setLineWrapMode setWordWrap
#define setPlainText setText


// QSlider
#define setTickPosition setTickmarks


// QStatusBar/QSplashScreen
#define showMessage message
#define clearMessage clear


// QDir
#define NoFilter DefaultFilter
#define homePath homeDirPath
#define rootPath rootDirPath
//#define absolutePath absPath


// QToolButton
#define setMenu setPopup


// QPixmap
#define transformed xForm


#define Q3ListView QListView
#define Q3ListViewItem QListViewItem
#define Q3ScrollView QScrollView

#define QMatrix QWMatrix
#define QIcon QIconSet

#define ShiftModifier ShiftButton
#define ControlModifier ControlButton

typedef unsigned int csize;

// some compat-stuff for older qt-versions...
#if QT_VERSION < 0x030200

#define wasCanceled wasCancelled

#endif

#if QT_VERSION < 0x030100

#include <qmutex.h>

// Qt 3.0.x doesn't have QMutexLocker, so we implement it on our own...
class QMutexLocker
{
public:
	QMutexLocker( QMutex * _m ) :
		m_mutex( _m )
	{
		m_mutex->lock();
	}
	~QMutexLocker()
	{
		m_mutex->unlock();
	}

private:
    QMutex * m_mutex;

} ;


#endif


#endif


#endif
