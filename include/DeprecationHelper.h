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

/**
 * @brief horizontalAdvance is a backwards-compatible adapter for
 * QFontMetrics::horizontalAdvance and width functions.
 * @param metrics
 * @param text
 * @return text's horizontal advance based on metrics.
 */
inline int horizontalAdvance(const QFontMetrics& metrics, const QString& text)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
	return metrics.horizontalAdvance(text);
#else
	return metrics.width(text);
#endif
}

/**
 * @brief position is a backwards-compatible adapter for
 * QWheelEvent::position and pos functions.
 * @param wheelEvent
 * @return the position of wheelEvent
 */
inline QPoint position(const QWheelEvent* wheelEvent)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
	return wheelEvent->position().toPoint();
#else
	return wheelEvent->pos();
#endif
}

/**
 * @brief position is a backwards-compatible adapter for
 * QDropEvent::position and pos functions.
 * @param me
 * @return the position of the drop event
 */
inline QPoint position(const QDropEvent* de)
{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	return de->position().toPoint();
#else
	return de->pos();
#endif
}

/**
 * @brief position is a backwards-compatible adapter for
 * QMouseEvent::position and pos functions.
 * @param me
 * @return the position of the mouse event
 */
inline QPoint position(const QMouseEvent* me)
{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	return me->position().toPoint();
#else
	return me->pos();
#endif
}

/**
 * @brief positionF is a backwards-compatible adapter for
 * QMouseEvent::position and localPos functions.
 * @param me
 * @return the position of the mouse event
 */
inline QPointF positionF(const QMouseEvent* me)
{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	return me->position();
#else
	return me->localPos();
#endif
}

/**
 * @brief globalPosition is a backwards-compatible adapter for
 * QMouseEvent::globalPosition and globalPos functions.
 * @param me
 * @return the global position of the mouse event
 */
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

/**
 * @brief Combines Qt key and modifier arguments together,
 * replacing `A | B` which was deprecated in C++20
 * due to the enums being different types. (P1120R0)
 * @param args Any number of Qt::Key, Qt::Modifier, or Qt::KeyboardModifier
 * @return The combination of the given keys/modifiers as a QKeySequence
 */
template<typename... Args> requires (detail::IsKeyOrModifier<Args> && ...)
inline QKeySequence keySequence(Args... args)
{
	return (0 | ... | static_cast<int>(args));
}

/**
 * @brief typeId is a backwards-compatible adapter for
 * QVariant::typeId and type functions.
 * @param variant
 * @return the type id of the variant
 */
inline QMetaType::Type typeId(const QVariant& variant)
{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	return static_cast<QMetaType::Type>(variant.typeId());
#else
	return static_cast<QMetaType::Type>(variant.type());
#endif
}

//! Backwards-compatible adapter for QDomDocument::setContent
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

//! Backwards-compatible adapter for QDomDocument::setContent
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
