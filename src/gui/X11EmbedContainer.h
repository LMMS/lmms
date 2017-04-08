/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#ifndef X11EMBEDCONTAINER_H
#define X11EMBEDCONTAINER_H

#include <QtGlobal>

#if QT_VERSION < 0x050000
#include <QX11EmbedContainer>
#else

#include <QAbstractNativeEventFilter>
#include <QtWidgets/QWidget>

class QX11EmbedContainerPrivate;
class Q_GUI_EXPORT QX11EmbedContainer : public QWidget, public QAbstractNativeEventFilter
{
	Q_OBJECT
public:
	QX11EmbedContainer(QWidget *parent = 0);
	~QX11EmbedContainer();

	void embedClient(WId id);
	void discardClient();

	WId clientWinId() const;

	QSize minimumSizeHint() const;

	enum Error {
		Unknown,
		Internal,
		InvalidWindowID
	};
	Error error() const;

public:
	bool nativeEventFilter(const QByteArray &eventType, void *message, long *result);

Q_SIGNALS:
	void clientIsEmbedded();
	void clientClosed();
	void error(QX11EmbedContainer::Error);

protected:
	bool eventFilter(QObject *, QEvent *) override;
	void paintEvent(QPaintEvent *e) override;
	void resizeEvent(QResizeEvent *);
	bool event(QEvent *);

private:
	Q_DECLARE_PRIVATE(QX11EmbedContainer)
	Q_DISABLE_COPY(QX11EmbedContainer)
	bool x11Event(void *e, long *result);
};

#endif // X11EMBEDCONTAINER_H

#endif
