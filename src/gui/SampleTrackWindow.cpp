/*
 * SampleTrackWindow.cpp
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 
#include "SampleTrackWindow.h"

#include <QCloseEvent>
#include <QMenu>

#include "embed.h"
#include "gui_templates.h"
#include "GuiApplication.h"
#include "Knob.h"
#include "MainWindow.h"
#include "Song.h"
#include "TabWidget.h"
#include "TrackLabelButton.h"
 

SampleTrackWindow::SampleTrackWindow(SampleTrackView * tv) :
	QWidget(),
	ModelView(NULL, this),
	m_track(tv->model()),
	m_stv(tv)
{
	// init own layout + widgets
	setFocusPolicy(Qt::StrongFocus);
	QVBoxLayout * vlayout = new QVBoxLayout(this);
	vlayout->setMargin(0);
	vlayout->setSpacing(0);

	TabWidget* generalSettingsWidget = new TabWidget(tr("GENERAL SETTINGS"), this);

	QVBoxLayout* generalSettingsLayout = new QVBoxLayout(generalSettingsWidget);

	generalSettingsLayout->setContentsMargins(8, 18, 8, 8);
	generalSettingsLayout->setSpacing(6);

	QWidget* nameWidget = new QWidget(generalSettingsWidget);
	QHBoxLayout* nameLayout = new QHBoxLayout(nameWidget);
	nameLayout->setContentsMargins(0, 0, 0, 0);
	nameLayout->setSpacing(2);

	// setup line edit for changing sample track name
	m_nameLineEdit = new QLineEdit;
	m_nameLineEdit->setFont(pointSize<9>(m_nameLineEdit->font()));
	connect(m_nameLineEdit, SIGNAL(textChanged(const QString &)),
				this, SLOT(textChanged(const QString &)));

	m_nameLineEdit->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
	nameLayout->addWidget(m_nameLineEdit);


	generalSettingsLayout->addWidget(nameWidget);


	QGridLayout* basicControlsLayout = new QGridLayout;
	basicControlsLayout->setHorizontalSpacing(3);
	basicControlsLayout->setVerticalSpacing(0);
	basicControlsLayout->setContentsMargins(0, 0, 0, 0);

	QString labelStyleSheet = "font-size: 6pt;";
	Qt::Alignment labelAlignment = Qt::AlignHCenter | Qt::AlignTop;
	Qt::Alignment widgetAlignment = Qt::AlignHCenter | Qt::AlignCenter;

	// set up volume knob
	m_volumeKnob = new Knob(knobBright_26, NULL, tr("Sample volume"));
	m_volumeKnob->setVolumeKnob(true);
	m_volumeKnob->setHintText(tr("Volume:"), "%");

	basicControlsLayout->addWidget(m_volumeKnob, 0, 0);
	basicControlsLayout->setAlignment(m_volumeKnob, widgetAlignment);

	QLabel *label = new QLabel(tr("VOL"), this);
	label->setStyleSheet(labelStyleSheet);
	basicControlsLayout->addWidget(label, 1, 0);
	basicControlsLayout->setAlignment(label, labelAlignment);


	// set up panning knob
	m_panningKnob = new Knob(knobBright_26, NULL, tr("Panning"));
	m_panningKnob->setHintText(tr("Panning:"), "");

	basicControlsLayout->addWidget(m_panningKnob, 0, 1);
	basicControlsLayout->setAlignment(m_panningKnob, widgetAlignment);

	label = new QLabel(tr("PAN"),this);
	label->setStyleSheet(labelStyleSheet);
	basicControlsLayout->addWidget(label, 1, 1);
	basicControlsLayout->setAlignment(label, labelAlignment);


	basicControlsLayout->setColumnStretch(2, 1);


	// setup spinbox for selecting FX-channel
	m_effectChannelNumber = new FxLineLcdSpinBox(2, NULL, tr("FX channel"), m_stv);

	basicControlsLayout->addWidget(m_effectChannelNumber, 0, 3);
	basicControlsLayout->setAlignment(m_effectChannelNumber, widgetAlignment);

	label = new QLabel(tr("FX"), this);
	label->setStyleSheet(labelStyleSheet);
	basicControlsLayout->addWidget(label, 1, 3);
	basicControlsLayout->setAlignment(label, labelAlignment);

	generalSettingsLayout->addLayout(basicControlsLayout);

	m_effectRack = new EffectRackView(tv->model()->audioPort()->effects());
	m_effectRack->setFixedSize(EffectRackView::DEFAULT_WIDTH, 242);

	vlayout->addWidget(generalSettingsWidget);
	vlayout->addWidget(m_effectRack);


	setModel(tv->model());

	QMdiSubWindow * subWin = gui->mainWindow()->addWindowedWidget(this);
	Qt::WindowFlags flags = subWin->windowFlags();
	flags |= Qt::MSWindowsFixedSizeDialogHint;
	flags &= ~Qt::WindowMaximizeButtonHint;
	subWin->setWindowFlags(flags);

	// Hide the Size and Maximize options from the system menu
	// since the dialog size is fixed.
	QMenu * systemMenu = subWin->systemMenu();
	systemMenu->actions().at(2)->setVisible(false); // Size
	systemMenu->actions().at(4)->setVisible(false); // Maximize

	subWin->setWindowIcon(embed::getIconPixmap("sample_track"));
	subWin->setFixedSize(subWin->size());
	subWin->hide();
}

SampleTrackWindow::~SampleTrackWindow()
{
}



void SampleTrackWindow::setSampleTrackView(SampleTrackView* tv)
{
	if(m_stv && tv)
	{
		m_stv->m_tlb->setChecked(false);
	}

	m_stv = tv;
}



void SampleTrackWindow::modelChanged()
{
	m_track = castModel<SampleTrack>();

	m_nameLineEdit->setText(m_track->name());

	m_track->disconnect(SIGNAL(nameChanged()), this);

	connect(m_track, SIGNAL(nameChanged()),
			this, SLOT(updateName()));

	m_volumeKnob->setModel(&m_track->m_volumeModel);
	m_panningKnob->setModel(&m_track->m_panningModel);
	m_effectChannelNumber->setModel(&m_track->m_effectChannelModel);

	updateName();
}




void SampleTrackWindow::updateName()
{
	setWindowTitle(m_track->name().length() > 25 ? (m_track->name().left(24) + "...") : m_track->name());

	if(m_nameLineEdit->text() != m_track->name())
	{
		m_nameLineEdit->setText(m_track->name());
	}
}



void SampleTrackWindow::textChanged(const QString& new_name)
{
	m_track->setName(new_name);
	Engine::getSong()->setModified();
}



void SampleTrackWindow::toggleVisibility(bool on)
{
	if(on)
	{
		show();
		parentWidget()->show();
		parentWidget()->raise();
	}
	else
	{
		parentWidget()->hide();
	}
}




void SampleTrackWindow::closeEvent(QCloseEvent* ce)
{
	ce->ignore();

	if(gui->mainWindow()->workspace())
	{
		parentWidget()->hide();
	}
	else
	{
		hide();
	}

	m_stv->m_tlb->setFocus();
	m_stv->m_tlb->setChecked(false);
}



void SampleTrackWindow::saveSettings(QDomDocument& doc, QDomElement & element)
{
	MainWindow::saveWidgetState(this, element);
	Q_UNUSED(element)
}



void SampleTrackWindow::loadSettings(const QDomElement& element)
{
	MainWindow::restoreWidgetState(this, element);
	if(isVisible())
	{
		m_stv->m_tlb->setChecked(true);
	}
}