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

Fader::Fader(FloatModel* model, const QString& name, QWidget* parent ) :
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

class PaintHelper
{
public:
	PaintHelper(float min, float max) :
		m_min(min),
		m_max(max),
		// We will need to divide by the span between min and max several times. It's more
		// efficient to calculate the reciprocal once and then to multiply.
		m_fullSpanReciprocal(1. / (max - min))
	{
	}

	float mapMaxZeroAndMinOne(float value)
	{
		return (m_max - value) * m_fullSpanReciprocal;
	}

	float mapMaxZeroAndMinOneClamped(float value)
	{
		return std::clamp(mapMaxZeroAndMinOne(value), 0.f, 1.f);
	}

	float mapMinZeroAndMaxOne(float value)
	{
		return 1. - mapMaxZeroAndMinOne(value);
	}

	float mapMinZeroAndMaxOneClamped(float value)
	{
		return std::clamp(mapMinZeroAndMaxOne(value), 0.f, 1.f);
	}

	QRect getMeterRect(QRect const & meterRect, float peak)
	{
		float const span = peak - m_min;
		int mappedHeight = meterRect.height() * span * m_fullSpanReciprocal;

		return meterRect.adjusted(0, meterRect.height() - mappedHeight, 0, 0);
	}

	QRect getPersistentPeakRect(QRect const & meterRect, float peak)
	{
		int persistentPeak_L = meterRect.height() * (1 - (peak - m_min) * m_fullSpanReciprocal);

		return QRect(meterRect.x(), persistentPeak_L, meterRect.width(), 1);
	}

private:
	float const m_min;
	float const m_max;
	float const m_fullSpanReciprocal;
};

void Fader::paintLevels(QPaintEvent * ev, QPainter & painter, bool linear)
{
	std::function<float(float value)> mapper = [this](float value) { return ampToDbfs(qMax<float>(0.0001, value)); };

	if (linear)
	{
		mapper = [this](float value) { return value; };
	}

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

	float const mappedMinPeak(mapper(m_fMinPeak));
	float const mappedMaxPeak(mapper(m_fMaxPeak));
	float const mappedPeakL(mapper(m_fPeakValue_L));
	float const mappedPeakR(mapper(m_fPeakValue_R));
	float const mappedPersistentPeakL(mapper(m_persistentPeak_L));
	float const mappedPersistentPeakR(mapper(m_persistentPeak_R));
	float const mappedUnity(mapper(1.f));
	float const mappedLastOk(mapper(dbfsToAmp(-12.f)));

	PaintHelper ph(mappedMinPeak, mappedMaxPeak);

	// Prepare the gradient for the meters
	QColor const & clippingColor = peakClip();
	QColor const & warnColor = peakWarn();
	QColor const & okColor = peakOk();

	QLinearGradient linearGrad(0, margin, 0, leftMeterRect.y() + leftMeterRect.height());
	linearGrad.setColorAt(0, clippingColor);
	linearGrad.setColorAt(ph.mapMaxZeroAndMinOne(mappedUnity), warnColor);
	linearGrad.setColorAt(ph.mapMaxZeroAndMinOne(mappedLastOk), okColor);
	linearGrad.setColorAt(1, okColor);

	// Draw left levels
	QRect leftMeterMargins = leftMeterRect.marginsRemoved(QMargins(1, 0, 1, 0));
	painter.fillRect(ph.getMeterRect(leftMeterMargins, mappedPeakL), linearGrad);

	// Draw left peaks
	auto const peakRectL = ph.getPersistentPeakRect(leftMeterMargins, mappedPersistentPeakL);
	painter.fillRect(peakRectL, linearGrad);

	// Draw right levels
	QRect rightMeterMargins = rightMeterRect.marginsRemoved(QMargins(1, 0, 1, 0));
	painter.fillRect(ph.getMeterRect(rightMeterMargins, mappedPeakR), linearGrad);

	// Draw right peaks
	auto const peakRectR = ph.getPersistentPeakRect(rightMeterMargins, mappedPersistentPeakR);
	painter.fillRect(peakRectR, linearGrad);

	// TODO
	QPen pen(QColor(255, 255, 255, 18));
	pen.setWidth(2);
	painter.setPen(pen);
	painter.drawPath(path);

	// Draw left and right unity lines (0 dbFS, 1.0 amplitude)
	QColor unityMarkerColor(127, 127, 127, 127);

	if (getRenderUnityLine())
	{
		auto const unityRectL = ph.getPersistentPeakRect(leftMeterRect, mappedUnity);
		painter.fillRect(unityRectL, unityMarkerColor);
	}

	if (getRenderUnityLine())
	{
		auto const unityRectR = ph.getPersistentPeakRect(rightMeterRect, mappedUnity);
		painter.fillRect(unityRectR, unityMarkerColor);
	}


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


} // namespace lmms::gui
