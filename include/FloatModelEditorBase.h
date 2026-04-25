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

#include <QPoint>
#include <QWidget>
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

	/**
	 * Sets the tooltip displayed when the mouse hovers over the control.
	 *
	 * Unlike the dynamic floating text from getDynamicFloatingText() which represents the
	 * current value of the model, this is static text intended to provide a helpful description
	 * of the control. That is, it's just a traditional tooltip, though it uses SimpleTextFloat
	 * rather than QWidget's own tooltip for consistency with the dynamic floating text.
	 *
	 * If no static tooltip is set (when this method is not called), dynamic floating text
	 * is used in its place. See @ref InteractionType for more information.
	 *
	 * @param tip the static tooltip. If empty, neither a static nor dynamic tooltip will be
	 *            displayed when the mouse hovers over the control.
	 */
	void setToolTip(const QString& tip)
	{
		m_staticToolTip.emplace(tip);
	}

	QString toolTip() const { return m_staticToolTip.value_or(QString{}); }

	/**
	 * Removes the static tooltip set by a previous call to setToolTip().
	 * The dynamic floating text will be used in its place.
	 * NOTE: This is currently unused.
	 */
	void unsetToolTip() { m_staticToolTip.reset(); }

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
	 * This method is called just prior to displaying dynamic floating text
	 * in order to set its value. If the getDynamicFloatingTextUpdate() method
	 * is not overridden, this method is also called to periodically update
	 * the floating text.
	 *
	 * Dynamic floating text is displayed in the following format:
	 *     "[description] [dynamic text][unit]"
	 *
	 * This method controls only the "dynamic text" portion.
	 * To modify the other portions, call setDescription() or setUnit().
	 */
	virtual QString getDynamicFloatingText();

	/**
	 * This method is called periodically while dynamic floating text is visible
	 * and the value of the float model is changing, allowing dynamic updates
	 * of the floating text.
	 *
	 * Dynamic floating text is displayed in the following format:
	 *     "[description] [dynamic text][unit]"
	 *
	 * This method controls only the "dynamic text" portion.
	 * To modify the other portions, call setDescription() or setUnit().
	 *
	 * @returns the up-to-date value for the dynamic floating text, or std::nullopt to
	 *          indicate the previous floating text value should continue being used
	 */
	virtual std::optional<QString> getDynamicFloatingTextUpdate()
	{
		return getDynamicFloatingText();
	}

	void doConnections() override;

	void showTextFloat(int msecBeforeDisplay, int msecDisplayTime, bool forceTextUpdate = false);
	void showTextFloat(bool forceTextUpdate = false);

	const SimpleTextFloat& textFloat() const { return *s_textFloat; }

	void setPosition(const QPoint & p);

	inline float pageSize() const
	{
		return (model()->maxValue() - model()->minValue()) / 100.0f;
	}

	DirectionOfManipulation directionOfManipulation() const { return m_directionOfManipulation; }

	//! Types of user interaction with the control
	enum class InteractionType : std::uint8_t
	{
		//! The user is not interacting with the control.
		//! No floating text is shown.
		None,

		//! The mouse is hovering over the control without any other interaction.
		//! If a static tooltip is set (@see setToolTip), it will be displayed,
		//! otherwise dynamic floating text will be displayed.
		MouseHover,

		//! The user is dragging the control to adjust the model's value.
		//! This always results in dynamic floating text, not a static tooltip.
		MouseDrag,

		//! The user is using the mouse wheel on the control to adjust the model's value.
		//! This always results in dynamic floating text, not a static tooltip.
		MouseWheel
	};

	//! @returns how the user is interacting with the control
	InteractionType currentInteraction() const { return m_interaction; }

	//! Updates m_interaction based on the event and current state
	void updateInteractionState(QEvent* event);

	enum class FloatingTextType : std::uint8_t
	{
		//! No floating text
		None,

		//! Traditional static tooltip
		Static,

		//! Dynamic floating text
		Dynamic
	};

	/**
	 * @returns which type of floating text is currently being displayed based on how the user
	 *          is interacting with the control and whether a static tooltip has been set for the control.
	 */
	FloatingTextType floatingTextType() const;

	QPoint m_lastMousePos; //!< mouse position in last mouseMoveEvent
	float m_leftOver;

private slots:
	virtual void enterValue();
	void friendlyUpdate();
	void toggleScale();

private:
	InteractionType m_interaction = InteractionType::None;

	DirectionOfManipulation m_directionOfManipulation;

	std::optional<QString> m_staticToolTip;

	static SimpleTextFloat* s_textFloat;
};

} // namespace lmms::gui

#endif // LMMS_GUI_FLOAT_MODEL_EDITOR_BASE_H
