/*
 * LOMMControlDialog.h
 *
 * Copyright (c) 2023 Lost Robot <r94231/at/gmail/dot/com>
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

#ifndef LMMS_GUI_LOMM_CONTROL_DIALOG_H
#define LMMS_GUI_LOMM_CONTROL_DIALOG_H

#include "EffectControlDialog.h"


#include "embed.h"
#include "Knob.h"
#include "LcdFloatSpinBox.h"
#include "PixmapButton.h"

namespace lmms
{

inline constexpr float LOMM_DISPLAY_MIN = -72;
inline constexpr float LOMM_DISPLAY_MAX = 0;
inline constexpr float LOMM_DISPLAY_X = 125;
inline constexpr float LOMM_DISPLAY_Y[6] = {24, 41, 106, 123, 186, 203};
inline constexpr float LOMM_DISPLAY_WIDTH = 150;
inline constexpr float LOMM_DISPLAY_HEIGHT = 13;
inline constexpr float LOMM_DISPLAY_DB_PER_PIXEL = (LOMM_DISPLAY_MAX - LOMM_DISPLAY_MIN) / LOMM_DISPLAY_WIDTH;

class LOMMControls;


namespace gui
{

class LOMMControlDialog : public EffectControlDialog
{
	Q_OBJECT
public:
	LOMMControlDialog(LOMMControls* controls);
	~LOMMControlDialog() override = default;
	
	int dbfsToX(float dbfs);
	float xToDbfs(int x);
	
	Knob* createKnob(KnobType knobType, QWidget* parent, int x, int y, FloatModel* model, const QString& hintText, const QString& unit, const QString& toolTip)
	{
		Knob* knob = new Knob(knobType, parent);
		knob->move(x, y);
		knob->setModel(model);
		knob->setHintText(hintText, unit);
		knob->setToolTip(toolTip);
		return knob;
	}
	
	LcdFloatSpinBox* createLcdFloatSpinBox(int integerDigits, int decimalDigits, const QString& color, const QString& unit, QWidget* parent, int x, int y, FloatModel* model, const QString& toolTip)
	{
		LcdFloatSpinBox* spinBox = new LcdFloatSpinBox(integerDigits, decimalDigits, color, unit, parent);
		spinBox->move(x, y);
		spinBox->setModel(model);
		spinBox->setSeamless(true, true);
		spinBox->setToolTip(toolTip);
		return spinBox;
	}
	
	PixmapButton* createPixmapButton(const QString& text, QWidget* parent, int x, int y, BoolModel* model,
		std::string_view activeIcon, std::string_view inactiveIcon, const QString& tooltip)
	{
		PixmapButton* button = new PixmapButton(parent, text);
		button->move(x, y);
		button->setCheckable(true);
		if (model) { button->setModel(model); }
		button->setActiveGraphic(PLUGIN_NAME::getIconPixmap(activeIcon));
		button->setInactiveGraphic(PLUGIN_NAME::getIconPixmap(inactiveIcon));
		button->setToolTip(tooltip);
		return button;
	}

protected:
	void paintEvent(QPaintEvent *event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;

private:
	LOMMControls* m_controls;
	
	QPoint m_lastMousePos;
	bool m_buttonPressed = false;
	int m_bandDrag = 0;
	int m_dragType = -1;
	
	PixmapButton* m_feedbackButton;
	PixmapButton* m_lowSideUpwardSuppressButton;

private slots:
	void updateFeedbackVisibility();
	void updateLowSideUpwardSuppressVisibility();
	void updateDisplay();
};


} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_LOMM_CONTROL_DIALOG_H
