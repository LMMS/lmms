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
#include <QPainterPath>

#include "lmms_math.h"
#include "embed.h"
#include "CaptionMenu.h"
#include "ConfigManager.h"
#include "KeyboardShortcuts.h"
#include "SimpleTextFloat.h"

namespace lmms::gui
{

SimpleTextFloat* Fader::s_textFloat = nullptr;

Fader::Fader(FloatModel* model, const QString& name, QWidget* parent) :
	QWidget(parent),
	FloatModelView(model, this)
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
}


Fader::Fader(FloatModel* model, const QString& name, QWidget* parent, const QPixmap& knob) :
	Fader(model, name, parent)
{
	m_knob = knob;
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
	if (m_moveStartPoint >= 0)
	{
		int dy = m_moveStartPoint - mouseEvent->globalY();

		float delta = dy * (model()->maxValue() - model()->minValue()) / (float)(height() - (m_knob).height());

		const auto step = model()->step<float>();
		float newValue = static_cast<float>(static_cast<int>((m_startValue + delta) / step + 0.5)) * step;
		model()->setValue(newValue);

		updateTextFloat();
	}
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

		if (mouseEvent->y() >= knobPosY() - (m_knob).height() && mouseEvent->y() < knobPosY())
		{
			updateTextFloat();
			s_textFloat->show();

			m_moveStartPoint = mouseEvent->globalY();
			m_startValue = model()->value();

			mouseEvent->accept();
		}
		else
		{
			m_moveStartPoint = -1;
		}
	}
	else
	{
		AutomatableModelView::mousePressEvent(mouseEvent);
	}
}



void Fader::mouseDoubleClickEvent(QMouseEvent* mouseEvent)
{
	bool ok;
	// TODO: dbFS handling
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

	s_textFloat->hide();
}


void Fader::wheelEvent (QWheelEvent* ev)
{
	ev->accept();
	const int direction = (ev->angleDelta().y() > 0 ? 1 : -1) * (ev->inverted() ? -1 : 1);

	model()->incValue(direction);
	updateTextFloat();
	s_textFloat->setVisibilityTimeOut(1000);
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
	if (ConfigManager::inst()->value("app", "displaydbfs").toInt() && m_conversionFactor == 100.0)
	{
		QString label(tr("Volume: %1 dBFS"));

		auto const modelValue = model()->value();
		if (modelValue <= 0.)
		{
			s_textFloat->setText(label.arg("-∞"));
		}
		else
		{
			s_textFloat->setText(label.arg(ampToDbfs(modelValue), 3, 'f', 2));
		}
	}
	else
	{
		s_textFloat->setText(m_description + " " + QString("%1 ").arg(model()->value() * m_conversionFactor) + " " + m_unit);
	}

	s_textFloat->moveGlobal(this, QPoint(width() + 2, knobPosY() - s_textFloat->height() / 2));
}


void Fader::paintEvent(QPaintEvent* ev)
{
	QPainter painter(this);

	// Draw the levels with peaks
	paintLevels(ev, painter, !m_levelsDisplayedInDBFS);

	// Draw the knob
	painter.drawPixmap((width() - m_knob.width()) / 2, knobPosY() - m_knob.height(), m_knob);
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

} // namespace lmms::gui
