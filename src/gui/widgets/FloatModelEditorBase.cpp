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

QString FloatModelEditorBase::s_shortcutMessage = "";

SimpleTextFloat * FloatModelEditorBase::s_textFloat = nullptr;

FloatModelEditorBase::FloatModelEditorBase(DirectionOfManipulation directionOfManipulation, QWidget * parent, const QString & name) :
	InteractiveModelView(parent, InteractiveModelView::getTypeId<FloatModelEditorBase>()),
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
	// inherited from `InteractiveModelView`
	addActions(m_actionArray);

	if (s_textFloat == nullptr)
	{
		s_textFloat = new SimpleTextFloat;
	}

	setWindowTitle(name);

	setFocusPolicy(Qt::ClickFocus);

	doConnections();
}

void FloatModelEditorBase::addActions(std::vector<ActionStruct>& targetList)
{
	// NOTE: ONLY USE `doAction()` IN QT FUNCTIONS OR ACTION FUNCTIONS
	// actions are meant to be triggered by users, triggering them from an internal function could lead to bad journalling
	auto copyValueAc = new QAction(tr("Copy value"), this);
	auto pasteNoReturnAc = new QAction(tr("Paste value"), this);
	auto pasteAc = new QAction(tr("Paste value (with return)"), this);
	auto linkAc = new QAction(tr("Link widget to"), this);
	auto unlinkAc = new QAction(tr("Unlink widget from"), this);
	auto getLinkAc = new QAction(tr("Link widget"), this);
	auto unlinkAllAc = new QAction(tr("Unlink from all"), this);
	auto increaseValueAc = new QAction(tr("Increase value"), this);
	auto decreaseValueAc = new QAction(tr("Decrease value"), this);
	auto addValueAc = new QAction(tr("Add value"), this);
	auto subtractValueAc = new QAction(tr("Subtract value"), this);
	auto openInputDialogAc = new QAction(tr("Open input dialog"), this);
	auto setScaleLinearAc = new QAction(tr("Set scale linear"), this);
	auto setScaleLogarithmicAc = new QAction(tr("Set scale logarithmic"), this);

	targetList =
	{
		ActionStruct(1, *copyValueAc, nullptr, true, Clipboard::DataType::Any),
		ActionStruct(2, *pasteNoReturnAc, nullptr, true, Clipboard::DataType::FloatValue),
		ActionStruct(3, *getLinkAc, nullptr, true, Clipboard::DataType::Any),
		ActionStruct(4, *increaseValueAc, nullptr, true, Clipboard::DataType::Any),
		ActionStruct(5, *decreaseValueAc, nullptr, true, Clipboard::DataType::Any),
		ActionStruct(6, *unlinkAllAc, nullptr, true, Clipboard::DataType::Any),
		ActionStruct(7, *setScaleLinearAc, setScaleLogarithmicAc, true, Clipboard::DataType::Any),
		ActionStruct(8, *setScaleLogarithmicAc, setScaleLinearAc, true, Clipboard::DataType::Any),
		ActionStruct(9, *openInputDialogAc, nullptr, true, Clipboard::DataType::Any),
		ActionStruct(10, *addValueAc, subtractValueAc, true, Clipboard::DataType::Any),
		ActionStruct(11, *subtractValueAc, addValueAc, true, Clipboard::DataType::Any),
		ActionStruct(12, *linkAc, unlinkAc, true, Clipboard::DataType::Any),
		ActionStruct(13, *unlinkAc, linkAc, true, Clipboard::DataType::Any),
		ActionStruct(14, *pasteAc, nullptr, true, Clipboard::DataType::FloatValue)
	};

	connect(copyValueAc, &QAction::triggered, this, &FloatModelEditorBase::copyValueAction);
	connect(pasteNoReturnAc, &QAction::triggered, this, &FloatModelEditorBase::pasteNoReturnAction);
	connect(pasteAc, &QAction::triggered, this, [this, &targetList]{ pasteAction(*targetList[getIndexFromId(14)].getData()->getValue<bool*>()); });
	connect(linkAc, &QAction::triggered, this, [this, &targetList]{ linkAction(*targetList[getIndexFromId(12)].getData()->getValue<int>()); });
	connect(unlinkAc, &QAction::triggered, this, [this, &targetList]{ unlinkAction(*targetList[getIndexFromId(13)].getData()->getValue<int>()); });
	connect(getLinkAc, &QAction::triggered, this, &FloatModelEditorBase::getLinkAction);
	connect(unlinkAllAc, &QAction::triggered, this, &FloatModelEditorBase::unlinkAllAction);
	connect(increaseValueAc, &QAction::triggered, this, &FloatModelEditorBase::increaseValueAction);
	connect(decreaseValueAc, &QAction::triggered, this, &FloatModelEditorBase::decreaseValueAction);
	connect(addValueAc, &QAction::triggered, this, [this, &targetList]{ addValueAction(*targetList[getIndexFromId(10)].getData()->getValue<float>()); });
	connect(subtractValueAc, &QAction::triggered, this, [this, &targetList]{ subtractValueAction(*targetList[getIndexFromId(11)].getData()->getValue<float>()); });
	connect(openInputDialogAc, &QAction::triggered, this, &FloatModelEditorBase::openInputDialogAction);
	connect(setScaleLinearAc, &QAction::triggered, this, &FloatModelEditorBase::setScaleLinearAction);
	connect(setScaleLogarithmicAc, &QAction::triggered, this, &FloatModelEditorBase::setScaleLogarithmicAction);

	targetList[0].setShortcut(Qt::Key_C, Qt::ControlModifier, 0, false);
	targetList[1].setShortcut(Qt::Key_V, Qt::ControlModifier, 0, false);
	targetList[2].setShortcut(Qt::Key_C, Qt::ControlModifier, 1, false);
	targetList[3].setShortcut(Qt::Key_E, Qt::ShiftModifier, 0, false);
	targetList[4].setShortcut(Qt::Key_Q, Qt::ShiftModifier, 0, false);
	targetList[5].setShortcut(Qt::Key_U, Qt::ControlModifier, 0, false);

	targetList[getIndexFromId(2)].addAcceptedDataType(Clipboard::DataType::AutomatableModelLink);
	targetList[getIndexFromId(14)].addAcceptedDataType(Clipboard::DataType::AutomatableModelLink);

	if (s_shortcutMessage.size() <= 0)
	{
		s_shortcutMessage = InteractiveModelView::buildShortcutMessage(targetList);
	}
}

void FloatModelEditorBase::copyValueAction()
{
	Clipboard::copyStringPair(Clipboard::DataType::FloatValue, Clipboard::encodeFloatValue(model()->value() * getConversionFactor()));
	InteractiveModelView::startHighlighting(Clipboard::DataType::FloatValue);
}
void FloatModelEditorBase::pasteNoReturnAction()
{
	FloatModelEditorBase::pasteAction(nullptr);
}
void FloatModelEditorBase::pasteAction(bool* isSuccessful)
{
	Clipboard::DataType type = Clipboard::decodeKey(Clipboard::getMimeData());
	QString value = Clipboard::decodeValue(Clipboard::getMimeData());

	bool shouldAccept = false;
	if (type == Clipboard::DataType::FloatValue)
	{
		float increasedValue = LocaleHelper::toFloat(value) - model()->value();
		doAction(10, GuiActionIO(increasedValue), true);
		shouldAccept = true;
	}
	else if (type == Clipboard::DataType::AutomatableModelLink)
	{
		doAction(12, GuiActionIO(value.toInt()), true);
		shouldAccept = true;
	}
	if (shouldAccept)
	{
		if (isSuccessful != nullptr) { *isSuccessful = true; }
		InteractiveModelView::stopHighlighting();
	}
}
void FloatModelEditorBase::linkAction(int id)
{
	auto mod = dynamic_cast<AutomatableModel*>(Engine::projectJournal()->journallingObject(id));
	if (mod != nullptr)
	{
		AutomatableModel::linkModels(model(), mod);
		mod->setValue(model()->value());
	}
}
void FloatModelEditorBase::unlinkAction(int id)
{
	auto mod = dynamic_cast<AutomatableModel*>(Engine::projectJournal()->journallingObject(id));
	if (mod != nullptr)
	{
		AutomatableModel::unlinkModels(model(), mod);
	}
}
void FloatModelEditorBase::getLinkAction()
{
	Clipboard::copyStringPair(Clipboard::DataType::AutomatableModelLink, Clipboard::encodeAutomatableModelLink(*model()));
	InteractiveModelView::startHighlighting(Clipboard::DataType::AutomatableModelLink);
}
void FloatModelEditorBase::unlinkAllAction()
{
	model()->unlinkAllModels();
}
void FloatModelEditorBase::increaseValueAction()
{
	doAction(10, GuiActionIO(model()->range() / 20.0f), true);
}
void FloatModelEditorBase::decreaseValueAction()
{
	doAction(10, GuiActionIO(-model()->range() / 20.0f), true);
}
void FloatModelEditorBase::addValueAction(float floatValue)
{
	model()->setValue(model()->value() + floatValue);
}
void FloatModelEditorBase::subtractValueAction(float floatValue)
{
	model()->setValue(model()->value() - floatValue);
}
void FloatModelEditorBase::openInputDialogAction()
{
}
void FloatModelEditorBase::setScaleLinearAction()
{
}
void FloatModelEditorBase::setScaleLogarithmicAction()
{
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
	static std::vector<Clipboard::DataType> acceptedKeys = {
		Clipboard::DataType::FloatValue,
		Clipboard::DataType::AutomatableModelLink
	};
	StringPairDrag::processDragEnterEvent(dee, &acceptedKeys);
}


void FloatModelEditorBase::dropEvent(QDropEvent * de)
{
	bool canAccept = false;
	doAction(14, GuiActionIO(&canAccept));
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
		setPosition(me->pos() - m_lastMousePos);
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
	model()->incValue(inc);

	s_textFloat->setText(displayValue());
	s_textFloat->moveGlobal(this, QPoint(width() + 2, 0));
	s_textFloat->setVisibilityTimeOut(1000);

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
			tr("Please enter a new value between "
					"-96.0 dBFS and 6.0 dBFS:"),
				initialDbValue, -96.0, 6.0, model()->getDigitCount(), &ok);

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
