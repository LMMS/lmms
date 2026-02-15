/*
 * FloatModelEditorBase.cpp - Base editor for float models
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2023 Michael Gregorius
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


namespace lmms::gui
{

SimpleTextFloat * FloatModelEditorBase::s_textFloat = nullptr;

FloatModelEditorBase::FloatModelEditorBase(DirectionOfManipulation directionOfManipulation, QWidget * parent, const QString & name) :
	QWidget(parent),
	FloatModelView(new FloatModel(0, 0, 0, 1, nullptr, name, true), this),
	m_volumeKnob(false),
	m_volumeRatio(100.0, 0.0, 1000000.0),
	m_buttonPressed(false),
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


void FloatModelEditorBase::showTextFloat(int msecBeforeDisplay, int msecDisplayTime)
{
	s_textFloat->setText(displayValue());
	s_textFloat->moveGlobal(this, QPoint(width() + 2, 0));
	s_textFloat->showWithDelay(msecBeforeDisplay, msecDisplayTime);
}


float FloatModelEditorBase::getValue(const QPoint & p)
{
	// Find out which direction/coordinate is relevant for this control
	int const coordinate = m_directionOfManipulation == DirectionOfManipulation::Vertical ? p.y() : -p.x();

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
	if (me->button() == Qt::LeftButton &&
			! (me->modifiers() & KBD_COPY_MODIFIER) &&
			! (me->modifiers() & Qt::ShiftModifier))
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

		showTextFloat(0, 0);

		s_textFloat->setText(displayValue());
		s_textFloat->moveGlobal(this,
				QPoint(width() + 2, 0));
		s_textFloat->show();
		m_buttonPressed = true;
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
	const auto pos = position(me);

	if (m_buttonPressed && pos != m_lastMousePos)
	{
		// knob position is changed depending on last mouse position
		setPosition(pos - m_lastMousePos);
		emit sliderMoved(model()->value());
		// original position for next time is current position
		m_lastMousePos = pos;
	}
	s_textFloat->setText(displayValue());
	s_textFloat->show();
}


void FloatModelEditorBase::mouseReleaseEvent(QMouseEvent* event)
{
	if (event && event->button() == Qt::LeftButton)
	{
		AutomatableModel *thisModel = model();
		if (thisModel)
		{
			thisModel->restoreJournallingState();
		}
	}

	m_buttonPressed = false;

	emit sliderReleased();

	QApplication::restoreOverrideCursor();

	s_textFloat->hide();
}

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
void FloatModelEditorBase::enterEvent(QEnterEvent*)
#else
void FloatModelEditorBase::enterEvent(QEvent*)
#endif
{
	showTextFloat(700, 2000);
}


void FloatModelEditorBase::leaveEvent(QEvent *event)
{
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

	QColor const foreground(3, 94, 97);

	auto const * mod = model();
	auto const minValue = mod->minValue();
	auto const maxValue = mod->maxValue();
	auto const range = maxValue - minValue;

	// Compute the percentage
	// min + x * (max - min) = v <=> x = (v - min) / (max - min)
	auto const percentage = range == 0 ? 1. : (mod->value() - minValue) / range;

	QRect r = rect();
	p.setPen(foreground);
	p.setBrush(foreground);
	p.drawRect(QRect(r.topLeft(), QPoint(r.width() * percentage, r.height())));
}


void FloatModelEditorBase::wheelEvent(QWheelEvent * we)
{
	we->accept();
	const int deltaY = we->angleDelta().y();
	float direction = deltaY > 0 ? 1 : -1;

	auto * m = model();
	float const step = m->step<float>();
	float const range = m->range();

	// This is the default number of steps or mouse wheel events that it takes to sweep
	// from the lowest value to the highest value.
	// It might be modified if the user presses modifier keys. See below.
	float numberOfStepsForFullSweep = 100.;

	auto const modKeys = we->modifiers();
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
			int const deltaX = we->angleDelta().x();
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

	s_textFloat->setText(displayValue());
	s_textFloat->moveGlobal(this, QPoint(width() + 2, 0));
	s_textFloat->showWithTimeout(1000);

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


void FloatModelEditorBase::enterValue()
{
	bool ok;
	float new_val;

	if (isVolumeKnob())
	{
		auto const initalValue = model()->getRoundedValue() / 100.0;
		auto const initialDbValue = initalValue > 0. ? ampToDbfs(initalValue) : -96;

		new_val = QInputDialog::getDouble(
			this, tr("Set value"),
			tr("Please enter a new value between -96.0 dBFS and %1 dBFS:").arg(ampToDbfs(model()->maxValue() / 100.0f)),
				initialDbValue, -96.0, ampToDbfs(model()->maxValue() / 100.0f), model()->getDigitCount(), &ok);

		if (new_val <= -96.0)
		{
			new_val = 0.0f;
		}
		else
		{
			new_val = dbfsToAmp(new_val) * 100.0;
		}
	}
	else
	{
		new_val = QInputDialog::getDouble(
				this, tr("Set value"),
				tr("Please enter a new value between "
						"%1 and %2:").
						arg(model()->minValue()).
						arg(model()->maxValue()),
					model()->getRoundedValue(),
					model()->minValue(),
					model()->maxValue(), model()->getDigitCount(), &ok);
	}

	if (ok)
	{
		model()->setValue(new_val);
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

	update();
}


QString FloatModelEditorBase::displayValue() const
{
	if (isVolumeKnob())
	{
		auto const valueToVolumeRatio = model()->getRoundedValue() / volumeRatio();
		return m_description.trimmed() + (
			valueToVolumeRatio == 0.
			? QString(" -∞ dBFS")
			: QString(" %1 dBFS").arg(ampToDbfs(valueToVolumeRatio), 3, 'f', 2)
		);
	}

	return m_description.trimmed() + QString(" %1").
					arg(model()->getRoundedValue()) + m_unit;
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
