/*
 * SampleClipView.cpp
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
 
#include "SampleClipView.h"

#include <QApplication>
#include <QMenu>
#include <QPainter>

#include "FileDialog.h"
#include "GuiApplication.h"
#include "AutomationEditor.h"
#include "embed.h"
#include "PathUtil.h"
#include "SampleClip.h"
#include "SampleThumbnail.h"
#include "Song.h"
#include "StringPairDrag.h"
#include "TrackContainerView.h"
#include "TrackView.h"

namespace lmms::gui
{


SampleClipView::SampleClipView( SampleClip * _clip, TrackView * _tv, int offset ) :
	ClipView( _clip, _tv, offset ),
	m_clip( _clip ),
	m_paintPixmap(),
	m_paintPixmapXPosition(0)
{
	// update UI and tooltip
	updateSample();

	// track future changes of SampleClip
	connect(m_clip, SIGNAL(sampleChanged()), this, SLOT(updateSample()));

	connect(m_clip, SIGNAL(wasReversed()), this, SLOT(update()));

	setStyle( QApplication::style() );

	if ( offset == 0 && m_clip->loopCount() > 0 )
	{
		int loopCount = m_clip->loopCount();
		while ( m_clip->loopCount() > 0 )
		{
			// We set the loop count to 0 so the lastLoopView() check in loop() returns the expected value
			m_clip->decreaseLoopCount();
		}
		while ( m_clip->loopCount() < loopCount )
		{
			loop();
		}
	}
}

void SampleClipView::updateSample()
{
	update();

	m_sampleThumbnail = SampleThumbnail{m_clip->m_sample};

	// set tooltip to filename so that user can see what sample this
	// sample-clip contains
	setToolTip(
		!m_clip->m_sample.sampleFile().isEmpty()
			? PathUtil::toAbsolute(m_clip->m_sample.sampleFile())
			: tr("Double-click to open sample")
	);
}




void SampleClipView::constructContextMenu(QMenu* cm)
{
	cm->addSeparator();

	/*contextMenu.addAction( embed::getIconPixmap( "record" ),
				tr( "Set/clear record" ),
						m_clip, SLOT(toggleRecord()));*/

	cm->addAction(
		embed::getIconPixmap("flip_x"),
		tr("Reverse sample"),
		this,
		SLOT(reverseSample())
	);

	cm->addAction(
		embed::getIconPixmap("automation_ghost_note"),
		tr("Set as ghost in automation editor"),
		this,
		SLOT(setAutomationGhost())
	);

}




void SampleClipView::dragEnterEvent( QDragEnterEvent * _dee )
{
	if( StringPairDrag::processDragEnterEvent( _dee,
					"samplefile,sampledata" ) == false )
	{
		ClipView::dragEnterEvent( _dee );
	}
}






void SampleClipView::dropEvent( QDropEvent * _de )
{
	if( StringPairDrag::decodeKey( _de ) == "samplefile" )
	{
		m_clip->setSampleFile( StringPairDrag::decodeValue( _de ) );
		_de->accept();
	}
	else if( StringPairDrag::decodeKey( _de ) == "sampledata" )
	{
		m_clip->setSampleBuffer(SampleBuffer::fromBase64(StringPairDrag::decodeValue(_de)));
		m_clip->updateLength();
		update();
		_de->accept();
	}
	else
	{
		ClipView::dropEvent( _de );
	}
}




void SampleClipView::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton &&
		_me->modifiers() & Qt::ControlModifier &&
		_me->modifiers() & Qt::ShiftModifier )
	{
		m_clip->toggleRecord();
	}
	else
	{
		if( _me->button() == Qt::MiddleButton && _me->modifiers() == Qt::ControlModifier )
		{
			auto sClip = dynamic_cast<SampleClip*>(getClip());
			if( sClip )
			{
				sClip->updateTrackClips();
			}
		}
		ClipView::mousePressEvent( _me );
	}
}




void SampleClipView::mouseReleaseEvent(QMouseEvent *_me)
{
	if( _me->button() == Qt::MiddleButton && !_me->modifiers() )
	{
		auto sClip = dynamic_cast<SampleClip*>(getClip());
		if( sClip )
		{
			sClip->playbackPositionChanged();
		}
	}
	ClipView::mouseReleaseEvent( _me );
}




void SampleClipView::mouseDoubleClickEvent( QMouseEvent * )
{
	if (m_trackView->trackContainerView()->knifeMode()) { return; }

	const QString selectedAudioFile = FileDialog::openAudioFile();

	if (selectedAudioFile.isEmpty()) { return; }
	
	if (!m_clip->hasSampleFileLoaded(selectedAudioFile))
	{
		auto sampleBuffer = SampleBuffer::fromFile(selectedAudioFile);
		if (sampleBuffer != SampleBuffer::emptyBuffer())
		{
			m_clip->setSampleBuffer(sampleBuffer);
		}
	}
	m_clip->updateLength();
}




void SampleClipView::paintEvent( QPaintEvent * pe )
{
	QPainter painter( this );

	if( !needsUpdate() )
	{
		painter.drawPixmap(m_paintPixmapXPosition, 0, m_paintPixmap);
		return;
	}

	setNeedsUpdate( false );

	const auto trackViewWidth = getTrackView()->rect().width();

	// Use the clip's height to avoid artifacts when rendering while something else is overlaying the clip.
	const auto viewPortRect = QRect(0, 0, trackViewWidth * 2, rect().height());

	m_paintPixmapXPosition = std::max(0, pe->rect().x() - trackViewWidth);

	if (m_paintPixmap.isNull() || m_paintPixmap.size() != viewPortRect.size())
	{
		m_paintPixmap = QPixmap(viewPortRect.size());
	}

	QPainter p( &m_paintPixmap );

	bool muted = m_clip->getTrack()->isMuted() || m_clip->isMuted();
	bool selected = isSelected();

	QLinearGradient lingrad(0, 0, 0, height());
	QColor c = painter.background().color();
	if (muted) { c = c.darker(150); }
	if (selected) { c = c.darker(150); }

	lingrad.setColorAt( 1, c.darker( 300 ) );
	lingrad.setColorAt( 0, c );

	// paint a black rectangle under the clip to prevent glitches with transparent backgrounds
	p.fillRect( rect(), QColor( 0, 0, 0 ) );

	auto clipColor = m_clip->color().value_or(m_clip->getTrack()->color().value_or(painter.pen().brush().color()));

	if( gradient() )
	{
		p.fillRect( rect(), lingrad );
	}
	else
	{
		if (this->offset() == 0)
		{
			p.fillRect(rect(), c);
		}
		// Draw loop views with a slight color difference and hatch them (after painting the shape of the waveform bellow)
		else
		{
			p.fillRect(rect(), c.darker( 150 ));
			p.setPen( clipColor.darker( 150 ) );

			// Change the pen's width in a rather painfull way
			QPen previousPen = p.pen();
			QPen newPen(previousPen);
			newPen.setWidth(2);
			p.setPen(newPen);

			for (int x = -height(); x < width(); x += 10)
			{
				int y1, y2;
				y1 = 0;
				if (x < 0)
				{
					y1 = -x;
				}
				y2 = height();
				if (x + height() > width())
				{
					y2 = height() - (x + height() - width());
				}
				if (y1 < height() && y2 > 0)
				{
					p.drawLine(std::max( 0, x ), y1, std::min( width(), x + height() ), y2);
				}
			}
			p.setPen(previousPen);
		}
	}

	p.setPen(clipColor);

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

	const int spacing = BORDER_WIDTH + 1;
	const float ppb = fixedClips() ?
			( parentWidget()->width() - 2 * BORDER_WIDTH )
					/ (float) m_clip->length().getBar() :
								pixelsPerBar();

	float nom = Engine::getSong()->getTimeSigModel().getNumerator();
	float den = Engine::getSong()->getTimeSigModel().getDenominator();
	float ticksPerBar = DefaultTicksPerBar * nom / den;
	float offsetStart = m_clip->startTimeOffset() / ticksPerBar * pixelsPerBar();
	float sampleLength = m_clip->sampleLength() * ppb / ticksPerBar;

	const auto& sample = m_clip->m_sample;

	const auto sampleRextX = static_cast<int>(offsetStart) - m_paintPixmapXPosition;

	if (sample.sampleSize() > 0)
	{
		const auto param = SampleThumbnail::VisualizeParameters{
			.sampleRect = QRect(sampleRextX, spacing, sampleLength, height() - spacing),
			.viewportRect = viewPortRect,
			.amplification = sample.amplification(),
			.reversed = sample.reversed()
		};

		m_sampleThumbnail.visualize(param, p);
	}

	if ( this->offset() == 0 )
	{
		// Only draw the name on the first loop view, otherwise it would be drawn multiple times and look bad
		QString name = PathUtil::cleanName(m_clip->m_sample.sampleFile());
		paintTextLabel(name, p);
	}

	// disable antialiasing for borders, since its not needed
	p.setRenderHint( QPainter::Antialiasing, false );

	if ( this->offset() == 0 )
	{
		// inner border
		p.setPen( c.lighter( 135 ) );
		p.drawRect(
			-m_paintPixmapXPosition + 1,
			1,
			rect().right() - BORDER_WIDTH,
			rect().bottom() - BORDER_WIDTH );

		// outer border
		p.setPen( c.darker( 200 ) );
		p.drawRect(-m_paintPixmapXPosition, 0, rect().right(), rect().bottom());
	}
	// In case of a loop view, we don't draw inner border and don't draw borders between loop views
	else
	{
		p.setPen( c.darker( 300 ) );
		p.drawLine( 0, 0, rect().right(), 0 );
		p.drawLine( 0, rect().bottom(), rect().right(), rect().bottom() );

		// Last loop view gets a right border
		if ( this->offset() == m_clip->loopCount() )
		{
			p.drawLine( rect().right(), 0, rect().right(), rect().bottom() );
		}
	}

	// draw the 'muted' pixmap only if the clip was manually muted
	if( m_clip->isMuted()  && this->offset() == 0 )
	{
		const int spacing = BORDER_WIDTH;
		const int size = 14;
		p.drawPixmap( spacing, height() - ( size + spacing ),
			embed::getIconPixmap( "muted", size, size ) );
	}

	if ( m_marker  && this->offset() == 0 )
	{
		p.setPen(markerColor());
		p.drawLine(m_markerPos, rect().bottom(), m_markerPos, rect().top());
	}
	// recording sample tracks is not possible at the moment

	/* if( m_clip->isRecord() )
	{
		p.setFont( pointSize<7>( p.font() ) );

		p.setPen( textShadowColor() );
		p.drawText( 10, p.fontMetrics().height()+1, "Rec" );
		p.setPen( textColor() );
		p.drawText( 9, p.fontMetrics().height(), "Rec" );

		p.setBrush( QBrush( textColor() ) );
		p.drawEllipse( 4, 5, 4, 4 );
	}*/

	p.end();

	painter.drawPixmap(m_paintPixmapXPosition, 0, m_paintPixmap);
}




void SampleClipView::loop()
{
	// We don't create a loop if there's already one
	if ( lastLoopView() )
	{
		SampleClipView* newLoop = new SampleClipView(m_clip, m_trackView, offset() + 1);
		connect(this, SIGNAL(closing()), newLoop, SLOT(closeLoopViews()));
		connect(this, SIGNAL(extandLoop()), newLoop, SLOT(loop()));
	}
	else
	{
		extandLoop();
	}
}




void SampleClipView::reverseSample()
{
	m_clip->m_sample.setReversed(!m_clip->m_sample.reversed());
	m_clip->setStartTimeOffset(m_clip->length() - m_clip->startTimeOffset() - m_clip->sampleLength());
	Engine::getSong()->setModified();
	update();
}



void SampleClipView::setAutomationGhost()
{
	auto aEditor = gui::getGUI()->automationEditor();
	aEditor->setGhostSample(m_clip);
	aEditor->parentWidget()->show();
	aEditor->show();
	aEditor->setFocus();
}

} // namespace lmms::gui
