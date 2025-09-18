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
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QVBoxLayout>

#include "AutomatableButton.h"
#include "EffectRackView.h"
#include "embed.h"
#include "GuiApplication.h"
#include "Knob.h"
#include "MainWindow.h"
#include "MixerChannelLcdSpinBox.h"
#include "SampleTrackView.h"
#include "Song.h"
#include "SubWindow.h"
#include "TrackLabelButton.h"

namespace lmms::gui
{


SampleTrackWindow::SampleTrackWindow(SampleTrackView * tv) :
	QWidget(),
	ModelView(nullptr, this),
	m_track(tv->model()),
	m_stv(tv)
{
#if QT_VERSION < 0x50C00
	// Workaround for a bug in Qt versions below 5.12,
	// where argument-dependent-lookup fails for QFlags operators
	// declared inside a namepsace.
	// This affects the Q_DECLARE_OPERATORS_FOR_FLAGS macro in Instrument.h
	// See also: https://codereview.qt-project.org/c/qt/qtbase/+/225348

	using ::operator|;
#endif

	// init own layout + widgets
	setFocusPolicy(Qt::StrongFocus);
	auto vlayout = new QVBoxLayout(this);
	vlayout->setContentsMargins(0, 0, 0, 0);
	vlayout->setSpacing(0);

	auto generalSettingsWidget = new QWidget(this);
	auto generalSettingsLayout = new QVBoxLayout(generalSettingsWidget);

	auto nameWidget = new QWidget(generalSettingsWidget);
	auto nameLayout = new QHBoxLayout(nameWidget);
	nameLayout->setContentsMargins(0, 0, 0, 0);
	nameLayout->setSpacing(2);

	// setup line edit for changing sample track name
	m_nameLineEdit = new QLineEdit;
	connect(m_nameLineEdit, SIGNAL(textChanged(const QString&)),
				this, SLOT(textChanged(const QString&)));

	m_nameLineEdit->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
	nameLayout->addWidget(m_nameLineEdit);


	generalSettingsLayout->addWidget(nameWidget);

	auto basicControlsLayout = new QGridLayout;
	basicControlsLayout->setHorizontalSpacing(3);
	basicControlsLayout->setVerticalSpacing(0);
	basicControlsLayout->setContentsMargins(0, 0, 0, 0);

	QString labelStyleSheet = "font-size: 10px;";
	Qt::Alignment labelAlignment = Qt::AlignHCenter | Qt::AlignTop;
	Qt::Alignment widgetAlignment = Qt::AlignHCenter | Qt::AlignCenter;
	
	auto soloMuteLayout = new QVBoxLayout();
	soloMuteLayout->setContentsMargins(0, 0, 2, 0);
	soloMuteLayout->setSpacing(2);

	m_muteBtn = new AutomatableButton(this, tr("Mute"));
	m_muteBtn->setModel(&m_track->m_mutedModel);
	m_muteBtn->setObjectName("btn-mute");
	m_muteBtn->setCheckable(true);
	m_muteBtn->setToolTip(tr("Mute this sample track"));
	soloMuteLayout->addWidget(m_muteBtn, 0, widgetAlignment);

	m_soloBtn = new AutomatableButton(this, tr("Solo"));
	m_soloBtn->setModel(&m_track->m_soloModel);
	m_soloBtn->setObjectName("btn-solo");
	m_soloBtn->setCheckable(true);
	m_soloBtn->setToolTip(tr("Solo this sample track"));
	soloMuteLayout->addWidget(m_soloBtn, 0, widgetAlignment);

	basicControlsLayout->addLayout(soloMuteLayout, 0, 0, 2, 1, widgetAlignment);

	// set up volume knob
	m_volumeKnob = new Knob(KnobType::Bright26, nullptr, tr("Sample volume"));
	m_volumeKnob->setVolumeKnob(true);
	m_volumeKnob->setHintText(tr("Volume:"), "%");

	basicControlsLayout->addWidget(m_volumeKnob, 0, 1);
	basicControlsLayout->setAlignment(m_volumeKnob, widgetAlignment);

	auto label = new QLabel(tr("VOL"), this);
	label->setStyleSheet(labelStyleSheet);
	basicControlsLayout->addWidget(label, 1, 1);
	basicControlsLayout->setAlignment(label, labelAlignment);

	// set up panning knob
	m_panningKnob = new Knob(KnobType::Bright26, nullptr, tr("Panning"));
	m_panningKnob->setHintText(tr("Panning:"), "");

	basicControlsLayout->addWidget(m_panningKnob, 0, 2);
	basicControlsLayout->setAlignment(m_panningKnob, widgetAlignment);

	label = new QLabel(tr("PAN"),this);
	label->setStyleSheet(labelStyleSheet);
	basicControlsLayout->addWidget(label, 1, 2);
	basicControlsLayout->setAlignment(label, labelAlignment);

	basicControlsLayout->setColumnStretch(3, 1);


	// setup spinbox for selecting Mixer-channel
	m_mixerChannelNumber = new MixerChannelLcdSpinBox(2, nullptr, tr("Mixer channel"), m_stv);

	basicControlsLayout->addWidget(m_mixerChannelNumber, 0, 4);
	basicControlsLayout->setAlignment(m_mixerChannelNumber, widgetAlignment);

	label = new QLabel(tr("CHAN"), this);
	label->setStyleSheet(labelStyleSheet);
	basicControlsLayout->addWidget(label, 1, 4);
	basicControlsLayout->setAlignment(label, labelAlignment);

	generalSettingsLayout->addLayout(basicControlsLayout);

	m_effectRack = new EffectRackView(tv->model()->audioBusHandle()->effects());
	m_effectRack->setFixedSize(EffectRackView::DEFAULT_WIDTH, 242);

	vlayout->addWidget(generalSettingsWidget);
	vlayout->addWidget(m_effectRack);


	setModel(tv->model());

	QMdiSubWindow * subWin = getGUI()->mainWindow()->addWindowedWidget(this);
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
	m_mixerChannelNumber->setModel(&m_track->m_mixerChannelModel);

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

	if(getGUI()->mainWindow()->workspace())
	{
		parentWidget()->hide();
	}
	else
	{
		hide();
	}

	m_stv->setFocus();
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


} // namespace lmms::gui
