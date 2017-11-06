/*
 * SampleTCOView.cpp
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 
#include "SampleTCOView.h"

#include <QMenu>
#include <QPainter>

#include "embed.h"
#include "gui_templates.h"
#include "PathUtil.h"
#include "Song.h"
#include "StringPairDrag.h"
#include "ToolTip.h"

SampleTCOView::SampleTCOView( SampleTCO * _tco, TrackView * _tv ) :
	TrackContentObjectView( _tco, _tv ),
	m_tco( _tco ),
	m_paintPixmap()
{
	// update UI and tooltip
	updateSample();

	// track future changes of SampleTCO
	connect(m_tco, SIGNAL(sampleChanged()), this, SLOT(updateSample()));

	connect(m_tco, SIGNAL(wasReversed()), this, SLOT(update()));

	setStyle( QApplication::style() );
}

void SampleTCOView::updateSample()
{
	update();
	// set tooltip to filename so that user can see what sample this
	// sample-tco contains
	ToolTip::add( this, ( m_tco->m_sampleBuffer->audioFile() != "" ) ?
					PathUtil::toAbsolute(m_tco->m_sampleBuffer->audioFile()) :
					tr( "Double-click to open sample" ) );
}




void SampleTCOView::constructContextMenu(QMenu* cm)
{
	cm->addSeparator();


	cm->addAction(embed::getIconPixmap( "record" ),
                          tr("Set/clear record"),
                          m_tco, SLOT(toggleRecord()));

	cm->addAction(
		embed::getIconPixmap("flip_x"),
		tr("Reverse sample"),
		this,
		SLOT(reverseSample())
	);


}




void SampleTCOView::dragEnterEvent( QDragEnterEvent * _dee )
{
	if( StringPairDrag::processDragEnterEvent( _dee,
					"samplefile,sampledata" ) == false )
	{
		TrackContentObjectView::dragEnterEvent( _dee );
	}
}






void SampleTCOView::dropEvent( QDropEvent * _de )
{
	if( StringPairDrag::decodeKey( _de ) == "samplefile" )
	{
		m_tco->setSampleFile( StringPairDrag::decodeValue( _de ) );
		_de->accept();
	}
	else if( StringPairDrag::decodeKey( _de ) == "sampledata" )
	{
		m_tco->m_sampleBuffer->loadFromBase64(
					StringPairDrag::decodeValue( _de ) );
		m_tco->updateLength();
		update();
		_de->accept();
		Engine::getSong()->setModified();
	}
	else
	{
		TrackContentObjectView::dropEvent( _de );
	}
}




void SampleTCOView::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton &&
		_me->modifiers() & Qt::ControlModifier &&
		_me->modifiers() & Qt::ShiftModifier )
	{
		m_tco->toggleRecord();
	}
	else
	{
		if( _me->button() == Qt::MiddleButton && _me->modifiers() == Qt::ControlModifier )
		{
			SampleTCO * sTco = dynamic_cast<SampleTCO*>( getTrackContentObject() );
			if( sTco )
			{
				sTco->updateTrackTcos();
			}
		}
		TrackContentObjectView::mousePressEvent( _me );
	}
}




void SampleTCOView::mouseReleaseEvent(QMouseEvent *_me)
{
	if( _me->button() == Qt::MiddleButton && !_me->modifiers() )
	{
		SampleTCO * sTco = dynamic_cast<SampleTCO*>( getTrackContentObject() );
		if( sTco )
		{
			sTco->playbackPositionChanged();
		}
	}
	TrackContentObjectView::mouseReleaseEvent( _me );
}




void SampleTCOView::mouseDoubleClickEvent( QMouseEvent * )
{
	QString af = m_tco->m_sampleBuffer->openAudioFile();

	if ( af.isEmpty() ) {} //Don't do anything if no file is loaded
	else if ( af == m_tco->m_sampleBuffer->audioFile() )
	{	//Instead of reloading the existing file, just reset the size
		int length = (int) ( m_tco->m_sampleBuffer->frames() / Engine::framesPerTick() );
		m_tco->changeLength(length);
	}
	else
	{	//Otherwise load the new file as ususal
		m_tco->setSampleFile( af );
		Engine::getSong()->setModified();
	}
}




void SampleTCOView::paintEvent( QPaintEvent * pe )
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

	bool muted = m_tco->getTrack()->isMuted() || m_tco->isMuted();
	bool selected = isSelected();

	QLinearGradient lingrad(0, 0, 0, height());
	QColor c = painter.background().color();
	if (muted) { c = c.darker(150); }
	if (selected) { c = c.darker(150); }

	lingrad.setColorAt( 1, c.darker( 300 ) );
	lingrad.setColorAt( 0, c );

	// paint a black rectangle under the pattern to prevent glitches with transparent backgrounds
	p.fillRect( rect(), QColor( 0, 0, 0 ) );

	if( gradient() )
	{
		p.fillRect( rect(), lingrad );
	}
	else
	{
		p.fillRect( rect(), c );
	}

	auto tcoColor = m_tco->hasColor()
			? (m_tco->usesCustomClipColor()
				? m_tco->color()
				: m_tco->getTrack()->color())
			: painter.pen().brush().color();

	p.setPen(tcoColor);

	if (muted)
	{
		QColor penColor = p.pen().brush().color();
		penColor.setHsv(penColor.hsvHue(), penColor.hsvSaturation() / 4, penColor.value());
		p.setPen(penColor.darker(250));
	}
	if (selected)
	{
		p.setPen(p.pen().brush().color().darker(150));
	}

	const int spacing = TCO_BORDER_WIDTH + 1;
	const float ppb = fixedTCOs() ?
			( parentWidget()->width() - 2 * TCO_BORDER_WIDTH )
					/ (float) m_tco->length().getBar() :
								pixelsPerBar();

	float nom = Engine::getSong()->getTimeSigModel().getNumerator();
	float den = Engine::getSong()->getTimeSigModel().getDenominator();
	float ticksPerBar = DefaultTicksPerBar * nom / den;

	float offset =  m_tco->startTimeOffset() / ticksPerBar * pixelsPerBar();
	QRect r = QRect( offset, spacing,
			qMax( static_cast<int>( m_tco->sampleLength() * ppb / ticksPerBar ), 1 ), rect().bottom() - 2 * spacing );
	m_tco->m_sampleBuffer->visualize( p, r, pe->rect() );

	QString name = PathUtil::cleanName(m_tco->m_sampleBuffer->audioFile());
	paintTextLabel(name, p);

	// disable antialiasing for borders, since its not needed
	p.setRenderHint( QPainter::Antialiasing, false );

	// inner border
	p.setPen( c.lighter( 135 ) );
	p.drawRect( 1, 1, rect().right() - TCO_BORDER_WIDTH,
		rect().bottom() - TCO_BORDER_WIDTH );

	// outer border
	p.setPen( c.darker( 200 ) );
	p.drawRect( 0, 0, rect().right(), rect().bottom() );

	// draw the 'muted' pixmap only if the pattern was manualy muted
	if( m_tco->isMuted() )
	{
		const int spacing = TCO_BORDER_WIDTH;
		const int size = 14;
		p.drawPixmap( spacing, height() - ( size + spacing ),
			embed::getIconPixmap( "muted", size, size ) );
	}

	if ( m_marker )
	{
		p.drawLine(m_markerPos, rect().bottom(), m_markerPos, rect().top());
	}
	// recording sample tracks is not possible at the moment

	if(m_tco->isRecord())
	{
		p.setFont( pointSize<7>( p.font() ) );

		p.setPen( textShadowColor() );
		p.drawText( 10, p.fontMetrics().height()+1, "Rec" );
		p.setPen( textColor() );
		p.drawText( 9, p.fontMetrics().height(), "Rec" );

		p.setBrush( QBrush( textColor() ) );
		p.drawEllipse( 4, 5, 4, 4 );
	}

	p.end();

	painter.drawPixmap( 0, 0, m_paintPixmap );
}




void SampleTCOView::reverseSample()
{
	m_tco->sampleBuffer()->setReversed(!m_tco->sampleBuffer()->reversed());
	Engine::getSong()->setModified();
	update();
}




//! Split this TCO.
/*! \param pos the position of the split, relative to the start of the clip */
bool SampleTCOView::splitTCO( const TimePos pos )
{
	setMarkerEnabled( false );

	const TimePos splitPos = m_initialTCOPos + pos;

	//Don't split if we slid off the TCO or if we're on the clip's start/end
	//Cutting at exactly the start/end position would create a zero length
	//clip (bad), and a clip the same length as the original one (pointless).
	if ( splitPos > m_initialTCOPos && splitPos < m_initialTCOEnd )
	{
		m_tco->getTrack()->addJournalCheckPoint();
		m_tco->getTrack()->saveJournallingState( false );

		SampleTCO * rightTCO = new SampleTCO ( *m_tco );

		m_tco->changeLength( splitPos - m_initialTCOPos );

		rightTCO->movePosition( splitPos );
		rightTCO->changeLength( m_initialTCOEnd - splitPos );
		rightTCO->setStartTimeOffset( m_tco->startTimeOffset() - m_tco->length() );

		m_tco->getTrack()->restoreJournallingState();
		return true;
	}
	else { return false; }
}