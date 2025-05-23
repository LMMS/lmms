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
	InteractiveModelView(parent, getTypeId<FloatModelEditorBase>()),
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

	addCommands(m_commandArray);

	setFocusPolicy(Qt::ClickFocus);

	doConnections();
}

void FloatModelEditorBase::addCommands(std::vector<CommandData>& targetList)
{
	targetList =
	{
		CommandData(1, "Copy value", BasicCommandFnPtr<FloatModelEditorBase>(&FloatModelEditorBase::copyValueCommand), true),
		CommandData(2, "Paste value", BasicCommandFnPtr<FloatModelEditorBase>(&FloatModelEditorBase::pasteNoReturnCommand), true),
		CommandData(3, "Paste value", TypedCommandFnPtr<FloatModelEditorBase, bool*>(&FloatModelEditorBase::pasteCommand), true),
		CommandData(4, "Link widget", TypedCommandFnPtr<FloatModelEditorBase, int>(&FloatModelEditorBase::linkCommand),
			TypedCommandFnPtr<FloatModelEditorBase, int>(&FloatModelEditorBase::unlinkCommand), true),
		CommandData(5, "Copy link", BasicCommandFnPtr<FloatModelEditorBase>(&FloatModelEditorBase::getLinkCommand), true),
		CommandData(6, "Unlink from all", BasicCommandFnPtr<FloatModelEditorBase>(&FloatModelEditorBase::unlinkAllCommand), true),
		CommandData(7, "Increase value", BasicCommandFnPtr<FloatModelEditorBase>(&FloatModelEditorBase::increaseValueCommand), true),
		CommandData(8, "Decrease value", BasicCommandFnPtr<FloatModelEditorBase>(&FloatModelEditorBase::decreaseValueCommand), true),
		CommandData(9, "Set value", TypedCommandFnPtr<FloatModelEditorBase, float>(&FloatModelEditorBase::setValueCommand), true),
		CommandData(11, "Edit value", BasicCommandFnPtr<FloatModelEditorBase>(&FloatModelEditorBase::openInputDialogCommand), true),
		CommandData(12, "Toggle scale", BasicCommandFnPtr<FloatModelEditorBase>(&FloatModelEditorBase::toggleScaleCommand), true),
		CommandData(13, "Set scale logarithmic", TypedCommandFnPtr<FloatModelEditorBase, bool>(&FloatModelEditorBase::setScaleLogarithmicCommand), true),
		CommandData(14, "Set position", TypedCommandFnPtr<FloatModelEditorBase, QPoint>(&FloatModelEditorBase::setPositionCommand), true)
	};
}

void FloatModelEditorBase::copyValueCommand()
{
	Clipboard::copyStringPair(Clipboard::DataType::FloatValue, Clipboard::encodeFloatValue(model()->value() * getConversionFactor()));
	InteractiveModelView::startHighlighting(Clipboard::DataType::FloatValue);
}
void FloatModelEditorBase::pasteNoReturnCommand()
{
	FloatModelEditorBase::pasteCommand(nullptr);
}
void FloatModelEditorBase::pasteCommand(bool* isSuccessful)
{
	Clipboard::DataType type = Clipboard::decodeKey(Clipboard::getMimeData());
	QString value = Clipboard::decodeValue(Clipboard::getMimeData());

	bool shouldAccept = false;
	if (type == Clipboard::DataType::FloatValue)
	{
		doCommand<float>(9, LocaleHelper::toFloat(value), model()->value(), true); // setValueCommand
		shouldAccept = true;
	}
	else if (type == Clipboard::DataType::AutomatableModelLink)
	{
		doCommand<int>(4, value.toInt(), true); // linkCommand
		shouldAccept = true;
	}
	if (shouldAccept)
	{
		if (isSuccessful != nullptr) { *isSuccessful = true; }
		InteractiveModelView::stopHighlighting();
	}
}
void FloatModelEditorBase::linkCommand(int id)
{
	auto mod = dynamic_cast<AutomatableModel*>(Engine::projectJournal()->journallingObject(id));
	if (mod != nullptr)
	{
		doCommand<float>(9, mod->value<float>(), model()->value(), true); // setValueCommand
		AutomatableModel::linkModels(model(), mod);
	}
}
void FloatModelEditorBase::unlinkCommand(int id)
{
	auto mod = dynamic_cast<AutomatableModel*>(Engine::projectJournal()->journallingObject(id));
	if (mod != nullptr)
	{
		AutomatableModel::unlinkModels(model(), mod);
	}
}
void FloatModelEditorBase::getLinkCommand()
{
	Clipboard::copyStringPair(Clipboard::DataType::AutomatableModelLink, Clipboard::encodeAutomatableModelLink(*model()));
	InteractiveModelView::startHighlighting(Clipboard::DataType::AutomatableModelLink);
}
void FloatModelEditorBase::unlinkAllCommand()
{
	model()->unlinkAllModels();
}
void FloatModelEditorBase::increaseValueCommand()
{
	doCommand<float>(9, model()->value() + model()->range() / 20.0f, model()->value(), true); // setValueCommand
}
void FloatModelEditorBase::decreaseValueCommand()
{
	doCommand<float>(9, model()->value() - model()->range() / 20.0f, model()->value(), true); // setValueCommand
}
void FloatModelEditorBase::setValueCommand(float floatValue)
{
	model()->setValue(floatValue);
}
void FloatModelEditorBase::openInputDialogCommand()
{
	bool ok = false;
	float newValue = 0;

	if (isVolumeKnob())
	{
		auto const initalValue = model()->getRoundedValue() / 100.0;
		auto const initialDbValue = initalValue > 0. ? ampToDbfs(initalValue) : -96;

		newValue = QInputDialog::getDouble(
			this, tr("Set value"),
			tr("Please enter a new value between -96.0 dBFS and 6.0 dBFS:"),
			initialDbValue, -96.0, 6.0, model()->getDigitCount(), &ok);

		if (newValue <= -96.0) { newValue = 0.0f; }
		else { newValue = dbfsToAmp(newValue) * 100.0; }
	}
	else
	{
		newValue = QInputDialog::getDouble(
			this, tr("Set value"),
			tr("Please enter a new value between %1 and %2:")
			.arg(model()->minValue()).arg(model()->maxValue()),
			model()->getRoundedValue(),
			model()->minValue(), model()->maxValue(),
			model()->getDigitCount(), &ok);
	}

	if (ok)
	{
		doCommand<float>(9, newValue, model()->value(), true); // setValueCommand
	}
}
void FloatModelEditorBase::toggleScaleCommand()
{
	doCommand<bool>(13, !model()->isScaleLogarithmic(), model()->isScaleLogarithmic(), true); // setScaleLogarithmicCommand
}
void FloatModelEditorBase::setScaleLogarithmicCommand(bool isLogarithmic)
{
	model()->setScaleLogarithmic(true);
}
void FloatModelEditorBase::setPositionCommand(QPoint p)
{
	const float valueOffset = getValue(p) + m_leftOver;
	const float currentValue = model()->value();
	const float scaledValueOffset = currentValue - model()->scaledValue(model()->inverseScaledValue(currentValue) - valueOffset);
	const auto step = model()->step<float>();
	const float roundedValue = std::round((currentValue - scaledValueOffset) / step) * step;

	if (!approximatelyEqual(roundedValue, currentValue))
	{
		doCommand<float>(9, roundedValue, model()->value(), true); // setValueCommand
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
		this, [this]{ this->doCommand(12); /* toggleScaleCommand */ });
	contextMenu.addSeparator();
	contextMenu.exec(QCursor::pos());
}

void FloatModelEditorBase::dragEnterEvent(QDragEnterEvent * dee)
{
	static std::vector<Clipboard::DataType> acceptedKeys = {
		Clipboard::DataType::FloatValue,
		Clipboard::DataType::AutomatableModelLink
	};
	StringPairDrag::processDragEnterEvent(dee, &acceptedKeys);
}


void FloatModelEditorBase::dropEvent(QDropEvent * de)
{
	bool canAccept = false;
	// TODO this checks the wrong mime data when pasting leading to commands returning
	doCommand<bool*>(3, &canAccept);
	if (canAccept == true)
	{
		de->accept();
	}
	else
	{
		de->ignore();
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

		const QPoint & p = me->pos();
		m_lastMousePos = p;
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
		new StringPairDrag(Clipboard::DataType::FloatValue,
					Clipboard::encodeFloatValue(model()->value()),
							QPixmap(), this);
	}
	else
	{
		FloatModelView::mousePressEvent(me);
	}
}


void FloatModelEditorBase::mouseMoveEvent(QMouseEvent * me)
{
	if (m_buttonPressed && me->pos() != m_lastMousePos)
	{
		// knob position is changed depending on last mouse position
		doCommand<QPoint>(14, me->pos() - m_lastMousePos, me->pos());
		emit sliderMoved(model()->value());
		// original position for next time is current position
		m_lastMousePos = me->pos();
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


void FloatModelEditorBase::enterEvent(QEvent *event)
{
	InteractiveModelView::enterEvent(event);
	showTextFloat(700, 2000);
}


void FloatModelEditorBase::leaveEvent(QEvent *event)
{
	InteractiveModelView::leaveEvent(event);
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
	doCommand(11); // openInputDialogCommand
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
	drawAutoHighlight(&p);
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
	doCommand<float>(9, model()->value() + inc, model()->value()); // setValueCommand

	s_textFloat->setText(displayValue());
	s_textFloat->moveGlobal(this, QPoint(width() + 2, 0));
	s_textFloat->setVisibilityTimeOut(1000);

	emit sliderMoved(model()->value());
}




void FloatModelEditorBase::friendlyUpdate()
{
	if (model())
	{
		bool shouldUpdate = false;
		if (model()->controllerConnection() == nullptr)
		{
			shouldUpdate = true;
		}
		else
		{
			if (model()->controllerConnection()->getController()->frequentUpdates() == false)
			{
				if (Controller::runningFrames() % (256*4) == 0) { shouldUpdate = true; }
			}
		}
		if (shouldUpdate)
		{
			update();
		}
	}
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
