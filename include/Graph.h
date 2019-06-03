/*
 * Graph.h - a QT widget for displaying and manipulating waveforms
 *
 * Copyright (c)2006-2007 Andreas Brandmaier <andy/at/brandmaier/dot/de>
 *               2008 Paul Giblock <drfaygo/at/gmail/dot/com>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option)any later version.
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


#ifndef GRAPH_H
#define GRAPH_H

#include <QWidget>
#include <QPixmap>
#include <QCursor>

#include "Model.h"
#include "ModelView.h"
#include "lmms_basics.h"

class graphModel;


class LMMS_EXPORT Graph : public QWidget, public ModelView
{
	Q_OBJECT
public:
	enum graphStyle
	{
		NearestStyle, //!< draw as stairs
		LinearStyle, //!< connect each 2 samples with a line, with wrapping
		LinearNonCyclicStyle, //!< LinearStyle without wrapping
		BarStyle, //!< draw thick bars
		NumGraphStyles,
		BarCenterGradStyle //!< draw color gradient coming from center
	};

	/**
	 * @brief Constructor
	 * @param width Pixel width of widget
	 * @param height Pixel height of widget
	 */
	Graph(QWidget * parent, graphStyle style = Graph::LinearStyle,
		int width = 132,
		int height = 104
	);
	virtual ~Graph()= default;

	void setForeground(const QPixmap & pixmap);


	void setGraphColor(const QColor);

	inline graphModel * model()
	{
		return castModel<graphModel>();
	}

	inline graphStyle getGraphStyle()
	{
		return m_graphStyle;
	}


	inline void setGraphStyle(graphStyle s)
	{
		m_graphStyle = s;
		update();
	}

signals:
	void drawn();
protected:
	virtual void paintEvent(QPaintEvent * pe);
	virtual void dropEvent(QDropEvent * de);
	virtual void dragEnterEvent(QDragEnterEvent * dee);
	virtual void mousePressEvent(QMouseEvent * me);
	virtual void mouseMoveEvent(QMouseEvent * me);
	virtual void mouseReleaseEvent(QMouseEvent * me);

protected slots:
	void updateGraph(int startPos, int endPos);
	void updateGraph();

private:
	virtual void modelChanged();

	void changeSampleAt(int x, int y);
	void drawLineAt(int x, int y, int lastx);


	QPixmap m_foreground;
	QColor m_graphColor;

	graphStyle m_graphStyle;

	bool m_mouseDown;
	int m_lastCursorX;

} ;


/**
	@brief 2 dimensional function plot

	Function plot graph with discrete x scale and continous y scale
	This makes it possible to display "#x" samples
*/
class LMMS_EXPORT graphModel : public Model
{
	Q_OBJECT
public:
	/**
	 * @brief Constructor
	 * @param min Minimum y value to display
	 * @param max Maximum y value to display
	 * @param size Number of samples (e.g. x value)
	 * @param step Step size on y axis where values snap to, or 0.0f
	 *   for "no snapping"
	 */
	graphModel(float min,
			float max,
			int size,
			:: Model * parent,
			bool default_constructed = false,
			float step = 0.0);

	virtual ~graphModel()= default;

	// TODO: saveSettings, loadSettings?

	inline float minValue()const
	{
		return m_minValue;
	}

	inline float maxValue()const
	{
		return m_maxValue;
	}

	inline int length()const
	{
		return m_length;
	}

	inline const float * samples()const
	{
		return m_samples.data();
	}

	//! Make cyclic convolution
	//! @param convolution Samples to convolve with
	//! @param convolutionLength Number of samples to take for each sum
	//! @param centerOffset Offset for resulting values
	void convolve(const float *convolution,
		const int convolutionLength, const int centerOffset);

public slots:
	//! Set range of y values
	void setRange(float min, float max);

	void setLength(int size);
	//! Update one sample
	void setSampleAt(int x, float val);
	//! Update samples array
	void setSamples(const float * value);

	void setWaveToSine();
	void setWaveToTriangle();
	void setWaveToSaw();
	void setWaveToSquare();
	void setWaveToNoise();
	QString setWaveToUser();

	void smooth();
	void smoothNonCyclic();
	void normalize();
	void invert();
	void shiftPhase(int deg);
	void clear();
	void clearInvisible();

signals:
	void lengthChanged();
	void samplesChanged(int startPos, int endPos);
	void rangeChanged();

private:
	void drawSampleAt(int x, float val);

	QVector<float> m_samples;
	int m_length;
	float m_minValue;
	float m_maxValue;
	float m_step;

	friend class Graph;

};

#endif
