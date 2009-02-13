/*
 * automation_editor.h - declaration of class automationEditor which is a window
 *                       where you can edit dynamic values in an easy way
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2006-2008 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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


#ifndef _AUTOMATION_EDITOR_H
#define _AUTOMATION_EDITOR_H

#include <QtGui/QWidget>

#include "lmms_basics.h"
#include "journalling_object.h"
#include "midi_time.h"
#include "automation_pattern.h"
#include "combobox_model.h"
#include "fluiq/collapsible_widget.h"


class QPainter;
class QPixmap;
class QScrollBar;

class comboBox;
class notePlayHandle;
class timeLine;
class toolButton;


class automationEditor : public FLUIQ::CollapsibleWidget,
				public journallingObject
{
	Q_OBJECT
public:
	void setCurrentPattern( automationPattern * _new_pattern );

	inline const automationPattern * currentPattern( void ) const
	{
		return m_pattern;
	}

	inline bool validPattern( void ) const
	{
		return m_pattern != NULL;
	}

	int quantization( void ) const;


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );
	inline virtual QString nodeName( void ) const
	{
		return "automationeditor";
	}


public slots:
	void update( void );


protected:
	typedef automationPattern::timeMap timeMap;

	virtual void closeEvent( QCloseEvent * _ce );
	virtual void keyPressEvent( QKeyEvent * _ke );
	virtual void leaveEvent( QEvent * _e );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void resizeEvent( QResizeEvent * _re );
	virtual void wheelEvent( QWheelEvent * _we );

	float getLevel( int _y );
	static inline void drawValueRect( QPainter & _p, int _x, int _y,
						int _width, int _height,
						const bool _is_selected );
	void removeSelection( void );
	void selectAll( void );
	void getSelectedValues( timeMap & _selected_values );

	void drawLine( int x0, float y0, int x1, float y1 );


protected slots:
	void play( void );
	void stop( void );

	void horScrolled( int _new_pos );
	void verScrolled( int _new_pos );

	void drawButtonToggled( void );
	void eraseButtonToggled( void );
	void selectButtonToggled( void );
	void moveButtonToggled( void );

	void copySelectedValues( void );
	void cutSelectedValues( void );
	void pasteValues( void );
	void deleteSelectedValues( void );

	void updatePosition( const midiTime & _t );

	void zoomingXChanged( void );
	void zoomingYChanged( void );


private:

	enum EditModes
	{
		ModeDraw,
		ModeErase,
		ModeSelect,
		ModeMove
	} ;

	enum Actions
	{
		ActionNone,
		ActionMoveValue,
		ActionSelectValues,
		ActionMoveSelection
	} ;

	// some constants...
	static const int InitialWidth = 740;
	static const int InitialHeight = 480;

	static const int ScrollBarSize = 16;
	static const int TopMargin = 32 +
				FLUIQ::CollapsibleWidgetHeader::MinimalHeight;

	static const int DefaultYDelta = 6;
	static const int DefaultStepsPerTact = 16;
	static const int DefaultPixelsPerTact = 12 * DefaultStepsPerTact;

	static const int ValuesWidth = 64;


	automationEditor( void );
	automationEditor( const automationEditor & );
	virtual ~automationEditor();


	static QPixmap * s_toolDraw;
	static QPixmap * s_toolErase;
	static QPixmap * s_toolSelect;
	static QPixmap * s_toolMove;


	QWidget * m_toolBar;

	toolButton * m_playButton;
	toolButton * m_stopButton;

	toolButton * m_drawButton;
	toolButton * m_eraseButton;
	toolButton * m_selectButton;
	toolButton * m_moveButton;

	toolButton * m_cutButton;
	toolButton * m_copyButton;
	toolButton * m_pasteButton;

	comboBox * m_zoomingXComboBox;
	comboBox * m_zoomingYComboBox;
	comboBox * m_quantizeComboBox;

	comboBoxModel m_zoomingXModel;
	comboBoxModel m_zoomingYModel;
	comboBoxModel m_quantizeModel;

	automationPattern * m_pattern;
	float m_minLevel;
	float m_maxLevel;
	float m_step;
	float m_scrollLevel;
	float m_bottomLevel;
	float m_topLevel;

	void updateTopBottomLevels( void );

	QScrollBar * m_leftRightScroll;
	QScrollBar * m_topBottomScroll;

	midiTime m_currentPosition;

	Actions m_action;

	tick m_selectStartTick;
	tick m_selectedTick;
	float m_selectStartLevel;
	float m_selectedLevels;

	float m_moveStartLevel;
	tick m_moveStartTick;
	int m_moveXOffset;

	float m_drawLastLevel;
	tick m_drawLastTick;

	int m_ppt;
	int m_y_delta;
	bool m_y_auto;

	timeMap m_valuesToCopy;
	timeMap m_selValuesForMove;


	EditModes m_editMode;


	timeLine * m_timeLine;
	bool m_scrollBack;

	void drawCross( QPainter & _p );
	bool inBBEditor( void );



	friend class engine;


signals:
	void positionChanged( const midiTime & );

} ;


#endif

