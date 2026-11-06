/*
 * ClipContent.h - Clip content base class & content linking infrastructure
 *
 * Copyright (c) 2026 Itreza <louis.bossy/at/etu.univ-tours.fr>
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

#ifndef LMMS_CLIP_CONTENT_H
#define LMMS_CLIP_CONTENT_H

#include <memory>
#include <QString>
#include <qobject.h>
#include <qobjectdefs.h>
#include <unordered_map>


namespace lmms
{

/**
 * @brief base storage class for clips shareable content.
 * Clips that allows linked copies should have their data stored into an object derived
 * from ClipContent, of which they should hold a reference through a smart pointer.
 */
class ClipContent: public QObject
{
Q_OBJECT

public:
	enum Type
	{
		Midi
	};

	QString linkName() { return m_link; }
	void setLinkName(QString name) { m_link = name; }

	Type type() { return m_type; }

	// public flag used by clips to know if the link should be serialized
	bool m_shareToCopy;

signals:
	void contentChanged();

protected:
	ClipContent() = default;

	QString m_link;
	Type m_type;
};


/**
 * @brief static repository that enable clips content sharing.
 * The repository hold weak references to currently shared contents.
 */
class SharedContentRepository
{
public:
	/** @param linkName the name of the created link. If empty, an available name will be attributed. */
	static QString submit(std::weak_ptr<ClipContent> content, QString linkName = "");

	/**
	 * @return a shared pointer toward a content associated with the requested link name,
	 * if there is one, nullptr otherwise
	 */
	static std::shared_ptr<ClipContent> get(QString linkName);

private:
	inline static std::unordered_map<QString, std::weak_ptr<ClipContent>> s_contents;
};

} // namespace lmms

#endif // LMMS_CLIP_CONTENT_H
