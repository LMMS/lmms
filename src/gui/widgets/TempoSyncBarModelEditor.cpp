/*
 * TempoSyncBarModelEditor.cpp - adds bpm to ms conversion for the bar editor class
 *
 * Copyright (c) 2005-2007 Danny McRae <khjklujn/at/yahoo.com>
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QMdiArea>

#include "TempoSyncBarModelEditor.h"
#include "Engine.h"
#include "CaptionMenu.h"
#include "embed.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "MeterDialog.h"
#include "Song.h"
#include "SubWindow.h"


namespace lmms::gui
{

TempoSyncBarModelEditor::TempoSyncBarModelEditor(QString text, FloatModel * floatModel, QWidget * parent) :
	BarModelEditor(text, floatModel, parent),
	m_tempoSyncIcon(embed::getIconPixmap("tempo_sync")),
	m_tempoSyncDescription(tr("Tempo Sync")),
	m_custom(nullptr)
{
	modelChanged();
}


TempoSyncBarModelEditor::~TempoSyncBarModelEditor()
{
	if(m_custom)
	{
		delete m_custom->parentWidget();
	}
}


void TempoSyncBarModelEditor::modelChanged()
{
	TempoSyncKnobModel * tempoSyncModel = model();

	if(tempoSyncModel == nullptr)
	{
		qWarning("no TempoSyncKnobModel has been set!");
	}

	if(m_custom != nullptr)
	{
		m_custom->setModel(&tempoSyncModel->getCustomMeterModel());
	}

	connect(tempoSyncModel, &TempoSyncKnobModel::syncModeChanged, this, &TempoSyncBarModelEditor::updateDescAndIcon);
	connect(this, SIGNAL(sliderMoved(float)), tempoSyncModel, SLOT(disableSync()));

	updateDescAndIcon();
}


void TempoSyncBarModelEditor::contextMenuEvent(QContextMenuEvent *)
{
	mouseReleaseEvent(nullptr);

	TempoSyncKnobModel * tempoSyncModel = model();

	CaptionMenu contextMenu(tempoSyncModel->displayName(), this);
	addDefaultActions(&contextMenu);
	contextMenu.addAction(QPixmap(),
		model()->isScaleLogarithmic() ? tr("Set linear") : tr("Set logarithmic"),
		this, SLOT(toggleScale()));

	contextMenu.addSeparator();

	float limit = 60000.0f / (Engine::getSong()->getTempo() * tempoSyncModel->scale());

	QMenu * syncMenu = contextMenu.addMenu(m_tempoSyncIcon, m_tempoSyncDescription);

	float const maxValue = tempoSyncModel->maxValue();

	if(limit / 8.0f <= maxValue)
	{
		connect(syncMenu, SIGNAL(triggered(QAction*)), tempoSyncModel, SLOT(setTempoSync(QAction*)));

		syncMenu->addAction(embed::getIconPixmap("note_none"),
			tr("No Sync"))->setData((int) TempoSyncKnobModel::SyncMode::None);

		if(limit / 0.125f <= maxValue)
		{
			syncMenu->addAction(embed::getIconPixmap("note_double_whole"),
				tr("Eight beats"))->setData((int) TempoSyncKnobModel::SyncMode::DoubleWholeNote);
		}

		if(limit / 0.25f <= maxValue)
		{
			syncMenu->addAction(embed::getIconPixmap("note_whole"),
				tr("Whole note"))->setData((int) TempoSyncKnobModel::SyncMode::WholeNote);
		}

		if(limit / 0.5f <= maxValue)
		{
			syncMenu->addAction(embed::getIconPixmap("note_half"),
				tr("Half note"))->setData((int) TempoSyncKnobModel::SyncMode::HalfNote);
		}

		if(limit <= maxValue)
		{
			syncMenu->addAction(embed::getIconPixmap("note_quarter"),
				tr("Quarter note"))->setData((int) TempoSyncKnobModel::SyncMode::QuarterNote);
		}

		if(limit / 2.0f <= maxValue)
		{
			syncMenu->addAction(embed::getIconPixmap("note_eighth"),
				tr("8th note"))->setData((int) TempoSyncKnobModel::SyncMode::EighthNote);
		}

		if(limit / 4.0f <= maxValue)
		{
			syncMenu->addAction(embed::getIconPixmap("note_sixteenth"),
				tr("16th note"))->setData((int) TempoSyncKnobModel::SyncMode::SixteenthNote);
		}

		syncMenu->addAction(embed::getIconPixmap("note_thirtysecond"),
			tr("32nd note"))->setData((int) TempoSyncKnobModel::SyncMode::ThirtysecondNote);

		syncMenu->addAction(embed::getIconPixmap("dont_know"),
			tr("Custom..."), this, SLOT(showCustom()))->setData((int) TempoSyncKnobModel::SyncMode::Custom);

		contextMenu.addSeparator();
	}

	contextMenu.exec(QCursor::pos());

	delete syncMenu;
}

void TempoSyncBarModelEditor::updateDescAndIcon()
{
	updateTextDescription();

	if(m_custom != nullptr && model()->syncMode() != TempoSyncKnobModel::SyncMode::Custom)
	{
		m_custom->parentWidget()->hide();
	}

	updateIcon();

	emit syncDescriptionChanged(m_tempoSyncDescription);
	emit syncIconChanged();
}


const QString & TempoSyncBarModelEditor::syncDescription()
{
	return m_tempoSyncDescription;
}


void TempoSyncBarModelEditor::setSyncDescription(const QString & new_description)
{
	m_tempoSyncDescription = new_description;
	emit syncDescriptionChanged(new_description);
}


const QPixmap & TempoSyncBarModelEditor::syncIcon()
{
	return m_tempoSyncIcon;
}


void TempoSyncBarModelEditor::setSyncIcon(const QPixmap & new_icon)
{
	m_tempoSyncIcon = new_icon;
	emit syncIconChanged();
}


void TempoSyncBarModelEditor::showCustom()
{
	if(m_custom == nullptr)
	{
		m_custom = new MeterDialog(getGUI()->mainWindow()->workspace());
		QMdiSubWindow * subWindow = getGUI()->mainWindow()->addWindowedWidget(m_custom);
		Qt::WindowFlags flags = subWindow->windowFlags();
		flags &= ~Qt::WindowMaximizeButtonHint;
		subWindow->setWindowFlags(flags);
		subWindow->setFixedSize(subWindow->size());
		m_custom->setWindowTitle("Meter");
		m_custom->setModel(&model()->getCustomMeterModel());
	}

	m_custom->parentWidget()->show();
	model()->setTempoSync(TempoSyncKnobModel::SyncMode::Custom);
}


void TempoSyncBarModelEditor::updateTextDescription()
{
	TempoSyncKnobModel * tempoSyncModel = model();

	auto const syncMode = tempoSyncModel->syncMode();

	switch(syncMode)
	{
		case TempoSyncKnobModel::SyncMode::None:
			m_tempoSyncDescription = tr("Tempo Sync");
			break;
		case TempoSyncKnobModel::SyncMode::Custom:
			m_tempoSyncDescription = tr("Custom ") +
				"(" +
				QString::number(tempoSyncModel->getCustomMeterModel().numeratorModel().value()) +
				"/" +
				QString::number(tempoSyncModel->getCustomMeterModel().denominatorModel().value()) +
				")";
			break;
		case TempoSyncKnobModel::SyncMode::DoubleWholeNote:
			m_tempoSyncDescription = tr("Synced to Eight Beats");
			break;
		case TempoSyncKnobModel::SyncMode::WholeNote:
			m_tempoSyncDescription = tr("Synced to Whole Note");
			break;
		case TempoSyncKnobModel::SyncMode::HalfNote:
			m_tempoSyncDescription = tr("Synced to Half Note");
			break;
		case TempoSyncKnobModel::SyncMode::QuarterNote:
			m_tempoSyncDescription = tr("Synced to Quarter Note");
			break;
		case TempoSyncKnobModel::SyncMode::EighthNote:
			m_tempoSyncDescription = tr("Synced to 8th Note");
			break;
		case TempoSyncKnobModel::SyncMode::SixteenthNote:
			m_tempoSyncDescription = tr("Synced to 16th Note");
			break;
		case TempoSyncKnobModel::SyncMode::ThirtysecondNote:
			m_tempoSyncDescription = tr("Synced to 32nd Note");
			break;
		default: ;
	}
}

void TempoSyncBarModelEditor::updateIcon()
{
	switch(model()->syncMode())
	{
		case TempoSyncKnobModel::SyncMode::None:
			m_tempoSyncIcon = embed::getIconPixmap("tempo_sync");
			break;
		case TempoSyncKnobModel::SyncMode::Custom:
			m_tempoSyncIcon = embed::getIconPixmap("dont_know");
			break;
		case TempoSyncKnobModel::SyncMode::DoubleWholeNote:
			m_tempoSyncIcon = embed::getIconPixmap("note_double_whole");
			break;
		case TempoSyncKnobModel::SyncMode::WholeNote:
			m_tempoSyncIcon = embed::getIconPixmap("note_whole");
			break;
		case TempoSyncKnobModel::SyncMode::HalfNote:
			m_tempoSyncIcon = embed::getIconPixmap("note_half");
			break;
		case TempoSyncKnobModel::SyncMode::QuarterNote:
			m_tempoSyncIcon = embed::getIconPixmap("note_quarter");
			break;
		case TempoSyncKnobModel::SyncMode::EighthNote:
			m_tempoSyncIcon = embed::getIconPixmap("note_eighth");
			break;
		case TempoSyncKnobModel::SyncMode::SixteenthNote:
			m_tempoSyncIcon = embed::getIconPixmap("note_sixteenth");
			break;
		case TempoSyncKnobModel::SyncMode::ThirtysecondNote:
			m_tempoSyncIcon = embed::getIconPixmap("note_thirtysecond");
			break;
		default:
			qWarning("TempoSyncKnob::calculateTempoSyncTime:"
						"invalid TempoSyncMode");
			break;
	}
}


} // namespace lmms::gui
