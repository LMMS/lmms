/*
 * Model.h - declaration of Model base class
 *
 * Copyright (c) 2007-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_MODEL_H
#define LMMS_MODEL_H

#include <QString>
#include <QObject>

#include "lmms_export.h"

namespace lmms
{

class LMMS_EXPORT Model : public QObject
{
	Q_OBJECT
public:
	Model(Model* parent, QString displayName = QString(),
		  bool defaultConstructed = false);

	~Model() override = default;

	bool isDefaultConstructed() const;

	Model* parentModel() const;

	virtual QString displayName() const;

	virtual void setDisplayName(const QString& displayName);

	virtual QString fullDisplayName() const;


private:
	QString m_displayName;
	bool m_defaultConstructed;


signals:
	// emitted if actual data of the model (e.g. values) have changed
	void dataChanged();

	// emitted in case new data was not set as it's been equal to old data
	void dataUnchanged();

	// emitted if properties of the model (e.g. ranges) have changed
	void propertiesChanged();

} ;


} // namespace lmms

#endif // LMMS_MODEL_H
