/*
 * FloatModelEditorBase.cpp - Base editor for float models
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "FloatModelEditorBase.h"

#include <QApplication>
#include <QInputDialog>
#include <QPainter>
#include <QTimerEvent>

#include "lmms_math.h"
#include "DeprecationHelper.h"
#include "CaptionMenu.h"
#include "ControllerConnection.h"
#include "GuiApplication.h"
#include "KeyboardShortcuts.h"
#include "LocaleHelper.h"
#include "MainWindow.h"
#include "ProjectJournal.h"
#include "SimpleTextFloat.h"
#include "StringPairDrag.h"


namespace lmms::gui {

namespace {

//! Whether the mouse is adjusting the control by dragging
auto isMouseDragAdjustment(QMouseEvent* event) -> bool
{
	return event->button() == Qt::LeftButton
		&& !(event->modifiers() & KBD_COPY_MODIFIER)
		&& !(event->modifiers() & Qt::ShiftModifier);
}

} // namespace

SimpleTextFloat * FloatModelEditorBase::s_textFloat = nullptr;

FloatModelEditorBase::FloatModelEditorBase(DirectionOfManipulation directionOfManipulation, QWidget * parent, const QString & name) :
	QWidget(parent),
	FloatModelView(new FloatModel(0, 0, 0, 1, nullptr, name, true), this),
	m_directionOfManipulation(directionOfManipulation)
{
	initUi(name);
}


void FloatModelEditorBase::initUi(const QString & name)
{
	if (s_textFloat == nullptr)
	{
		s_textFloat = new SimpleTextFloat;
	}

	setWindowTitle(name);

	setFocusPolicy(Qt::ClickFocus);

	doConnections();
}


void FloatModelEditorBase::showTextFloat(int msecBeforeDisplay, int msecDisplayTime, bool forceTextUpdate)
{
	assert(m_interaction != InteractionType::None);

	// First, check if the text needs to be updated
	if (s_textFloat->source() != this || forceTextUpdate)
	{
		s_textFloat->setSource(this);

		// Next, set the floating text depending on the floating text type
		if (currentFloatingText() == FloatingTextType::Static)
		{
			// Using static floating text
			assert(m_staticToolTip.has_value());
			if (m_staticToolTip->isEmpty())
			{
				// Using neither static nor dynamic floating text - don't display anything
				s_textFloat->hide();
				return;
			}
			s_textFloat->setText(*m_staticToolTip);
		}
		else
		{
			// Using dynamic floating text
			s_textFloat->setText(m_description + ' ' + getDynamicFloatingText() + m_unit);
		}
	}

	s_textFloat->moveGlobal(this, QPoint(width() + 2, 0));
	s_textFloat->showWithDelay(msecBeforeDisplay, msecDisplayTime);
}


void FloatModelEditorBase::showTextFloat(bool forceTextUpdate)
{
	showTextFloat(0, 0, forceTextUpdate);
}


float FloatModelEditorBase::getValue(const QPoint & p)
{
	// Find out which direction/coordinate is relevant for this control
	const int coordinate = m_directionOfManipulation == DirectionOfManipulation::Vertical ? p.y() : -p.x();

	// knob value increase is linear to mouse movement
	float value = .4f * coordinate;

	// if shift pressed we want slower movement
	if (getGUI()->mainWindow()->isShiftPressed())
	{
		value /= 4.0f;
		value = qBound(-4.0f, value, 4.0f);
	}

	return value * pageSize();
}


void FloatModelEditorBase::contextMenuEvent(QContextMenuEvent *)
{
	// for the case, the user clicked right while pressing left mouse-
	// button, the context-menu appears while mouse-cursor is still hidden
	// and it isn't shown again until user does something which causes
	// an QApplication::restoreOverrideCursor()-call...
	mouseReleaseEvent(nullptr);

	CaptionMenu contextMenu(model()->displayName(), this);
	addDefaultActions(&contextMenu);
	contextMenu.addAction(QPixmap(),
		model()->isScaleLogarithmic() ? tr("Set linear") : tr("Set logarithmic"),
		this, SLOT(toggleScale()));
	contextMenu.addSeparator();
	contextMenu.exec(QCursor::pos());
}


void FloatModelEditorBase::toggleScale()
{
	model()->setScaleLogarithmic(! model()->isScaleLogarithmic());
	update();
}


void FloatModelEditorBase::dragEnterEvent(QDragEnterEvent * dee)
{
	StringPairDrag::processDragEnterEvent(dee, "float_value,"
							"automatable_model");
}


void FloatModelEditorBase::dropEvent(QDropEvent * de)
{
	QString type = StringPairDrag::decodeKey(de);
	QString val = StringPairDrag::decodeValue(de);
	if (type == "float_value")
	{
		model()->setValue(LocaleHelper::toFloat(val));
		de->accept();
	}
	else if (type == "automatable_model")
	{
		auto mod = dynamic_cast<AutomatableModel*>(Engine::projectJournal()->journallingObject(val.toInt()));
		if (mod != nullptr)
		{
			model()->linkToModel(mod);
		}
	}
}


void FloatModelEditorBase::mousePressEvent(QMouseEvent * me)
{
	updateInteractionState(me);

	if (isMouseDragAdjustment(me))
	{
		AutomatableModel *thisModel = model();
		if (thisModel)
		{
			thisModel->addJournalCheckPoint();
			thisModel->saveJournallingState(false);
		}

		m_lastMousePos = position(me);
		m_leftOver = 0.0f;

		emit sliderPressed();

		showTextFloat(true);
	}
	else if (me->button() == Qt::LeftButton &&
			(me->modifiers() & Qt::ShiftModifier))
	{
		new StringPairDrag("float_value",
					QString::number(model()->value()),
							QPixmap(), this);
	}
	else
	{
		FloatModelView::mousePressEvent(me);
	}
}


void FloatModelEditorBase::mouseMoveEvent(QMouseEvent * me)
{
	updateInteractionState(me);

	const auto pos = position(me);
	if (m_interaction == InteractionType::MouseDrag && pos != m_lastMousePos)
	{
		// knob position is changed depending on last mouse position
		setPosition(pos - m_lastMousePos);
		emit sliderMoved(model()->value());
		// original position for next time is current position
		m_lastMousePos = pos;
	}

	showTextFloat();
}


void FloatModelEditorBase::mouseReleaseEvent(QMouseEvent* event)
{
	updateInteractionState(event);

	if (event && event->button() == Qt::LeftButton)
	{
		AutomatableModel *thisModel = model();
		if (thisModel)
		{
			thisModel->restoreJournallingState();
		}
	}

	emit sliderReleased();

	QApplication::restoreOverrideCursor();

	s_textFloat->hide();
}

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
void FloatModelEditorBase::enterEvent(QEnterEvent* event)
#else
void FloatModelEditorBase::enterEvent(QEvent* event)
#endif
{
	updateInteractionState(event);
	showTextFloat(700, 2000);
}


void FloatModelEditorBase::leaveEvent(QEvent *event)
{
	updateInteractionState(event);
	s_textFloat->hide();
}


void FloatModelEditorBase::focusOutEvent(QFocusEvent * fe)
{
	// make sure we don't loose mouse release event
	mouseReleaseEvent(nullptr);
	QWidget::focusOutEvent(fe);
}


void FloatModelEditorBase::mouseDoubleClickEvent(QMouseEvent *)
{
	enterValue();
}


void FloatModelEditorBase::paintEvent(QPaintEvent *)
{
	QPainter p(this);

	const auto foreground = QColor{3, 94, 97};

	const auto* mod = model();
	const auto minValue = mod->minValue();
	const auto maxValue = mod->maxValue();
	const auto range = maxValue - minValue;

	// Compute the percentage
	// min + x * (max - min) = v <=> x = (v - min) / (max - min)
	const auto percentage = range == 0 ? 1. : (mod->value() - minValue) / range;

	QRect r = rect();
	p.setPen(foreground);
	p.setBrush(foreground);
	p.drawRect(QRect(r.topLeft(), QPoint(r.width() * percentage, r.height())));
}


void FloatModelEditorBase::wheelEvent(QWheelEvent * we)
{
	const auto oldInteraction = m_interaction;
	updateInteractionState(we);

	we->accept();
	const int deltaY = we->angleDelta().y();
	float direction = deltaY > 0 ? 1 : -1;

	auto * m = model();
	const float step = m->step<float>();
	const float range = m->range();

	// This is the default number of steps or mouse wheel events that it takes to sweep
	// from the lowest value to the highest value.
	// It might be modified if the user presses modifier keys. See below.
	float numberOfStepsForFullSweep = 100.;

	const auto modKeys = we->modifiers();
	if (modKeys == Qt::ShiftModifier)
	{
		// The shift is intended to go through the values in very coarse steps as in:
		// "Shift into overdrive"
		numberOfStepsForFullSweep = 10;
	}
	else if (modKeys == Qt::ControlModifier)
	{
		// The control key gives more control, i.e. it enables more fine-grained adjustments
		numberOfStepsForFullSweep = 1000;
	}
	else if (modKeys == Qt::AltModifier)
	{
		// The alt key enables even finer adjustments
		numberOfStepsForFullSweep = 2000;

		// It seems that on some systems pressing Alt with mess with the directions,
		// i.e. scrolling the mouse wheel is interpreted as pressing the mouse wheel
		// left and right. Account for this quirk.
		if (deltaY == 0)
		{
			const int deltaX = we->angleDelta().x();
			if (deltaX != 0)
			{
				direction = deltaX > 0 ? 1 : -1;
			}
		}
	}

	// Handle "natural" scrolling, which is common on trackpads and touch devices
	if (we->inverted()) {
		direction = -direction;
	}

	// Compute the number of steps but make sure that we always do at least one step
	const float currentValue = model()->value();
	const float valueOffset = range / numberOfStepsForFullSweep;
	const float scaledValueOffset = model()->scaledValue(model()->inverseScaledValue(currentValue) + valueOffset) - currentValue;
	const float stepMult = std::max(scaledValueOffset / step, 1.f);
	const int inc = direction * stepMult;
	model()->incValue(inc);

	// Only force a text update for the 1st wheel event
	showTextFloat(0, 1000, m_interaction != oldInteraction);

	emit sliderMoved(model()->value());
}


void FloatModelEditorBase::setPosition(const QPoint & p)
{
	const float valueOffset = getValue(p) + m_leftOver;
	const float currentValue = model()->value();
	const float scaledValueOffset = currentValue - model()->scaledValue(model()->inverseScaledValue(currentValue) - valueOffset);
	const auto step = model()->step<float>();
	const float roundedValue = std::round((currentValue - scaledValueOffset) / step) * step;

	if (!approximatelyEqual(roundedValue, currentValue))
	{
		model()->setValue(roundedValue);
		m_leftOver = 0.0f;
	}
	else
	{
		if (valueOffset > 0 && approximatelyEqual(currentValue, model()->minValue()))
		{
			m_leftOver = 0.0f;
		}
		else
		{
			m_leftOver = valueOffset;
		}
	}
}

void FloatModelEditorBase::updateInteractionState(QEvent* event)
{
	// This is a state machine for updating m_interaction

	if (!event)
	{
		m_interaction = InteractionType::None;
		return;
	}

	switch (event->type())
	{
		case QEvent::Type::MouseButtonPress:
			if (isMouseDragAdjustment(static_cast<QMouseEvent*>(event)))
			{
				m_interaction = InteractionType::MouseDrag;
			}
			break;
		case QEvent::Type::MouseButtonRelease:
			if (static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton)
			{
				m_interaction = InteractionType::None;
			}
			break;
		case QEvent::Type::MouseMove:
			if (m_interaction == InteractionType::None)
			{
				m_interaction = InteractionType::MouseHover;
			}
			break;
		case QEvent::Type::Enter:
			if (m_interaction == InteractionType::None)
			{
				m_interaction = InteractionType::MouseHover;
			}
			break;
		case QEvent::Type::Leave:
			// Preserve MouseDrag because the user can drag the mouse outside the bounds of the control
			// while adjusting its value, but the floating text should still be shown
			if (m_interaction != InteractionType::MouseDrag)
			{
				m_interaction = InteractionType::None;
			}
			break;
		case QEvent::Type::Wheel:
			if (m_interaction != InteractionType::MouseDrag)
			{
				m_interaction = InteractionType::MouseWheel;
			}
			break;
		default:
			throw std::logic_error{"updateInteractionState: unknown event type"};
	}
}

auto FloatModelEditorBase::currentFloatingText() const -> FloatingTextType
{
	switch (m_interaction)
	{
		case InteractionType::None: return FloatingTextType::None;
		case InteractionType::MouseHover:
			if (s_textFloat->source() != this)
			{
				// The mouse is hovering over the control but not long enough
				// for the floating text to be shown
				return FloatingTextType::None;
			}

			return m_staticToolTip
				? FloatingTextType::Static
				: FloatingTextType::Dynamic;
		default: break;
	}

	// For MouseDrag or MouseWheel interactions, check whether the floating
	// text is shown for this control
	return s_textFloat->source() == this
		? FloatingTextType::Dynamic
		: FloatingTextType::None;
}


void FloatModelEditorBase::enterValue()
{
	bool ok = false;
	const float newVal = QInputDialog::getDouble(
		this,
		tr("Set value"),
		tr("Please enter a new value between %1 and %2:")
			.arg(model()->minValue())
			.arg(model()->maxValue()),
		model()->getRoundedValue(),
		model()->minValue(),
		model()->maxValue(),
		model()->getDigitCount(),
		&ok
	);

	if (ok)
	{
		model()->setValue(newVal);
	}
}


void FloatModelEditorBase::friendlyUpdate()
{
	if (model() == nullptr) { return; }

	// If the controller changes constantly, only repaint every 1024th frame
	if (model()->useControllerValue()
		&& model()->controllerConnection()
		&& model()->controllerConnection()->getController()->frequentUpdates()
		&& Controller::runningFrames() % (256 * 4) != 0)
	{ return; }

	// If this float model is currently controlling dynamic floating text...
	if (currentFloatingText() == FloatingTextType::Dynamic)
	{
		// ...and if the text changed since last time...
		if (auto updatedText = getDynamicFloatingTextUpdate())
		{
			// ...then update the floating text
			s_textFloat->setText(m_description + ' ' + std::move(*updatedText) + m_unit);
		}
	}

	update();
}


QString FloatModelEditorBase::getDynamicFloatingText()
{
	return QString::number(model()->getRoundedValue());
}


void FloatModelEditorBase::doConnections()
{
	if (model() != nullptr)
	{
		QObject::connect(model(), SIGNAL(dataChanged()),
					this, SLOT(friendlyUpdate()));

		QObject::connect(model(), SIGNAL(propertiesChanged()),
						this, SLOT(update()));
	}
}

} // namespace lmms::gui
