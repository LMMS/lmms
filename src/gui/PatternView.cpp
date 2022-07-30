/*
 * Pattern.cpp - implementation of class pattern which holds notes
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2005-2007 Danny McRae <khjklujn/at/yahoo.com>
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

#include "PatternView.h"

#include <QApplication>
#include <QMenu>

#include "ConfigManager.h"
#include "DeprecationHelper.h"
#include "GuiApplication.h"
#include "InstrumentTrack.h"
#include "PianoRoll.h"
#include "RenameDialog.h"

PatternView::PatternView( Pattern* pattern, TrackView* parent ) :
	TrackContentObjectView( pattern, parent ),
	m_pat( pattern ),
	m_paintPixmap(),
	m_noteFillColor(255, 255, 255, 220),
	m_noteBorderColor(255, 255, 255, 220),
	m_mutedNoteFillColor(100, 100, 100, 220),
	m_mutedNoteBorderColor(100, 100, 100, 220),
	m_legacySEBB(ConfigManager::inst()->value("ui","legacysebb","0").toInt())
{
	connect( getGUI()->pianoRoll(), SIGNAL( currentPatternChanged() ),
			this, SLOT( update() ) );

	if( s_stepBtnOn0 == nullptr )
	{
		s_stepBtnOn0 = new QPixmap( embed::getIconPixmap(
							"step_btn_on_0" ) );
	}

	if( s_stepBtnOn200 == nullptr )
	{
		s_stepBtnOn200 = new QPixmap( embed::getIconPixmap(
							"step_btn_on_200" ) );
	}

	if( s_stepBtnOff == nullptr )
	{
		s_stepBtnOff = new QPixmap( embed::getIconPixmap(
							"step_btn_off" ) );
	}

	if( s_stepBtnOffLight == nullptr )
	{
		s_stepBtnOffLight = new QPixmap( embed::getIconPixmap(
						"step_btn_off_light" ) );
	}

	update();

	setStyle( QApplication::style() );
}




Pattern* PatternView::getPattern()
{
	return m_pat;
}




void PatternView::update()
{
	ToolTip::add(this, m_pat->name());

	TrackContentObjectView::update();
}




void PatternView::openInPianoRoll()
{
	getGUI()->pianoRoll()->setCurrentPattern( m_pat );
	getGUI()->pianoRoll()->parentWidget()->show();
	getGUI()->pianoRoll()->show();
	getGUI()->pianoRoll()->setFocus();
}





void PatternView::setGhostInPianoRoll()
{
	getGUI()->pianoRoll()->setGhostPattern( m_pat );
	getGUI()->pianoRoll()->parentWidget()->show();
	getGUI()->pianoRoll()->show();
	getGUI()->pianoRoll()->setFocus();
}




void PatternView::resetName() { m_pat->setName(""); }




void PatternView::changeName()
{
	QString s = m_pat->name();
	RenameDialog rename_dlg( s );
	rename_dlg.exec();
	m_pat->setName( s );
}




void PatternView::constructContextMenu( QMenu * _cm )
{
	QAction * a = new QAction( embed::getIconPixmap( "piano" ),
					tr( "Open in piano-roll" ), _cm );
	_cm->insertAction( _cm->actions()[0], a );
	connect( a, SIGNAL( triggered( bool ) ),
					this, SLOT( openInPianoRoll() ) );

	QAction * b = new QAction( embed::getIconPixmap( "ghost_note" ),
						tr( "Set as ghost in piano-roll" ), _cm );
	if( m_pat->empty() ) { b->setEnabled( false ); }
	_cm->insertAction( _cm->actions()[1], b );
	connect( b, SIGNAL( triggered( bool ) ),
					this, SLOT( setGhostInPianoRoll() ) );
	_cm->insertSeparator( _cm->actions()[2] );
	_cm->addSeparator();

	_cm->addAction( embed::getIconPixmap( "edit_erase" ),
			tr( "Clear all notes" ), m_pat, SLOT( clear() ) );
	_cm->addSeparator();

	_cm->addAction( embed::getIconPixmap( "reload" ), tr( "Reset name" ),
						this, SLOT( resetName() ) );
	_cm->addAction( embed::getIconPixmap( "edit_rename" ),
						tr( "Change name" ),
						this, SLOT( changeName() ) );

	if ( m_pat->type() == Pattern::BeatPattern )
	{
		_cm->addSeparator();

		_cm->addAction( embed::getIconPixmap( "step_btn_add" ),
			tr( "Add steps" ), m_pat, SLOT( addSteps() ) );
		_cm->addAction( embed::getIconPixmap( "step_btn_remove" ),
			tr( "Remove steps" ), m_pat, SLOT( removeSteps() ) );
		_cm->addAction( embed::getIconPixmap( "step_btn_duplicate" ),
			tr( "Clone Steps" ), m_pat, SLOT( cloneSteps() ) );
	}
}




void PatternView::mousePressEvent( QMouseEvent * _me )
{
	bool displayBB = fixedTCOs() || (pixelsPerBar() >= 96 && m_legacySEBB);
	if( _me->button() == Qt::LeftButton &&
		m_pat->m_patternType == Pattern::BeatPattern &&
		displayBB && _me->y() > height() - s_stepBtnOff->height() )

	// when mouse button is pressed in beat/bassline -mode

	{
//	get the step number that was clicked on and
//	do calculations in floats to prevent rounding errors...
		float tmp = ( ( float(_me->x()) - TCO_BORDER_WIDTH ) *
				float( m_pat -> m_steps ) ) / float(width() - TCO_BORDER_WIDTH*2);

		int step = int( tmp );

//	debugging to ensure we get the correct step...
//		qDebug( "Step (%f) %d", tmp, step );

		if( step >= m_pat->m_steps )
		{
			qDebug( "Something went wrong in pattern.cpp: step %d doesn't exist in pattern!", step );
			return;
		}

		Note * n = m_pat->noteAtStep( step );

		if( n == nullptr )
		{
			m_pat->addStepNote( step );
		}
		else // note at step found
		{
			m_pat->addJournalCheckPoint();
			m_pat->setStep( step, false );
		}

		Engine::getSong()->setModified();
		update();

		if( getGUI()->pianoRoll()->currentPattern() == m_pat )
		{
			getGUI()->pianoRoll()->update();
		}
	}
	else

	// if not in beat/bassline -mode, let parent class handle the event

	{
		TrackContentObjectView::mousePressEvent( _me );
	}
}

void PatternView::mouseDoubleClickEvent(QMouseEvent *_me)
{
	if( _me->button() != Qt::LeftButton )
	{
		_me->ignore();
		return;
	}
	if( m_pat->m_patternType == Pattern::MelodyPattern || !fixedTCOs() )
	{
		openInPianoRoll();
	}
}




void PatternView::wheelEvent(QWheelEvent * we)
{
	if(m_pat->m_patternType == Pattern::BeatPattern &&
				(fixedTCOs() || pixelsPerBar() >= 96) &&
				position(we).y() > height() - s_stepBtnOff->height())
	{
//	get the step number that was wheeled on and
//	do calculations in floats to prevent rounding errors...
		float tmp = ((float(position(we).x()) - TCO_BORDER_WIDTH) *
				float(m_pat -> m_steps)) / float(width() - TCO_BORDER_WIDTH*2);

		int step = int( tmp );

		if( step >= m_pat->m_steps )
		{
			return;
		}

		Note * n = m_pat->noteAtStep( step );
		if(!n && we->angleDelta().y() > 0)
		{
			n = m_pat->addStepNote( step );
			n->setVolume( 0 );
		}
		if( n != nullptr )
		{
			int vol = n->getVolume();

			if(we->angleDelta().y() > 0)
			{
				n->setVolume( qMin( 100, vol + 5 ) );
			}
			else
			{
				n->setVolume( qMax( 0, vol - 5 ) );
			}

			Engine::getSong()->setModified();
			update();
			if( getGUI()->pianoRoll()->currentPattern() == m_pat )
			{
				getGUI()->pianoRoll()->update();
			}
		}
		we->accept();
	}
	else
	{
		TrackContentObjectView::wheelEvent(we);
	}
}


static int computeNoteRange(int minKey, int maxKey)
{
	return (maxKey - minKey) + 1;
}

void PatternView::paintEvent( QPaintEvent * )
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

	QColor c;
	bool const muted = m_pat->getTrack()->isMuted() || m_pat->isMuted();
	bool current = getGUI()->pianoRoll()->currentPattern() == m_pat;
	bool beatPattern = m_pat->m_patternType == Pattern::BeatPattern;

	if( beatPattern )
	{
		// Do not paint BBTCOs how we paint pattern TCOs
		c = BBPatternBackground();
	}
	else
	{
		c = getColorForDisplay( painter.background().color() );
	}

	// invert the gradient for the background in the B&B editor
	QLinearGradient lingrad( 0, 0, 0, height() );
	lingrad.setColorAt( beatPattern ? 0 : 1, c.darker( 300 ) );
	lingrad.setColorAt( beatPattern ? 1 : 0, c );

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

	// Check whether we will paint a text box and compute its potential height
	// This is needed so we can paint the notes underneath it.
	bool const drawName = !m_pat->name().isEmpty();
	bool const drawTextBox = !beatPattern && drawName;

	// TODO Warning! This might cause problems if TrackContentObjectView::paintTextLabel changes
	int textBoxHeight = 0;
	const int textTop = TCO_BORDER_WIDTH + 1;
	if (drawTextBox)
	{
		QFont labelFont = this->font();
		labelFont.setHintingPreference( QFont::PreferFullHinting );

		QFontMetrics fontMetrics(labelFont);
		textBoxHeight = fontMetrics.height() + 2 * textTop;
	}

	// Compute pixels per bar
	const int baseWidth = fixedTCOs() ? parentWidget()->width() - 2 * TCO_BORDER_WIDTH
						: width() - TCO_BORDER_WIDTH;
	const float pixelsPerBar = baseWidth / (float) m_pat->length().getBar();

	// Length of one bar/beat in the [0,1] x [0,1] coordinate system
	const float barLength = 1. / m_pat->length().getBar();
	const float tickLength = barLength / TimePos::ticksPerBar();

	const int x_base = TCO_BORDER_WIDTH;

	bool displayBB = fixedTCOs() || (pixelsPerBar >= 96 && m_legacySEBB);
	// melody pattern paint event
	NoteVector const & noteCollection = m_pat->m_notes;
	if( m_pat->m_patternType == Pattern::MelodyPattern && !noteCollection.empty() )
	{
		// Compute the minimum and maximum key in the pattern
		// so that we know how much there is to draw.
		int maxKey = std::numeric_limits<int>::min();
		int minKey = std::numeric_limits<int>::max();

		for (Note const * note : noteCollection)
		{
			int const key = note->key();
			maxKey = qMax( maxKey, key );
			minKey = qMin( minKey, key );
		}

		// If needed adjust the note range so that we always have paint a certain interval
		int const minimalNoteRange = 12; // Always paint at least one octave
		int const actualNoteRange = computeNoteRange(minKey, maxKey);

		if (actualNoteRange < minimalNoteRange)
		{
			int missingNumberOfNotes = minimalNoteRange - actualNoteRange;
			minKey = std::max(0, minKey - missingNumberOfNotes / 2);
			maxKey = maxKey + missingNumberOfNotes / 2;
			if (missingNumberOfNotes % 2 == 1)
			{
				// Put more range at the top to bias drawing towards the bottom
				++maxKey;
			}
		}

		int const adjustedNoteRange = computeNoteRange(minKey, maxKey);

		// Transform such that [0, 1] x [0, 1] paints in the correct area
		float distanceToTop = textBoxHeight;

		// This moves the notes smoothly under the text
		int widgetHeight = height();
		int fullyAtTopAtLimit = MINIMAL_TRACK_HEIGHT;
		int fullyBelowAtLimit = 4 * fullyAtTopAtLimit;
		if (widgetHeight <= fullyBelowAtLimit)
		{
			if (widgetHeight <= fullyAtTopAtLimit)
			{
				distanceToTop = 0;
			}
			else
			{
				float const a = 1. / (fullyAtTopAtLimit - fullyBelowAtLimit);
				float const b = - float(fullyBelowAtLimit) / (fullyAtTopAtLimit - fullyBelowAtLimit);
				float const scale = a * widgetHeight + b;
				distanceToTop = (1. - scale) * textBoxHeight;
			}
		}

		int const notesBorder = 4; // Border for the notes towards the top and bottom in pixels

		// The relavant painting code starts here
		p.save();

		p.translate(0., distanceToTop + notesBorder);
		p.scale(width(), height() - distanceToTop - 2 * notesBorder);

		// set colour based on mute status
		QColor noteFillColor = muted ? getMutedNoteFillColor() : getNoteFillColor();
		QColor noteBorderColor = muted ? getMutedNoteBorderColor()
									   : ( m_pat->hasColor() ? c.lighter( 200 ) : getNoteBorderColor() );

		bool const drawAsLines = height() < 64;
		if (drawAsLines)
		{
			p.setPen(noteFillColor);
		}
		else
		{
			p.setPen(noteBorderColor);
			p.setRenderHint(QPainter::Antialiasing);
		}

		// Needed for Qt5 although the documentation for QPainter::setPen(QColor) as it's used above
		// states that it should already set a width of 0.
		QPen pen = p.pen();
		pen.setWidth(0);
		p.setPen(pen);

		float const noteHeight = 1. / adjustedNoteRange;

		// scan through all the notes and draw them on the pattern
		for (Note const * currentNote : noteCollection)
		{
			// Map to 0, 1, 2, ...
			int mappedNoteKey = currentNote->key() - minKey;
			int invertedMappedNoteKey = adjustedNoteRange - mappedNoteKey - 1;

			float const noteStartX = currentNote->pos() * tickLength;
			float const noteLength = currentNote->length() * tickLength;

			float const noteStartY = invertedMappedNoteKey * noteHeight;

			QRectF noteRectF( noteStartX, noteStartY, noteLength, noteHeight);
			if (drawAsLines)
			{
				p.drawLine(QPointF(noteStartX, noteStartY + 0.5 * noteHeight),
					   QPointF(noteStartX + noteLength, noteStartY + 0.5 * noteHeight));
			}
			else
			{
				p.fillRect( noteRectF, noteFillColor );
				p.drawRect( noteRectF );
			}
		}

		p.restore();
	}
	// beat pattern paint event
	else if( beatPattern &&	displayBB )
	{
		QPixmap stepon0;
		QPixmap stepon200;
		QPixmap stepoff;
		QPixmap stepoffl;
		const int steps = qMax( 1,
					m_pat->m_steps );
		const int w = width() - 2 * TCO_BORDER_WIDTH;

		// scale step graphics to fit the beat pattern length
		stepon0 = s_stepBtnOn0->scaled( w / steps,
					      s_stepBtnOn0->height(),
					      Qt::IgnoreAspectRatio,
					      Qt::SmoothTransformation );
		stepon200 = s_stepBtnOn200->scaled( w / steps,
					      s_stepBtnOn200->height(),
					      Qt::IgnoreAspectRatio,
					      Qt::SmoothTransformation );
		stepoff = s_stepBtnOff->scaled( w / steps,
						s_stepBtnOff->height(),
						Qt::IgnoreAspectRatio,
						Qt::SmoothTransformation );
		stepoffl = s_stepBtnOffLight->scaled( w / steps,
						s_stepBtnOffLight->height(),
						Qt::IgnoreAspectRatio,
						Qt::SmoothTransformation );

		for( int it = 0; it < steps; it++ )	// go through all the steps in the beat pattern
		{
			Note * n = m_pat->noteAtStep( it );

			// figure out x and y coordinates for step graphic
			const int x = TCO_BORDER_WIDTH + static_cast<int>( it * w / steps );
			const int y = height() - s_stepBtnOff->height() - 1;

			if( n )
			{
				const int vol = n->getVolume();
				p.drawPixmap( x, y, stepoffl );
				p.drawPixmap( x, y, stepon0 );
				p.setOpacity( sqrt( vol / 200.0 ) );
				p.drawPixmap( x, y, stepon200 );
				p.setOpacity( 1 );
			}
			else if( ( it / 4 ) % 2 )
			{
				p.drawPixmap( x, y, stepoffl );
			}
			else
			{
				p.drawPixmap( x, y, stepoff );
			}
		} // end for loop

		// draw a transparent rectangle over muted patterns
		if ( muted )
		{
			p.setBrush( mutedBackgroundColor() );
			p.setOpacity( 0.5 );
			p.drawRect( 0, 0, width(), height() );
		}
	}

	// bar lines
	const int lineSize = 3;
	p.setPen( c.darker( 200 ) );

	for( bar_t t = 1; t < m_pat->length().getBar(); ++t )
	{
		p.drawLine( x_base + static_cast<int>( pixelsPerBar * t ) - 1,
				TCO_BORDER_WIDTH, x_base + static_cast<int>(
						pixelsPerBar * t ) - 1, TCO_BORDER_WIDTH + lineSize );
		p.drawLine( x_base + static_cast<int>( pixelsPerBar * t ) - 1,
				rect().bottom() - ( lineSize + TCO_BORDER_WIDTH ),
				x_base + static_cast<int>( pixelsPerBar * t ) - 1,
				rect().bottom() - TCO_BORDER_WIDTH );
	}

	// pattern name
	if (drawTextBox)
	{
		paintTextLabel(m_pat->name(), p);
	}

	if( !( fixedTCOs() && beatPattern ) )
	{
		// inner border
		p.setPen( c.lighter( current ? 160 : 130 ) );
		p.drawRect( 1, 1, rect().right() - TCO_BORDER_WIDTH,
			rect().bottom() - TCO_BORDER_WIDTH );

		// outer border
		p.setPen( current ? c.lighter( 130 ) : c.darker( 300 ) );
		p.drawRect( 0, 0, rect().right(), rect().bottom() );
	}

	// draw the 'muted' pixmap only if the pattern was manually muted
	if( m_pat->isMuted() )
	{
		const int spacing = TCO_BORDER_WIDTH;
		const int size = 14;
		p.drawPixmap( spacing, height() - ( size + spacing ),
			embed::getIconPixmap( "muted", size, size ) );
	}

	painter.drawPixmap( 0, 0, m_paintPixmap );
}
