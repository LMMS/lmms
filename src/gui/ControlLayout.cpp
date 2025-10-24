/*
 * ControlLayout.cpp - implementation for ControlLayout.h
 *
 * Copyright (c) 2019-2019 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include "ControlLayout.h"

#include <QWidget>
#include <QLayoutItem>
#include <QLineEdit>
#include <QRect>
#include <QString>
#include <utility>


namespace lmms::gui
{

constexpr const int ControlLayout::m_minWidth;

ControlLayout::ControlLayout(QWidget *parent, int margin, int hSpacing, int vSpacing)
	: QLayout(parent), m_hSpace(hSpacing), m_vSpace(vSpacing),
	  m_searchBar(new QLineEdit(parent))
{
	setContentsMargins(margin, margin, margin, margin);
	m_searchBar->setPlaceholderText("filter");
	m_searchBar->setObjectName(s_searchBarName);
	connect(m_searchBar, SIGNAL(textChanged(const QString&)),
		this, SLOT(onTextChanged(const QString& )));
	addWidget(m_searchBar);
	m_searchBar->setHidden(true); // nothing to filter yet
}

ControlLayout::~ControlLayout()
{
	while (auto item = takeAt(0)) { delete item; }
}

void ControlLayout::onTextChanged(const QString&)
{
	invalidate();
	update();
}

void ControlLayout::addItem(QLayoutItem *item)
{
	QWidget* widget = item->widget();
	const QString str = widget ? widget->objectName() : QString("unnamed");
	m_itemMap.insert(str, item);
	invalidate();
}

int ControlLayout::horizontalSpacing() const
{
	if (m_hSpace >= 0) { return m_hSpace; }
	else
	{
		return smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
	}
}

int ControlLayout::verticalSpacing() const
{
	if (m_vSpace >= 0) { return m_vSpace; }
	else
	{
		return smartSpacing(QStyle::PM_LayoutVerticalSpacing);
	}
}

int ControlLayout::count() const
{
	return m_itemMap.size() - 1;
}

ControlLayout::ControlLayoutMap::const_iterator
ControlLayout::pairAt(int index) const
{
	if (index < 0) { return m_itemMap.cend(); }

	auto skip = [&](QLayoutItem* item) -> bool
	{
		return item->widget()->objectName() == s_searchBarName;
	};

	auto itr = m_itemMap.cbegin();
	for (; itr != m_itemMap.cend() && (index > 0 || skip(itr.value())); ++itr)
	{
		if(!skip(itr.value())) { index--; }
	}
	return itr;
}

// linear time :-(
QLayoutItem *ControlLayout::itemAt(int index) const
{
	auto itr = pairAt(index);
	return (itr == m_itemMap.end()) ? nullptr : itr.value();
}

QLayoutItem *ControlLayout::itemByString(const QString &key) const
{
	auto itr = m_itemMap.find(key);
	return (itr == m_itemMap.end()) ? nullptr : *itr;
}

// linear time :-(
QLayoutItem *ControlLayout::takeAt(int index)
{
	auto itr = pairAt(index);
	return (itr == m_itemMap.end()) ? nullptr : m_itemMap.take(itr.key());
}

void ControlLayout::removeFocusFromSearchBar()
{
	m_searchBar->clearFocus();
}

Qt::Orientations ControlLayout::expandingDirections() const
{
	return Qt::Orientations();
}

bool ControlLayout::hasHeightForWidth() const
{
	return true;
}

int ControlLayout::heightForWidth(int width) const
{
	int height = doLayout(QRect(0, 0, width, 0), true);
	return height;
}

void ControlLayout::setGeometry(const QRect &rect)
{
	QLayout::setGeometry(rect);
	doLayout(rect, false);
}

QSize ControlLayout::sizeHint() const
{
	return minimumSize();
}

QSize ControlLayout::minimumSize() const
{
	// original formula from Qt's FlowLayout example:
	// get maximum height and width for all children.
	// as Qt will later call heightForWidth, only the width here really matters
	QSize size;
	for (const QLayoutItem *item : std::as_const(m_itemMap))
	{
		size = size.expandedTo(item->minimumSize());
	}
	const QMargins margins = contentsMargins();
	size += QSize(margins.left() + margins.right(), margins.top() + margins.bottom());

	// the original formula would leed to ~1 widget per row
	// bash it at least to 400 so we have ~4 knobs per row
	size.setWidth(qMax(size.width(), m_minWidth));
	return size;
}

int ControlLayout::doLayout(const QRect &rect, bool testOnly) const
{
	int left, top, right, bottom;
	getContentsMargins(&left, &top, &right, &bottom);
	QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
	int x = effectiveRect.x();
	int y = effectiveRect.y();
	int lineHeight = 0;

	const QString filterText = m_searchBar->text();
	bool first = true;

	for (auto itr = m_itemMap.cbegin(); itr != m_itemMap.cend(); ++itr)
	{
		QLayoutItem* item = itr.value();
		QWidget *wid = item->widget();
		if (wid)
		{
			if (	first || // do not filter search bar
				filterText.isEmpty() || // no filter - pass all
				itr.key().contains(filterText, Qt::CaseInsensitive))
			{
				if (first)
				{
					// for the search bar, only show it if there are at least
					// five control widgets (i.e. at least 6 widgets)
					if (m_itemMap.size() > 5) { wid->show(); }
					else { wid->hide(); }
				}
				else { wid->show(); }

				int spaceX = horizontalSpacing();
				if (spaceX == -1)
				{
					spaceX = wid->style()->layoutSpacing(
						QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Horizontal);
				}
				int spaceY = verticalSpacing();
				if (spaceY == -1)
				{
					spaceY = wid->style()->layoutSpacing(
						QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Vertical);
				}
				int nextX = x + item->sizeHint().width() + spaceX;
				if (nextX - spaceX > effectiveRect.right() && lineHeight > 0)
				{
					x = effectiveRect.x();
					y = y + lineHeight + spaceY;
					nextX = x + item->sizeHint().width() + spaceX;
					lineHeight = 0;
				}

				if (!testOnly)
				{
					item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));
				}

				x = nextX;
				lineHeight = qMax(lineHeight, item->sizeHint().height());
				first = false;
			}
			else
			{
				wid->hide();
			}
		}
	}
	return y + lineHeight - rect.y() + bottom;
}

int ControlLayout::smartSpacing(QStyle::PixelMetric pm) const
{
	QObject *parent = this->parent();
	if (!parent) { return -1; }
	else if (parent->isWidgetType())
	{
		auto pw = static_cast<QWidget*>(parent);
		return pw->style()->pixelMetric(pm, nullptr, pw);
	}
	else { return static_cast<QLayout *>(parent)->spacing(); }
}


} // namespace lmms::gui
