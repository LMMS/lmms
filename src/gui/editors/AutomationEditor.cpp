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

#include <QApplication>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>  // IWYU pragma: keep
#include <QPushButton>
#include <QScrollBar>
#include <QStyleOption>
#include <QToolTip>
#include <cmath>

#include "SampleClip.h"
#include "SampleThumbnail.h"

#include "ActionGroup.h"
#include "AutomationNode.h"
#include "ComboBox.h"
#include "DeprecationHelper.h"
#include "DetuningHelper.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "Knob.h"
#include "MainWindow.h"
#include "MidiClip.h"
#include "PatternStore.h"
#include "PianoRoll.h"
#include "ProjectJournal.h"
#include "StringPairDrag.h"
#include "TextFloat.h"
#include "TimeLineWidget.h"
#include "embed.h"
#include "FontHelper.h"


namespace lmms::gui
{
const std::array<float, 7> AutomationEditor::m_zoomXLevels =
		{ 0.125f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f };



AutomationEditor::AutomationEditor() :
	QWidget(),
	m_zoomingXModel(),
	m_zoomingYModel(),
	m_quantizeModel(),
	m_clip(nullptr),
	m_minLevel( 0 ),
	m_maxLevel( 0 ),
	m_step( 1 ),
	m_scrollLevel( 0 ),
	m_bottomLevel( 0 ),
	m_topLevel( 0 ),
	m_currentPosition(),
	m_action( Action::None ),
	m_drawLastLevel( 0.0f ),
	m_drawLastTick( 0 ),
	m_ppb( DEFAULT_PPB ),
	m_y_delta( DEFAULT_Y_DELTA ),
	m_y_auto( true ),
	m_editMode( EditMode::Draw ),
	m_mouseDownLeft(false),
	m_mouseDownRight( false ),
	m_scrollBack( false ),
	m_barLineColor(0, 0, 0),
	m_beatLineColor(0, 0, 0),
	m_lineColor(0, 0, 0),
	m_graphColor(Qt::SolidPattern),
	m_nodeInValueColor(0, 0, 0),
	m_nodeOutValueColor(0, 0, 0),
	m_nodeTangentLineColor(0, 0, 0),
	m_scaleColor(Qt::SolidPattern),
	m_crossColor(0, 0, 0),
	m_backgroundShade(0, 0, 0),
	m_ghostNoteColor(0, 0, 0),
	m_outOfBoundsShade(0, 0, 0, 128)
{
	connect( this, SIGNAL(currentClipChanged()),
				this, SLOT(updateAfterClipChange()),
				Qt::QueuedConnection );
	connect( Engine::getSong(), SIGNAL(timeSignatureChanged(int,int)),
						this, SLOT(update()));

	//keeps the direction of the widget, undepended on the locale
	setLayoutDirection( Qt::LeftToRight );

	// Set up tension model
	m_tensionModel = new FloatModel(1.f, 0.f, 1.f, 0.01f);
	m_tensionModel->setJournalling(false);
	connect( m_tensionModel, SIGNAL(dataChanged()),
				this, SLOT(setTension()));

	// Set up quantization model
	for (auto q : Quantizations) {
		m_quantizeModel.addItem(QString("1/%1").arg(q));
	}

	connect( &m_quantizeModel, SIGNAL(dataChanged()),
					this, SLOT(setQuantization()));
	m_quantizeModel.setValue( m_quantizeModel.findText( "1/8" ) );

	// add time-line
	m_timeLine = new TimeLineWidget(VALUES_WIDTH, 0, m_ppb,
		Engine::getSong()->getPlayPos(Song::PlayMode::AutomationClip),
		Engine::getSong()->getTimeline(Song::PlayMode::AutomationClip),
		m_currentPosition, Song::PlayMode::AutomationClip, this
	);
	connect(this, &AutomationEditor::positionChanged, m_timeLine, &TimeLineWidget::updatePosition);
	connect( m_timeLine, SIGNAL( positionChanged( const lmms::TimePos& ) ),
			this, SLOT( updatePosition( const lmms::TimePos& ) ) );

	// init scrollbars
	m_leftRightScroll = new QScrollBar( Qt::Horizontal, this );
	m_leftRightScroll->setSingleStep( 1 );
	connect( m_leftRightScroll, SIGNAL(valueChanged(int)), this,
						SLOT(horScrolled(int)));

	m_topBottomScroll = new QScrollBar( Qt::Vertical, this );
	m_topBottomScroll->setSingleStep( 1 );
	m_topBottomScroll->setPageStep( 20 );
	connect( m_topBottomScroll, SIGNAL(valueChanged(int)), this,
						SLOT(verScrolled(int)));

	setCurrentClip(nullptr);

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




void AutomationEditor::setCurrentClip(AutomationClip * new_clip )
{
	if (m_clip)
	{
		m_clip->disconnect(this);
	}

	m_clip = new_clip;

	if (m_clip != nullptr)
	{
		connect(m_clip, SIGNAL(dataChanged()), this, SLOT(update()));
		connect(m_clip, &AutomationClip::lengthChanged, this, qOverload<>(&QWidget::update));
	}

	emit currentClipChanged();
}




void AutomationEditor::saveSettings(QDomDocument & doc, QDomElement & dom_parent)
{
	MainWindow::saveWidgetState( parentWidget(), dom_parent );
}




void AutomationEditor::loadSettings( const QDomElement & dom_parent)
{
	MainWindow::restoreWidgetState(parentWidget(), dom_parent);
}




void AutomationEditor::updateAfterClipChange()
{
	m_currentPosition = 0;

	if( !validClip() )
	{
		m_minLevel = m_maxLevel = m_scrollLevel = 0;
		m_step = 1;
		resizeEvent(nullptr);
		return;
	}

	m_minLevel = m_clip->firstObject()->minValue<float>();
	m_maxLevel = m_clip->firstObject()->maxValue<float>();
	m_step = m_clip->firstObject()->step<float>();
	centerTopBottomScroll();

	m_tensionModel->setValue( m_clip->getTension() );

	// resizeEvent() does the rest for us (scrolling, range-checking
	// of levels and so on...)
	resizeEvent(nullptr);

	update();
}




void AutomationEditor::update()
{
	QWidget::update();

	// Note detuning?
	if( m_clip && !m_clip->getTrack() )
	{
		getGUI()->pianoRoll()->update();
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
			ke->ignore();
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
	int x0 = Note::quantized( x0In, AutomationClip::quantization() );
	int x1 = Note::quantized( x1In, AutomationClip::quantization() );
	int deltax = qAbs( x1 - x0 );
	auto deltay = qAbs<float>(y1 - y0);
	int x = x0;
	float y = y0;

	if( deltax < AutomationClip::quantization() )
	{
		return;
	}

	deltax /= AutomationClip::quantization();

	float yscale = deltay / ( deltax );

	int xstep = (x0 < x1 ? 1 : -1) * AutomationClip::quantization();
	int ystep = y0 < y1 ? 1 : -1;
	float lineAdjust = ystep * yscale;

	for (int i = 0; i < deltax; ++i)
	{
		y = y0 + ( ystep * yscale * i ) + lineAdjust;
		x += xstep;
		m_clip->removeNode(TimePos(x));
		m_clip->putValue( TimePos( x ), y );
	}
}




bool AutomationEditor::fineTuneValue(timeMap::iterator node, bool editingOutValue)
{
	if (node == m_clip->getTimeMap().end()) { return false; }

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
		m_clip->firstObject()->minValue<float>(),
		m_clip->firstObject()->maxValue<float>(),
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
	if( !validClip() )
	{
		return;
	}

	// Some helper lambda functions to avoid repetition of code
	auto eraseNode = [this](timeMap::iterator node)
	{
		if (node != m_clip->getTimeMap().end())
		{
			m_clip->removeNode(POS(node));
			Engine::getSong()->setModified();
		}
	};
	auto resetNode = [this](timeMap::iterator node)
	{
		if (node != m_clip->getTimeMap().end())
		{
			node.value().resetOutValue();
			Engine::getSong()->setModified();
		}
	};
	auto resetTangent = [this](timeMap::iterator node)
	{
		if (node != m_clip->getTimeMap().end())
		{
			// Unlock the tangents from that node
			node.value().setLockedTangents(false);
			// Recalculate the tangents
			m_clip->generateTangents(node, 1);
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

		// Get the time map of current clip
		timeMap & tm = m_clip->getTimeMap();

		m_mouseDownLeft = (mouseEvent->button() == Qt::LeftButton);
		m_mouseDownRight = (mouseEvent->button() == Qt::RightButton);

		// Some actions require that we know if we clicked the inValue of
		// a node, while others require that we know if we clicked the outValue
		// of a node.
		bool editingOutValue = (
			m_editMode == EditMode::DrawOutValues
			|| (m_editMode == EditMode::Erase && m_mouseDownRight)
		);

		timeMap::iterator clickedNode = getNodeAt(mouseEvent->x(), mouseEvent->y(), editingOutValue);

		switch (m_editMode)
		{
			case EditMode::Draw:
			{
				m_clip->addJournalCheckPoint();

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
						m_action = Action::DrawLine;
					}
					else // No shift, we are just creating/moving nodes
					{
						// Starts actually moving/draging the node
						TimePos newTime = m_clip->setDragValue(
							// The TimePos of either the clicked node or a new one
							TimePos(
								clickedNode == tm.end()
								? posTicks
								: POS(clickedNode)
							),
							level,
							clickedNode == tm.end(),
							mouseEvent->modifiers() & Qt::ControlModifier
						);

						// We need to update our iterator because we are either creating a new node
						// or dragging an existing one. In the latter, setDragValue removes the node that
						// is being dragged, so if we don't update it we have a bogus iterator
						clickedNode = tm.find(newTime);

						// Set the action to Action::MoveValue so moveMouseEvent() knows we are moving a node
						m_action = Action::MoveValue;

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
				else if (m_mouseDownRight) // Right click on EditMode::Draw mode erases values
				{
					// Update the last clicked position so we remove all nodes from
					// that point up to the point we release the mouse button
					m_drawLastTick = posTicks;

					// If we right-clicked a node, remove it
					eraseNode(clickedNode);

					m_action = Action::EraseValues;
				}
			break;
			}
			case EditMode::Erase:
			{
				m_clip->addJournalCheckPoint();

				// On erase mode, left click removes nodes
				if (m_mouseDownLeft)
				{
					// Update the last clicked position so we remove all nodes from
					// that point up to the point we release the mouse button
					m_drawLastTick = posTicks;

					// If we right-clicked a node, remove it
					eraseNode(clickedNode);

					m_action = Action::EraseValues;
				}
				else if (m_mouseDownRight) // And right click resets outValues
				{
					// If we clicked an outValue reset it
					resetNode(clickedNode);

					// Update the last clicked position so we reset all outValues from
					// that point up to the point we release the mouse button
					m_drawLastTick = posTicks;

					m_action = Action::ResetOutValues;
				}
			break;
			}
			case EditMode::DrawOutValues:
			{
				m_clip->addJournalCheckPoint();

				// On this mode, left click sets the outValue
				if (m_mouseDownLeft)
				{
					// If we clicked an outValue
					if (clickedNode != tm.end())
					{
						m_draggedOutValueKey = POS(clickedNode);

						clickedNode.value().setOutValue(level);

						m_action = Action::MoveOutValue;

						Engine::getSong()->setModified();
					}
					else // If we didn't click an outValue
					{
						// We check if the quantized position of the time we clicked has a
						// node and set its outValue
						TimePos quantizedPos = Note::quantized(
							TimePos(posTicks),
							m_clip->quantization()
						);

						clickedNode = tm.find(quantizedPos);

						if (clickedNode != tm.end())
						{
							m_draggedOutValueKey = POS(clickedNode);
							clickedNode.value().setOutValue(level);

							m_action = Action::MoveOutValue;

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

					m_action = Action::ResetOutValues;
				}
			break;
			}
			case EditMode::EditTangents:
			{
				if (!m_clip->canEditTangents())
				{
					update();
					return;
				}

				m_clip->addJournalCheckPoint();

				// Gets the closest node to the mouse click
				timeMap::iterator node = getClosestNode(mouseEvent->x());

				// Starts dragging a tangent
				if (m_mouseDownLeft && node != tm.end())
				{
					// Lock the tangents from that node, so it can only be
					// manually edited
					node.value().setLockedTangents(true);

					m_draggedTangentTick = POS(node);

					// Are we dragging the out or in tangent?
					m_draggedOutTangent = posTicks >= m_draggedTangentTick;

					m_action = Action::MoveTangent;
				}
				// Resets node's tangent
				else if (m_mouseDownRight)
				{
					// Resets tangent from node
					resetTangent(node);

					// Update the last clicked position so we reset all tangents from
					// that point up to the point we release the mouse button
					m_drawLastTick = posTicks;

					m_action = Action::ResetTangents;
				}
			break;
			}
		}

		update();
	}
}




void AutomationEditor::mouseDoubleClickEvent(QMouseEvent * mouseEvent)
{
	if (!validClip()) { return; }

	// If we double clicked outside the AutomationEditor viewport return
	if (mouseEvent->y() <= TOP_MARGIN || mouseEvent->x() < VALUES_WIDTH) { return; }

	// Are we fine tuning the inValue or outValue?
	const bool isOutVal = (m_editMode == EditMode::DrawOutValues);
	timeMap::iterator clickedNode = getNodeAt(mouseEvent->x(), mouseEvent->y(), isOutVal);

	switch (m_editMode)
	{
		case EditMode::Draw:
		case EditMode::DrawOutValues:
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

	if (m_editMode == EditMode::Draw)
	{
		if (m_action == Action::MoveValue)
		{
			// Actually apply the value of the node being dragged
			m_clip->applyDragValue();
		}

		QApplication::restoreOverrideCursor();
	}

	m_action = Action::None;

	if (mustRepaint) { repaint(); }
}




void AutomationEditor::mouseMoveEvent(QMouseEvent * mouseEvent )
{
	if( !validClip() )
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
			case EditMode::Draw:
			{
				// We are dragging a node
				if (m_mouseDownLeft)
				{
					if (m_action == Action::MoveValue)
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
						m_clip->setDragValue(
							TimePos(posTicks),
							level,
							true,
							mouseEvent->modifiers() & Qt::ControlModifier
						);

						Engine::getSong()->setModified();
					}
					/* else if (m_action == Action::DrawLine)
					{
						// We are drawing a line. For now do nothing (as before), but later logic
						// could be added here so the line is updated according to the new mouse position
						// until the button is released
					}*/
				}
				else if (m_mouseDownRight) // We are removing nodes
				{
					if (m_action == Action::EraseValues)
					{
						// If we moved the mouse past the beginning correct the position in ticks
						posTicks = qMax(posTicks, 0);

						// Removing automation nodes

						// Removes all values from the last clicked tick up to the current position tick
						m_clip->removeNodes(m_drawLastTick, posTicks);

						Engine::getSong()->setModified();
					}
				}
			break;
			}
			case EditMode::Erase:
			{
				// If we moved the mouse past the beginning correct the position in ticks
				posTicks = qMax(posTicks, 0);

				// Left button removes nodes
				if (m_mouseDownLeft)
				{
					if (m_action == Action::EraseValues)
					{
						// Removing automation nodes

						// Removes all values from the last clicked tick up to the current position tick
						m_clip->removeNodes(m_drawLastTick, posTicks);

						Engine::getSong()->setModified();
					}
				}
				else if (m_mouseDownRight) // Right button resets outValues
				{
					if (m_action == Action::ResetOutValues)
					{
						// Reseting outValues

						// Resets all values from the last clicked tick up to the current position tick
						m_clip->resetNodes(m_drawLastTick, posTicks);

						Engine::getSong()->setModified();
					}
				}
			break;
			}
			case EditMode::DrawOutValues:
			{
				// If we moved the mouse past the beginning correct the position in ticks
				posTicks = qMax(posTicks, 0);

				// Left button moves outValues
				if (m_mouseDownLeft)
				{
					if (m_action == Action::MoveOutValue)
					{
						// We are moving the outValue of the node
						timeMap & tm = m_clip->getTimeMap();

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
					if (m_action == Action::ResetOutValues)
					{
						// Reseting outValues

						// Resets all values from the last clicked tick up to the current position tick
						m_clip->resetNodes(m_drawLastTick, posTicks);

						Engine::getSong()->setModified();
					}
				}
			break;
			}
			case EditMode::EditTangents:
			{
				// If we moved the mouse past the beginning correct the position in ticks
				posTicks = std::max(posTicks, 0);

				if (m_mouseDownLeft && m_action == Action::MoveTangent)
				{
					timeMap& tm = m_clip->getTimeMap();
					auto it = tm.find(m_draggedTangentTick);

					// Safety check
					if (it == tm.end())
					{
						update();
						return;
					}

					// Calculate new tangent
					float y = m_draggedOutTangent
						? yCoordOfLevel(OUTVAL(it))
						: yCoordOfLevel(INVAL(it));
					float dy = m_draggedOutTangent
						? y - mouseEvent->y()
						: mouseEvent->y() - y;
					float dx = std::abs(posTicks - POS(it));
					float newTangent = dy / std::max(dx, 1.0f);

					if (m_draggedOutTangent)
					{
						it.value().setOutTangent(newTangent);
					}
					else
					{
						it.value().setInTangent(newTangent);
					}
				}
				else if (m_mouseDownRight && m_action == Action::ResetTangents)
				{
					// Resets all tangents from the last clicked tick up to the current position tick
					m_clip->resetTangents(m_drawLastTick, posTicks);

					Engine::getSong()->setModified();
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

	float scaledLevel = m_clip->firstObject()->scaledValue( level );

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
	// Below (m_ppb * AutomationClip::quantization() / 576) is used because:
	// 1 bar equals to 192/quantization() notes. Hence, to calculate the number of pixels
	// per note we would have (m_ppb * 1 bar / (192/quantization()) notes per bar), or
	// (m_ppb * quantization / 192). If we want 1/3 of the number of pixels per note we
	// get (m_ppb * quantization() / 192*3) or (m_ppb * quantization() / 576)
	const int outerRadius = qBound(3, (m_ppb * AutomationClip::quantization()) / 576, 5);

	// Draw a circle for the outValue
	int y = yCoordOfLevel(OUTVAL(it));
	p.setPen(QPen(m_nodeOutValueColor.lighter(200)));
	p.setBrush(QBrush(m_nodeOutValueColor));
	p.drawEllipse(x - outerRadius, y - outerRadius, outerRadius * 2, outerRadius * 2);

	// Draw a circle for the inValue
	y = yCoordOfLevel(INVAL(it));
	p.setPen(QPen(m_nodeInValueColor.lighter(200)));
	p.setBrush(QBrush(m_nodeInValueColor));
	p.drawEllipse(x - outerRadius, y - outerRadius, outerRadius * 2, outerRadius * 2);
}




inline void AutomationEditor::drawAutomationTangents(QPainter& p, timeMap::iterator it)
{
	int x = xCoordOfTick(POS(it));

	// The tangent value correlates the variation in the node value related to the increase
	// in ticks. So to have a proportionate drawing of the tangent line, we need to find the
	// relation between the number of pixels per tick and the number of pixels per value level.
	float viewportHeight = (height() - SCROLLBAR_SIZE - 1) - TOP_MARGIN;
	float pixelsPerTick = m_ppb / TimePos::ticksPerBar();
	// std::abs just in case the topLevel is smaller than the bottomLevel for some reason
	float pixelsPerLevel = std::abs(viewportHeight / (m_topLevel - m_bottomLevel));
	float proportion = pixelsPerLevel / pixelsPerTick;

	p.setPen(QPen(m_nodeTangentLineColor));
	p.setBrush(QBrush(m_nodeTangentLineColor));

	int y = yCoordOfLevel(INVAL(it));
	int tx = x - 20;
	int ty = y + 20 * INTAN(it) * proportion;
	p.drawLine(x, y, tx, ty);
	p.setBrush(QBrush(m_nodeTangentLineColor.darker(200)));
	p.drawEllipse(tx - 3, ty - 3, 6, 6);

	p.setBrush(QBrush(m_nodeTangentLineColor));

	y = yCoordOfLevel(OUTVAL(it));
	tx = x + 20;
	ty = y - 20 * OUTTAN(it) * proportion;
	p.drawLine(x, y, tx, ty);
	p.setBrush(QBrush(m_nodeTangentLineColor.darker(200)));
	p.drawEllipse(tx - 3, ty - 3, 6, 6);
}

void AutomationEditor::setGhostMidiClip(MidiClip* newMidiClip)
{
	// Expects a pointer to a MIDI clip or nullptr.
	m_ghostNotes = newMidiClip;
	m_renderSample = false;
}

void AutomationEditor::setGhostSample(SampleClip* newGhostSample)
{
	// Expects a pointer to a Sample buffer or nullptr.
	m_ghostSample = newGhostSample;
	m_renderSample = true;
	m_sampleThumbnail = SampleThumbnail{newGhostSample->sample()};
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

	p.setFont(adjustedToPixelSize(p.font(), DEFAULT_FONT_SIZE));

	int grid_height = height() - TOP_MARGIN - SCROLLBAR_SIZE;

	// start drawing at the bottom
	int grid_bottom = height() - SCROLLBAR_SIZE - 1;

	p.fillRect(0, TOP_MARGIN, VALUES_WIDTH, height() - TOP_MARGIN, m_scaleColor);

	// print value numbers
	int font_height = p.fontMetrics().height();
	auto text_flags = (Qt::Alignment)(Qt::AlignRight | Qt::AlignVCenter);

	if( validClip() )
	{
		if( m_y_auto )
		{
			auto y = std::array{grid_bottom, TOP_MARGIN + font_height / 2};
			auto level = std::array{m_minLevel, m_maxLevel};
			for( int i = 0; i < 2; ++i )
			{
				const QString & label = m_clip->firstObject()
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
				const QString & label = m_clip->firstObject()
							->displayValue( level );
				int y = yCoordOfLevel(level);
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

	if( m_clip )
	{
		int q;
		int x_line_end = (int)( m_y_auto || m_topLevel < m_maxLevel ?
			TOP_MARGIN :
			grid_bottom - ( m_topLevel - m_bottomLevel ) * m_y_delta );

		if( m_zoomingXModel.value() > 3 )
		{
			// If we're over 100% zoom, we allow all quantization level grids
			q = AutomationClip::quantization();
		}
		else if( AutomationClip::quantization() % 3 != 0 )
		{
			// If we're under 100% zoom, we allow quantization grid up to 1/24 for triplets
			// to ensure a dense doesn't fill out the background
			q = AutomationClip::quantization() < 8 ? 8 : AutomationClip::quantization();
		}
		else {
			// If we're under 100% zoom, we allow quantization grid up to 1/32 for normal notes
			q = AutomationClip::quantization() < 6 ? 6 : AutomationClip::quantization();
		}

		// 3 independent loops, because quantization might not divide evenly into
		// exotic denominators (e.g. 7/11 time), which are allowed ATM.
		// First quantization grid...
		for (int tick = m_currentPosition - m_currentPosition % q, x = xCoordOfTick(tick); x <= width();
			 tick += q, x = xCoordOfTick(tick))
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
			for( int level = (int)m_bottomLevel; level <= m_topLevel; level++)
			{
				float y = yCoordOfLevel(static_cast<float>(level));

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

		for (int tick = m_currentPosition - m_currentPosition % ticksPerBeat, x = xCoordOfTick(tick); x <= width();
			 tick += ticksPerBeat, x = xCoordOfTick(tick))
		{
			p.setPen(m_beatLineColor);
			p.drawLine( x, grid_bottom, x, x_line_end );
		}

		// draw ghost sample
		if (m_ghostSample != nullptr && m_ghostSample->sample().sampleSize() > 1 && m_renderSample)
		{
			int sampleFrames = m_ghostSample->sample().sampleSize();
			int length = static_cast<float>(sampleFrames) / Engine::framesPerTick();
			int editorHeight = grid_bottom - TOP_MARGIN;

			int startPos = xCoordOfTick(0);
			int sampleWidth = xCoordOfTick(length) - startPos;
			int sampleHeight = std::min(editorHeight - SAMPLE_MARGIN, MAX_SAMPLE_HEIGHT);
			int yOffset = (editorHeight - sampleHeight) / 2.0f + TOP_MARGIN;

			p.setPen(m_ghostSampleColor);

			const auto& sample = m_ghostSample->sample();

			const auto param = SampleThumbnail::VisualizeParameters{
				.sampleRect = QRect(startPos, yOffset, sampleWidth, sampleHeight),
				.viewportRect = rect(),
				.amplification = sample.amplification(),
				.sampleStart = static_cast<float>(sample.startFrame()) / sample.sampleSize(),
				.sampleEnd = static_cast<float>(sample.endFrame()) / sample.sampleSize(),
				.reversed = sample.reversed()
			};

			m_sampleThumbnail.visualize(param, p);
		}

		// draw ghost notes
		if (m_ghostNotes != nullptr && !m_renderSample)
		{
			const NoteVector& notes = m_ghostNotes->notes();
			int minKey = 128;
			int maxKey = 0;
			int detuningOffset = 0;
			const Note* detuningNote = nullptr;

			for (const Note* note : notes)
			{
				int noteKey = note->key();

				if (note->detuning()->automationClip() == m_clip) {
					detuningOffset = note->pos();
					detuningNote = note;
				}

				maxKey = std::max(maxKey, noteKey);
				minKey = std::min(minKey, noteKey);
			}

			for (const Note* note : notes)
			{
				int lenTicks = note->length();
				int notePos = note->pos();

				// offset note if detuning
				if (notePos+lenTicks < detuningOffset) { continue; }
				notePos -= detuningOffset;

				// remove/change after #5902
				if (lenTicks == 0) { continue; }
				else if (lenTicks < 0) { lenTicks = 4; }

				int note_width = lenTicks * m_ppb / TimePos::ticksPerBar();
				int keyRange = maxKey - minKey;

				if (keyRange < MIN_NOTE_RANGE) 
				{
					int padding = (MIN_NOTE_RANGE - keyRange) / 2.0f;
					maxKey += padding;
					minKey -= padding;
					keyRange = MIN_NOTE_RANGE;
				}

				float absNoteHeight = static_cast<float>(note->key() - minKey) / (maxKey - minKey);
				int graphHeight = grid_bottom - NOTE_HEIGHT - NOTE_MARGIN - TOP_MARGIN;
				const int y = (graphHeight - graphHeight * absNoteHeight) + NOTE_HEIGHT / 2.0f + TOP_MARGIN;
				const int x = xCoordOfTick(notePos);

				if (note == detuningNote) {
					p.fillRect(x, y, note_width, NOTE_HEIGHT, m_detuningNoteColor);
				} else {
					p.fillRect(x, y, note_width, NOTE_HEIGHT, m_ghostNoteColor);
				}
			}
		}

		// and finally bars
		for (int tick = m_currentPosition - m_currentPosition % TimePos::ticksPerBar(), x = xCoordOfTick(tick);
			 x <= width(); tick += TimePos::ticksPerBar(), x = xCoordOfTick(tick))
		{
			p.setPen(m_barLineColor);
			p.drawLine( x, grid_bottom, x, x_line_end );
		}
	}



	// following code draws all visible values

	if( validClip() )
	{
		//NEEDS Change in CSS
		//int len_ticks = 4;
		timeMap & time_map = m_clip->getTimeMap();

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

				float *values = m_clip->valuesAfter(POS(it));

				// We are creating a path to draw a polygon representing the values between two
				// nodes. When we have two nodes with discrete progression, we will basically have
				// a rectangle with the outValue of the first node (that's why nextValue will match
				// the outValue of the current node). When we have nodes with linear or cubic progression
				// the value of the end of the shape between the two nodes will be the inValue of
				// the next node.
				float nextValue = m_clip->progressionType() == AutomationClip::ProgressionType::Discrete
					? OUTVAL(it)
					: INVAL(it + 1);

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
				// Draw tangents if necessary (only for manually edited tangents)
				if (m_clip->canEditTangents() && LOCKEDTAN(it))
				{
					drawAutomationTangents(p, it);
				}

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
			// Draw tangents if necessary (only for manually edited tangents)
			if (m_clip->canEditTangents() && LOCKEDTAN(it))
			{
				drawAutomationTangents(p, it);
			}
		}

		// draw clip bounds overlay
		p.fillRect(
			xCoordOfTick(m_clip->length() - m_clip->startTimeOffset()),
			TOP_MARGIN,
			width() - 10,
			grid_bottom,
			m_outOfBoundsShade
		);
		p.fillRect(
			0,
			TOP_MARGIN,
			xCoordOfTick(-m_clip->startTimeOffset()),
			grid_bottom,
			m_outOfBoundsShade
		);
	}
	else
	{
		QFont f = font();
		f.setBold( true );
		p.setFont(f);
		p.setPen( QApplication::palette().color( QPalette::Active,
							QPalette::BrightText ) );
		p.drawText( VALUES_WIDTH + 20, TOP_MARGIN + 40,
				width() - VALUES_WIDTH - 20 - SCROLLBAR_SIZE,
				grid_height - 40, Qt::TextWordWrap,
				tr( "Please open an automation clip by "
					"double-clicking on it!" ) );
	}

	// TODO: Get this out of paint event
	int l = validClip() ? (int) m_clip->length() - m_clip->startTimeOffset() : 0;

	// reset scroll-range
	if( m_leftRightScroll->maximum() != l )
	{
		m_leftRightScroll->setRange( 0, l );
		m_leftRightScroll->setPageStep( l );
	}

	if(validClip() && GuiApplication::instance()->automationEditor()->m_editor->hasFocus())
	{
		drawCross( p );
	}

	const QPixmap * cursor = nullptr;
	// draw current edit-mode-icon below the cursor
	switch( m_editMode )
	{
		case EditMode::Draw:
		{
			if (m_action == Action::EraseValues) { cursor = &m_toolErase; }
			else if (m_action == Action::MoveValue) { cursor = &m_toolMove; }
			else { cursor = &m_toolDraw; }
			break;
		}
		case EditMode::Erase:
		{
			cursor = &m_toolErase;
			break;
		}
		case EditMode::DrawOutValues:
		{
			if (m_action == Action::ResetOutValues) { cursor = &m_toolErase; }
			else if (m_action == Action::MoveOutValue) { cursor = &m_toolMove; }
			else { cursor = &m_toolDrawOut; }
			break;
		}
		case EditMode::EditTangents:
		{
			cursor = m_action == Action::MoveTangent ? &m_toolMove : &m_toolEditTangents;
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
		const int y_start = yCoordOfLevel(value);
		const int y_end = grid_bottom + (grid_bottom - TOP_MARGIN) * m_minLevel / (m_maxLevel - m_minLevel);
		const int rect_height = m_y_auto ? y_end - y_start : value * m_y_delta;

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
	// If a clip exists...
	if (m_clip)
	{
		// get time map of current clip
		timeMap & time_map = m_clip->getTimeMap();
		// If time_map is not empty...
		if (!time_map.empty())
		{
			// set the position to the inverted value ((max + min) - value)
			// If we set just (max - value), we're off by m_clip's minimum
			pos = m_clip->getMax() + m_clip->getMin() - static_cast<int>(INVAL(time_map.begin()));
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

	m_timeLine->setFixedWidth(width());

	updateTopBottomLevels();
	update();
}

void AutomationEditor::adjustLeftRightScoll(int value)
{
	m_leftRightScroll->setValue(m_leftRightScroll->value() -
							value * 0.3f / m_zoomXLevels[m_zoomingXModel.value()]);
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
	else if (std::abs(we->angleDelta().x()) > std::abs(we->angleDelta().y())) // scrolling is horizontal
	{
		adjustLeftRightScoll(we->angleDelta().x());
	}
	else if(we->modifiers() & Qt::ShiftModifier)
	{
		adjustLeftRightScoll(we->angleDelta().y());
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
	float level = std::roundf( ( m_bottomLevel + ( m_y_auto ?
			( m_maxLevel - m_minLevel ) * ( level_line_y - y )
					/ (float)( level_line_y - ( TOP_MARGIN + 2 ) ) :
			( level_line_y - y ) / (float)m_y_delta ) ) / m_step ) * m_step;
	// some range-checking-stuff
	level = qBound(std::roundf(m_bottomLevel), level, std::roundf(m_topLevel));

	return level;
}




inline bool AutomationEditor::inPatternEditor()
{
	return (validClip() && m_clip->getTrack()->trackContainer() == Engine::patternStore());
}




void AutomationEditor::play()
{
	if( !validClip() )
	{
		return;
	}

	if( !m_clip->getTrack() )
	{
		if( Engine::getSong()->playMode() != Song::PlayMode::MidiClip )
		{
			Engine::getSong()->stop();
			Engine::getSong()->playMidiClip( getGUI()->pianoRoll()->currentMidiClip() );
		}
		else if( Engine::getSong()->isStopped() == false )
		{
			Engine::getSong()->togglePause();
		}
		else
		{
			Engine::getSong()->playMidiClip( getGUI()->pianoRoll()->currentMidiClip() );
		}
	}
	else if (inPatternEditor())
	{
		Engine::patternStore()->play();
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
	if( !validClip() )
	{
		return;
	}
	if (m_clip->getTrack() && inPatternEditor())
	{
		Engine::patternStore()->stop();
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




void AutomationEditor::setEditMode(AutomationEditor::EditMode mode)
{
	if (m_editMode == mode)
		return;

	m_editMode = mode;

	update();
}




void AutomationEditor::setEditMode(int mode)
{
	setEditMode((AutomationEditor::EditMode) mode);
}




void AutomationEditor::setProgressionType(AutomationClip::ProgressionType type)
{
	if (validClip())
	{
		m_clip->addJournalCheckPoint();
		m_clip->setProgressionType(type);
		Engine::getSong()->setModified();
		update();
	}
}

void AutomationEditor::setProgressionType(int type)
{
	setProgressionType((AutomationClip::ProgressionType) type);
}




void AutomationEditor::setTension()
{
	if ( m_clip )
	{
		m_clip->setTension( QString::number( m_tensionModel->value() ) );
		update();
	}
}




void AutomationEditor::updatePosition(const TimePos & t )
{
	if( ( Engine::getSong()->isPlaying() &&
			Engine::getSong()->playMode() ==
					Song::PlayMode::AutomationClip ) ||
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
	AutomationClip::setQuantization(DefaultTicksPerBar / Quantizations[m_quantizeModel.value()]);

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

	// Get our clip timeMap and create a iterator so we can check the nodes
	timeMap & tm = m_clip->getTimeMap();
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

AutomationEditor::timeMap::iterator AutomationEditor::getClosestNode(int x)
{
	// Remove the VALUES_WIDTH from the x position, so we have the actual viewport x
	x -= VALUES_WIDTH;
	// Convert the x position to the position in ticks
	int posTicks = (x * TimePos::ticksPerBar() / m_ppb) + m_currentPosition;

	// Get our pattern timeMap and create a iterator so we can check the nodes
	timeMap& tm = m_clip->getTimeMap();

	if (tm.isEmpty()) { return tm.end(); }

	// Get the node with an equal or higher position
	auto it = tm.lowerBound(posTicks);

	// If there are no nodes equal or higher than the position return
	// the one before it
	if (it == tm.end())
	{
		--it;
		return it;
	}
	// If the node returned is the first, return it
	else if (it == tm.begin())
	{
		return it;
	}
	// Else return the closest node
	else
	{
		// Distance from node to the right
		int distanceRight = std::abs(POS(it) - posTicks);
		// Distance from node to the left
		int distanceLeft = std::abs(POS(--it) - posTicks);

		if (distanceLeft >= distanceRight)
		{
			++it;
		}
		return it;
	}
}




AutomationEditorWindow::AutomationEditorWindow() :
	Editor(),
	m_editor(new AutomationEditor())
{
	setCentralWidget(m_editor);



	// Play/stop buttons
	m_playAction->setToolTip(tr( "Play/pause current clip (Space)" ));

	m_stopAction->setToolTip(tr("Stop playing of current clip (Space)"));

	// Edit mode buttons
	DropToolBar *editActionsToolBar = addDropToolBarToTop(tr("Edit actions"));

	auto editModeGroup = new ActionGroup(this);
	m_drawAction = editModeGroup->addAction(embed::getIconPixmap("edit_draw"), tr("Draw mode (Shift+D)"));
	m_drawAction->setShortcut(combine(Qt::SHIFT, Qt::Key_D));
	m_drawAction->setChecked(true);

	m_eraseAction = editModeGroup->addAction(embed::getIconPixmap("edit_erase"), tr("Erase mode (Shift+E)"));
	m_eraseAction->setShortcut(combine(Qt::SHIFT, Qt::Key_E));

	m_drawOutAction = editModeGroup->addAction(embed::getIconPixmap("edit_draw_outvalue"), tr("Draw outValues mode (Shift+C)"));
	m_drawOutAction->setShortcut(combine(Qt::SHIFT, Qt::Key_C));

	m_editTanAction = editModeGroup->addAction(embed::getIconPixmap("edit_tangent"), tr("Edit tangents mode (Shift+T)"));
	m_editTanAction->setShortcut(combine(Qt::SHIFT, Qt::Key_T));
	m_editTanAction->setEnabled(false);

	m_flipYAction = new QAction(embed::getIconPixmap("flip_y"), tr("Flip vertically"), this);
	m_flipXAction = new QAction(embed::getIconPixmap("flip_x"), tr("Flip horizontally"), this);

	connect(editModeGroup, SIGNAL(triggered(int)), m_editor, SLOT(setEditMode(int)));

	editActionsToolBar->addAction(m_drawAction);
	editActionsToolBar->addAction(m_eraseAction);
	editActionsToolBar->addAction(m_drawOutAction);
	editActionsToolBar->addAction(m_editTanAction);
	editActionsToolBar->addAction(m_flipXAction);
	editActionsToolBar->addAction(m_flipYAction);

	// Interpolation actions
	DropToolBar *interpolationActionsToolBar = addDropToolBarToTop(tr("Interpolation controls"));

	auto progression_type_group = new ActionGroup(this);

	m_discreteAction = progression_type_group->addAction(
				embed::getIconPixmap("progression_discrete"), tr("Discrete progression"));
	m_discreteAction->setChecked(true);

	m_linearAction = progression_type_group->addAction(
				embed::getIconPixmap("progression_linear"), tr("Linear progression"));
	m_cubicHermiteAction = progression_type_group->addAction(
				embed::getIconPixmap("progression_cubic_hermite"), tr( "Cubic Hermite progression"));

	connect(progression_type_group, SIGNAL(triggered(int)), this, SLOT(setProgressionType(int)));

	// setup tension-stuff
	m_tensionKnob = new Knob( KnobType::Small17, this, "Tension" );
	m_tensionKnob->setModel(m_editor->m_tensionModel);
	m_tensionKnob->setToolTip(tr("Tension value for spline"));

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

	auto zoom_x_label = new QLabel(zoomToolBar);
	zoom_x_label->setPixmap( embed::getIconPixmap( "zoom_x" ) );

	m_zoomingXComboBox = new ComboBox( zoomToolBar );
	m_zoomingXComboBox->setFixedSize( 80, ComboBox::DEFAULT_HEIGHT );
	m_zoomingXComboBox->setToolTip( tr( "Horizontal zooming" ) );

	for( float const & zoomLevel : m_editor->m_zoomXLevels )
	{
		m_editor->m_zoomingXModel.addItem(QString("%1%").arg(zoomLevel * 100));
	}
	m_editor->m_zoomingXModel.setValue( m_editor->m_zoomingXModel.findText( "100%" ) );

	m_zoomingXComboBox->setModel( &m_editor->m_zoomingXModel );

	connect( &m_editor->m_zoomingXModel, SIGNAL(dataChanged()),
			m_editor, SLOT(zoomingXChanged()));

	auto zoom_y_label = new QLabel(zoomToolBar);
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

	connect( &m_editor->m_zoomingYModel, SIGNAL(dataChanged()),
			m_editor, SLOT(zoomingYChanged()));

	zoomToolBar->addWidget( zoom_x_label );
	zoomToolBar->addWidget( m_zoomingXComboBox );
	zoomToolBar->addSeparator();
	zoomToolBar->addWidget( zoom_y_label );
	zoomToolBar->addWidget( m_zoomingYComboBox );

	// Quantization controls
	DropToolBar *quantizationActionsToolBar = addDropToolBarToTop(tr("Quantization controls"));

	auto quantize_lbl = new QLabel(m_toolBar);
	quantize_lbl->setPixmap( embed::getIconPixmap( "quantize" ) );

	m_quantizeComboBox = new ComboBox( m_toolBar );
	m_quantizeComboBox->setFixedSize( 60, ComboBox::DEFAULT_HEIGHT );
	m_quantizeComboBox->setToolTip( tr( "Quantization" ) );

	m_quantizeComboBox->setModel( &m_editor->m_quantizeModel );

	quantizationActionsToolBar->addWidget( quantize_lbl );
	quantizationActionsToolBar->addWidget( m_quantizeComboBox );

	m_resetGhostNotes = new QPushButton(m_toolBar);
	m_resetGhostNotes->setIcon(embed::getIconPixmap("clear_ghost_note"));
	m_resetGhostNotes->setToolTip(tr("Clear ghost notes"));
	m_resetGhostNotes->setEnabled(true);

	connect(m_resetGhostNotes, &QPushButton::pressed, m_editor, &AutomationEditor::resetGhostNotes);

	quantizationActionsToolBar->addSeparator();
	quantizationActionsToolBar->addWidget(m_resetGhostNotes);

	// Setup our actual window
	setFocusPolicy(Qt::StrongFocus);
	setFocus();
	setWindowIcon( embed::getIconPixmap( "automation" ) );
	setAcceptDrops( true );
	m_toolBar->setAcceptDrops( true );
}


void AutomationEditorWindow::setCurrentClip(AutomationClip* clip)
{
	// Disconnect our old clip
	if (currentClip() != nullptr)
	{
		m_editor->m_clip->disconnect(this);
		m_flipXAction->disconnect();
		m_flipYAction->disconnect();
	}

	m_editor->setCurrentClip(clip);

	// Set our window's title
	if (clip == nullptr)
	{
		setWindowTitle( tr( "Automation Editor - no clip" ) );
		return;
	}

	setWindowTitle( tr( "Automation Editor - %1" ).arg( m_editor->m_clip->name() ) );


	switch(m_editor->m_clip->progressionType())
	{
	case AutomationClip::ProgressionType::Discrete:
		m_discreteAction->setChecked(true);
		m_tensionKnob->setEnabled(false);
		break;
	case AutomationClip::ProgressionType::Linear:
		m_linearAction->setChecked(true);
		m_tensionKnob->setEnabled(false);
		break;
	case AutomationClip::ProgressionType::CubicHermite:
		m_cubicHermiteAction->setChecked(true);
		m_tensionKnob->setEnabled(true);
		break;
	}

	// Connect new clip
	if (clip)
	{
		connect(clip, SIGNAL(dataChanged()), this, SLOT(update()));
		connect( clip, SIGNAL(dataChanged()), this, SLOT(updateWindowTitle()));
		connect(clip, SIGNAL(destroyed()), this, SLOT(clearCurrentClip()));

		connect(m_flipXAction, SIGNAL(triggered()), clip, SLOT(flipX()));
		connect(m_flipYAction, SIGNAL(triggered()), clip, SLOT(flipY()));
	}

	updateEditTanButton();
	emit currentClipChanged();
}


const AutomationClip* AutomationEditorWindow::currentClip()
{
	return m_editor->currentClip();
}

void AutomationEditorWindow::dropEvent( QDropEvent *_de )
{
	QString type = StringPairDrag::decodeKey( _de );
	QString val = StringPairDrag::decodeValue( _de );
	if( type == "automatable_model" )
	{
		auto mod = dynamic_cast<AutomatableModel*>(Engine::projectJournal()->journallingObject(val.toInt()));
		if (mod != nullptr)
		{
			bool added = m_editor->m_clip->addObject( mod );
			if ( !added )
			{
				TextFloat::displayMessage( mod->displayName(),
							   tr( "Model is already connected "
							   "to this clip." ),
							   embed::getIconPixmap( "automation" ),
							   2000 );
			}
			setCurrentClip( m_editor->m_clip );
		}
	}

	update();
}

void AutomationEditorWindow::dragEnterEvent( QDragEnterEvent *_dee )
{
	if (! m_editor->validClip() ) {
		return;
	}
	StringPairDrag::processDragEnterEvent(_dee, {"automatable_model"});
}

void AutomationEditorWindow::open(AutomationClip* clip)
{
	setCurrentClip(clip);
	parentWidget()->show();
	show();
	setFocus();
}

QSize AutomationEditorWindow::sizeHint() const
{
	return {INITIAL_WIDTH, INITIAL_HEIGHT};
}

void AutomationEditorWindow::clearCurrentClip()
{
	m_editor->m_clip = nullptr;
	setCurrentClip(nullptr);
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
	if (m_editor->m_clip == nullptr)
	{
		setWindowTitle( tr( "Automation Editor - no clip" ) );
		return;
	}

	setWindowTitle( tr( "Automation Editor - %1" ).arg( m_editor->m_clip->name() ) );
}

void AutomationEditorWindow::setProgressionType(int progType)
{
	m_editor->setProgressionType(progType);
	updateEditTanButton();
}

void AutomationEditorWindow::updateEditTanButton()
{
	auto progType = currentClip()->progressionType();
	m_editTanAction->setEnabled(AutomationClip::supportsTangentEditing(progType));
	if (!m_editTanAction->isEnabled() && m_editTanAction->isChecked()) { m_drawAction->trigger(); }
}

} // namespace lmms::gui
