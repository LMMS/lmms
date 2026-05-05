/*
 * TempoSyncBarModelEditor.h - adds bpm to ms conversion for the bar editor class
 *
 * Copyright (c) 2005-2008 Danny McRae <khjklujn/at/yahoo.com>
 * Copyright (c) 2009-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2023 Michael Gregorius
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

#ifndef LMMS_GUI_TEMPO_SYNC_BAR_MODEL_EDITOR_H
#define LMMS_GUI_TEMPO_SYNC_BAR_MODEL_EDITOR_H

#include <QPixmap>
#include <QPointer>

#include "BarModelEditor.h"
#include "TempoSyncKnobModel.h"

namespace lmms::gui
{

class MeterDialog;

class LMMS_EXPORT TempoSyncBarModelEditor : public BarModelEditor
{
	Q_OBJECT
public:
	TempoSyncBarModelEditor(QString text, FloatModel * floatModel, QWidget * parent = nullptr);
	~TempoSyncBarModelEditor() override;

	const QString & syncDescription();
	void setSyncDescription(const QString & new_description);

	const QPixmap & syncIcon();
	void setSyncIcon(const QPixmap & new_pix);

	TempoSyncKnobModel * model()
	{
		return castModel<TempoSyncKnobModel>();
	}

	void modelChanged() override;


signals:
	void syncDescriptionChanged(const QString & new_description);
	void syncIconChanged();


protected:
	void contextMenuEvent(QContextMenuEvent * me) override;


protected slots:
	void updateDescAndIcon();
	void showCustom();


private:
	void updateTextDescription();
	void updateIcon();


private:
	QPixmap m_tempoSyncIcon;
	QString m_tempoSyncDescription;

	QPointer<MeterDialog> m_custom;
};

} // namespace lmms::gui

#endif // LMMS_GUI_TEMPO_SYNC_BAR_MODEL_EDITOR_H
