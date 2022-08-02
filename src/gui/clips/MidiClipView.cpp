/*
 * MidiClipView.cpp - implementation of class MidiClipView which displays notes
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

#include "MidiClipView.h"

#include <cmath>
#include <QApplication>
#include <QInputDialog>
#include <QMenu>
#include <QPainter>

#include "ConfigManager.h"
#include "DeprecationHelper.h"
#include "GuiApplication.h"
#include "MidiClip.h"
#include "PianoRoll.h"
#include "RenameDialog.h"
#include "TrackView.h"

namespace lmms::gui
{


MidiClipView::MidiClipView( MidiClip* clip, TrackView* parent ) :
	ClipView( clip, parent ),
	m_clip( clip ),
	m_paintPixmap(),
	m_noteFillColor(255, 255, 255, 220),
	m_noteBorderColor(255, 255, 255, 220),
	m_mutedNoteFillColor(100, 100, 100, 220),
	m_mutedNoteBorderColor(100, 100, 100, 220),
	// TODO if this option is ever added to the GUI, rename it to legacysepattern
	m_legacySEPattern(ConfigManager::inst()->value("ui", "legacysebb", "0").toInt())
{
	connect( getGUI()->pianoRoll(), SIGNAL(currentMidiClipChanged()),
			this, SLOT(update()));

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




MidiClip* MidiClipView::getMidiClip()
{
	return m_clip;
}




void MidiClipView::update()
{
	setToolTip(m_clip->name());

	ClipView::update();
}




void MidiClipView::openInPianoRoll()
{
	getGUI()->pianoRoll()->setCurrentMidiClip( m_clip );
	getGUI()->pianoRoll()->parentWidget()->show();
	getGUI()->pianoRoll()->show();
	getGUI()->pianoRoll()->setFocus();
}





void MidiClipView::setGhostInPianoRoll()
{
	getGUI()->pianoRoll()->setGhostMidiClip( m_clip );
	getGUI()->pianoRoll()->parentWidget()->show();
	getGUI()->pianoRoll()->show();
	getGUI()->pianoRoll()->setFocus();
}




void MidiClipView::resetName() { m_clip->setName(""); }




void MidiClipView::changeName()
{
	QString s = m_clip->name();
	RenameDialog rename_dlg( s );
	rename_dlg.exec();
	m_clip->setName( s );
}




void MidiClipView::transposeSelection()
{
	const auto selection = getClickedClips();

	// Calculate the key boundries for all clips
	int highest = 0;
	int lowest = NumKeys - 1;
	for (ClipView* clipview: selection)
	{
		if (auto mcv = qobject_cast<MidiClipView*>(clipview))
		{
			if (auto bounds = boundsForNotes(mcv->getMidiClip()->notes()))
			{
				lowest = std::min(bounds->lowest, lowest);
				highest = std::max(bounds->highest, highest);
			}
		}
	}

	int semitones = QInputDialog::getInt(this, tr("Transpose"), tr("Semitones to transpose by:"),
		/*start*/ 0, /*min*/ -lowest, /*max*/ (NumKeys - 1 - highest));

	if (semitones == 0) { return; }

	// TODO make this not crash
	// Engine::getSong()->addJournalCheckPoint();

	QSet<Track*> m_changedTracks;
	for (ClipView* clipview: selection)
	{
		auto mcv = qobject_cast<MidiClipView*>(clipview);
		if (!mcv) { continue; }

		auto clip = mcv->getMidiClip();
		if (clip->notes().empty()) { continue; }

		auto track = clipview->getTrackView()->getTrack();
		if (!m_changedTracks.contains(track))
		{
			track->addJournalCheckPoint();
			m_changedTracks.insert(track);
		}

		for (Note* note: clip->notes())
		{
			note->setKey(note->key() + semitones);
		}
		emit clip->dataChanged();
	}
	// At least one clip must have notes to show the transpose dialog, so something *has* changed
	Engine::getSong()->setModified();
}




void MidiClipView::constructContextMenu( QMenu * _cm )
{
	bool isBeat = m_clip->type() == MidiClip::BeatClip;

	QAction * a = new QAction( embed::getIconPixmap( "piano" ),
					tr( "Open in piano-roll" ), _cm );
	_cm->insertAction( _cm->actions()[0], a );
	connect( a, SIGNAL(triggered(bool)),
					this, SLOT(openInPianoRoll()));

	QAction * b = new QAction( embed::getIconPixmap( "ghost_note" ),
						tr( "Set as ghost in piano-roll" ), _cm );
	if( m_clip->empty() ) { b->setEnabled( false ); }
	_cm->insertAction( _cm->actions()[1], b );
	connect( b, SIGNAL(triggered(bool)),
					this, SLOT(setGhostInPianoRoll()));
	_cm->insertSeparator( _cm->actions()[2] );
	_cm->addSeparator();

	_cm->addAction( embed::getIconPixmap( "edit_erase" ),
			tr( "Clear all notes" ), m_clip, SLOT(clear()));
	if (!isBeat)
	{
		_cm->addAction(embed::getIconPixmap("scale"), tr("Transpose"), this, &MidiClipView::transposeSelection);
	}
	_cm->addSeparator();

	_cm->addAction( embed::getIconPixmap( "reload" ), tr( "Reset name" ),
						this, SLOT(resetName()));
	_cm->addAction( embed::getIconPixmap( "edit_rename" ),
						tr( "Change name" ),
						this, SLOT(changeName()));

	if (isBeat)
	{
		_cm->addSeparator();

		_cm->addAction( embed::getIconPixmap( "step_btn_add" ),
			tr( "Add steps" ), m_clip, SLOT(addSteps()));
		_cm->addAction( embed::getIconPixmap( "step_btn_remove" ),
			tr( "Remove steps" ), m_clip, SLOT(removeSteps()));
		_cm->addAction( embed::getIconPixmap( "step_btn_duplicate" ),
			tr( "Clone Steps" ), m_clip, SLOT(cloneSteps()));
	}
}




void MidiClipView::mousePressEvent( QMouseEvent * _me )
{
	bool displayPattern = fixedClips() || (pixelsPerBar() >= 96 && m_legacySEPattern);
	if( _me->button() == Qt::LeftButton &&
		m_clip->m_clipType == MidiClip::BeatClip &&
		displayPattern && _me->y() > height() - s_stepBtnOff->height() )

	// when mouse button is pressed in pattern mode

	{
//	get the step number that was clicked on and
//	do calculations in floats to prevent rounding errors...
		float tmp = ( ( float(_me->x()) - BORDER_WIDTH ) *
				float( m_clip -> m_steps ) ) / float(width() - BORDER_WIDTH*2);

		int step = int( tmp );

//	debugging to ensure we get the correct step...
//		qDebug( "Step (%f) %d", tmp, step );

		if( step >= m_clip->m_steps )
		{
			qDebug( "Something went wrong in clip.cpp: step %d doesn't exist in clip!", step );
			return;
		}

		Note * n = m_clip->noteAtStep( step );

		if( n == nullptr )
		{
			m_clip->addStepNote( step );
		}
		else // note at step found
		{
			m_clip->addJournalCheckPoint();
			m_clip->setStep( step, false );
		}

		Engine::getSong()->setModified();
		update();

		if( getGUI()->pianoRoll()->currentMidiClip() == m_clip )
		{
			getGUI()->pianoRoll()->update();
		}
	}
	else

	// if not in pattern mode, let parent class handle the event

	{
		ClipView::mousePressEvent( _me );
	}
}

void MidiClipView::mouseDoubleClickEvent(QMouseEvent *_me)
{
	if( _me->button() != Qt::LeftButton )
	{
		_me->ignore();
		return;
	}
	if( m_clip->m_clipType == MidiClip::MelodyClip || !fixedClips() )
	{
		openInPianoRoll();
	}
}




void MidiClipView::wheelEvent(QWheelEvent * we)
{
	if(m_clip->m_clipType == MidiClip::BeatClip &&
				(fixedClips() || pixelsPerBar() >= 96) &&
				position(we).y() > height() - s_stepBtnOff->height())
	{
//	get the step number that was wheeled on and
//	do calculations in floats to prevent rounding errors...
		float tmp = ((float(position(we).x()) - BORDER_WIDTH) *
				float(m_clip -> m_steps)) / float(width() - BORDER_WIDTH*2);

		int step = int( tmp );

		if( step >= m_clip->m_steps )
		{
			return;
		}

		Note * n = m_clip->noteAtStep( step );
		if(!n && we->angleDelta().y() > 0)
		{
			n = m_clip->addStepNote( step );
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
			if( getGUI()->pianoRoll()->currentMidiClip() == m_clip )
			{
				getGUI()->pianoRoll()->update();
			}
		}
		we->accept();
	}
	else
	{
		ClipView::wheelEvent(we);
	}
}


static int computeNoteRange(int minKey, int maxKey)
{
	return (maxKey - minKey) + 1;
}

void MidiClipView::paintEvent( QPaintEvent * )
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
	bool const muted = m_clip->getTrack()->isMuted() || m_clip->isMuted();
	bool current = getGUI()->pianoRoll()->currentMidiClip() == m_clip;
	bool beatClip = m_clip->m_clipType == MidiClip::BeatClip;

	if( beatClip )
	{
		// Do not paint PatternClips how we paint MidiClips
		c = patternClipBackground();
	}
	else
	{
		c = getColorForDisplay( painter.background().color() );
	}

	// invert the gradient for the background in the B&B editor
	QLinearGradient lingrad( 0, 0, 0, height() );
	lingrad.setColorAt( beatClip ? 0 : 1, c.darker( 300 ) );
	lingrad.setColorAt( beatClip ? 1 : 0, c );

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

	// Check whether we will paint a text box and compute its potential height
	// This is needed so we can paint the notes underneath it.
	bool const drawName = !m_clip->name().isEmpty();
	bool const drawTextBox = !beatClip && drawName;

	// TODO Warning! This might cause problems if ClipView::paintTextLabel changes
	int textBoxHeight = 0;
	const int textTop = BORDER_WIDTH + 1;
	if (drawTextBox)
	{
		QFont labelFont = this->font();
		labelFont.setHintingPreference( QFont::PreferFullHinting );

		QFontMetrics fontMetrics(labelFont);
		textBoxHeight = fontMetrics.height() + 2 * textTop;
	}

	// Compute pixels per bar
	const int baseWidth = fixedClips() ? parentWidget()->width() - 2 * BORDER_WIDTH
						: width() - BORDER_WIDTH;
	const float pixelsPerBar = baseWidth / (float) m_clip->length().getBar();

	// Length of one bar/beat in the [0,1] x [0,1] coordinate system
	const float barLength = 1. / m_clip->length().getBar();
	const float tickLength = barLength / TimePos::ticksPerBar();

	const int x_base = BORDER_WIDTH;

	bool displayPattern = fixedClips() || (pixelsPerBar >= 96 && m_legacySEPattern);
	// melody clip paint event
	NoteVector const & noteCollection = m_clip->m_notes;
	if( m_clip->m_clipType == MidiClip::MelodyClip && !noteCollection.empty() )
	{
		// Compute the minimum and maximum key in the clip
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
									   : ( m_clip->hasColor() ? c.lighter( 200 ) : getNoteBorderColor() );

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

		// scan through all the notes and draw them on the clip
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
	// beat clip paint event
	else if (beatClip && displayPattern)
	{
		QPixmap stepon0;
		QPixmap stepon200;
		QPixmap stepoff;
		QPixmap stepoffl;
		const int steps = qMax( 1,
					m_clip->m_steps );
		const int w = width() - 2 * BORDER_WIDTH;

		// scale step graphics to fit the beat clip length
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

		for( int it = 0; it < steps; it++ )	// go through all the steps in the beat clip
		{
			Note * n = m_clip->noteAtStep( it );

			// figure out x and y coordinates for step graphic
			const int x = BORDER_WIDTH + static_cast<int>( it * w / steps );
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

		// draw a transparent rectangle over muted clips
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

	for( bar_t t = 1; t < m_clip->length().getBar(); ++t )
	{
		p.drawLine( x_base + static_cast<int>( pixelsPerBar * t ) - 1,
				BORDER_WIDTH, x_base + static_cast<int>(
						pixelsPerBar * t ) - 1, BORDER_WIDTH + lineSize );
		p.drawLine( x_base + static_cast<int>( pixelsPerBar * t ) - 1,
				rect().bottom() - ( lineSize + BORDER_WIDTH ),
				x_base + static_cast<int>( pixelsPerBar * t ) - 1,
				rect().bottom() - BORDER_WIDTH );
	}

	// clip name
	if (drawTextBox)
	{
		paintTextLabel(m_clip->name(), p);
	}

	if( !( fixedClips() && beatClip ) )
	{
		// inner border
		p.setPen( c.lighter( current ? 160 : 130 ) );
		p.drawRect( 1, 1, rect().right() - BORDER_WIDTH,
			rect().bottom() - BORDER_WIDTH );

		// outer border
		p.setPen( current ? c.lighter( 130 ) : c.darker( 300 ) );
		p.drawRect( 0, 0, rect().right(), rect().bottom() );
	}

	// draw the 'muted' pixmap only if the clip was manually muted
	if( m_clip->isMuted() )
	{
		const int spacing = BORDER_WIDTH;
		const int size = 14;
		p.drawPixmap( spacing, height() - ( size + spacing ),
			embed::getIconPixmap( "muted", size, size ) );
	}

	painter.drawPixmap( 0, 0, m_paintPixmap );
}


} // namespace lmms::gui
