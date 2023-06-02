/*
 * TapTempoUi.h - Tap Tempo UI designer file
 *
 * Copyright (c) 2022 saker <sakertooth@gmail.com>
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

#ifndef LMMS_GUI_TAP_TEMPO_UI_H
#define LMMS_GUI_TAP_TEMPO_UI_H

#include <QApplication>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

namespace lmms::gui {

class TapTempoUi
{
public:
	QVBoxLayout* windowLayout;
	QVBoxLayout* mainLayout;
	QPushButton* tapButton;
	QHBoxLayout* sidebarLayout;
	QLabel* msLabel;
	QLabel* hzLabel;
	QVBoxLayout* verticalLayout;
	QCheckBox* precisionCheckBox;
	QCheckBox* muteCheckBox;

	void setupUi(QWidget* tapTempo)
	{
		if (tapTempo->objectName().isEmpty()) tapTempo->setObjectName(QString::fromUtf8("tapTempo"));
		tapTempo->setWindowModality(Qt::NonModal);
		tapTempo->setEnabled(true);
		tapTempo->resize(250, 288);

		QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		sizePolicy.setHorizontalStretch(0);
		sizePolicy.setVerticalStretch(0);
		sizePolicy.setHeightForWidth(tapTempo->sizePolicy().hasHeightForWidth());

		tapTempo->setSizePolicy(sizePolicy);
		tapTempo->setMinimumSize(QSize(250, 288));
		tapTempo->setMaximumSize(QSize(250, 288));

		QIcon icon;
		icon.addFile(QString::fromUtf8("logo.png"), QSize(), QIcon::Normal, QIcon::Off);
		tapTempo->setWindowIcon(icon);
		tapTempo->setAutoFillBackground(true);
		tapTempo->setStyleSheet(QString::fromUtf8("QLabel {\n"
												  " color: rgb(255, 255, 255);\n"
												  " background-color: transparent;\n"
												  "}\n"
												  ""));
		windowLayout = new QVBoxLayout(tapTempo);
		windowLayout->setSpacing(0);
		windowLayout->setObjectName(QString::fromUtf8("windowLayout"));
		windowLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
		windowLayout->setContentsMargins(0, 0, 0, 0);

		mainLayout = new QVBoxLayout();
		mainLayout->setSpacing(5);
		mainLayout->setObjectName(QString::fromUtf8("mainLayout"));
		mainLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
		mainLayout->setContentsMargins(2, 2, 2, 2);

		tapButton = new QPushButton(tapTempo);
		tapButton->setObjectName(QString::fromUtf8("tapButton"));
		tapButton->setEnabled(true);

		QSizePolicy sizePolicy1(QSizePolicy::Minimum, QSizePolicy::Minimum);
		sizePolicy1.setHorizontalStretch(0);
		sizePolicy1.setVerticalStretch(0);
		sizePolicy1.setHeightForWidth(tapButton->sizePolicy().hasHeightForWidth());

		tapButton->setSizePolicy(sizePolicy1);
		tapButton->setMinimumSize(QSize(200, 200));
		tapButton->setMaximumSize(QSize(16777215, 16777215));
		tapButton->setStyleSheet(
			QString::fromUtf8("QPushButton {\n"
							  "	font-size: 24px;\n"
							  "	color: rgb(255, 255, 255);\n"
							  "    background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 "
							  "rgba(62, 62, 62, 255), stop:1 rgba(32, 32, 32, 255));\n"
							  "    border: 2px solid rgb(0, 0, 0);\n"
							  "    border-radius: 15px;\n"
							  "}\n"
							  "\n"
							  "QPushButton:hover {\n"
							  "    background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 "
							  "rgba(52, 52, 52, 255), stop:1 rgba(22, 22, 22, 255));\n"
							  "}\n"
							  "\n"
							  "QPushButton:pressed {\n"
							  "    background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 "
							  "rgba(32, 32, 32, 255), stop:1 rgba(12, 12, 12, 255));\n"
							  "}\n"
							  "\n"
							  "QPushButton:disabled {\n"
							  "    background-color: rgba(62, 62, 62, 255);\n"
							  "    border: 2px solid rgba(100, 100, 100, 255);\n"
							  "}\n"
							  "\n"
							  "QPushButton:disabled:hover {\n"
							  "    background-color: rgba(52, 52, 52, 255);\n"
							  "}"));

		mainLayout->addWidget(tapButton, 0, Qt::AlignHCenter | Qt::AlignVCenter);

		sidebarLayout = new QHBoxLayout();
		sidebarLayout->setSpacing(5);
		sidebarLayout->setObjectName(QString::fromUtf8("sidebarLayout"));
		sidebarLayout->setSizeConstraint(QLayout::SetDefaultConstraint);

		msLabel = new QLabel(tapTempo);
		msLabel->setObjectName(QString::fromUtf8("msLabel"));
		msLabel->setStyleSheet(QString::fromUtf8(""));

		sidebarLayout->addWidget(msLabel, 0, Qt::AlignHCenter);

		hzLabel = new QLabel(tapTempo);
		hzLabel->setObjectName(QString::fromUtf8("hzLabel"));
		hzLabel->setStyleSheet(QString::fromUtf8(""));

		sidebarLayout->addWidget(hzLabel, 0, Qt::AlignHCenter);

		verticalLayout = new QVBoxLayout();
		verticalLayout->setSpacing(5);
		verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));

		precisionCheckBox = new QCheckBox(tapTempo);
		precisionCheckBox->setObjectName(QString::fromUtf8("precisionCheckBox"));
		precisionCheckBox->setStyleSheet(QString::fromUtf8("color: rgb(255, 255, 255);\n"
														   "\n"
														   "QCheckBox::indicator::checked {\n"
														   "	color: rgb(0, 0, 0)\n"
														   "}"));

		verticalLayout->addWidget(precisionCheckBox);

		muteCheckBox = new QCheckBox(tapTempo);
		muteCheckBox->setObjectName(QString::fromUtf8("muteCheckBox"));
		muteCheckBox->setStyleSheet(QString::fromUtf8("color: rgb(255, 255, 255);"));

		verticalLayout->addWidget(muteCheckBox);

		sidebarLayout->addLayout(verticalLayout);

		mainLayout->addLayout(sidebarLayout);

		windowLayout->addLayout(mainLayout);

		retranslateUi(tapTempo);

		QMetaObject::connectSlotsByName(tapTempo);
	} // setupUi

	void retranslateUi(QWidget* tapTempo)
	{
		tapTempo->setWindowTitle(QObject::tr("Tap Tempo"));
		tapButton->setText(QObject::tr("TAP"));
		msLabel->setToolTip(QObject::tr("BPM in milliseconds"));
		msLabel->setText(QObject::tr("0.0 ms"));
		hzLabel->setToolTip(QObject::tr("Frequency of BPM"));
		hzLabel->setText(QObject::tr("0.0 hz"));
		precisionCheckBox->setToolTip(QObject::tr("Display in high precision"));
		precisionCheckBox->setText(QObject::tr("Precision"));
		muteCheckBox->setToolTip(QObject::tr("Mute metronome"));
		muteCheckBox->setText(QObject::tr("Mute"));
	} // retranslateUi
};
} // namespace lmms::gui

#endif // LMMS_GUI_TAP_TEMPO_UI_H
