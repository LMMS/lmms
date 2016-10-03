/*
 * AutomationEditor.h - declaration of class AutomationEditor which is a window
 *					  where you can edit dynamic values in an easy way
 *
 * Copyright (c) 2006-2008 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#ifndef AUTOMATION_EDITOR_H
#define AUTOMATION_EDITOR_H

#include <QtCore/QMutex>
#include <QVector>
#include <QWidget>

#include "Editor.h"

#include "lmms_basics.h"
#include "JournallingObject.h"
#include "MidiTime.h"
#include "AutomationPattern.h"
#include "ComboBoxModel.h"
#include "Knob.h"

class QPainter;
class QPixmap;
class QScrollBar;

class ComboBox;
class NotePlayHandle;
class TimeLineWidget;



class AutomationEditor : public QWidget, public JournallingObject
{
	Q_OBJECT
	Q_PROPERTY(QColor gridColor READ gridColor WRITE setGridColor)
	Q_PROPERTY(QColor vertexColor READ vertexColor WRITE setVertexColor)
	Q_PROPERTY(QBrush scaleColor READ scaleColor WRITE setScaleColor)
	Q_PROPERTY(QBrush graphColor READ graphColor WRITE setGraphColor)
	Q_PROPERTY(QColor crossColor READ crossColor WRITE setCrossColor)
	Q_PROPERTY(QColor backgroundShade READ backgroundShade WRITE setBackgroundShade)
public:
	void setCurrentPattern(AutomationPattern * new_pattern);

	inline const AutomationPattern * currentPattern() const
	{
		return m_pattern;
	}

	inline bool validPattern() const
	{
		return m_pattern != nullptr;
	}

	virtual void saveSettings(QDomDocument & doc, QDomElement & parent);
	virtual void loadSettings(const QDomElement & parent);
	QString nodeName() const
	{
		return "automationeditor";
	}

	// qproperty access methods
	QColor gridColor() const;
	QBrush graphColor() const;
	QColor vertexColor() const;
	QBrush scaleColor() const;
	QColor crossColor() const;
	QColor backgroundShade() const;
	void setGridColor(const QColor& c);
	void setGraphColor(const QBrush& c);
	void setVertexColor(const QColor& c);
	void setScaleColor(const QBrush& c);
	void setCrossColor(const QColor& c);
	void setBackgroundShade(const QColor& c);

	enum EditModes
	{
		DRAW,
		ERASE,
		SELECT,
		MOVE
	};

public slots:
	void update();
	void updateAfterPatternChange();


protected:
	typedef AutomationPattern::timeMap timeMap;

	virtual void keyPressEvent(QKeyEvent * ke);
	virtual void leaveEvent(QEvent * e);
	virtual void mousePressEvent(QMouseEvent * mouseEvent);
	virtual void mouseReleaseEvent(QMouseEvent * mouseEvent);
	virtual void mouseMoveEvent(QMouseEvent * mouseEvent);
	virtual void paintEvent(QPaintEvent * pe);
	virtual void resizeEvent(QResizeEvent * re);
	virtual void wheelEvent(QWheelEvent * we);

	float getLevel( int y );
	int xCoordOfTick( int tick );
	int yCoordOfLevel( float level );
	inline void drawLevelTick( QPainter & p, int tick,
					float value, bool is_selected );
	void removeSelection();
	void selectAll();
	void getSelectedValues(timeMap & selected_values );

	void drawLine( int x0, float y0, int x1, float y1 );

protected slots:
	void play();
	void stop();

	void horScrolled( int new_pos );
	void verScrolled( int new_pos );

	void setEditMode(AutomationEditor::EditModes mode);
	void setEditMode(int mode);

	void setProgressionType(AutomationPattern::ProgressionTypes type);
	void setProgressionType(int type);
	void setTension();

	void copySelectedValues();
	void cutSelectedValues();
	void pasteValues();
	void deleteSelectedValues();

	void updatePosition( const MidiTime & t );

	void zoomingXChanged();
	void zoomingYChanged();

	/// Updates the pattern's quantization using the current user selected value.
	void setQuantization();

private:

	enum Actions
	{
		NONE,
		MOVE_VALUE,
		SELECT_VALUES,
		MOVE_SELECTION
	} ;

	// some constants...
	static const int SCROLLBAR_SIZE = 12;
	static const int TOP_MARGIN = 16;

	static const int DEFAULT_Y_DELTA = 6;
	static const int DEFAULT_STEPS_PER_TACT = 16;
	static const int DEFAULT_PPT = 12 * DEFAULT_STEPS_PER_TACT;

	static const int VALUES_WIDTH = 64;

	AutomationEditor();
	AutomationEditor( const AutomationEditor & );
	virtual ~AutomationEditor();

	static QPixmap * s_toolDraw;
	static QPixmap * s_toolErase;
	static QPixmap * s_toolSelect;
	static QPixmap * s_toolMove;
	static QPixmap * s_toolYFlip;
	static QPixmap * s_toolXFlip;

	ComboBoxModel m_zoomingXModel;
	ComboBoxModel m_zoomingYModel;
	ComboBoxModel m_quantizeModel;

	static const QVector<double> m_zoomXLevels;

	FloatModel * m_tensionModel;

	QMutex m_patternMutex;
	AutomationPattern * m_pattern;
	float m_minLevel;
	float m_maxLevel;
	float m_step;
	float m_scrollLevel;
	float m_bottomLevel;
	float m_topLevel;

	void updateTopBottomLevels();

	QScrollBar * m_leftRightScroll;
	QScrollBar * m_topBottomScroll;

	MidiTime m_currentPosition;

	Actions m_action;

	tick_t m_selectStartTick;
	tick_t m_selectedTick;
	float m_selectStartLevel;
	float m_selectedLevels;

	float m_moveStartLevel;
	tick_t m_moveStartTick;
	int m_moveXOffset;

	float m_drawLastLevel;
	tick_t m_drawLastTick;

	int m_ppt;
	int m_y_delta;
	bool m_y_auto;

	timeMap m_valuesToCopy;
	timeMap m_selValuesForMove;


	EditModes m_editMode;


	TimeLineWidget * m_timeLine;
	bool m_scrollBack;

	void drawCross(QPainter & p );
	void drawAutomationPoint( QPainter & p, timeMap::iterator it );
	bool inBBEditor();

	QColor m_gridColor;
	QBrush m_graphColor;
	QColor m_vertexColor;
	QBrush m_scaleColor;
	QColor m_crossColor;
	QColor m_backgroundShade;

	friend class AutomationEditorWindow;


signals:
	void currentPatternChanged();
	void positionChanged( const MidiTime & );
} ;




class AutomationEditorWindow : public Editor
{
	Q_OBJECT

	static const int INITIAL_WIDTH = 860;
	static const int INITIAL_HEIGHT = 480;
public:
	AutomationEditorWindow();
	~AutomationEditorWindow();

	void setCurrentPattern(AutomationPattern* pattern);
	const AutomationPattern* currentPattern();

	virtual void dropEvent( QDropEvent * _de );
	virtual void dragEnterEvent( QDragEnterEvent * _dee );

	void open(AutomationPattern* pattern);

	AutomationEditor* m_editor;

	QSize sizeHint() const;

public slots:
	void clearCurrentPattern();

signals:
	void currentPatternChanged();

protected slots:
	void play();
	void stop();

private slots:
	void updateWindowTitle();

private:
	QAction* m_discreteAction;
	QAction* m_linearAction;
	QAction* m_cubicHermiteAction;

	QAction* m_flipYAction;
	QAction* m_flipXAction;

	Knob * m_tensionKnob;

	ComboBox * m_zoomingXComboBox;
	ComboBox * m_zoomingYComboBox;
	ComboBox * m_quantizeComboBox;
};


#endif
