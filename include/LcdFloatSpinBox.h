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


#ifndef LCD_FLOATSPINBOX_H
#define LCD_FLOATSPINBOX_H

#include <QString>

#include "LcdWidget.h"
#include "AutomatableModelView.h"


class LMMS_EXPORT LcdFloatSpinBox : public QWidget, public FloatModelView
{
	Q_OBJECT
public:
	LcdFloatSpinBox(int numWhole, int numFrac, QWidget* parent, const QString& name = QString());
	LcdFloatSpinBox(int numWhole, int numFrac, const QString& style, QWidget* parent, const QString& name = QString());
	virtual ~LcdFloatSpinBox() = default;

	void modelChanged() override
	{
		ModelView::modelChanged();
		update();
	}

	/*! Sets an offset which is always added to value of model so we can
	    display values in a user-friendly way if they internally start at 0 */
	void setDisplayOffset(int offset)
	{
		m_displayOffset = offset;
	}

	/*! \brief Returns internal offset for displaying values */
	int displayOffset() const
	{
		return m_displayOffset;
	}

	void setLabel(const QString &label) {m_label = label;}

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
	float getStep();

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

typedef FloatModel LcdFloatSpinBoxModel;

#endif
