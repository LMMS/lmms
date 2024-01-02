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


namespace lmms::gui
{

class SimpleTextFloat;


class LMMS_EXPORT Fader : public QWidget, public FloatModelView
{
	Q_OBJECT
public:
	Q_PROPERTY(QColor peakOk READ peakOk WRITE setPeakOk)
	Q_PROPERTY(QColor peakClip READ peakClip WRITE setPeakClip)
	Q_PROPERTY(QColor peakWarn READ peakWarn WRITE setPeakWarn)
	Q_PROPERTY(bool levelsDisplayedInDBFS READ getLevelsDisplayedInDBFS WRITE setLevelsDisplayedInDBFS)
	Q_PROPERTY(bool renderUnityLine READ getRenderUnityLine WRITE setRenderUnityLine)

	Fader(FloatModel* model, const QString& name, QWidget* parent);
	Fader(FloatModel* model, const QString& name, QWidget* parent, const QPixmap& knob);
	~Fader() override = default;

	void setPeak_L( float fPeak );
	float getPeak_L() {	return m_fPeakValue_L;	}

	void setPeak_R( float fPeak );
	float getPeak_R() {	return m_fPeakValue_R;	}

	inline float getMinPeak() const { return m_fMinPeak; }
	inline void setMinPeak(float minPeak) { m_fMinPeak = minPeak; }

	inline float getMaxPeak() const { return m_fMaxPeak; }
	inline void setMaxPeak(float maxPeak) { m_fMaxPeak = maxPeak; }

	QColor const & peakOk() const;
	void setPeakOk(const QColor& c);

	QColor const & peakClip() const;
	void setPeakClip(const QColor& c);

	QColor const & peakWarn() const;
	void setPeakWarn(const QColor& c);

	inline bool getLevelsDisplayedInDBFS() const { return m_levelsDisplayedInDBFS; }
	inline void setLevelsDisplayedInDBFS(bool value = true) { m_levelsDisplayedInDBFS = value; }

	inline bool getRenderUnityLine() const { return m_renderUnityLine; }
	inline void setRenderUnityLine(bool value = true) { m_renderUnityLine = value; }

	void setDisplayConversion( bool b )
	{
		m_conversionFactor = b ? 100.0 : 1.0;
	}

	inline void setHintText( const QString & _txt_before,
						const QString & _txt_after )
	{
		setDescription( _txt_before );
		setUnit( _txt_after );
	}

private:
	void contextMenuEvent( QContextMenuEvent * _me ) override;
	void mousePressEvent( QMouseEvent *ev ) override;
	void mouseDoubleClickEvent( QMouseEvent* mouseEvent ) override;
	void mouseMoveEvent( QMouseEvent *ev ) override;
	void mouseReleaseEvent( QMouseEvent * _me ) override;
	void wheelEvent( QWheelEvent *ev ) override;
	void paintEvent( QPaintEvent *ev ) override;

	inline bool clips(float const & value) const { return value >= 1.0f; }

	void paintLevels(QPaintEvent *ev, QPainter & painter, bool linear = false);

	int knobPosY() const
	{
		float fRange = model()->maxValue() - model()->minValue();
		float realVal = model()->value() - model()->minValue();

		return height() - ((height() - m_knob.height()) * (realVal / fRange));
	}

	void setPeak( float fPeak, float &targetPeak, float &persistentPeak, QElapsedTimer &lastPeakTimer );

	void updateTextFloat();

	// Private members
private:
	float m_fPeakValue_L;
	float m_fPeakValue_R;
	float m_persistentPeak_L;
	float m_persistentPeak_R;
	float m_fMinPeak;
	float m_fMaxPeak;

	QElapsedTimer m_lastPeakTimer_L;
	QElapsedTimer m_lastPeakTimer_R;

	QPixmap m_knob;

	bool m_levelsDisplayedInDBFS;

	int m_moveStartPoint;
	float m_startValue;

	static SimpleTextFloat * s_textFloat;

	QColor m_peakOk;
	QColor m_peakClip;
	QColor m_peakWarn;

	bool m_renderUnityLine;
} ;


} // namespace lmms::gui

#endif // LMMS_GUI_FADER_H
