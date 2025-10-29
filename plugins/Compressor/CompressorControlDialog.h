/*
 * CompressorControlDialog.h
 *
 * Copyright (c) 2020 Lost Robot <r94231@gmail.com>
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

#ifndef COMPRESSOR_CONTROL_DIALOG_H
#define COMPRESSOR_CONTROL_DIALOG_H

#include <QBasicTimer>
#include <QElapsedTimer>
#include <QPainter>

#include "EffectControlDialog.h"

class QLabel;

namespace lmms
{

constexpr float COMP_NOISE_FLOOR = 0.000001f;// -120 dbFs

class CompressorControls;


namespace gui
{

constexpr int COMP_MILLI_PER_PIXEL = 6;
constexpr int MIN_COMP_SCREEN_X = 800;
constexpr int MIN_COMP_SCREEN_Y = 360;
constexpr int MAX_COMP_SCREEN_X = 1920;
constexpr int MAX_COMP_SCREEN_Y = 1080;
constexpr int COMP_SCREEN_X = 800;
constexpr int COMP_SCREEN_Y = 560;
constexpr int KNEE_SCREEN_X = COMP_SCREEN_Y;
constexpr int KNEE_SCREEN_Y = COMP_SCREEN_Y;
constexpr int COMP_KNEE_LINES = 20;
constexpr int COMP_BOX_X = 720;
constexpr int COMP_BOX_Y = 280;
constexpr float COMP_GRID_SPACING = 3.f;// 3 db per grid line
constexpr float COMP_GRID_MAX = 96.f;// Can't zoom out past 96 db

class AutomatableButtonGroup;
class Knob;
class PixmapButton;
class EqFader;


class CompressorControlDialog : public EffectControlDialog
{
	Q_OBJECT
public:
	CompressorControlDialog(CompressorControls* controls);

	bool isResizable() const override {return true;}
	QSize sizeHint() const override {return QSize(COMP_SCREEN_X, COMP_SCREEN_Y);}

	// For theming purposes
	Q_PROPERTY(QColor inVolAreaColor MEMBER m_inVolAreaColor)
	Q_PROPERTY(QColor inVolColor MEMBER m_inVolColor)
	Q_PROPERTY(QColor outVolAreaColor MEMBER m_outVolAreaColor)
	Q_PROPERTY(QColor outVolColor MEMBER m_outVolColor)
	Q_PROPERTY(QColor gainReductionColor MEMBER m_gainReductionColor)
	Q_PROPERTY(QColor kneeColor MEMBER m_kneeColor)
	Q_PROPERTY(QColor kneeColor2 MEMBER m_kneeColor2)
	Q_PROPERTY(QColor threshColor MEMBER m_threshColor)
	Q_PROPERTY(QColor textColor MEMBER m_textColor)
	Q_PROPERTY(QColor graphColor MEMBER m_graphColor)
	Q_PROPERTY(QColor resetColor MEMBER m_resetColor)
	Q_PROPERTY(QColor backgroundColor MEMBER m_backgroundColor)

protected:
	void resizeEvent(QResizeEvent *event) override;
	void paintEvent(QPaintEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;

private slots:
	void updateDisplay();
	void peakmodeChanged();
	void stereoLinkChanged();
	void lookaheadChanged();
	void limiterChanged();

private:
	void makeLargeKnob(Knob * knob, QString hint, QString unit);
	void makeSmallKnob(Knob * knob, QString hint, QString unit);
	void resetCompressorView();
	void drawVisPixmap();
	void redrawKnee();
	void drawKneePixmap2();
	void drawMiscPixmap();
	void drawGraph();
	void mouseDoubleClickEvent(QMouseEvent* event) override;
	void setGuiVisibility(bool isVisible);

	QPainter m_p;

	QBasicTimer m_updateTimer;

	CompressorControls * m_controls;

	inline int dbfsToYPoint(float inDbfs);
	inline int dbfsToXPoint(float inDbfs);

	QPixmap m_visPixmap = QPixmap(MAX_COMP_SCREEN_X, MAX_COMP_SCREEN_Y);
	QPixmap m_kneePixmap = QPixmap(MAX_COMP_SCREEN_X, MAX_COMP_SCREEN_Y);
	QPixmap m_kneePixmap2 = QPixmap(MAX_COMP_SCREEN_X, MAX_COMP_SCREEN_Y);
	QPixmap m_miscPixmap = QPixmap(MAX_COMP_SCREEN_X, MAX_COMP_SCREEN_Y);
	QPixmap m_graphPixmap = QPixmap(MAX_COMP_SCREEN_X, MAX_COMP_SCREEN_Y);

	int m_lastPoint;
	int m_lastGainPoint;
	int m_lastKneePoint = 0;

	int m_windowSizeX = size().width();
	int m_windowSizeY = size().height();
	int m_kneeWindowSizeX = m_windowSizeY;
	int m_kneeWindowSizeY = m_windowSizeY;
	int m_controlsBoxX = 0;
	int m_controlsBoxY = 0;

	float m_dbRange = 36;

	QColor m_inVolAreaColor = QColor(209, 216, 228, 17);
	QColor m_inVolColor = QColor(209, 216, 228, 100);
	QColor m_outVolAreaColor = QColor(209, 216, 228, 30);
	QColor m_outVolColor = QColor(209, 216, 228, 240);
	QColor m_gainReductionColor = QColor(180, 100, 100, 210);
	QColor m_kneeColor = QColor(39, 171, 95, 255);
	QColor m_kneeColor2 = QColor(9, 171, 160, 255);
	QColor m_threshColor = QColor(39, 171, 95, 100);
	QColor m_textColor = QColor(209, 216, 228, 50);
	QColor m_graphColor = QColor(209, 216, 228, 50);
	QColor m_resetColor = QColor(200, 100, 15, 200);
	QColor m_backgroundColor = QColor(7, 8, 9, 255);

	float m_peakAvg;
	float m_gainAvg;

	float m_yPoint;
	float m_yGainPoint;

	int m_threshYPoint;
	int m_threshXPoint;

	int m_compPixelMovement;

	QLabel * m_controlsBoxLabel;
	QLabel * m_rmsEnabledLabel;
	QLabel * m_blendEnabledLabel;
	QLabel * m_lookaheadEnabledLabel;
	QLabel * m_ratioEnabledLabel;

	Knob * m_thresholdKnob;
	Knob * m_ratioKnob;
	Knob * m_attackKnob;
	Knob * m_releaseKnob;
	Knob * m_kneeKnob;
	Knob * m_rangeKnob;
	Knob * m_lookaheadLengthKnob;
	Knob * m_holdKnob;

	Knob * m_rmsKnob;
	Knob * m_inBalanceKnob;
	Knob * m_outBalanceKnob;
	Knob * m_stereoBalanceKnob;
	Knob * m_blendKnob;
	Knob * m_tiltKnob;
	Knob * m_tiltFreqKnob;
	Knob * m_mixKnob;

	Knob * m_autoAttackKnob;
	Knob * m_autoReleaseKnob;

	EqFader * m_outFader;
	EqFader * m_inFader;

	PixmapButton * rmsButton;
	PixmapButton * peakButton;
	AutomatableButtonGroup * rmsPeakGroup;

	PixmapButton * leftRightButton;
	PixmapButton * midSideButton;
	AutomatableButtonGroup * leftRightMidSideGroup;

	PixmapButton * compressButton;
	PixmapButton * limitButton;
	AutomatableButtonGroup * compressLimitGroup;

	PixmapButton * unlinkedButton;
	PixmapButton * maximumButton;
	PixmapButton * averageButton;
	PixmapButton * minimumButton;
	PixmapButton * blendButton;
	AutomatableButtonGroup * stereoLinkGroup;

	PixmapButton * autoMakeupButton;
	PixmapButton * auditionButton;
	PixmapButton * feedbackButton;
	PixmapButton * lookaheadButton;

	bool m_guiVisibility = true;

	QElapsedTimer m_timeElapsed;
	int m_timeSinceLastUpdate = 0;

	friend class CompressorControls;
} ;



} // namespace gui

} // namespace lmms

#endif
