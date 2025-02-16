/*
 * LadspaMatrixControlDialog.h - Dialog for displaying and editing control port
 *                               values for LADSPA plugins in a matrix display
 *
 * Copyright (c) 2015 Michael Gregorius <michaelgregorius/at/web[dot]de>
 *
 * This file is part of LMMS - http://lmms.io
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

#ifndef LADSPA_MATRIX_CONTROL_DIALOG_H
#define LADSPA_MATRIX_CONTROL_DIALOG_H

#include "EffectControlDialog.h"

#include "LmmsTypes.h"


class QGridLayout;
class QScrollArea;

namespace lmms
{

class LadspaControls;

namespace gui
{

class LedCheckBox;


class LadspaMatrixControlDialog : public EffectControlDialog
{
	Q_OBJECT
public:
	LadspaMatrixControlDialog(LadspaControls* ctl);
	bool isResizable() const override;

private slots:
	void updateEffectView(LadspaControls* ctl);

private:
	/**
	 * @brief Checks if a link column is needed for the current effect controls.
	 * @return true if a link column is needed.
	 */
	bool needsLinkColumn() const;

	/**
	 * @brief Arranges widgets for the current controls in a grid/matrix layout.
	 * @param parent The parent of all created widgets
	 * @param gridLayout The layout into which the controls are organized
	 */
	void arrangeControls(QWidget * parent, QGridLayout* gridLayout);

	/**
	 * @brief Creates a widget that holds the widgets of the current controls in a matrix arrangement.
	 * @param ladspaControls
	 * @return
	 */
	QWidget * createMatrixWidget();

	LadspaControls * getLadspaControls() const;
	ch_cnt_t getChannelCount() const;

private:
	QScrollArea* m_scrollArea;
	LedCheckBox* m_stereoLink;
};

} // namespace gui

} // namespace lmms

#endif
