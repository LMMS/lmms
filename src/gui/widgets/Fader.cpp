/*
 * Fader.cpp - fader-widget used in mixer - partly taken from Hydrogen
 *
 * Copyright (c) 2008-2012 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 *
 * http://www.hydrogen-music.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "Fader.h"

#include <QInputDialog>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>  // IWYU pragma: keep

#include "lmms_math.h"
#include "CaptionMenu.h"
#include "ConfigManager.h"
#include "KeyboardShortcuts.h"
#include "SimpleTextFloat.h"

namespace
{
	constexpr auto c_dBScalingExponent = 3.f;
	//! The dbFS amount after which we drop down to -inf dbFS
	constexpr auto c_faderMinDb = -120.f;
}

namespace lmms::gui
{

SimpleTextFloat* Fader::s_textFloat = nullptr;

Fader::Fader(FloatModel* model, const QString& name, QWidget* parent, bool modelIsLinear) :
	QWidget(parent),
	FloatModelView(model, this),
	m_knobSize(embed::logicalSize(m_knob)),
	m_modelIsLinear(modelIsLinear)
{
	if (s_textFloat == nullptr)
	{
		s_textFloat = new SimpleTextFloat;
	}

	setWindowTitle(name);
	// For now resize the widget to the size of the previous background image "fader_background.png" as it was found in the classic and default theme
	constexpr QSize minimumSize(23, 116);
	setMinimumSize(minimumSize);
	resize(minimumSize);
	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	setModel(model);
	setHintText("Volume:", "%");

	m_conversionFactor = 100.0;

	if (model)
	{
		// We currently assume that the model is not changed later on and only connect here once

		// This is for example used to update the tool tip which shows the current value of the fader
		connect(model, &FloatModel::dataChanged, this, &Fader::modelValueChanged);

		// Trigger manually so that the tool tip is initialized correctly
		modelValueChanged();
	}
}


Fader::Fader(FloatModel* model, const QString& name, QWidget* parent, const QPixmap& knob, bool modelIsLinear) :
	Fader(model, name, parent, modelIsLinear)
{
	m_knob = knob;
}

void Fader::adjust(const Qt::KeyboardModifiers & modifiers, AdjustmentDirection direction)
{
	const auto adjustmentDb = determineAdjustmentDelta(modifiers) * (direction == AdjustmentDirection::Down ? -1. : 1.);
	adjustByDecibelDelta(adjustmentDb);
}

void Fader::adjustByDecibelDelta(float value)
{
	adjustModelByDBDelta(value);

	updateTextFloat();
	s_textFloat->showWithTimeout(1000);
}

void Fader::adjustByDialog()
{
	bool ok;

	if (modelIsLinear())
	{
		auto maxDB = ampToDbfs(model()->maxValue());
		const auto currentValue = model()->value() <= 0. ? c_faderMinDb : ampToDbfs(model()->value());

		float enteredValue = QInputDialog::getDouble(this, tr("Set value"),
							tr("Please enter a new value between %1 and %2:").arg(c_faderMinDb).arg(maxDB),
							currentValue, c_faderMinDb, maxDB, model()->getDigitCount(), &ok);

		if (ok)
		{
			model()->setValue(dbfsToAmp(enteredValue));
		}
		return;
	}
	else
	{
		// The model already is in dB
		auto minv = model()->minValue() * m_conversionFactor;
		auto maxv = model()->maxValue() * m_conversionFactor;
		float enteredValue = QInputDialog::getDouble(this, tr("Set value"),
							tr("Please enter a new value between %1 and %2:").arg(minv).arg(maxv),
							model()->getRoundedValue() * m_conversionFactor, minv, maxv, model()->getDigitCount(), &ok);

		if (ok)
		{
			model()->setValue(enteredValue / m_conversionFactor);
		}
	}
}

void Fader::contextMenuEvent(QContextMenuEvent* ev)
{
	CaptionMenu contextMenu(windowTitle());
	addDefaultActions(&contextMenu);
	contextMenu.exec(QCursor::pos());
	ev->accept();
}




void Fader::mouseMoveEvent(QMouseEvent* mouseEvent)
{
	const int localY = mouseEvent->y();

	setVolumeByLocalPixelValue(localY);

	updateTextFloat();

	mouseEvent->accept();
}




void Fader::mousePressEvent(QMouseEvent* mouseEvent)
{
	if (mouseEvent->button() == Qt::LeftButton &&
			!(mouseEvent->modifiers() & KBD_COPY_MODIFIER))
	{
		AutomatableModel* thisModel = model();
		if (thisModel)
		{
			thisModel->addJournalCheckPoint();
			thisModel->saveJournallingState(false);
		}

		const int localY = mouseEvent->y();
		const auto knobLowerPosY = calculateKnobPosYFromModel();
		const auto knobUpperPosY = knobLowerPosY - m_knobSize.height();

		const auto clickedOnKnob = localY >= knobUpperPosY && localY <= knobLowerPosY;

		if (clickedOnKnob)
		{
			// If the users clicked on the knob we want to compensate for the offset to the center line
			// of the knob when dealing with mouse move events.
			// This will make it feel like the users have grabbed the knob where they clicked.
			const auto knobCenterPos = knobLowerPosY - (m_knobSize.height() / 2);
			m_knobCenterOffset = localY - knobCenterPos;

			// In this case we also will not call setVolumeByLocalPixelValue, i.e. we do not make any immediate
			// changes. This should only happen if the users actually move the mouse while grabbing the knob.
			// This makes the knobs less "jumpy".
		}
		else
		{
			// If the users did not click on the knob then we assume that the fader knob's center should move to
			// the position of the click. We do not compensate for any offset.
			m_knobCenterOffset = 0;

			setVolumeByLocalPixelValue(localY);
		}

		updateTextFloat();
		s_textFloat->show();

		mouseEvent->accept();
	}
	else
	{
		AutomatableModelView::mousePressEvent(mouseEvent);
	}
}



void Fader::mouseDoubleClickEvent(QMouseEvent* mouseEvent)
{
	adjustByDialog();

	mouseEvent->accept();
}



void Fader::mouseReleaseEvent(QMouseEvent* mouseEvent)
{
	if (mouseEvent && mouseEvent->button() == Qt::LeftButton)
	{
		AutomatableModel* thisModel = model();
		if (thisModel)
		{
			thisModel->restoreJournallingState();
		}
	}

	// Always reset the offset to 0 regardless of which mouse button is pressed
	m_knobCenterOffset = 0;

	s_textFloat->hide();
}


void Fader::wheelEvent (QWheelEvent* ev)
{
	const int direction = (ev->angleDelta().y() > 0 ? 1 : -1) * (ev->inverted() ? -1 : 1);

	const float increment = determineAdjustmentDelta(ev->modifiers()) * direction;

	adjustByDecibelDelta(increment);

	ev->accept();
}

float Fader::determineAdjustmentDelta(const Qt::KeyboardModifiers & modifiers) const
{
	if (modifiers == Qt::ShiftModifier)
	{
		// The shift is intended to go through the values in very coarse steps as in:
		// "Shift into overdrive"
		return 3.f;
	}
	else if (modifiers == Qt::ControlModifier)
	{
		// The control key gives more control, i.e. it enables more fine-grained adjustments
		return 0.1f;
	}
	else if (modifiers & Qt::AltModifier)
	{
		// Work around a Qt bug in conjunction with the scroll wheel and the Alt key
		return 0.f;
	}

	return 1.f;
}

void Fader::adjustModelByDBDelta(float value)
{
	if (modelIsLinear())
	{
		const auto modelValue = model()->value();

		if (modelValue <= 0.)
		{
			// We are at -inf dB. Do nothing if we user wishes to decrease.
			if (value > 0)
			{
				// Otherwise set the model to the minimum value supported by the fader.
				model()->setValue(dbfsToAmp(c_faderMinDb));
			}
		}
		else
		{
			// We can safely compute the dB value as the value is greater than 0
			const auto valueInDB = ampToDbfs(modelValue);

			const auto adjustedValue = valueInDB + value;

			model()->setValue(adjustedValue < c_faderMinDb ? 0. : dbfsToAmp(adjustedValue));
		}
	}
	else
	{
		const auto adjustedValue = std::clamp(model()->value() + value, model()->minValue(), model()->maxValue());

		model()->setValue(adjustedValue);
	}
}

int Fader::calculateKnobPosYFromModel() const
{
	auto* m = model();

	auto const minV = m->minValue();
	auto const maxV = m->maxValue();
	auto const value = m->value();

	if (modelIsLinear())
	{
		// This method calculates the pixel position where the lower end of
		// the fader knob should be for the amplification value in the model.
		//
		// The following assumes that the model describes an amplification,
		// i.e. that values are in [0, max] and that 1 is unity, i.e. 0 dbFS.

		auto const distanceToMin = value - minV;

		// Prevent dbFS calculations with zero or negative values
		if (distanceToMin <= 0)
		{
			return height();
		}
		else
		{
			// Make sure that we do not get values less that the minimum fader dbFS
			// for the calculations that will follow.
			auto const actualDb = std::max(c_faderMinDb, ampToDbfs(value));

			const auto scaledRatio = computeScaledRatio(actualDb);

			// This returns results between:
			// * m_knobSize.height()  for a ratio of 1
			// * height()         for a ratio of 0
			return height() - (height() - m_knobSize.height()) * scaledRatio;
		}
	}
	else
	{
		// The model is in dB so we just show that in a linear fashion
		
		auto const clampedValue = std::clamp(value, minV, maxV);

		auto const ratio = (clampedValue - minV) / (maxV - minV);

		// This returns results between:
		// * m_knobSize.height()  for a ratio of 1
		// * height()         for a ratio of 0
		return height() - (height() - m_knobSize.height()) * ratio;
	}
}


void Fader::setVolumeByLocalPixelValue(int y)
{
	auto* m = model();

	// Compensate the offset where users have actually clicked
	y -= m_knobCenterOffset;

	// The y parameter gives us where the mouse click went.
	// Assume that the middle of the fader should go there.
	int const lowerFaderKnob = y + (m_knobSize.height() / 2);

	// In some cases we need the clamped lower position of the fader knob so we can ensure
	// that we only set allowed values in the model range.
	int const clampedLowerFaderKnob = std::clamp(lowerFaderKnob, m_knobSize.height(), height());

	if (modelIsLinear())
	{
		if (lowerFaderKnob >= height())
		{
			// Check the non-clamped value because otherwise we wouldn't be able to set -inf dB!
			model()->setValue(0);
		}
		else
		{
			// We are in the case where we set a value that's different from -inf dB so we use the clamped value
			// of the lower knob position so that we only set allowed values in the model range.

			// First map the lower knob position to [0, 1] so that we can apply some curve mapping, e.g.
			// square, cube, etc.
			LinearMap<float> knobMap(float(m_knobSize.height()), 1., float(height()), 0.);

			// Apply the inverse of what is done in calculateKnobPosYFromModel
			auto const knobPos = std::pow(knobMap.map(clampedLowerFaderKnob), 1./c_dBScalingExponent);

			float const maxDb = ampToDbfs(m->maxValue());

			LinearMap<float> dbMap(1., maxDb, 0., c_faderMinDb);

			float const dbValue = dbMap.map(knobPos);

			// Pull everything that's quieter than the minimum fader dbFS value down to 0 amplification.
			// This should not happen due to the steps above but let's be sure.
			// Otherwise compute the amplification value from the mapped dbFS value but make sure that we
			// do not exceed the maximum dbValue of the model
			float ampValue = dbValue < c_faderMinDb ? 0. : dbfsToAmp(std::min(maxDb, dbValue));

			model()->setValue(ampValue);
		}
	}
	else
	{
		LinearMap<float> valueMap(float(m_knobSize.height()), model()->maxValue(), float(height()), model()->minValue());

		model()->setValue(valueMap.map(clampedLowerFaderKnob));
	}
}

float Fader::computeScaledRatio(float dBValue) const
{
	const auto maxDb = ampToDbfs(model()->maxValue());

	const auto ratio = (dBValue - c_faderMinDb) / (maxDb - c_faderMinDb);

	return std::pow(ratio, c_dBScalingExponent);
}


///
/// Set peak value (0.0 .. 1.0)
///
void Fader::setPeak(float fPeak, float& targetPeak, float& persistentPeak, QElapsedTimer& lastPeakTimer)
{
	if (targetPeak != fPeak)
	{
		targetPeak = fPeak;
		if (targetPeak >= persistentPeak)
		{
			persistentPeak = targetPeak;
			lastPeakTimer.restart();
			emit peakChanged(persistentPeak);
		}
		update();
	}

	if (persistentPeak > 0 && lastPeakTimer.elapsed() > 1500)
	{
		persistentPeak = qMax<float>(0, persistentPeak-0.05);
		emit peakChanged(persistentPeak);
		update();
	}
}



void Fader::setPeak_L(float fPeak)
{
	setPeak(fPeak, m_fPeakValue_L, m_persistentPeak_L, m_lastPeakTimer_L);
}



void Fader::setPeak_R(float fPeak)
{
	setPeak(fPeak, m_fPeakValue_R, m_persistentPeak_R, m_lastPeakTimer_R);
}



// update tooltip showing value and adjust position while changing fader value
void Fader::updateTextFloat()
{
	if (m_conversionFactor == 100.0)
	{
		s_textFloat->setText(getModelValueAsDbString());
	}
	else
	{
		s_textFloat->setText(m_description + " " + QString("%1 ").arg(model()->value() * m_conversionFactor) + " " + m_unit);
	}

	s_textFloat->moveGlobal(this, QPoint(width() + 2, calculateKnobPosYFromModel() - s_textFloat->height() / 2));
}

void Fader::modelValueChanged()
{
	setToolTip(getModelValueAsDbString());
}

QString Fader::getModelValueAsDbString() const
{
	QString label(tr("Volume: %1 dB"));
	// Check that the pointer isn't dangling, which can happen if the
	// model was dropped in order to load a new project from an existing one.
	auto* newModel = model();
	if (!newModel)
	{
		// model() was a dangling pointer, so return a sane default value.
		return label.arg(tr("-inf"));
	}
	const auto value = newModel->value();

	if (modelIsLinear())
	{
		if (value <= 0.)
		{
			return label.arg(tr("-inf"));
		}
		else
		{
			return label.arg(ampToDbfs(value), 3, 'f', 2);
		}
	}
	else
	{
		return label.arg(value, 3, 'f', 2);
	}
}

void Fader::paintEvent(QPaintEvent* ev)
{
	QPainter painter(this);

	// Draw the levels with peaks
	paintLevels(ev, painter, !m_levelsDisplayedInDBFS);

	if (ConfigManager::inst()->value( "ui", "showfaderticks" ).toInt() && modelIsLinear())
	{
		paintFaderTicks(painter);
	}

	// Draw the knob
	painter.drawPixmap((width() - m_knobSize.width()) / 2, calculateKnobPosYFromModel() - m_knobSize.height(), m_knob);
}

void Fader::paintLevels(QPaintEvent* ev, QPainter& painter, bool linear)
{
	const auto mapper = linear
		? +[](float value) -> float { return value; }
		: +[](float value) -> float { return ampToDbfs(qMax(0.0001f, value)); };

	const float mappedMinPeak = mapper(m_fMinPeak);
	const float mappedMaxPeak = mapper(m_fMaxPeak);
	const float mappedPeakL = mapper(m_fPeakValue_L);
	const float mappedPeakR = mapper(m_fPeakValue_R);
	const float mappedPersistentPeakL = mapper(m_persistentPeak_L);
	const float mappedPersistentPeakR = mapper(m_persistentPeak_R);
	const float mappedUnity = mapper(1.f);

	painter.save();

	const QRect baseRect = rect();

	const int height = baseRect.height();

	const int margin = 1;
	const int distanceBetweenMeters = 2;

	const int numberOfMeters = 2;

	// Compute the width of a single meter by removing the margins and the space between meters
	const int leftAndRightMargin = 2 * margin;
	const int pixelsBetweenAllMeters = distanceBetweenMeters * (numberOfMeters - 1);
	const int remainingPixelsForMeters = baseRect.width() - leftAndRightMargin - pixelsBetweenAllMeters;
	const int meterWidth = remainingPixelsForMeters / numberOfMeters;

	QRect leftMeterOutlineRect(margin, margin, meterWidth, height - 2 * margin);
	QRect rightMeterOutlineRect(baseRect.width() - margin - meterWidth, margin, meterWidth, height - 2 * margin);

	QMargins removedMargins(1, 1, 1, 1);
	QRect leftMeterRect = leftMeterOutlineRect.marginsRemoved(removedMargins);
	QRect rightMeterRect = rightMeterOutlineRect.marginsRemoved(removedMargins);

	QPainterPath path;
	qreal radius = 2;
	path.addRoundedRect(leftMeterOutlineRect, radius, radius);
	path.addRoundedRect(rightMeterOutlineRect, radius, radius);
	painter.fillPath(path, Qt::black);

	// Now clip everything to the paths of the meters
	painter.setClipPath(path);

	// This linear map performs the following mapping:
	// Value (dbFS or linear) -> window coordinates of the widget
	// It is for example used to determine the height of peaks, markers and to define the gradient for the levels
	const LinearMap<float> valuesToWindowCoordinates(mappedMaxPeak, leftMeterRect.y(), mappedMinPeak, leftMeterRect.y() + leftMeterRect.height());

	// This lambda takes a value (in dbFS or linear) and a rectangle and computes a rectangle
	// that represent the value within the rectangle. It is for example used to compute the unity indicators.
	const auto computeLevelMarkerRect = [&valuesToWindowCoordinates](const QRect& rect, float peak) -> QRect
	{
		return QRect(rect.x(), valuesToWindowCoordinates.map(peak), rect.width(), 1);
	};

	// This lambda takes a peak value (in dbFS or linear) and a rectangle and computes a rectangle
	// that represent the peak value within the rectangle. It's used to compute the peak indicators
	// which "dance" on top of the level meters.
	const auto computePeakRect = [&valuesToWindowCoordinates](const QRect& rect, float peak) -> QRect
	{
		return QRect(rect.x(), valuesToWindowCoordinates.map(peak), rect.width(), 1);
	};

	// This lambda takes a peak value (in dbFS or linear) and a rectangle and returns an adjusted copy of the
	// rectangle that represents the peak value. It is used to compute the level meters themselves.
	const auto computeLevelRect = [&valuesToWindowCoordinates](const QRect& rect, float peak) -> QRect
	{
		QRect result(rect);
		result.setTop(valuesToWindowCoordinates.map(peak));

		return result;
	};

	// Draw left and right level markers for the unity lines (0 dbFS, 1.0 amplitude)
	if (getRenderUnityLine())
	{
		const auto unityRectL = computeLevelMarkerRect(leftMeterRect, mappedUnity);
		painter.fillRect(unityRectL, m_unityMarker);

		const auto unityRectR = computeLevelMarkerRect(rightMeterRect, mappedUnity);
		painter.fillRect(unityRectR, m_unityMarker);
	}

	// These values define where the gradient changes values, i.e. the ranges
	// for clipping, warning and ok.
	// Please ensure that "clip starts" is the maximum value and that "ok ends"
	// is the minimum value and that all other values lie inbetween. Otherwise
	// there will be warnings when the gradient is defined.
	const float mappedClipStarts = mapper(dbfsToAmp(0.f));
	const float mappedWarnEnd = mapper(dbfsToAmp(-0.01f));
	const float mappedWarnStart = mapper(dbfsToAmp(-6.f));
	const float mappedOkEnd = mapper(dbfsToAmp(-12.f));

	// Prepare the gradient for the meters
	//
	// The idea is the following. We want to be able to render arbitrary ranges of min and max values.
	// Therefore we first compute the start and end point of the gradient in window coordinates.
	// The gradient is assumed to start with the clip color and to end with the ok color with warning values in between.
	// We know the min and max peaks that map to a rectangle where we draw the levels. We can use the values of the min and max peaks
	// as well as the Y-coordinates of the rectangle to compute a map which will give us the coordinates of the value where the clipping
	// starts and where the ok area end. These coordinates are used to initialize the gradient. Please note that the gradient might thus
	// extend the rectangle into which we paint.
	float clipStartYCoord = valuesToWindowCoordinates.map(mappedClipStarts);
	float okEndYCoord = valuesToWindowCoordinates.map(mappedOkEnd);

	QLinearGradient linearGrad(0, clipStartYCoord, 0, okEndYCoord);

	// We already know for the gradient that the clip color will be at 0 and that the ok color is at 1.
	// What's left to do is to map the inbetween values into the interval [0,1].
	const LinearMap<float> mapBetweenClipAndOk(mappedClipStarts, 0.f, mappedOkEnd, 1.f);

	linearGrad.setColorAt(0, m_peakClip);
	linearGrad.setColorAt(mapBetweenClipAndOk.map(mappedWarnEnd), m_peakWarn);
	linearGrad.setColorAt(mapBetweenClipAndOk.map(mappedWarnStart), m_peakWarn);
	linearGrad.setColorAt(1, m_peakOk);

	// Draw left levels
	if (mappedPeakL > mappedMinPeak)
	{
		QPainterPath leftMeterPath;
		leftMeterPath.addRoundedRect(computeLevelRect(leftMeterRect, mappedPeakL), radius, radius);
		painter.fillPath(leftMeterPath, linearGrad);
	}

	// Draw left peaks
	if (mappedPersistentPeakL > mappedMinPeak)
	{
		const auto peakRectL = computePeakRect(leftMeterRect, mappedPersistentPeakL);
		painter.fillRect(peakRectL, linearGrad);
	}

	// Draw right levels
	if (mappedPeakR > mappedMinPeak)
	{
		QPainterPath rightMeterPath;
		rightMeterPath.addRoundedRect(computeLevelRect(rightMeterRect, mappedPeakR), radius, radius);
		painter.fillPath(rightMeterPath, linearGrad);
	}

	// Draw right peaks
	if (mappedPersistentPeakR > mappedMinPeak)
	{
		const auto peakRectR = computePeakRect(rightMeterRect, mappedPersistentPeakR);
		painter.fillRect(peakRectR, linearGrad);
	}

	painter.restore();
}

void Fader::paintFaderTicks(QPainter& painter)
{
	painter.save();

	const QPen zeroPen(QColor(255, 255, 255, 216), 2.5);
	const QPen nonZeroPen(QColor(255, 255, 255, 128), 1.);

	// We use the maximum dB value of the model to calculate the nearest multiple
	// of the step size that we use to paint the ticks so that we know the start point.
	// This code will paint ticks with steps that are defined by the step size around
	// the 0 dB marker.
	const auto maxDB = ampToDbfs(model()->maxValue());
	const auto stepSize = 6.f;
	const auto startValue = std::floor(maxDB / stepSize) * stepSize;

	for (float i = startValue; i >= c_faderMinDb; i-= stepSize)
	{
		const auto scaledRatio = computeScaledRatio(i);
		const auto maxHeight = height() - (height() - m_knobSize.height()) * scaledRatio - (m_knobSize.height() / 2);

		if (approximatelyEqual(i, 0.))
		{
			painter.setPen(zeroPen);
		}
		else
		{
			painter.setPen(nonZeroPen);
		}

		painter.drawLine(QPointF(0, maxHeight), QPointF(1, maxHeight));
		painter.drawLine(QPointF(width() - 1, maxHeight), QPointF(width(), maxHeight));
	}

	painter.restore();
}

} // namespace lmms::gui
