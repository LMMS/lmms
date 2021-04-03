/*
 * AutomationEditor.cpp - implementation of AutomationEditor which is used for
 *						actual setting of dynamic values
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2008-2013 Paul Giblock <pgib/at/users.sourceforge.net>
 * Copyright (c) 2006-2008 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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

#include "AutomationEditor.h"

#include <cmath>

#include <QApplication>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QMdiArea>
#include <QPainter>
#include <QPainterPath>
#include <QScrollBar>
#include <QStyleOption>
#include <QToolTip>

#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif

#include "ActionGroup.h"
#include "AutomationNode.h"
#include "BBTrackContainer.h"
#include "ComboBox.h"
#include "debug.h"
#include "DeprecationHelper.h"
#include "embed.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "gui_templates.h"
#include "MainWindow.h"
#include "PianoRoll.h"
#include "ProjectJournal.h"
#include "SongEditor.h"
#include "StringPairDrag.h"
#include "TextFloat.h"
#include "TimeLineWidget.h"
#include "ToolTip.h"


QPixmap * AutomationEditor::s_toolDraw = nullptr;
QPixmap * AutomationEditor::s_toolErase = nullptr;
QPixmap * AutomationEditor::s_toolDrawOut = nullptr;
QPixmap * AutomationEditor::s_toolMove = nullptr;
QPixmap * AutomationEditor::s_toolYFlip = nullptr;
QPixmap * AutomationEditor::s_toolXFlip = nullptr;

const QVector<double> AutomationEditor::m_zoomXLevels =
		{ 0.125f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f };



AutomationEditor::AutomationEditor() :
	QWidget(),
	m_zoomingXModel(),
	m_zoomingYModel(),
	m_quantizeModel(),
	m_pattern(nullptr),
	m_minLevel( 0 ),
	m_maxLevel( 0 ),
	m_step( 1 ),
	m_scrollLevel( 0 ),
	m_bottomLevel( 0 ),
	m_topLevel( 0 ),
	m_currentPosition(),
	m_action( NONE ),
	m_drawLastLevel( 0.0f ),
	m_drawLastTick( 0 ),
	m_ppb( DEFAULT_PPB ),
	m_y_delta( DEFAULT_Y_DELTA ),
	m_y_auto( true ),
	m_editMode( DRAW ),
	m_mouseDownLeft(false),
	m_mouseDownRight( false ),
	m_scrollBack( false ),
	m_barLineColor(0, 0, 0),
	m_beatLineColor(0, 0, 0),
	m_lineColor(0, 0, 0),
	m_graphColor(Qt::SolidPattern),
	m_nodeInValueColor(0, 0, 0),
	m_nodeOutValueColor(0, 0, 0),
	m_scaleColor(Qt::SolidPattern),
	m_crossColor(0, 0, 0),
	m_backgroundShade(0, 0, 0)
{
	connect( this, SIGNAL( currentPatternChanged() ),
				this, SLOT( updateAfterPatternChange() ),
				Qt::QueuedConnection );
	connect( Engine::getSong(), SIGNAL( timeSignatureChanged( int, int ) ),
						this, SLOT( update() ) );

	setAttribute( Qt::WA_OpaquePaintEvent, true );

	//keeps the direction of the widget, undepended on the locale
	setLayoutDirection( Qt::LeftToRight );

	m_tensionModel = new FloatModel(1.0, 0.0, 1.0, 0.01);
	connect( m_tensionModel, SIGNAL( dataChanged() ),
				this, SLOT( setTension() ) );

	for (auto q : Quantizations) {
		m_quantizeModel.addItem(QString("1/%1").arg(q));
	}

	connect( &m_quantizeModel, SIGNAL(dataChanged() ),
					this, SLOT( setQuantization() ) );
	m_quantizeModel.setValue( m_quantizeModel.findText( "1/8" ) );

	if (s_toolYFlip == nullptr)
	{
		s_toolYFlip = new QPixmap( embed::getIconPixmap(
							"flip_y" ) );
	}
	if (s_toolXFlip == nullptr)
	{
		s_toolXFlip = new QPixmap( embed::getIconPixmap(
							"flip_x" ) );
	}

	// add time-line
	m_timeLine = new TimeLineWidget( VALUES_WIDTH, 0, m_ppb,
				Engine::getSong()->getPlayPos(
					Song::Mode_PlayAutomationPattern ),
					m_currentPosition,
					Song::Mode_PlayAutomationPattern, this );
	connect( this, SIGNAL( positionChanged( const TimePos & ) ),
		m_timeLine, SLOT( updatePosition( const TimePos & ) ) );
	connect( m_timeLine, SIGNAL( positionChanged( const TimePos & ) ),
			this, SLOT( updatePosition( const TimePos & ) ) );

	// init scrollbars
	m_leftRightScroll = new QScrollBar( Qt::Horizontal, this );
	m_leftRightScroll->setSingleStep( 1 );
	connect( m_leftRightScroll, SIGNAL( valueChanged( int ) ), this,
						SLOT( horScrolled( int ) ) );

	m_topBottomScroll = new QScrollBar( Qt::Vertical, this );
	m_topBottomScroll->setSingleStep( 1 );
	m_topBottomScroll->setPageStep( 20 );
	connect( m_topBottomScroll, SIGNAL( valueChanged( int ) ), this,
						SLOT( verScrolled( int ) ) );

	// init pixmaps
	if (s_toolDraw == nullptr)
	{
		s_toolDraw = new QPixmap(embed::getIconPixmap("edit_draw"));
	}
	if (s_toolErase == nullptr)
	{
		s_toolErase= new QPixmap(embed::getIconPixmap("edit_erase"));
	}
	if (s_toolDrawOut == nullptr)
	{
		s_toolDrawOut = new QPixmap(embed::getIconPixmap("edit_draw_outvalue"));
	}
	if (s_toolMove == nullptr)
	{
		s_toolMove = new QPixmap(embed::getIconPixmap("edit_move"));
	}

	setCurrentPattern(nullptr);

	setMouseTracking( true );
	setFocusPolicy( Qt::StrongFocus );
	setFocus();
}




AutomationEditor::~AutomationEditor()
{
	m_zoomingXModel.disconnect();
	m_zoomingYModel.disconnect();
	m_quantizeModel.disconnect();
	m_tensionModel->disconnect();

	delete m_tensionModel;
}




void AutomationEditor::setCurrentPattern(AutomationPattern * new_pattern )
{
	if (m_pattern)
	{
		m_pattern->disconnect(this);
	}

	m_pattern = new_pattern;

	if (m_pattern != nullptr)
	{
		connect(m_pattern, SIGNAL(dataChanged()), this, SLOT(update()));
	}

	emit currentPatternChanged();
}




void AutomationEditor::saveSettings(QDomDocument & doc, QDomElement & dom_parent)
{
	MainWindow::saveWidgetState( parentWidget(), dom_parent );
}




void AutomationEditor::loadSettings( const QDomElement & dom_parent)
{
	MainWindow::restoreWidgetState(parentWidget(), dom_parent);
}




void AutomationEditor::updateAfterPatternChange()
{
	m_currentPosition = 0;

	if( !validPattern() )
	{
		m_minLevel = m_maxLevel = m_scrollLevel = 0;
		m_step = 1;
		resizeEvent(nullptr);
		return;
	}

	m_minLevel = m_pattern->firstObject()->minValue<float>();
	m_maxLevel = m_pattern->firstObject()->maxValue<float>();
	m_step = m_pattern->firstObject()->step<float>();
	centerTopBottomScroll();

	m_tensionModel->setValue( m_pattern->getTension() );

	// resizeEvent() does the rest for us (scrolling, range-checking
	// of levels and so on...)
	resizeEvent(nullptr);

	update();
}




void AutomationEditor::update()
{
	QWidget::update();

	// Note detuning?
	if( m_pattern && !m_pattern->getTrack() )
	{
		gui->pianoRoll()->update();
	}
}




void AutomationEditor::keyPressEvent(QKeyEvent * ke )
{
	switch( ke->key() )
	{
		case Qt::Key_Up:
			m_topBottomScroll->setValue(
					m_topBottomScroll->value() - 1 );
			ke->accept();
			break;

		case Qt::Key_Down:
			m_topBottomScroll->setValue(
					m_topBottomScroll->value() + 1 );
			ke->accept();
			break;

		case Qt::Key_Left:
			if( ( m_timeLine->pos() -= 16 ) < 0 )
			{
				m_timeLine->pos().setTicks( 0 );
			}
			m_timeLine->updatePosition();
			ke->accept();
			break;

		case Qt::Key_Right:
			m_timeLine->pos() += 16;
			m_timeLine->updatePosition();
			ke->accept();
			break;

		case Qt::Key_Home:
			m_timeLine->pos().setTicks( 0 );
			m_timeLine->updatePosition();
			ke->accept();
			break;

		default:
			break;
	}
}




void AutomationEditor::leaveEvent(QEvent * e )
{
	while (QApplication::overrideCursor() != nullptr)
	{
		QApplication::restoreOverrideCursor();
	}
	QWidget::leaveEvent( e );
	update();
}


void AutomationEditor::drawLine( int x0In, float y0, int x1In, float y1 )
{
	int x0 = Note::quantized( x0In, AutomationPattern::quantization() );
	int x1 = Note::quantized( x1In, AutomationPattern::quantization() );
	int deltax = qAbs( x1 - x0 );
	float deltay = qAbs<float>( y1 - y0 );
	int x = x0;
	float y = y0;
	int xstep;
	int ystep;

	if( deltax < AutomationPattern::quantization() )
	{
		return;
	}

	deltax /= AutomationPattern::quantization();

	float yscale = deltay / ( deltax );

	if( x0 < x1 )
	{
		xstep = AutomationPattern::quantization();
	}
	else
	{
		xstep = -( AutomationPattern::quantization() );
	}

	float lineAdjust;
	if( y0 < y1 )
	{
		ystep = 1;
		lineAdjust = yscale;
	}
	else
	{
		ystep = -1;
		lineAdjust = -( yscale );
	}

	int i = 0;
	while( i < deltax )
	{
		y = y0 + ( ystep * yscale * i ) + lineAdjust;

		x += xstep;
		i += 1;
		m_pattern->removeNode(TimePos(x));
		m_pattern->putValue( TimePos( x ), y );
	}
}




bool AutomationEditor::fineTuneValue(timeMap::iterator node, bool editingOutValue)
{
	if (node == m_pattern->getTimeMap().end()) { return false; }

	// Display dialog to edit the value
	bool ok;
	double value = QInputDialog::getDouble(
		this,
		tr("Edit Value"),
		editingOutValue
			? tr("New outValue")
			: tr("New inValue"),
		editingOutValue
			? OUTVAL(node)
			: INVAL(node),
		m_pattern->firstObject()->minValue<float>(),
		m_pattern->firstObject()->maxValue<float>(),
		3,
		&ok
	);

	// If dialog failed return false
	if (!ok) { return false; }

	// Set the new inValue/outValue
	if (editingOutValue)
	{
		node.value().setOutValue(value);
	}
	else
	{
		// If the outValue is equal to the inValue we
		// set both to the given value
		if (OFFSET(node) == 0)
		{
			node.value().setOutValue(value);
		}
		node.value().setInValue(value);
	}

	Engine::getSong()->setModified();
	return true;
}




void AutomationEditor::mousePressEvent( QMouseEvent* mouseEvent )
{
	if( !validPattern() )
	{
		return;
	}

	// Some helper lambda functions to avoid repetition of code
	auto eraseNode = [this](timeMap::iterator node)
	{
		if (node != m_pattern->getTimeMap().end())
		{
			m_pattern->removeNode(POS(node));
			Engine::getSong()->setModified();
		}
	};
	auto resetNode = [this](timeMap::iterator node)
	{
		if (node != m_pattern->getTimeMap().end())
		{
			node.value().resetOutValue();
			Engine::getSong()->setModified();
		}
	};

	// If we clicked inside the AutomationEditor viewport (where the nodes are represented)
	if (mouseEvent->y() > TOP_MARGIN && mouseEvent->x() >= VALUES_WIDTH)
	{
		float level = getLevel( mouseEvent->y() );

		// Get the viewport X
		int x = mouseEvent->x() - VALUES_WIDTH;

		// Get tick in which the user clicked
		int posTicks = (x * TimePos::ticksPerBar() / m_ppb) + m_currentPosition;

		// Get the time map of current pattern
		timeMap & tm = m_pattern->getTimeMap();

		m_mouseDownLeft = (mouseEvent->button() == Qt::LeftButton);
		m_mouseDownRight = (mouseEvent->button() == Qt::RightButton);

		// Some actions require that we know if we clicked the inValue of
		// a node, while others require that we know if we clicked the outValue
		// of a node.
		bool editingOutValue = (
			m_editMode == DRAW_OUTVALUES
			|| (m_editMode == ERASE && m_mouseDownRight)
		);

		timeMap::iterator clickedNode = getNodeAt(mouseEvent->x(), mouseEvent->y(), editingOutValue);

		switch (m_editMode)
		{
			case DRAW:
			{
				m_pattern->addJournalCheckPoint();

				if (m_mouseDownLeft)
				{
					// If we are pressing shift, make a line of nodes from the last
					// clicked position to this position
					if (mouseEvent->modifiers() & Qt::ShiftModifier)
					{
						drawLine(m_drawLastTick, m_drawLastLevel,
							posTicks, level);

						// Update the last clicked position for the next time
						m_drawLastTick = posTicks;
						m_drawLastLevel = level;

						// Changes the action to drawing a line of nodes
						m_action = DRAW_LINE;
					}
					else // No shift, we are just creating/moving nodes
					{
						// Starts actually moving/draging the node
						TimePos newTime = m_pattern->setDragValue(
							// The TimePos of either the clicked node or a new one
							TimePos(
								clickedNode == tm.end()
								? posTicks
								: POS(clickedNode)
							),
							level,
							true,
							mouseEvent->modifiers() & Qt::ControlModifier
						);

						// We need to update our iterator because we are either creating a new node
						// or dragging an existing one. In the latter, setDragValue removes the node that
						// is being dragged, so if we don't update it we have a bogus iterator
						clickedNode = tm.find(newTime);

						// Set the action to MOVE_VALUE so moveMouseEvent() knows we are moving a node
						m_action = MOVE_VALUE;

						// Calculate the offset from the place the mouse click happened in comparison
						// to the center of the node
						int alignedX = (POS(clickedNode) - m_currentPosition) * m_ppb / TimePos::ticksPerBar();
						m_moveXOffset = x - alignedX - 1;

						// Set move-cursor
						QCursor c(Qt::SizeAllCursor);
						QApplication::setOverrideCursor(c);

						// Update the last clicked position so it can be used if we draw a line later
						m_drawLastTick = posTicks;
						m_drawLastLevel = level;
					}

					Engine::getSong()->setModified();
				}
				else if (m_mouseDownRight) // Right click on DRAW mode erases values
				{
					// Update the last clicked position so we remove all nodes from
					// that point up to the point we release the mouse button
					m_drawLastTick = posTicks;

					// If we right-clicked a node, remove it
					eraseNode(clickedNode);

					m_action = ERASE_VALUES;
				}
			break;
			}
			case ERASE:
			{
				m_pattern->addJournalCheckPoint();

				// On erase mode, left click removes nodes
				if (m_mouseDownLeft)
				{
					// Update the last clicked position so we remove all nodes from
					// that point up to the point we release the mouse button
					m_drawLastTick = posTicks;

					// If we right-clicked a node, remove it
					eraseNode(clickedNode);

					m_action = ERASE_VALUES;
				}
				else if (m_mouseDownRight) // And right click resets outValues
				{
					// If we clicked an outValue reset it
					resetNode(clickedNode);

					// Update the last clicked position so we reset all outValues from
					// that point up to the point we release the mouse button
					m_drawLastTick = posTicks;

					m_action = RESET_OUTVALUES;
				}
			break;
			}
			case DRAW_OUTVALUES:
			{
				m_pattern->addJournalCheckPoint();

				// On this mode, left click sets the outValue
				if (m_mouseDownLeft)
				{
					// If we clicked an outValue
					if (clickedNode != tm.end())
					{
						m_draggedOutValueKey = POS(clickedNode);

						clickedNode.value().setOutValue(level);

						m_action = MOVE_OUTVALUE;

						Engine::getSong()->setModified();
					}
					else // If we didn't click an outValue
					{
						// We check if the quantized position of the time we clicked has a
						// node and set its outValue
						TimePos quantizedPos = Note::quantized(
							TimePos(posTicks),
							m_pattern->quantization()
						);

						clickedNode = tm.find(quantizedPos);

						if (clickedNode != tm.end())
						{
							m_draggedOutValueKey = POS(clickedNode);
							clickedNode.value().setOutValue(level);

							m_action = MOVE_OUTVALUE;

							Engine::getSong()->setModified();
						}
					}
				}
				else if (m_mouseDownRight) // Right click resets outValues
				{
					// If we clicked an outValue reset it
					resetNode(clickedNode);

					// Update the last clicked position so we reset all outValues from
					// that point up to the point we release the mouse button
					m_drawLastTick = posTicks;

					m_action = RESET_OUTVALUES;
				}
			break;
			}
		}

		update();
	}
}




void AutomationEditor::mouseDoubleClickEvent(QMouseEvent * mouseEvent)
{
	if (!validPattern()) { return; }

	// If we double clicked outside the AutomationEditor viewport return
	if (mouseEvent->y() <= TOP_MARGIN || mouseEvent->x() < VALUES_WIDTH) { return; }

	// Are we fine tuning the inValue or outValue?
	const bool isOutVal = (m_editMode == DRAW_OUTVALUES);
	timeMap::iterator clickedNode = getNodeAt(mouseEvent->x(), mouseEvent->y(), isOutVal);

	switch (m_editMode)
	{
		case DRAW:
		case DRAW_OUTVALUES:
			if (fineTuneValue(clickedNode, isOutVal)) { update(); }
			break;
		default:
			break;
	}
}




void AutomationEditor::mouseReleaseEvent(QMouseEvent * mouseEvent )
{
	bool mustRepaint = false;

	if (mouseEvent->button() == Qt::LeftButton)
	{
		m_mouseDownLeft = false;
		mustRepaint = true;
	}
	if (mouseEvent->button() == Qt::RightButton)
	{
		m_mouseDownRight = false;
		mustRepaint = true;
	}

	if (m_editMode == DRAW)
	{
		if (m_action == MOVE_VALUE)
		{
			// Actually apply the value of the node being dragged
			m_pattern->applyDragValue();
		}

		QApplication::restoreOverrideCursor();
	}

	m_action = NONE;

	if (mustRepaint) { repaint(); }
}




void AutomationEditor::mouseMoveEvent(QMouseEvent * mouseEvent )
{
	if( !validPattern() )
	{
		update();
		return;
	}

	// If the mouse y position is inside the Automation Editor viewport
	if (mouseEvent->y() > TOP_MARGIN)
	{
		float level = getLevel(mouseEvent->y());
		// Get the viewport X position where the mouse is at
		int x = mouseEvent->x() - VALUES_WIDTH;

		// Get the X position in ticks
		int posTicks = (x * TimePos::ticksPerBar() / m_ppb) + m_currentPosition;

		switch (m_editMode)
		{
			case DRAW:
			{
				// We are dragging a node
				if (m_mouseDownLeft)
				{
					if (m_action == MOVE_VALUE)
					{
						// When we clicked the node, we might have clicked slightly off
						// so we account for that offset for a smooth drag
						x -= m_moveXOffset;
						posTicks = (x * TimePos::ticksPerBar() / m_ppb) + m_currentPosition;

						// If we moved the mouse past the beginning correct the position in ticks
						posTicks = qMax(posTicks, 0);

						m_drawLastTick = posTicks;
						m_drawLastLevel = level;

						// Updates the drag value of the moved node
						m_pattern->setDragValue(
							TimePos(posTicks),
							level,
							true,
							mouseEvent->modifiers() & Qt::ControlModifier
						);

						Engine::getSong()->setModified();
					}
					/* else if (m_action == DRAW_LINE)
					{
						// We are drawing a line. For now do nothing (as before), but later logic
						// could be added here so the line is updated according to the new mouse position
						// until the button is released
					}*/
				}
				else if (m_mouseDownRight) // We are removing nodes
				{
					if (m_action == ERASE_VALUES)
					{
						// If we moved the mouse past the beginning correct the position in ticks
						posTicks = qMax(posTicks, 0);

						// Removing automation nodes

						// Removes all values from the last clicked tick up to the current position tick
						m_pattern->removeNodes(m_drawLastTick, posTicks);

						Engine::getSong()->setModified();
					}
				}
			break;
			}
			case ERASE:
			{
				// If we moved the mouse past the beginning correct the position in ticks
				posTicks = qMax(posTicks, 0);

				// Left button removes nodes
				if (m_mouseDownLeft)
				{
					if (m_action == ERASE_VALUES)
					{
						// Removing automation nodes

						// Removes all values from the last clicked tick up to the current position tick
						m_pattern->removeNodes(m_drawLastTick, posTicks);

						Engine::getSong()->setModified();
					}
				}
				else if (m_mouseDownRight) // Right button resets outValues
				{
					if (m_action == RESET_OUTVALUES)
					{
						// Reseting outValues

						// Resets all values from the last clicked tick up to the current position tick
						m_pattern->resetNodes(m_drawLastTick, posTicks);

						Engine::getSong()->setModified();
					}
				}
			break;
			}
			case DRAW_OUTVALUES:
			{
				// If we moved the mouse past the beginning correct the position in ticks
				posTicks = qMax(posTicks, 0);

				// Left button moves outValues
				if (m_mouseDownLeft)
				{
					if (m_action == MOVE_OUTVALUE)
					{
						// We are moving the outValue of the node
						timeMap & tm = m_pattern->getTimeMap();

						timeMap::iterator it = tm.find(m_draggedOutValueKey);
						// Safety check
						if (it != tm.end())
						{
							it.value().setOutValue(level);
							Engine::getSong()->setModified();
						}
					}
				}
				else if (m_mouseDownRight) // Right button resets them
				{
					if (m_action == RESET_OUTVALUES)
					{
						// Reseting outValues

						// Resets all values from the last clicked tick up to the current position tick
						m_pattern->resetNodes(m_drawLastTick, posTicks);

						Engine::getSong()->setModified();
					}
				}
			break;
			}
		}
	}
	else // If the mouse Y position is above the AutomationEditor viewport
	{
		QApplication::restoreOverrideCursor();
	}

	update();
}




inline void AutomationEditor::drawCross( QPainter & p )
{
	QPoint mouse_pos = mapFromGlobal( QCursor::pos() );
	int grid_bottom = height() - SCROLLBAR_SIZE - 1;
	float level = getLevel( mouse_pos.y() );
	float cross_y = m_y_auto ?
		grid_bottom - ( ( grid_bottom - TOP_MARGIN )
				* ( level - m_minLevel )
				/ (float)( m_maxLevel - m_minLevel ) ) :
		grid_bottom - ( level - m_bottomLevel ) * m_y_delta;

	p.setPen(m_crossColor);
	p.drawLine( VALUES_WIDTH, (int) cross_y, width(), (int) cross_y );
	p.drawLine( mouse_pos.x(), TOP_MARGIN, mouse_pos.x(), height() - SCROLLBAR_SIZE );


	QPoint tt_pos =  QCursor::pos();
	tt_pos.ry() -= 51;
	tt_pos.rx() += 26;

	float scaledLevel = m_pattern->firstObject()->scaledValue( level );

	// Limit the scaled-level tooltip to the grid
	if( mouse_pos.x() >= 0 &&
		mouse_pos.x() <= width() - SCROLLBAR_SIZE &&
		mouse_pos.y() >= 0 &&
		mouse_pos.y() <= height() - SCROLLBAR_SIZE )
	{
		QToolTip::showText( tt_pos, QString::number( scaledLevel ), this );
	}
}




inline void AutomationEditor::drawAutomationPoint(QPainter & p, timeMap::iterator it)
{
	int x = xCoordOfTick(POS(it));
	int y;
	// Below (m_ppb * AutomationPattern::quantization() / 576) is used because:
	// 1 bar equals to 192/quantization() notes. Hence, to calculate the number of pixels
	// per note we would have (m_ppb * 1 bar / (192/quantization()) notes per bar), or
	// (m_ppb * quantization / 192). If we want 1/3 of the number of pixels per note we
	// get (m_ppb * quantization() / 192*3) or (m_ppb * quantization() / 576)
	const int outerRadius = qBound(3, (m_ppb * AutomationPattern::quantization()) / 576, 5);

	// Draw a circle for the outValue
	y = yCoordOfLevel(OUTVAL(it));
	p.setPen(QPen(m_nodeOutValueColor.lighter(200)));
	p.setBrush(QBrush(m_nodeOutValueColor));
	p.drawEllipse(x - outerRadius, y - outerRadius, outerRadius * 2, outerRadius * 2);

	// Draw a circle for the inValue
	y = yCoordOfLevel(INVAL(it));
	p.setPen(QPen(m_nodeInValueColor.lighter(200)));
	p.setBrush(QBrush(m_nodeInValueColor));
	p.drawEllipse(x - outerRadius, y - outerRadius, outerRadius * 2, outerRadius * 2);
}




void AutomationEditor::paintEvent(QPaintEvent * pe )
{
	QStyleOption opt;
	opt.initFrom( this );
	QPainter p( this );
	style()->drawPrimitive( QStyle::PE_Widget, &opt, &p, this );

	// get foreground color
	QColor fgColor = p.pen().brush().color();
	// get background color and fill background
	QBrush bgColor = p.background();
	p.fillRect( 0, 0, width(), height(), bgColor );

	// set font-size to 8
	p.setFont( pointSize<8>( p.font() ) );

	int grid_height = height() - TOP_MARGIN - SCROLLBAR_SIZE;

	// start drawing at the bottom
	int grid_bottom = height() - SCROLLBAR_SIZE - 1;

	p.fillRect(0, TOP_MARGIN, VALUES_WIDTH, height() - TOP_MARGIN, m_scaleColor);

	// print value numbers
	int font_height = p.fontMetrics().height();
	Qt::Alignment text_flags =
		(Qt::Alignment)( Qt::AlignRight | Qt::AlignVCenter );

	if( validPattern() )
	{
		if( m_y_auto )
		{
			int y[] = { grid_bottom, TOP_MARGIN + font_height / 2 };
			float level[] = { m_minLevel, m_maxLevel };
			for( int i = 0; i < 2; ++i )
			{
				const QString & label = m_pattern->firstObject()
						->displayValue( level[i] );
				p.setPen( QApplication::palette().color( QPalette::Active,
							QPalette::Shadow ) );
				p.drawText( 1, y[i] - font_height + 1,
					VALUES_WIDTH - 10, 2 * font_height,
					text_flags, label );
				p.setPen( fgColor );
				p.drawText( 0, y[i] - font_height,
					VALUES_WIDTH - 10, 2 * font_height,
					text_flags, label );
			}
		}
		else
		{
			int y;
			int level = (int) m_bottomLevel;
			int printable = qMax( 1, 5 * DEFAULT_Y_DELTA
								/ m_y_delta );
			int module = level % printable;
			if( module )
			{
				int inv_module = ( printable - module )
								% printable;
				level += inv_module;
			}
			for( ; level <= m_topLevel; level += printable )
			{
				const QString & label = m_pattern->firstObject()
							->displayValue( level );
				y = yCoordOfLevel( level );
				p.setPen( QApplication::palette().color( QPalette::Active,
							QPalette::Shadow ) );
				p.drawText( 1, y - font_height + 1,
					VALUES_WIDTH - 10, 2 * font_height,
					text_flags, label );
				p.setPen( fgColor );
				p.drawText( 0, y - font_height,
					VALUES_WIDTH - 10, 2 * font_height,
					text_flags, label );
			}
		}
	}

	// set clipping area, because we are not allowed to paint over
	// keyboard...
	p.setClipRect( VALUES_WIDTH, TOP_MARGIN, width() - VALUES_WIDTH,
								grid_height  );

	// draw vertical raster

	if( m_pattern )
	{
		int tick, x, q;
		int x_line_end = (int)( m_y_auto || m_topLevel < m_maxLevel ?
			TOP_MARGIN :
			grid_bottom - ( m_topLevel - m_bottomLevel ) * m_y_delta );

		if( m_zoomingXModel.value() > 3 )
		{
			// If we're over 100% zoom, we allow all quantization level grids
			q = AutomationPattern::quantization();
		}
		else if( AutomationPattern::quantization() % 3 != 0 )
		{
			// If we're under 100% zoom, we allow quantization grid up to 1/24 for triplets
			// to ensure a dense doesn't fill out the background
			q = AutomationPattern::quantization() < 8 ? 8 : AutomationPattern::quantization();
		}
		else {
			// If we're under 100% zoom, we allow quantization grid up to 1/32 for normal notes
			q = AutomationPattern::quantization() < 6 ? 6 : AutomationPattern::quantization();
		}

		// 3 independent loops, because quantization might not divide evenly into
		// exotic denominators (e.g. 7/11 time), which are allowed ATM.
		// First quantization grid...
		for( tick = m_currentPosition - m_currentPosition % q,
				 x = xCoordOfTick( tick );
			 x<=width();
			 tick += q, x = xCoordOfTick( tick ) )
		{
			p.setPen(m_lineColor);
			p.drawLine( x, grid_bottom, x, x_line_end );
		}

		/// \todo move this horizontal line drawing code into the same loop as the value ticks?
		if( m_y_auto )
		{
			QPen pen(m_beatLineColor);
			pen.setStyle( Qt::DotLine );
			p.setPen( pen );
			float y_delta = ( grid_bottom - TOP_MARGIN ) / 8.0f;
			for( int i = 1; i < 8; ++i )
			{
				int y = (int)( grid_bottom - i * y_delta );
				p.drawLine( VALUES_WIDTH, y, width(), y );
			}
		}
		else
		{
			float y;
			for( int level = (int)m_bottomLevel; level <= m_topLevel; level++)
			{
				y =  yCoordOfLevel( (float)level );

				p.setPen(level % 10 == 0 ? m_beatLineColor : m_lineColor);

				// draw level line
				p.drawLine( VALUES_WIDTH, (int) y, width(), (int) y );
			}
		}


		// alternating shades for better contrast
		float timeSignature = static_cast<float>( Engine::getSong()->getTimeSigModel().getNumerator() )
				/ static_cast<float>( Engine::getSong()->getTimeSigModel().getDenominator() );
		float zoomFactor = m_zoomXLevels[m_zoomingXModel.value()];
		//the bars which disappears at the left side by scrolling
		int leftBars = m_currentPosition * zoomFactor / TimePos::ticksPerBar();

		//iterates the visible bars and draw the shading on uneven bars
		for( int x = VALUES_WIDTH, barCount = leftBars; x < width() + m_currentPosition * zoomFactor / timeSignature; x += m_ppb, ++barCount )
		{
			if( ( barCount + leftBars )  % 2 != 0 )
			{
				p.fillRect(
					x - m_currentPosition * zoomFactor / timeSignature,
					TOP_MARGIN,
					m_ppb,
					height() - (SCROLLBAR_SIZE + TOP_MARGIN),
					m_backgroundShade
				);
			}
		}

		// Draw the beat grid
		int ticksPerBeat = DefaultTicksPerBar /
			Engine::getSong()->getTimeSigModel().getDenominator();

		for( tick = m_currentPosition - m_currentPosition % ticksPerBeat,
				 x = xCoordOfTick( tick );
			 x<=width();
			 tick += ticksPerBeat, x = xCoordOfTick( tick ) )
		{
			p.setPen(m_beatLineColor);
			p.drawLine( x, grid_bottom, x, x_line_end );
		}

		// and finally bars
		for( tick = m_currentPosition - m_currentPosition % TimePos::ticksPerBar(),
				 x = xCoordOfTick( tick );
			 x<=width();
			 tick += TimePos::ticksPerBar(), x = xCoordOfTick( tick ) )
		{
			p.setPen(m_barLineColor);
			p.drawLine( x, grid_bottom, x, x_line_end );
		}
	}



	// following code draws all visible values

	if( validPattern() )
	{
		//NEEDS Change in CSS
		//int len_ticks = 4;
		timeMap & time_map = m_pattern->getTimeMap();

		//Don't bother doing/rendering anything if there is no automation points
		if( time_map.size() > 0 )
		{
			timeMap::iterator it = time_map.begin();
			while( it+1 != time_map.end() )
			{
				// skip this section if it occurs completely before the
				// visible area
				int next_x = xCoordOfTick(POS(it+1));
				if( next_x < 0 )
				{
					++it;
					continue;
				}

				int x = xCoordOfTick(POS(it));
				if( x > width() )
				{
					break;
				}

				float *values = m_pattern->valuesAfter(POS(it));

				// We are creating a path to draw a polygon representing the values between two
				// nodes. When we have two nodes with discrete progression, we will basically have
				// a rectangle with the outValue of the first node (that's why nextValue will match
				// the outValue of the current node). When we have nodes with linear or cubic progression
				// the value of the end of the shape between the two nodes will be the inValue of
				// the next node.
				float nextValue;
				if( m_pattern->progressionType() == AutomationPattern::DiscreteProgression )
				{
					nextValue = OUTVAL(it);
				}
				else
				{
					nextValue = INVAL(it + 1);
				}

				p.setRenderHints( QPainter::Antialiasing, true );
				QPainterPath path;
				path.moveTo(QPointF(xCoordOfTick(POS(it)), yCoordOfLevel(0)));
				for (int i = 0; i < POS(it + 1) - POS(it); i++)
				{
					path.lineTo(QPointF(xCoordOfTick(POS(it) + i), yCoordOfLevel(values[i])));
				}
				path.lineTo(QPointF(xCoordOfTick(POS(it + 1)), yCoordOfLevel(nextValue)));
				path.lineTo(QPointF(xCoordOfTick(POS(it + 1)), yCoordOfLevel(0)));
				path.lineTo(QPointF(xCoordOfTick(POS(it)), yCoordOfLevel(0)));
				p.fillPath(path, m_graphColor);
				p.setRenderHints( QPainter::Antialiasing, false );
				delete [] values;

				// Draw circle
				drawAutomationPoint(p, it);

				++it;
			}

			for (
				int i = POS(it), x = xCoordOfTick(i);
				x <= width();
				i++, x = xCoordOfTick(i)
			)
			{
				// Draws the rectangle representing the value after the last node (for
				// that reason we use outValue).
				drawLevelTick(p, i, OUTVAL(it));
			}
			// Draw circle(the last one)
			drawAutomationPoint(p, it);
		}
	}
	else
	{
		QFont f = p.font();
		f.setBold( true );
		p.setFont( pointSize<14>( f ) );
		p.setPen( QApplication::palette().color( QPalette::Active,
							QPalette::BrightText ) );
		p.drawText( VALUES_WIDTH + 20, TOP_MARGIN + 40,
				width() - VALUES_WIDTH - 20 - SCROLLBAR_SIZE,
				grid_height - 40, Qt::TextWordWrap,
				tr( "Please open an automation pattern with "
					"the context menu of a control!" ) );
	}

	// TODO: Get this out of paint event
	int l = validPattern() ? (int) m_pattern->length() : 0;

	// reset scroll-range
	if( m_leftRightScroll->maximum() != l )
	{
		m_leftRightScroll->setRange( 0, l );
		m_leftRightScroll->setPageStep( l );
	}

	if(validPattern() && GuiApplication::instance()->automationEditor()->m_editor->hasFocus())
	{
		drawCross( p );
	}

	const QPixmap * cursor = nullptr;
	// draw current edit-mode-icon below the cursor
	switch( m_editMode )
	{
		case DRAW:
		{
			if (m_action == ERASE_VALUES) { cursor = s_toolErase; }
			else if (m_action == MOVE_VALUE) { cursor = s_toolMove; }
			else { cursor = s_toolDraw; }
			break;
		}
		case ERASE:
		{
			cursor = s_toolErase;
			break;
		}
		case DRAW_OUTVALUES:
		{
			if (m_action == RESET_OUTVALUES) { cursor = s_toolErase; }
			else if (m_action == MOVE_OUTVALUE) { cursor = s_toolMove; }
			else { cursor = s_toolDrawOut; }
			break;
		}
	}
	QPoint mousePosition = mapFromGlobal( QCursor::pos() );
	if (cursor != nullptr && mousePosition.y() > TOP_MARGIN + SCROLLBAR_SIZE)
	{
		p.drawPixmap( mousePosition + QPoint( 8, 8 ), *cursor );
	}
}




int AutomationEditor::xCoordOfTick(int tick )
{
	return VALUES_WIDTH + ( ( tick - m_currentPosition )
		* m_ppb / TimePos::ticksPerBar() );
}




float AutomationEditor::yCoordOfLevel(float level )
{
	int grid_bottom = height() - SCROLLBAR_SIZE - 1;
	if( m_y_auto )
	{
		return ( grid_bottom - ( grid_bottom - TOP_MARGIN ) * ( level - m_minLevel ) / ( m_maxLevel - m_minLevel ) );
	}
	else
	{
		return ( grid_bottom - ( level - m_bottomLevel ) * m_y_delta );
	}
}




void AutomationEditor::drawLevelTick(QPainter & p, int tick, float value)
{
	int grid_bottom = height() - SCROLLBAR_SIZE - 1;
	const int x = xCoordOfTick( tick );
	int rect_width = xCoordOfTick( tick+1 ) - x;

	// is the level in visible area?
	if( ( value >= m_bottomLevel && value <= m_topLevel )
			|| ( value > m_topLevel && m_topLevel >= 0 )
			|| ( value < m_bottomLevel && m_bottomLevel <= 0 ) )
	{
		int y_start = yCoordOfLevel( value );
		int rect_height;

		if( m_y_auto )
		{
			int y_end = (int)( grid_bottom
						+ ( grid_bottom - TOP_MARGIN )
						* m_minLevel
						/ ( m_maxLevel - m_minLevel ) );

			rect_height = y_end - y_start;
		}
		else
		{
			rect_height = (int)( value * m_y_delta );
		}

		QBrush currentColor = m_graphColor;

		p.fillRect( x, y_start, rect_width, rect_height, currentColor );
	}
#ifdef LMMS_DEBUG
	else
	{
		printf("not in range\n");
	}
#endif
}




// Center the vertical scroll position on the first object's inValue
void AutomationEditor::centerTopBottomScroll()
{
	// default to the m_scrollLevel position
	int pos = static_cast<int>(m_scrollLevel);
	// If a pattern exists...
	if (m_pattern)
	{
		// get time map of current pattern
		timeMap & time_map = m_pattern->getTimeMap();
		// If time_map is not empty...
		if (!time_map.empty())
		{
			// set the position to the inverted value ((max + min) - value)
			// If we set just (max - value), we're off by m_pattern's minimum
			pos = m_pattern->getMax() + m_pattern->getMin() - static_cast<int>(INVAL(time_map.begin()));
		}
	}
	m_topBottomScroll->setValue(pos);
}




// responsible for moving/resizing scrollbars after window-resizing
void AutomationEditor::resizeEvent(QResizeEvent * re)
{
	m_leftRightScroll->setGeometry( VALUES_WIDTH, height() - SCROLLBAR_SIZE,
							width() - VALUES_WIDTH,
							SCROLLBAR_SIZE );

	int grid_height = height() - TOP_MARGIN - SCROLLBAR_SIZE;
	m_topBottomScroll->setGeometry( width() - SCROLLBAR_SIZE, TOP_MARGIN,
						SCROLLBAR_SIZE, grid_height );

	int half_grid = grid_height / 2;
	int total_pixels = (int)( ( m_maxLevel - m_minLevel ) * m_y_delta + 1 );
	if( !m_y_auto && grid_height < total_pixels )
	{
		int min_scroll = (int)( m_minLevel + floorf( half_grid
							/ (float)m_y_delta ) );
		int max_scroll = (int)( m_maxLevel - (int)floorf( ( grid_height
					- half_grid ) / (float)m_y_delta ) );
		m_topBottomScroll->setRange( min_scroll, max_scroll );
	}
	else
	{
		m_topBottomScroll->setRange( (int) m_scrollLevel,
							(int) m_scrollLevel );
	}
	centerTopBottomScroll();

	if( Engine::getSong() )
	{
		Engine::getSong()->getPlayPos( Song::Mode_PlayAutomationPattern
					).m_timeLine->setFixedWidth( width() );
	}

	updateTopBottomLevels();
	update();
}




// TODO: Move this method up so it's closer to the other mouse events
void AutomationEditor::wheelEvent(QWheelEvent * we )
{
	we->accept();
	if( we->modifiers() & Qt::ControlModifier && we->modifiers() & Qt::ShiftModifier )
	{
		int y = m_zoomingYModel.value();
		if(we->angleDelta().y() > 0)
		{
			y++;
		}
		else if(we->angleDelta().y() < 0)
		{
			y--;
		}
		y = qBound( 0, y, m_zoomingYModel.size() - 1 );
		m_zoomingYModel.setValue( y );
	}
	else if( we->modifiers() & Qt::ControlModifier && we->modifiers() & Qt::AltModifier )
	{
		int q = m_quantizeModel.value();
		if((we->angleDelta().x() + we->angleDelta().y()) > 0) // alt + scroll becomes horizontal scroll on KDE
		{
			q--;
		}
		else if((we->angleDelta().x() + we->angleDelta().y()) < 0) // alt + scroll becomes horizontal scroll on KDE
		{
			q++;
		}
		q = qBound( 0, q, m_quantizeModel.size() - 1 );
		m_quantizeModel.setValue( q );
		update();
	}
	else if( we->modifiers() & Qt::ControlModifier )
	{
		int x = m_zoomingXModel.value();
		if(we->angleDelta().y() > 0)
		{
			x++;
		}
		else if(we->angleDelta().y() < 0)
		{
			x--;
		}
		x = qBound( 0, x, m_zoomingXModel.size() - 1 );

		int mouseX = (position( we ).x() - VALUES_WIDTH)* TimePos::ticksPerBar();
		// ticks based on the mouse x-position where the scroll wheel was used
		int ticks = mouseX / m_ppb;
		// what would be the ticks in the new zoom level on the very same mouse x
		int newTicks = mouseX / (DEFAULT_PPB * m_zoomXLevels[x]);

		// scroll so the tick "selected" by the mouse x doesn't move on the screen
		m_leftRightScroll->setValue(m_leftRightScroll->value() + ticks - newTicks);


		m_zoomingXModel.setValue( x );
	}

	// FIXME: Reconsider if determining orientation is necessary in Qt6.
	else if(abs(we->angleDelta().x()) > abs(we->angleDelta().y())) // scrolling is horizontal
	{
		m_leftRightScroll->setValue(m_leftRightScroll->value() -
							we->angleDelta().x() * 2 / 15);
	}
	else if(we->modifiers() & Qt::ShiftModifier)
	{
		m_leftRightScroll->setValue(m_leftRightScroll->value() -
							we->angleDelta().y() * 2 / 15);
	}
	else
	{
		m_topBottomScroll->setValue(m_topBottomScroll->value() -
							(we->angleDelta().x() + we->angleDelta().y()) / 30);
	}
}




float AutomationEditor::getLevel(int y )
{
	int level_line_y = height() - SCROLLBAR_SIZE - 1;
	// pressed level
	float level = roundf( ( m_bottomLevel + ( m_y_auto ?
			( m_maxLevel - m_minLevel ) * ( level_line_y - y )
					/ (float)( level_line_y - ( TOP_MARGIN + 2 ) ) :
			( level_line_y - y ) / (float)m_y_delta ) ) / m_step ) * m_step;
	// some range-checking-stuff
	level = qBound( m_bottomLevel, level, m_topLevel );

	return( level );
}




inline bool AutomationEditor::inBBEditor()
{
	return( validPattern() &&
				m_pattern->getTrack()->trackContainer() == Engine::getBBTrackContainer() );
}




void AutomationEditor::play()
{
	if( !validPattern() )
	{
		return;
	}

	if( !m_pattern->getTrack() )
	{
		if( Engine::getSong()->playMode() != Song::Mode_PlayPattern )
		{
			Engine::getSong()->stop();
			Engine::getSong()->playPattern( gui->pianoRoll()->currentPattern() );
		}
		else if( Engine::getSong()->isStopped() == false )
		{
			Engine::getSong()->togglePause();
		}
		else
		{
			Engine::getSong()->playPattern( gui->pianoRoll()->currentPattern() );
		}
	}
	else if( inBBEditor() )
	{
		Engine::getBBTrackContainer()->play();
	}
	else
	{
		if( Engine::getSong()->isStopped() == true )
		{
			Engine::getSong()->playSong();
		}
		else
		{
			Engine::getSong()->togglePause();
		}
	}
}




void AutomationEditor::stop()
{
	if( !validPattern() )
	{
		return;
	}
	if( m_pattern->getTrack() && inBBEditor() )
	{
		Engine::getBBTrackContainer()->stop();
	}
	else
	{
		Engine::getSong()->stop();
	}
	m_scrollBack = true;
}




void AutomationEditor::horScrolled(int new_pos )
{
	m_currentPosition = new_pos;
	emit positionChanged( m_currentPosition );
	update();
}




void AutomationEditor::verScrolled(int new_pos )
{
	m_scrollLevel = new_pos;
	updateTopBottomLevels();
	update();
}




void AutomationEditor::setEditMode(AutomationEditor::EditModes mode)
{
	if (m_editMode == mode)
		return;

	m_editMode = mode;

	update();
}




void AutomationEditor::setEditMode(int mode)
{
	setEditMode((AutomationEditor::EditModes) mode);
}




void AutomationEditor::setProgressionType(AutomationPattern::ProgressionTypes type)
{
	if (validPattern())
	{
		m_pattern->addJournalCheckPoint();
		m_pattern->setProgressionType(type);
		Engine::getSong()->setModified();
		update();
	}
}

void AutomationEditor::setProgressionType(int type)
{
	setProgressionType((AutomationPattern::ProgressionTypes) type);
}




void AutomationEditor::setTension()
{
	if ( m_pattern )
	{
		m_pattern->setTension( QString::number( m_tensionModel->value() ) );
		update();
	}
}




void AutomationEditor::updatePosition(const TimePos & t )
{
	if( ( Engine::getSong()->isPlaying() &&
			Engine::getSong()->playMode() ==
					Song::Mode_PlayAutomationPattern ) ||
							m_scrollBack == true )
	{
		const int w = width() - VALUES_WIDTH;
		if( t > m_currentPosition + w * TimePos::ticksPerBar() / m_ppb )
		{
			m_leftRightScroll->setValue( t.getBar() *
							TimePos::ticksPerBar() );
		}
		else if( t < m_currentPosition )
		{
			TimePos t_ = qMax( t - w * TimePos::ticksPerBar() *
					TimePos::ticksPerBar() / m_ppb, 0 );
			m_leftRightScroll->setValue( t_.getBar() *
							TimePos::ticksPerBar() );
		}
		m_scrollBack = false;
	}
}




void AutomationEditor::zoomingXChanged()
{
	m_ppb = m_zoomXLevels[m_zoomingXModel.value()] * DEFAULT_PPB;

	assert( m_ppb > 0 );

	m_timeLine->setPixelsPerBar( m_ppb );
	update();
}




void AutomationEditor::zoomingYChanged()
{
	const QString & zfac = m_zoomingYModel.currentText();
	m_y_auto = zfac == "Auto";
	if( !m_y_auto )
	{
		m_y_delta = zfac.left( zfac.length() - 1 ).toInt()
							* DEFAULT_Y_DELTA / 100;
	}
#ifdef LMMS_DEBUG
	assert( m_y_delta > 0 );
#endif
	resizeEvent(nullptr);
}




void AutomationEditor::setQuantization()
{
	AutomationPattern::setQuantization(DefaultTicksPerBar / Quantizations[m_quantizeModel.value()]);

	update();
}




void AutomationEditor::updateTopBottomLevels()
{
	if( m_y_auto )
	{
		m_bottomLevel = m_minLevel;
		m_topLevel = m_maxLevel;
		return;
	}

	int total_pixels = (int)( ( m_maxLevel - m_minLevel ) * m_y_delta + 1 );
	int grid_height = height() - TOP_MARGIN - SCROLLBAR_SIZE;
	int half_grid = grid_height / 2;

	if( total_pixels > grid_height )
	{
		int centralLevel = (int)( m_minLevel + m_maxLevel - m_scrollLevel );

		m_bottomLevel = centralLevel - ( half_grid
							/ (float)m_y_delta );
		if( m_bottomLevel < m_minLevel )
		{
			m_bottomLevel = m_minLevel;
			m_topLevel = m_minLevel + (int)floorf( grid_height
							/ (float)m_y_delta );
		}
		else
		{
			m_topLevel = m_bottomLevel + (int)floorf( grid_height
							/ (float)m_y_delta );
			if( m_topLevel > m_maxLevel )
			{
				m_topLevel = m_maxLevel;
				m_bottomLevel = m_maxLevel - (int)floorf(
					grid_height / (float)m_y_delta );
			}
		}
	}
	else
	{
		m_bottomLevel = m_minLevel;
		m_topLevel = m_maxLevel;
	}
}




/**
 * @brief Given a mouse coordinate, returns a timeMap::iterator that points to
 *        the first node inside a square of side "r" pixels from those
 *        coordinates. In simpler terms, returns the automation node on those
 *        coordinates.
 * @param Int X coordinate
 * @param Int Y coordinate
 * @param Boolean. True to check if the outValue of the node was clicked instead
 *        (defaults to false)
 * @param Int R distance in pixels
 * @return timeMap::iterator with the clicked node, or timeMap.end() if none was clicked.
 */
AutomationEditor::timeMap::iterator AutomationEditor::getNodeAt(int x, int y, bool outValue /* = false */, int r /* = 5 */)
{
	// Remove the VALUES_WIDTH from the x position, so we have the actual viewport x
	x -= VALUES_WIDTH;
	// Convert the x position to the position in ticks
	int posTicks = (x * TimePos::ticksPerBar() / m_ppb) + m_currentPosition;

	// Get our pattern timeMap and create a iterator so we can check the nodes
	timeMap & tm = m_pattern->getTimeMap();
	timeMap::iterator it = tm.begin();

	// ticksOffset is the number of ticks that match "r" pixels
	int ticksOffset = TimePos::ticksPerBar() * r / m_ppb;

	while (it != tm.end())
	{
		// The node we are checking is past the coordinates already, so the others will be too
		if (posTicks < POS(it) - ticksOffset) { break; }

		// If the x coordinate is within "r" pixels of the node's position
		// POS(it) - ticksOffset <= posTicks <= POS(it) + ticksOffset
		if (posTicks <= POS(it) + ticksOffset)
		{
			// The y position of the node
			float valueY = yCoordOfLevel(
				outValue
				? OUTVAL(it)
				: INVAL(it)
			);
			// If the y coordinate is within "r" pixels of the node's value
			if (y >= (valueY - r) && y <= (valueY + r))
			{
				return it;
			}
		}
		++it;
	}

	return tm.end();
}




AutomationEditorWindow::AutomationEditorWindow() :
	Editor(),
	m_editor(new AutomationEditor())
{
	setCentralWidget(m_editor);



	// Play/stop buttons
	m_playAction->setToolTip(tr( "Play/pause current pattern (Space)" ));

	m_stopAction->setToolTip(tr("Stop playing of current pattern (Space)"));

	// Edit mode buttons
	DropToolBar *editActionsToolBar = addDropToolBarToTop(tr("Edit actions"));

	ActionGroup* editModeGroup = new ActionGroup(this);
	QAction* drawAction = editModeGroup->addAction(embed::getIconPixmap("edit_draw"), tr("Draw mode (Shift+D)"));
	drawAction->setShortcut(Qt::SHIFT | Qt::Key_D);
	drawAction->setChecked(true);

	QAction* eraseAction = editModeGroup->addAction(embed::getIconPixmap("edit_erase"), tr("Erase mode (Shift+E)"));
	eraseAction->setShortcut(Qt::SHIFT | Qt::Key_E);

	QAction* drawOutAction = editModeGroup->addAction(embed::getIconPixmap("edit_draw_outvalue"), tr("Draw outValues mode (Shift+C)"));
	drawOutAction->setShortcut(Qt::SHIFT | Qt::Key_C);

	m_flipYAction = new QAction(embed::getIconPixmap("flip_y"), tr("Flip vertically"), this);
	m_flipXAction = new QAction(embed::getIconPixmap("flip_x"), tr("Flip horizontally"), this);

	connect(editModeGroup, SIGNAL(triggered(int)), m_editor, SLOT(setEditMode(int)));

	editActionsToolBar->addAction(drawAction);
	editActionsToolBar->addAction(eraseAction);
	editActionsToolBar->addAction(drawOutAction);
	editActionsToolBar->addAction(m_flipXAction);
	editActionsToolBar->addAction(m_flipYAction);

	// Interpolation actions
	DropToolBar *interpolationActionsToolBar = addDropToolBarToTop(tr("Interpolation controls"));

	ActionGroup* progression_type_group = new ActionGroup(this);

	m_discreteAction = progression_type_group->addAction(
				embed::getIconPixmap("progression_discrete"), tr("Discrete progression"));
	m_discreteAction->setChecked(true);

	m_linearAction = progression_type_group->addAction(
				embed::getIconPixmap("progression_linear"), tr("Linear progression"));
	m_cubicHermiteAction = progression_type_group->addAction(
				embed::getIconPixmap("progression_cubic_hermite"), tr( "Cubic Hermite progression"));

	connect(progression_type_group, SIGNAL(triggered(int)), m_editor, SLOT(setProgressionType(int)));

	// setup tension-stuff
	m_tensionKnob = new Knob( knobSmall_17, this, "Tension" );
	m_tensionKnob->setModel(m_editor->m_tensionModel);
	ToolTip::add(m_tensionKnob, tr("Tension value for spline"));

	connect(m_cubicHermiteAction, SIGNAL(toggled(bool)), m_tensionKnob, SLOT(setEnabled(bool)));

	interpolationActionsToolBar->addSeparator();
	interpolationActionsToolBar->addAction(m_discreteAction);
	interpolationActionsToolBar->addAction(m_linearAction);
	interpolationActionsToolBar->addAction(m_cubicHermiteAction);
	interpolationActionsToolBar->addSeparator();
	interpolationActionsToolBar->addWidget( new QLabel( tr("Tension: "), interpolationActionsToolBar ));
	interpolationActionsToolBar->addWidget( m_tensionKnob );



	addToolBarBreak();

	// Zoom controls
	DropToolBar *zoomToolBar = addDropToolBarToTop(tr("Zoom controls"));

	QLabel * zoom_x_label = new QLabel( zoomToolBar );
	zoom_x_label->setPixmap( embed::getIconPixmap( "zoom_x" ) );

	m_zoomingXComboBox = new ComboBox( zoomToolBar );
	m_zoomingXComboBox->setFixedSize( 80, ComboBox::DEFAULT_HEIGHT );
	m_zoomingXComboBox->setToolTip( tr( "Horizontal zooming" ) );

	for( float const & zoomLevel : m_editor->m_zoomXLevels )
	{
		m_editor->m_zoomingXModel.addItem( QString( "%1\%" ).arg( zoomLevel * 100 ) );
	}
	m_editor->m_zoomingXModel.setValue( m_editor->m_zoomingXModel.findText( "100%" ) );

	m_zoomingXComboBox->setModel( &m_editor->m_zoomingXModel );

	connect( &m_editor->m_zoomingXModel, SIGNAL( dataChanged() ),
			m_editor, SLOT( zoomingXChanged() ) );


	QLabel * zoom_y_label = new QLabel( zoomToolBar );
	zoom_y_label->setPixmap( embed::getIconPixmap( "zoom_y" ) );

	m_zoomingYComboBox = new ComboBox( zoomToolBar );
	m_zoomingYComboBox->setFixedSize( 80, ComboBox::DEFAULT_HEIGHT );
	m_zoomingYComboBox->setToolTip( tr( "Vertical zooming" ) );

	m_editor->m_zoomingYModel.addItem( "Auto" );
	for( int i = 0; i < 7; ++i )
	{
		m_editor->m_zoomingYModel.addItem( QString::number( 25 << i ) + "%" );
	}
	m_editor->m_zoomingYModel.setValue( m_editor->m_zoomingYModel.findText( "Auto" ) );

	m_zoomingYComboBox->setModel( &m_editor->m_zoomingYModel );

	connect( &m_editor->m_zoomingYModel, SIGNAL( dataChanged() ),
			m_editor, SLOT( zoomingYChanged() ) );

	zoomToolBar->addWidget( zoom_x_label );
	zoomToolBar->addWidget( m_zoomingXComboBox );
	zoomToolBar->addSeparator();
	zoomToolBar->addWidget( zoom_y_label );
	zoomToolBar->addWidget( m_zoomingYComboBox );

	// Quantization controls
	DropToolBar *quantizationActionsToolBar = addDropToolBarToTop(tr("Quantization controls"));

	QLabel * quantize_lbl = new QLabel( m_toolBar );
	quantize_lbl->setPixmap( embed::getIconPixmap( "quantize" ) );

	m_quantizeComboBox = new ComboBox( m_toolBar );
	m_quantizeComboBox->setFixedSize( 60, ComboBox::DEFAULT_HEIGHT );
	m_quantizeComboBox->setToolTip( tr( "Quantization" ) );

	m_quantizeComboBox->setModel( &m_editor->m_quantizeModel );

	quantizationActionsToolBar->addWidget( quantize_lbl );
	quantizationActionsToolBar->addWidget( m_quantizeComboBox );

	// Setup our actual window
	setFocusPolicy( Qt::StrongFocus );
	setFocus();
	setWindowIcon( embed::getIconPixmap( "automation" ) );
	setAcceptDrops( true );
	m_toolBar->setAcceptDrops( true );
}


AutomationEditorWindow::~AutomationEditorWindow()
{
}


void AutomationEditorWindow::setCurrentPattern(AutomationPattern* pattern)
{
	// Disconnect our old pattern
	if (currentPattern() != nullptr)
	{
		m_editor->m_pattern->disconnect(this);
		m_flipXAction->disconnect();
		m_flipYAction->disconnect();
	}

	m_editor->setCurrentPattern(pattern);

	// Set our window's title
	if (pattern == nullptr)
	{
		setWindowTitle( tr( "Automation Editor - no pattern" ) );
		return;
	}

	setWindowTitle( tr( "Automation Editor - %1" ).arg( m_editor->m_pattern->name() ) );


	switch(m_editor->m_pattern->progressionType())
	{
	case AutomationPattern::DiscreteProgression:
		m_discreteAction->setChecked(true);
		m_tensionKnob->setEnabled(false);
		break;
	case AutomationPattern::LinearProgression:
		m_linearAction->setChecked(true);
		m_tensionKnob->setEnabled(false);
		break;
	case AutomationPattern::CubicHermiteProgression:
		m_cubicHermiteAction->setChecked(true);
		m_tensionKnob->setEnabled(true);
		break;
	}

	// Connect new pattern
	if (pattern)
	{
		connect(pattern, SIGNAL(dataChanged()), this, SLOT(update()));
		connect( pattern, SIGNAL( dataChanged() ), this, SLOT( updateWindowTitle() ) );
		connect(pattern, SIGNAL(destroyed()), this, SLOT(clearCurrentPattern()));

		connect(m_flipXAction, SIGNAL(triggered()), pattern, SLOT(flipX()));
		connect(m_flipYAction, SIGNAL(triggered()), pattern, SLOT(flipY()));
	}

	emit currentPatternChanged();
}


const AutomationPattern* AutomationEditorWindow::currentPattern()
{
	return m_editor->currentPattern();
}

void AutomationEditorWindow::dropEvent( QDropEvent *_de )
{
	QString type = StringPairDrag::decodeKey( _de );
	QString val = StringPairDrag::decodeValue( _de );
	if( type == "automatable_model" )
	{
		AutomatableModel * mod = dynamic_cast<AutomatableModel *>(
				Engine::projectJournal()->
					journallingObject( val.toInt() ) );
		if (mod != nullptr)
		{
			bool added = m_editor->m_pattern->addObject( mod );
			if ( !added )
			{
				TextFloat::displayMessage( mod->displayName(),
							   tr( "Model is already connected "
							   "to this pattern." ),
							   embed::getIconPixmap( "automation" ),
							   2000 );
			}
			setCurrentPattern( m_editor->m_pattern );
		}
	}

	update();
}

void AutomationEditorWindow::dragEnterEvent( QDragEnterEvent *_dee )
{
	if (! m_editor->validPattern() ) {
		return;
	}
	StringPairDrag::processDragEnterEvent( _dee, "automatable_model" );
}

void AutomationEditorWindow::open(AutomationPattern* pattern)
{
	setCurrentPattern(pattern);
	parentWidget()->show();
	show();
	setFocus();
}

QSize AutomationEditorWindow::sizeHint() const
{
	return {INITIAL_WIDTH, INITIAL_HEIGHT};
}

void AutomationEditorWindow::clearCurrentPattern()
{
	m_editor->m_pattern = nullptr;
	setCurrentPattern(nullptr);
}

void AutomationEditorWindow::focusInEvent(QFocusEvent * event)
{
	m_editor->setFocus( event->reason() );
}

void AutomationEditorWindow::play()
{
	m_editor->play();
	setPauseIcon(Engine::getSong()->isPlaying());
}

void AutomationEditorWindow::stop()
{
	m_editor->stop();
}

void AutomationEditorWindow::updateWindowTitle()
{
	if (m_editor->m_pattern == nullptr)
	{
		setWindowTitle( tr( "Automation Editor - no pattern" ) );
		return;
	}

	setWindowTitle( tr( "Automation Editor - %1" ).arg( m_editor->m_pattern->name() ) );
}
