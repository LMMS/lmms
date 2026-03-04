/*
 * FloatModelEditorBase.h - Base editor for float models
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2023 Michael Gregorius
 * Copyright (c) 2026 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#ifndef LMMS_GUI_FLOAT_MODEL_EDITOR_BASE_H
#define LMMS_GUI_FLOAT_MODEL_EDITOR_BASE_H

#include <QWidget>
#include <QPoint>
#include <optional>

#include "AutomatableModelView.h"

namespace lmms::gui
{

class SimpleTextFloat;

class LMMS_EXPORT FloatModelEditorBase : public QWidget, public FloatModelView
{
	Q_OBJECT

	void initUi(const QString & name); //!< to be called by ctors

public:
	enum class DirectionOfManipulation : bool
	{
		Vertical,
		Horizontal
	};

	FloatModelEditorBase(DirectionOfManipulation directionOfManipulation = DirectionOfManipulation::Vertical, QWidget * _parent = nullptr, const QString & _name = QString()); //!< default ctor
	FloatModelEditorBase(const FloatModelEditorBase& other) = delete;

	// TODO: remove
	inline void setHintText(const QString & txt_before, const QString & txt_after)
	{
		setDescription(txt_before);
		setUnit(txt_after);
	}

signals:
	void sliderPressed();
	void sliderReleased();
	void sliderMoved(float value);

protected:
	void contextMenuEvent(QContextMenuEvent * me) override;
	void dragEnterEvent(QDragEnterEvent * dee) override;
	void dropEvent(QDropEvent * de) override;
	void focusOutEvent(QFocusEvent * fe) override;
	void mousePressEvent(QMouseEvent * me) override;
	void mouseReleaseEvent(QMouseEvent * me) override;
	void mouseMoveEvent(QMouseEvent * me) override;
	void mouseDoubleClickEvent(QMouseEvent * me) override;
	void paintEvent(QPaintEvent * me) override;
	void wheelEvent(QWheelEvent * me) override;

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	void enterEvent(QEnterEvent*) override;
#else
	void enterEvent(QEvent*) override;
#endif
	void leaveEvent(QEvent *event) override;

	virtual float getValue(const QPoint & p);

	/**
	 * This method is called just prior to displaying the floating text
	 * in order to set its value. If the getCustomFloatingTextUpdate() method
	 * is not overridden, this method is also called to periodically update
	 * the floating text.
	 *
	 * Floating text is displayed in the following format:
	 *     "[description] [custom text][unit]"
	 *
	 * This method controls only the "custom text" portion.
	 * To modify the other portions, call setDescription() or setUnit().
	 */
	virtual QString getCustomFloatingText();

	/**
	 * This method is called periodically while the floating text is visible
	 * and the value of the float model is changing, allowing dynamic updates
	 * of the floating text.
	 *
	 * Floating text is displayed in the following format:
	 *     "[description] [custom text][unit]"
	 *
	 * This method controls only the "custom text" portion.
	 * To modify the other portions, call setDescription() or setUnit().
	 *
	 * @returns the up-to-date value for the floating text, or std::nullopt to indicate
	 *          nothing changed and the previous floating text value should continue being used
	 */
	virtual std::optional<QString> getCustomFloatingTextUpdate()
	{
		return getCustomFloatingText();
	}

	void doConnections() override;

	void showTextFloat(int msecBeforeDisplay, int msecDisplayTime);
	void showTextFloat();

	const SimpleTextFloat& textFloat() const { return *s_textFloat; }

	void setPosition(const QPoint & p);

	inline float pageSize() const
	{
		return (model()->maxValue() - model()->minValue()) / 100.0f;
	}

	QPoint m_lastMousePos; //!< mouse position in last mouseMoveEvent
	float m_leftOver;
	bool m_buttonPressed;

	DirectionOfManipulation m_directionOfManipulation;

private slots:
	virtual void enterValue();
	void friendlyUpdate();
	void toggleScale();

private:
	static SimpleTextFloat* s_textFloat;
};

} // namespace lmms::gui

#endif // LMMS_GUI_FLOAT_MODEL_EDITOR_BASE_H
