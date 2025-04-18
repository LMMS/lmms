/*
 * Fader.h - fader-widget used in Mixer - partly taken from Hydrogen
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

#ifndef LMMS_GUI_FADER_H
#define LMMS_GUI_FADER_H

#include <QElapsedTimer>
#include <QPixmap>
#include <QWidget>


#include "AutomatableModelView.h"
#include "embed.h"
#include "lmms_math.h"


namespace lmms::gui
{

class SimpleTextFloat;


class LMMS_EXPORT Fader : public QWidget, public FloatModelView
{
	Q_OBJECT
public:
	Q_PROPERTY(QColor peakOk MEMBER m_peakOk)
	Q_PROPERTY(QColor peakClip MEMBER m_peakClip)
	Q_PROPERTY(QColor peakWarn MEMBER m_peakWarn)
	Q_PROPERTY(bool levelsDisplayedInDBFS MEMBER m_levelsDisplayedInDBFS)
	Q_PROPERTY(bool renderUnityLine READ getRenderUnityLine WRITE setRenderUnityLine)
	Q_PROPERTY(QColor unityMarker MEMBER m_unityMarker)

	Fader(FloatModel* model, const QString& name, QWidget* parent, bool modelIsLinear = true);
	Fader(FloatModel* model, const QString& name, QWidget* parent, const QPixmap& knob, bool modelIsLinear = true);
	~Fader() override = default;

	void setPeak_L(float fPeak);
	float getPeak_L() {	return m_fPeakValue_L;	}

	void setPeak_R(float fPeak);
	float getPeak_R() {	return m_fPeakValue_R;	}

	inline float getMinPeak() const { return m_fMinPeak; }
	inline void setMinPeak(float minPeak) { m_fMinPeak = minPeak; }

	inline float getMaxPeak() const { return m_fMaxPeak; }
	inline void setMaxPeak(float maxPeak) { m_fMaxPeak = maxPeak; }

	inline bool getRenderUnityLine() const { return m_renderUnityLine; }
	inline void setRenderUnityLine(bool value = true) { m_renderUnityLine = value; }

	enum class AdjustmentDirection
	{
		Up,
		Down
	};

	void adjust(const Qt::KeyboardModifiers & modifiers, AdjustmentDirection direction);
	void adjustByDecibelDelta(float value);

	void adjustByDialog();

	void setDisplayConversion(bool b)
	{
		m_conversionFactor = b ? 100.0 : 1.0;
	}

	inline void setHintText(const QString& txt_before,
						const QString& txt_after)
	{
		setDescription(txt_before);
		setUnit(txt_after);
	}

signals:
	void peakChanged(float peak);

private:
	void contextMenuEvent(QContextMenuEvent* me) override;
	void mousePressEvent(QMouseEvent* ev) override;
	void mouseDoubleClickEvent(QMouseEvent* mouseEvent) override;
	void mouseMoveEvent(QMouseEvent* ev) override;
	void mouseReleaseEvent(QMouseEvent* me) override;
	void wheelEvent(QWheelEvent* ev) override;
	void paintEvent(QPaintEvent* ev) override;

	void paintLevels(QPaintEvent* ev, QPainter& painter, bool linear = false);
	void paintFaderTicks(QPainter& painter);

	float determineAdjustmentDelta(const Qt::KeyboardModifiers & modifiers) const;
	void adjustModelByDBDelta(float value);

	int calculateKnobPosYFromModel() const;
	void setVolumeByLocalPixelValue(int y);

	/**
	 * @brief Computes the scaled ratio between the maximum dB value supported by the model and the minimum
	 * dB value that's supported by the fader from the given actual dB value.
	 * 
	 * If the provided input value lies inside the aforementioned interval then the result will be
	 * a value between 0 (value == minimum value) and 1 (value == maximum model value).
	 * If you look at the graphical representation of the fader then 0 represents a point at the bottom
	 * of the fader and 1 a point at the top of the fader.
	 * The ratio is scaled by an internal exponent which is an implementation detail that cannot be
	 * changed for now.
	 */
	float computeScaledRatio(float dBValue) const;

	void setPeak(float fPeak, float& targetPeak, float& persistentPeak, QElapsedTimer& lastPeakTimer);

	void updateTextFloat();
	void modelValueChanged();
	QString getModelValueAsDbString() const;

	bool modelIsLinear() const { return m_modelIsLinear; }

	// Private members
private:
	float m_fPeakValue_L {0.};
	float m_fPeakValue_R {0.};
	float m_persistentPeak_L {0.};
	float m_persistentPeak_R {0.};
	float m_fMinPeak {dbfsToAmp(-42)};
	float m_fMaxPeak {dbfsToAmp(9)};

	QElapsedTimer m_lastPeakTimer_L;
	QElapsedTimer m_lastPeakTimer_R;

	QPixmap m_knob {embed::getIconPixmap("fader_knob")};
	QSize m_knobSize;

	/**
	 * @brief Stores the offset to the knob center when the user drags the fader knob
	 * 
	 * This is needed to make it feel like the users drag the knob without it
	 * jumping immediately to the click position.
	 */
	int m_knobCenterOffset {0};

	bool m_levelsDisplayedInDBFS {true};
	bool m_modelIsLinear {false};

	static SimpleTextFloat* s_textFloat;

	QColor m_peakOk {10, 212, 92};
	QColor m_peakClip {193, 32, 56};
	QColor m_peakWarn {214, 236, 82};
	QColor m_unityMarker {63, 63, 63, 255};

	bool m_renderUnityLine {true};
} ;


} // namespace lmms::gui

#endif // LMMS_GUI_FADER_H
