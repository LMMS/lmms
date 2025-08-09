/*
 * Graph.h - a QT widget for displaying and manipulating waveforms
 *
 * Copyright (c) 2006-2007 Andreas Brandmaier <andy/at/brandmaier/dot/de>
 *               2008 Paul Giblock <drfaygo/at/gmail/dot/com>
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

#ifndef LMMS_GUI_GRAPH_H
#define LMMS_GUI_GRAPH_H

#include <QWidget>
#include <QPixmap>
#include <QCursor>

#include "Model.h"
#include "ModelView.h"
#include "LmmsTypes.h"

namespace lmms
{


class graphModel;

namespace gui
{


class LMMS_EXPORT Graph : public QWidget, public ModelView
{
	Q_OBJECT
public:
	enum class Style
	{
		Nearest, //!< draw as stairs
		Linear, //!< connect each 2 samples with a line, with wrapping
		LinearNonCyclic, //!< Linear without wrapping
		Bar, //!< draw thick bars
	};

	/**
	 * @brief Constructor
	 * @param _width Pixel width of widget
	 * @param _height Pixel height of widget
	 */
	Graph( QWidget * _parent, Style _style = Style::Linear,
		int _width = 132,
		int _height = 104
	);
	~Graph() override = default;

	void setForeground( const QPixmap & _pixmap );


	void setGraphColor( const QColor );

	inline graphModel * model()
	{
		return castModel<graphModel>();
	}

	inline Style getGraphStyle()
	{
		return m_graphStyle;
	}


	inline void setGraphStyle( Style _s )
	{
		m_graphStyle = _s;
		update();
	}

signals:
	void drawn();
protected:
	void paintEvent( QPaintEvent * _pe ) override;
	void dropEvent( QDropEvent * _de ) override;
	void dragEnterEvent( QDragEnterEvent * _dee ) override;
	void mousePressEvent( QMouseEvent * _me ) override;
	void mouseMoveEvent( QMouseEvent * _me ) override;
	void mouseReleaseEvent( QMouseEvent * _me ) override;

protected slots:
	void updateGraph( int _startPos, int _endPos );
	void updateGraph();

private:
	void modelChanged() override;

	void changeSampleAt( int _x, int _y );
	void drawLineAt( int _x, int _y, int _lastx );


	QPixmap m_foreground;
	QColor m_graphColor;

	Style m_graphStyle;

	bool m_mouseDown;
	int m_lastCursorX;

} ;


} // namespace gui


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
	 * @param _min Minimum y value to display
	 * @param _max Maximum y value to display
	 * @param _size Number of samples (e.g. x value)
	 * @param _step Step size on y axis where values snap to, or 0.0f
	 *   for "no snapping"
	 */
	graphModel( float _min,
			float _max,
			int _size,
			Model * _parent,
			bool _default_constructed = false,
			float _step = 0.0 );

	~graphModel() override = default;

	// TODO: saveSettings, loadSettings?

	inline float minValue() const
	{
		return( m_minValue );
	}

	inline float maxValue() const
	{
		return( m_maxValue );
	}

	inline int length() const
	{
		return m_length;
	}

	inline const float * samples() const
	{
		return( m_samples.data() );
	}

	//! Make cyclic convolution
	//! @param convolution Samples to convolve with
	//! @param convolutionLength Number of samples to take for each sum
	//! @param centerOffset Offset for resulting values
	void convolve(const float *convolution,
		const int convolutionLength, const int centerOffset);

public slots:
	//! Set range of y values
	void setRange(float ymin, float ymax);

	void setLength( int _size );
	//! Update one sample
	void setSampleAt( int x, float val );
	//! Update samples array
	void setSamples( const float * _value );

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
	void shiftPhase( int _deg );
	void clear();
	void clearInvisible();

signals:
	void lengthChanged();
	void samplesChanged( int startPos, int endPos );
	void rangeChanged();

private:
	void drawSampleAt( int x, float val );

	QVector<float> m_samples;
	int m_length;
	float m_minValue;
	float m_maxValue;
	float m_step;

	friend class gui::Graph;

};


} // namespace lmms

#endif // LMMS_GUI_GRAPH_H
