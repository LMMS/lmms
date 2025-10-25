/*
 * Command.h - implements Commands, a layer between the core and gui
 *
 * Copyright (c) 2025 szeli1
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

#ifndef LMMS_GRID_VIEW_H
#define LMMS_GRID_VIEW_H



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


namespace lmms::gui
{

class GridView : public QWidget, public ModelView
{
public:
	GridView(QWidget* parent);
	~GridView() override = default;	

	GridModel* model()
	{
		return castModel<GridModel>();
	}

protected:
	void paintEvent(QPaintEvent* pe) override;
	void dropEvent(QDropEvent* de) override;
	void dragEnterEvent(QDragEnterEvent* dee) override;
	void mousePressEvent(QMouseEvent* me) override;
	void mouseMoveEvent(QMouseEvent* me) override;
	void mouseReleaseEvent(QMouseEvent* me) override;
private:
	void modelChanged() override;

	float selectStartX;
	float selectStartY;
	float selectEndX;
	float selectEndY;
};

} // namespace lmms::gui

#endif // LMMS_GRID_VIEW_H
