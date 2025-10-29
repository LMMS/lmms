/*
 * LcdFloatSpinBox.h - class LcdFloatSpinBox (LcdSpinBox for floats)
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2020 Martin Pavelek <he29.HS/at/gmail.com>
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

#ifndef LMMS_GUI_LCD_FLOATSPINBOX_H
#define LMMS_GUI_LCD_FLOATSPINBOX_H

#include <QString>

#include "LcdWidget.h"
#include "AutomatableModelView.h"

namespace lmms::gui
{


class LMMS_EXPORT LcdFloatSpinBox : public QWidget, public FloatModelView
{
	Q_OBJECT
public:
	LcdFloatSpinBox(int numWhole, int numFrac, const QString& name = QString(), QWidget* parent = nullptr);
	LcdFloatSpinBox(int numWhole, int numFrac, const QString& style, const QString& name, QWidget* parent = nullptr);

	void modelChanged() override
	{
		ModelView::modelChanged();
		update();
	}

	void setLabel(const QString &label) { m_label = label; }
	
	void setSeamless(bool left, bool right)
	{
		m_wholeDisplay.setSeamless(left, true);
		m_fractionDisplay.setSeamless(true, right);
	}

public slots:
	virtual void update();

protected:
	void contextMenuEvent(QContextMenuEvent *me) override;
	void mousePressEvent(QMouseEvent *me) override;
	void mouseMoveEvent(QMouseEvent *me) override;
	void mouseReleaseEvent(QMouseEvent *me) override;
	void wheelEvent(QWheelEvent *we) override;
	void mouseDoubleClickEvent(QMouseEvent *me) override;
	void paintEvent(QPaintEvent *pe) override;

private:
	void layoutSetup(const QString &style = QString("19green"));
	void enterValue();
	float getStep() const;

	LcdWidget m_wholeDisplay;
	LcdWidget m_fractionDisplay;
	bool m_mouseMoving;
	bool m_intStep;
	QPoint m_origMousePos;
	int m_displayOffset;
	QString m_label;

signals:
	void manualChange();

};

using LcdFloatSpinBoxModel = FloatModel;

} // namespace lmms::gui

#endif // LMMS_GUI_LCD_FLOATSPINBOX_H
