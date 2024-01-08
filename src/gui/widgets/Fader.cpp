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
#include "SimpleTextFloat.h"

namespace lmms::gui
{

SimpleTextFloat * Fader::s_textFloat = nullptr;

Fader::Fader(FloatModel* model, const QString& name, QWidget* parent) :
	QWidget(parent),
	FloatModelView(model, this),
	m_fPeakValue_L(0.0),
	m_fPeakValue_R(0.0),
	m_persistentPeak_L(0.0),
	m_persistentPeak_R(0.0),
	m_fMinPeak(dbfsToAmp(-42)),
	m_fMaxPeak(dbfsToAmp(9)),
	m_knob(embed::getIconPixmap("fader_knob")),
	m_levelsDisplayedInDBFS(true),
	m_moveStartPoint(-1),
	m_startValue(0),
	m_peakOk(10, 212, 92),
	m_peakClip(193, 32, 56),
	m_peakWarn(214, 236, 82),
	m_unityMarker(63, 63, 63, 255),
	m_renderUnityLine(true)
{
	if( s_textFloat == nullptr )
	{
		s_textFloat = new SimpleTextFloat;
	}

	setWindowTitle( name );
	setAttribute( Qt::WA_OpaquePaintEvent, false );
	// For now resize the widget to the size of the previous background image "fader_background.png" as it was found in the classic and default theme
	QSize minimumSize(23, 116);
	setMinimumSize(minimumSize);
	resize(minimumSize);
	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	setModel( model );
	setHintText( "Volume:","%");

	m_conversionFactor = 100.0;
}


Fader::Fader(FloatModel* model, const QString& name, QWidget* parent, const QPixmap& knob) :
	Fader(model, name, parent)
{
	m_knob = knob;
}


void Fader::contextMenuEvent( QContextMenuEvent * _ev )
{
	CaptionMenu contextMenu( windowTitle() );
	addDefaultActions( &contextMenu );
	contextMenu.exec( QCursor::pos() );
	_ev->accept();
}




void Fader::mouseMoveEvent( QMouseEvent *mouseEvent )
{
	if( m_moveStartPoint >= 0 )
	{
		int dy = m_moveStartPoint - mouseEvent->globalY();

		float delta = dy * (model()->maxValue() - model()->minValue()) / (float)(height() - (m_knob).height());

		const auto step = model()->step<float>();
		float newValue = static_cast<float>( static_cast<int>( ( m_startValue + delta ) / step + 0.5 ) ) * step;
		model()->setValue( newValue );

		updateTextFloat();
	}
}




void Fader::mousePressEvent( QMouseEvent* mouseEvent )
{
	if( mouseEvent->button() == Qt::LeftButton &&
			! ( mouseEvent->modifiers() & Qt::ControlModifier ) )
	{
		AutomatableModel *thisModel = model();
		if( thisModel )
		{
			thisModel->addJournalCheckPoint();
			thisModel->saveJournallingState( false );
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
		AutomatableModelView::mousePressEvent( mouseEvent );
	}
}



void Fader::mouseDoubleClickEvent( QMouseEvent* mouseEvent )
{
	bool ok;
	float newValue;
	// TODO: dbV handling
	newValue = QInputDialog::getDouble( this, tr( "Set value" ),
				tr( "Please enter a new value between %1 and %2:" ).
						arg( model()->minValue() * m_conversionFactor ).
						arg( model()->maxValue() * m_conversionFactor ),
					model()->getRoundedValue() * m_conversionFactor,
					model()->minValue() * m_conversionFactor,
					model()->maxValue() * m_conversionFactor, model()->getDigitCount(), &ok ) / m_conversionFactor;

	if( ok )
	{
		model()->setValue( newValue );
	}
}



void Fader::mouseReleaseEvent( QMouseEvent * mouseEvent )
{
	if( mouseEvent && mouseEvent->button() == Qt::LeftButton )
	{
		AutomatableModel *thisModel = model();
		if( thisModel )
		{
			thisModel->restoreJournallingState();
		}
	}

	s_textFloat->hide();
}


void Fader::wheelEvent ( QWheelEvent *ev )
{
	ev->accept();

	if (ev->angleDelta().y() > 0)
	{
		model()->incValue( 1 );
	}
	else
	{
		model()->incValue( -1 );
	}
	updateTextFloat();
	s_textFloat->setVisibilityTimeOut( 1000 );
}



///
/// Set peak value (0.0 .. 1.0)
///
void Fader::setPeak( float fPeak, float &targetPeak, float &persistentPeak, QElapsedTimer &lastPeakTimer )
{
	fPeak = std::clamp(fPeak, m_fMinPeak, m_fMaxPeak);

	if (targetPeak != fPeak)
	{
		targetPeak = fPeak;
		if (targetPeak >= persistentPeak)
		{
			persistentPeak = targetPeak;
			lastPeakTimer.restart();
		}
		update();
	}

	if (persistentPeak > 0 && lastPeakTimer.elapsed() > 1500)
	{
		persistentPeak = qMax<float>( 0, persistentPeak-0.05 );
		update();
	}
}



void Fader::setPeak_L( float fPeak )
{
	setPeak( fPeak, m_fPeakValue_L, m_persistentPeak_L, m_lastPeakTimer_L );
}



void Fader::setPeak_R( float fPeak )
{
	setPeak( fPeak, m_fPeakValue_R, m_persistentPeak_R, m_lastPeakTimer_R );
}



// update tooltip showing value and adjust position while changing fader value
void Fader::updateTextFloat()
{
	if( ConfigManager::inst()->value( "app", "displaydbfs" ).toInt() && m_conversionFactor == 100.0 )
	{
		s_textFloat->setText( QString("Volume: %1 dBFS").
				arg( ampToDbfs( model()->value() ), 3, 'f', 2 ) );
	}
	else
	{
		s_textFloat->setText( m_description + " " + QString("%1 ").arg( model()->value() * m_conversionFactor ) + " " + m_unit );
	}

	s_textFloat->moveGlobal(this, QPoint(width() + 2, knobPosY() - s_textFloat->height() / 2));
}


void Fader::paintEvent( QPaintEvent * ev)
{
	QPainter painter(this);

	// Draw the levels with peaks
	// TODO LMMS Compressor, EQ and Delay still use linear displays...
	paintLevels(ev, painter, !getLevelsDisplayedInDBFS());

	// Draw the knob
	painter.drawPixmap((width() - m_knob.width()) / 2, knobPosY() - m_knob.height(), m_knob);
}

void Fader::paintLevels(QPaintEvent * ev, QPainter & painter, bool linear)
{
	std::function<float(float value)> mapper = [this](float value) { return ampToDbfs(qMax<float>(0.0001, value)); };

	if (linear)
	{
		mapper = [this](float value) { return value; };
	}

	float const mappedMinPeak(mapper(m_fMinPeak));
	float const mappedMaxPeak(mapper(m_fMaxPeak));
	float const mappedPeakL(mapper(m_fPeakValue_L));
	float const mappedPeakR(mapper(m_fPeakValue_R));
	float const mappedPersistentPeakL(mapper(m_persistentPeak_L));
	float const mappedPersistentPeakR(mapper(m_persistentPeak_R));
	float const mappedUnity(mapper(1.f));

	painter.save();

	QRect const baseRect = rect();

	int const height = baseRect.height();

	int const margin = 2;
	int const distanceBetweenMeters = 2;

	int const numberOfMeters = 2;
	int const meterWidth = (baseRect.width() - 2 * margin - distanceBetweenMeters * (numberOfMeters - 1)) / numberOfMeters;

	QRect leftMeterRect(margin, margin, meterWidth, height - 2 * margin);
	QRect rightMeterRect(baseRect.width() - margin - meterWidth, margin, meterWidth, height - 2 * margin);

	QPainterPath path;
	qreal radius = 1;
	path.addRoundedRect(leftMeterRect, radius, radius);
	path.addRoundedRect(rightMeterRect, radius, radius);
	painter.fillPath(path, Qt::black);

	// Now clip everything to the paths of the meters
	painter.setClipPath(path);

	// This linear map performs the following mapping:
	// Value (dbFS or linear) -> window coordinates of the widget
	// It is for example used to determine the height of peaks, markers and to define the gradient for the levels
	LinearMap<float> const valuesToWindowCoordinates(mappedMaxPeak, leftMeterRect.y(), mappedMinPeak, leftMeterRect.y() + leftMeterRect.height());

	// This lambda takes a peak value (in dbFS or linear) and a rectangle and computes a rectangle
	// that represent the peak value within the rectangle. It's used to compute the peak indicators
	// which "dance" on top of the level meters.
	auto const computePeakRect = [&valuesToWindowCoordinates](QRect const & rect, float peak) -> QRect
	{
		return QRect(rect.x(), valuesToWindowCoordinates.map(peak), rect.width(), 1);
	};

	// This lambda takes a peak value (in dbFS or linear) and a rectangle and returns an adjusted copy of the
	// rectangle that represents the peak value. It is used to compute the level meters themselves.
	auto const computeLevelRect = [&valuesToWindowCoordinates](QRect const & rect, float peak) -> QRect
	{
		QRect result(rect);
		result.setTop(valuesToWindowCoordinates.map(peak));

		return result;
	};

	// Draw left and right unity lines (0 dbFS, 1.0 amplitude)
	if (getRenderUnityLine())
	{
		auto const unityRectL = computePeakRect(leftMeterRect, mappedUnity);
		painter.fillRect(unityRectL, getUnityMarker());

		auto const unityRectR = computePeakRect(rightMeterRect, mappedUnity);
		painter.fillRect(unityRectR, getUnityMarker());
	}

	// These values define where the gradient changes values, i.e. the ranges
	// for clipping, warning and ok.
	// Please ensure that "clip starts" is the maximum value and that "ok ends"
	// is the minimum value and that all other values lie inbetween. Otherwise
	// there will be warnings when the gradient is defined.
	float const mappedClipStarts(mapper(dbfsToAmp(0.f)));
	float const mappedWarnEnd(mapper(dbfsToAmp(-0.01)));
	float const mappedWarnStart(mapper(dbfsToAmp(-6.f)));
	float const mappedOkEnd(mapper(dbfsToAmp(-12.f)));

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
	LinearMap<float> const mapBetweenClipAndOk(mappedClipStarts, 0.f, mappedOkEnd, 1.f);

	linearGrad.setColorAt(0, peakClip());
	linearGrad.setColorAt(mapBetweenClipAndOk.map(mappedWarnEnd), peakWarn());
	linearGrad.setColorAt(mapBetweenClipAndOk.map(mappedWarnStart), peakWarn());
	linearGrad.setColorAt(1, peakOk());

	// Draw left levels
	QRect leftMeterMargins = leftMeterRect.marginsRemoved(QMargins(1, 0, 1, 0));
	painter.fillRect(computeLevelRect(leftMeterMargins, mappedPeakL), linearGrad);

	// Draw left peaks
	auto const peakRectL = computePeakRect(leftMeterMargins, mappedPersistentPeakL);
	painter.fillRect(peakRectL, linearGrad);

	// Draw right levels
	QRect rightMeterMargins = rightMeterRect.marginsRemoved(QMargins(1, 0, 1, 0));
	painter.fillRect(computeLevelRect(rightMeterMargins, mappedPeakR), linearGrad);

	// Draw right peaks
	auto const peakRectR = computePeakRect(rightMeterMargins, mappedPersistentPeakR);
	painter.fillRect(peakRectR, linearGrad);

	painter.restore();
}

QColor const & Fader::peakOk() const
{
	return m_peakOk;
}

QColor const & Fader::peakClip() const
{
	return m_peakClip;
}

QColor const & Fader::peakWarn() const
{
	return m_peakWarn;
}

void Fader::setPeakOk(const QColor& c)
{
	m_peakOk = c;
}

void Fader::setPeakClip(const QColor& c)
{
	m_peakClip = c;
}

void Fader::setPeakWarn(const QColor& c)
{
	m_peakWarn = c;
}

QColor const & Fader::getUnityMarker() const
{
	return m_unityMarker;
}

void Fader::setUnityMarker(const QColor& c)
{
	m_unityMarker = c;
}


} // namespace lmms::gui
