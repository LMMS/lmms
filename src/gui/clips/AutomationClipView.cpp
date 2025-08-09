/*
 * AutomationClipView.cpp - implementation of view for AutomationClip
 *
 * Copyright (c) 2008-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include "AutomationClipView.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>  // IWYU pragma: keep
#include <QMenu>

#include "AutomationEditor.h"
#include "embed.h"
#include "GuiApplication.h"
#include "ProjectJournal.h"
#include "RenameDialog.h"
#include "StringPairDrag.h"
#include "TextFloat.h"
#include "Track.h"
#include "TrackContainerView.h"
#include "TrackView.h"

#include "Engine.h"


namespace lmms::gui
{

AutomationClipView::AutomationClipView( AutomationClip * _clip,
						TrackView * _parent ) :
	ClipView( _clip, _parent ),
	m_clip( _clip ),
	m_paintPixmap()
{
	connect( m_clip, SIGNAL(dataChanged()),
			this, SLOT(update()));
	connect( getGUI()->automationEditor(), SIGNAL(currentClipChanged()),
			this, SLOT(update()));

	setToolTip(m_clip->name());
	setStyle( QApplication::style() );
	update();
}




void AutomationClipView::openInAutomationEditor()
{
	if(getGUI() != nullptr)
		getGUI()->automationEditor()->open(m_clip);
}


void AutomationClipView::update()
{
	setToolTip(m_clip->name());

	ClipView::update();
}



void AutomationClipView::resetName()
{
	m_clip->setName( QString() );
}




void AutomationClipView::changeName()
{
	QString s = m_clip->name();
	RenameDialog rename_dlg( s );
	rename_dlg.exec();
	m_clip->setName( s );
	update();
}




void AutomationClipView::disconnectObject( QAction * _a )
{
	JournallingObject * j = Engine::projectJournal()->
				journallingObject( _a->data().toInt() );
	if( j && dynamic_cast<AutomatableModel *>( j ) )
	{
		float oldMin = m_clip->getMin();
		float oldMax = m_clip->getMax();

		m_clip->m_objects.erase( std::find( m_clip->m_objects.begin(),
					m_clip->m_objects.end(),
				dynamic_cast<AutomatableModel *>( j ) ) );
		update();

		//If automation editor is opened, update its display after disconnection
		if( getGUI()->automationEditor() )
		{
			getGUI()->automationEditor()->m_editor->updateAfterClipChange();
		}

		//if there is no more connection connected to the AutomationClip
		if( m_clip->m_objects.size() == 0 )
		{
			//scale the points to fit the new min. and max. value
			this->scaleTimemapToFit( oldMin, oldMax );
		}
	}
}


void AutomationClipView::toggleRecording()
{
	m_clip->setRecording( ! m_clip->isRecording() );
	update();
}




void AutomationClipView::flipY()
{
	m_clip->flipY( m_clip->getMin(), m_clip->getMax() );
	update();
}




void AutomationClipView::flipX()
{
	m_clip->flipX(std::max(0, -m_clip->startTimeOffset()), std::max(0, m_clip->length() - m_clip->startTimeOffset()));
	update();
}




void AutomationClipView::constructContextMenu( QMenu * _cm )
{
	auto a = new QAction(embed::getIconPixmap("automation"), tr("Open in Automation editor"), _cm);
	_cm->insertAction( _cm->actions()[0], a );
	connect(a, SIGNAL(triggered()), this, SLOT(openInAutomationEditor()));
	_cm->insertSeparator( _cm->actions()[1] );

	_cm->addSeparator();

	_cm->addAction( embed::getIconPixmap( "edit_erase" ),
			tr( "Clear" ), m_clip, SLOT(clear()));
	_cm->addSeparator();

	_cm->addAction( embed::getIconPixmap( "reload" ), tr( "Reset name" ),
						this, SLOT(resetName()));
	_cm->addAction( embed::getIconPixmap( "edit_rename" ),
						tr( "Change name" ),
						this, SLOT(changeName()));
	_cm->addAction( embed::getIconPixmap( "record" ),
						tr( "Set/clear record" ),
						this, SLOT(toggleRecording()));
	_cm->addAction( embed::getIconPixmap( "flip_y" ),
						tr( "Flip Vertically (Visible)" ),
						this, SLOT(flipY()));
	_cm->addAction( embed::getIconPixmap( "flip_x" ),
						tr( "Flip Horizontally (Visible)" ),
						this, SLOT(flipX()));
	
	if (!m_clip->m_objects.empty())
	{
		_cm->addSeparator();
		auto m = new QMenu(tr("%1 Connections").arg(m_clip->m_objects.size()), _cm);
		for (const auto& object : m_clip->m_objects)
		{
			if (object)
			{
				a = new QAction(tr("Disconnect \"%1\"").arg(object->fullDisplayName()), m);
				a->setData(object->id());
				m->addAction( a );
			}
		}
		connect( m, SIGNAL(triggered(QAction*)),
				this, SLOT(disconnectObject(QAction*)));
		_cm->addMenu( m );
	}
}




void AutomationClipView::mouseDoubleClickEvent( QMouseEvent * me )
{
	if (m_trackView->trackContainerView()->knifeMode()) { return; }

	if(me->button() != Qt::LeftButton)
	{
		me->ignore();
		return;
	}
	openInAutomationEditor();
}




void AutomationClipView::paintEvent( QPaintEvent * )
{
	QPainter painter( this );

	if( !needsUpdate() )
	{
		painter.drawPixmap( 0, 0, m_paintPixmap );
		return;
	}

	setNeedsUpdate( false );

	if (m_paintPixmap.isNull() || m_paintPixmap.size() != size())
	{
		m_paintPixmap = QPixmap(size());
	}

	QPainter p( &m_paintPixmap );

	QLinearGradient lingrad( 0, 0, 0, height() );
	QColor c = getColorForDisplay( painter.background().color() );
	bool muted = m_clip->getTrack()->isMuted() || m_clip->isMuted();
	bool current = getGUI()->automationEditor()->currentClip() == m_clip;

	lingrad.setColorAt( 1, c.darker( 300 ) );
	lingrad.setColorAt( 0, c );

	// paint a black rectangle under the clip to prevent glitches with transparent backgrounds
	p.fillRect( rect(), QColor( 0, 0, 0 ) );

	if( gradient() )
	{
		p.fillRect( rect(), lingrad );
	}
	else
	{
		p.fillRect( rect(), c );
	}

	// pixels per bar
	const float ppb = fixedClips() ?
			( parentWidget()->width() - 2 * BORDER_WIDTH )
				/ (float) m_clip->timeMapLength().getBar() :
								pixelsPerBar();

	const auto min = m_clip->firstObject()->minValue<float>();
	const auto max = m_clip->firstObject()->maxValue<float>();

	const float y_scale = max - min;
	const float h = ( height() - 2 * BORDER_WIDTH ) / y_scale;
	const float ppTick  = ppb / TimePos::ticksPerBar();
	const int offset =  m_clip->startTimeOffset() * ppTick;

	p.translate( 0.0f, max * height() / y_scale - BORDER_WIDTH );
	p.scale( 1.0f, -h );

	QLinearGradient lin2grad( 0, min, 0, max );
	QColor col;

	col = !muted ? painter.pen().brush().color() : mutedColor();

	lin2grad.setColorAt( 1, col.lighter( 150 ) );
	lin2grad.setColorAt( 0.5, col );
	lin2grad.setColorAt( 0, col.darker( 150 ) );

	p.setRenderHints( QPainter::Antialiasing, true );
	for( AutomationClip::timeMap::const_iterator it =
						m_clip->getTimeMap().begin();
					it != m_clip->getTimeMap().end(); ++it )
	{
		if( it+1 == m_clip->getTimeMap().end() )
		{
			const float x1 = POS(it) * ppTick + offset;
			const auto x2 = (float)(width() - BORDER_WIDTH);
			if( x1 > ( width() - BORDER_WIDTH ) ) break;
			// We are drawing the space after the last node, so we use the outValue
			if( gradient() )
			{
				p.fillRect(QRectF(x1, 0.0f, x2 - x1, OUTVAL(it)), lin2grad);
			}
			else
			{
				p.fillRect(QRectF(x1, 0.0f, x2 - x1, OUTVAL(it)), col);
			}
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

		QPainterPath path;
		QPointF origin = QPointF(POS(it) * ppTick + offset, 0.0f);
		path.moveTo(origin);
		path.moveTo(QPointF(POS(it) * ppTick + offset, values[0]));
		for (int i = POS(it) + 1; i < POS(it + 1); i++)
		{
			float x = i * ppTick + offset;
			if(x > (width() - BORDER_WIDTH)) break;
			float value = values[i - POS(it)];
			path.lineTo(QPointF(x, value));
		}
		path.lineTo((POS(it + 1)) * ppTick + offset, nextValue);
		path.lineTo((POS(it + 1)) * ppTick + offset, 0.0f);
		path.lineTo(origin);

		if( gradient() )
		{
			p.fillPath( path, lin2grad );
		}
		else
		{
			p.fillPath( path, col );
		}
		delete [] values;
	}

	p.setRenderHints( QPainter::Antialiasing, false );
	p.resetTransform();

	// bar lines
	const int lineSize = 3;
	p.setPen( c.darker( 300 ) );

	for (bar_t b = 1; b < width() - BORDER_WIDTH; ++b)
	{
		const int bx = BORDER_WIDTH + static_cast<int>(ppb * b) - 2;

		//top line
		p.drawLine(bx + offset, BORDER_WIDTH, bx + offset, BORDER_WIDTH + lineSize);

		//bottom line
		p.drawLine(bx + offset, rect().bottom() - (lineSize + BORDER_WIDTH), bx + offset, rect().bottom() - BORDER_WIDTH);
	}

	// recording icon for when recording automation
	if( m_clip->isRecording() )
	{
		static auto s_clipRec = embed::getIconPixmap("clip_rec");
		p.drawPixmap(1, rect().bottom() - s_clipRec.height(), s_clipRec);
	}

	// clip name
	paintTextLabel(m_clip->name(), p);

	// inner border
	p.setPen( c.lighter( current ? 160 : 130 ) );
	p.drawRect( 1, 1, rect().right() - BORDER_WIDTH,
		rect().bottom() - BORDER_WIDTH );

	// outer border
	p.setPen( current? c.lighter( 130 ) : c.darker( 300 ) );
	p.drawRect( 0, 0, rect().right(), rect().bottom() );

	// draw the 'muted' pixmap only if the clip was manualy muted
	if( m_clip->isMuted() )
	{
		const int spacing = BORDER_WIDTH;
		const int size = 14;
		p.drawPixmap( spacing, height() - ( size + spacing ),
			embed::getIconPixmap( "muted", size, size ) );
	}
	
	if (m_marker)
	{
		p.setPen(markerColor());
		p.drawLine(m_markerPos, rect().bottom(), m_markerPos, rect().top());
	}

	p.end();

	painter.drawPixmap( 0, 0, m_paintPixmap );
}




void AutomationClipView::dragEnterEvent( QDragEnterEvent * _dee )
{
	StringPairDrag::processDragEnterEvent( _dee, "automatable_model" );
	if( !_dee->isAccepted() )
	{
		ClipView::dragEnterEvent( _dee );
	}
}




void AutomationClipView::dropEvent( QDropEvent * _de )
{
	QString type = StringPairDrag::decodeKey( _de );
	QString val = StringPairDrag::decodeValue( _de );
	if( type == "automatable_model" )
	{
		auto mod = dynamic_cast<AutomatableModel*>(Engine::projectJournal()->journallingObject(val.toInt()));
		if( mod != nullptr )
		{
			bool added = m_clip->addObject( mod );
			if ( !added )
			{
				TextFloat::displayMessage( mod->displayName(),
							   tr( "Model is already connected "
							   "to this clip." ),
							   embed::getIconPixmap( "automation" ),
							   2000 );
			}
		}
		update();

		if( getGUI()->automationEditor() &&
			getGUI()->automationEditor()->currentClip() == m_clip )
		{
			getGUI()->automationEditor()->setCurrentClip( m_clip );
		}
	}
	else
	{
		ClipView::dropEvent( _de );
	}
}




/**
 * @brief Preserves the auto points over different scale
 */
void AutomationClipView::scaleTimemapToFit( float oldMin, float oldMax )
{
	float newMin = m_clip->getMin();
	float newMax = m_clip->getMax();

	if( oldMin == newMin && oldMax == newMax )
	{
		return;
	}

	// TODO: Currently when rescaling the timeMap values to fit the new range of values (newMin and newMax)
	// only the inValue is being considered and the outValue is being reset to the inValue (so discrete jumps
	// are discarded). Possibly later we will want discrete jumps to be maintained so we will need to upgrade
	// the logic to account for them.
	for( AutomationClip::timeMap::iterator it = m_clip->m_timeMap.begin();
		it != m_clip->m_timeMap.end(); ++it )
	{
		// If the values are out of the previous range, fix them so they are
		// between oldMin and oldMax.
		if (INVAL(it) < oldMin)
		{
			it.value().setInValue(oldMin);
		}
		else if (INVAL(it) > oldMax)
		{
			it.value().setInValue(oldMax);
		}
		// Calculate what the value would be proportionally in the new range
		it.value().setInValue((INVAL(it) - oldMin) * (newMax - newMin) / (oldMax - oldMin) + newMin);
		// Read earlier TODO comment: For now I'm discarding the discrete jumps during the rescaling
		it.value().setOutValue(INVAL(it));
	}

	m_clip->generateTangents();
}

} // namespace lmms::gui
