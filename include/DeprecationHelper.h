/*
 * DeprecationHelper.h - This file contains the declarations of helper functions
 * which helps centralize the #ifdefs preprocessors regarding deprecation based on Qt versions.
 * The functions are defined differently based on the callers' Qt versions.
 *
 * Copyright (c) 2020 Tien Dat Nguyen <ntd.bk.k56/at/gmail.com>
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

#ifndef LMMS_DEPRECATIONHELPER_H
#define LMMS_DEPRECATIONHELPER_H

#include <type_traits>

#include <QDomDocument>
#include <QFontMetrics>
#include <QKeySequence>
#include <QVariant>
#include <QWheelEvent>

namespace lmms
{

//! @brief Backwards-compatible adapter for `QDropEvent`'s new
//! `position()` to the old `pos()` method
//! @param de A drop event
//! @return The position of the drop event, relative to the receiving
//! widget or item
//! @see Qt 6 [`QDropEvent::position()`](https://doc.qt.io/qt-6/qdropevent.html#position)
//! @see Qt 5.15 [`QDropEvent::pos()`](https://doc.qt.io/archives/qt-5.15/qdropevent.html#pos)
inline QPoint position(const QDropEvent* de)
{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	return de->position().toPoint();
#else
	return de->pos();
#endif
}

//! @brief Backwards-compatible adapter for `QMouseEvent`'s new
//! `position()` to the old `pos()` method
//! @param me A mouse event
//! @return The position of the mouse event, relative to the receiving
//! widget or item
//! @see Qt 6 [`QSinglePointEvent::position()`](https://doc.qt.io/qt-6/qsinglepointevent.html#position)
//! (inherited by `QMouseEvent`)
//! @see Qt 5.15 [`QMouseEvent::pos()`](https://doc.qt.io/archives/qt-5.15/qmouseevent.html#pos)
inline QPoint position(const QMouseEvent* me)
{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	return me->position().toPoint();
#else
	return me->pos();
#endif
}

//! @brief Backwards-compatible adapter for `QMouseEvent`'s new
//! `position()` to the old `localPos()` method
//! @param me A mouse event
//! @return The position of the mouse event, relative to the receiving
//! widget or item
//! @see Qt 6 [`QSinglePointEvent::position()`](https://doc.qt.io/qt-6/qsinglepointevent.html#position)
//! (inherited by `QMouseEvent`)
//! @see Qt 5.15 [`QMouseEvent::localPos()`](https://doc.qt.io/archives/qt-5.15/qmouseevent.html#localPos)
inline QPointF positionF(const QMouseEvent* me)
{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	return me->position();
#else
	return me->localPos();
#endif
}

//! @brief Backwards-compatible adapter for `QMouseEvent`'s new
//! `globalPosition()` to the old `globalPos()` method
//! @param me A mouse event
//! @return The global position of the mouse event, relative to the
//! receiving widget or item
//! @see Qt 6 [`QSinglePointEvent::globalPosition()`](https://doc.qt.io/qt-6/qsinglepointevent.html#globalPosition)
//! (inherited by `QMouseEvent`)
//! @see Qt 5.15 [`QMouseEvent::globalPos()`](https://doc.qt.io/archives/qt-5.15/qmouseevent.html#globalPos)
inline QPoint globalPosition(const QMouseEvent* me)
{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	return me->globalPosition().toPoint();
#else
	return me->globalPos();
#endif
}

namespace detail
{

template<typename T>
inline constexpr bool IsKeyOrModifier = std::is_same_v<T, Qt::Key>
	|| std::is_same_v<T, Qt::Modifier> || std::is_same_v<T, Qt::KeyboardModifier>;

} // namespace detail

//! @brief Combines Qt key and modifier arguments together
//!
//! Combines Qt key and modifier arguments together, replacing `A | B`
//! which was deprecated in C++20 due to the enums being different
//! types.
//!
//! @param args Any number of `Qt::Key`, `Qt::Modifier`, or `Qt::KeyboardModifier`
//! @return The combination of the given keys/modifiers as a `QKeySequence`
//! @see [WG21 P1120R0 “Consistency improvements for `<=>` and other comparison operators”](https://wg21.link/P1120R0)
template<typename... Args> requires (detail::IsKeyOrModifier<Args> && ...)
inline QKeySequence keySequence(Args... args)
{
	return (0 | ... | static_cast<int>(args));
}

//! @brief Backwards-compatible adapter for `QVariant`'s new
//! `typeId()` to the old `type()` method
//! @param variant A QVariant to get the type id of
//! @return The type id of the value in @p variant
//! @see Qt 6 [`QVariant::typeId()`](https://doc.qt.io/qt-6/qvariant.html#typeId)
//! @see Qt 5.15 [`QVariant::type()`](https://doc.qt.io/archives/qt-5.15/qvariant.html#type)
inline QMetaType::Type typeId(const QVariant& variant)
{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	return static_cast<QMetaType::Type>(variant.typeId());
#else
	return static_cast<QMetaType::Type>(variant.type());
#endif
}

//! @brief Backwards-compatible adapter for `QDomDocument::setContent()`
//! for `QByteArray` input
//! @see Qt 6.5 [`QDomDocument::setContent()`](https://doc.qt.io/qt-6/qdomdocument.html#setContent)
//! @see Qt 5.15 [`QDomDocument::setContent()`](https://doc.qt.io/archives/qt-5.15/qdomdocument.html#setContent-4)
inline bool setContent(QDomDocument& doc, const QByteArray& text,
	QString* errorMsg = nullptr, int* errorLine = nullptr, int* errorColumn = nullptr)
{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 5, 0))
	auto result = doc.setContent(text, QDomDocument::ParseOption::Default);
	if (result) { return true; }
	if (errorMsg) { *errorMsg = std::move(result.errorMessage); }
	if (errorLine) { *errorLine = static_cast<int>(result.errorLine); }
	if (errorColumn) { *errorColumn = static_cast<int>(result.errorColumn); }
	return false;
#else
	return doc.setContent(text, errorMsg, errorLine, errorColumn);
#endif
}

//! @brief Backwards-compatible adapter for `QDomDocument::setContent()`
//! for `QIODevice*` input
//! @see Qt 6.5 [`QDomDocument::setContent()`](https://doc.qt.io/qt-6/qdomdocument.html#setContent)
//! @see Qt 5.15 [`QDomDocument::setContent()`](https://doc.qt.io/archives/qt-5.15/qdomdocument.html#setContent-2)
inline bool setContent(QDomDocument& doc, QIODevice* dev, bool namespaceProcessing,
	QString* errorMsg = nullptr, int* errorLine = nullptr, int* errorColumn = nullptr)
{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 5, 0))
	const auto options = namespaceProcessing
		? QDomDocument::ParseOption::UseNamespaceProcessing
		: QDomDocument::ParseOption::Default;
	auto result = doc.setContent(dev, options);
	if (result) { return true; }
	if (errorMsg) { *errorMsg = std::move(result.errorMessage); }
	if (errorLine) { *errorLine = static_cast<int>(result.errorLine); }
	if (errorColumn) { *errorColumn = static_cast<int>(result.errorColumn); }
	return false;
#else
	return doc.setContent(dev, namespaceProcessing, errorMsg, errorLine, errorColumn);
#endif
}

} // namespace lmms

#endif // LMMS_DEPRECATIONHELPER_H
