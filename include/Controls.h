/*
 * Controls.h - labeled control widgets
 *
 * Copyright (c) 2019-2019 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#ifndef CONTROLS_H
#define CONTROLS_H


#include "Model.h"

// headers only required for covariance
#include "AutomatableModel.h"
#include "ComboBoxModel.h"


class QString;
class QWidget;
class AutomatableModel;


/**
	These classes provide
	- a control with a text label
	- a type safe way to set a model
		(justification: setting the wrong typed model to a widget will cause
		hard-to-find runtime errors)
*/
class Control
{
public:
	virtual QWidget* topWidget() = 0;
	virtual void setText(const QString& text) = 0;

	virtual void setModel(AutomatableModel* model) = 0;
	virtual AutomatableModel* model() = 0;

	virtual ~Control();
};


class KnobControl : public Control
{
	class Knob* m_knob;

public:
	void setText(const QString& text) override;
	QWidget* topWidget() override;

	void setModel(AutomatableModel* model) override;
	FloatModel* model() override;

	KnobControl(QWidget* parent = nullptr);
	~KnobControl() override;
};


class ComboControl : public Control
{
	QWidget* m_widget;
	class ComboBox* m_combo;
	class QLabel* m_label;

public:
	void setText(const QString& text) override;
	QWidget* topWidget() override { return m_widget; }

	void setModel(AutomatableModel* model) override;
	ComboBoxModel* model() override;

	ComboControl(QWidget* parent = nullptr);
	~ComboControl() override;
};


class LcdControl : public Control
{
	class LcdSpinBox* m_lcd;

public:
	void setText(const QString& text) override;
	QWidget* topWidget() override;

	void setModel(AutomatableModel* model) override;
	IntModel* model() override;

	LcdControl(int numDigits, QWidget* parent = nullptr);
	~LcdControl() override;
};


class CheckControl : public Control
{
	QWidget* m_widget;
	class LedCheckBox* m_checkBox;
	QLabel* m_label;

public:
	void setText(const QString& text) override;
	QWidget* topWidget() override;

	void setModel(AutomatableModel* model) override;
	BoolModel *model() override;

	CheckControl(QWidget* parent = nullptr);
	~CheckControl() override;
};


#endif // CONTROLS_H
