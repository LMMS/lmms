/*
 * PianoView.h - declaration of PianoView, an interactive piano/keyboard-widget
 *
 * Copyright (c) 2004-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_GUI_PIANO_VIEW_H
#define LMMS_GUI_PIANO_VIEW_H

#include <QPixmap>
#include <QScrollBar>

#include "AutomatableModel.h"
#include "ModelView.h"
#include "embed.h"

namespace lmms
{

class Piano;

namespace gui
{

class PianoView : public QWidget, public ModelView
{
	Q_OBJECT
public:
	//! @brief Create a new keyboard display view
	//! @param parent the parent instrument plugin window
	//! @todo Are the descriptions of the m_startkey and m_lastkey properties correct?
	PianoView(QWidget* parent);
	~PianoView() override = default;

	//! @brief Map a keyboard key being pressed to a note in our keyboard view
	static int getKeyFromKeyEvent( QKeyEvent * _ke );


public:
	//! @brief Handle a key press event on the piano display view
	//!
	//! We determine our key number from the getKeyFromKeyEvent() method, and pass the event on to the piano's
	//! handleKeyPress() method if auto-repeat is off.
	//!
	//! @param ke the KeyEvent to handle.
	void keyPressEvent(QKeyEvent* ke) override;

	//! @brief Handle a key release event on the piano display view
	//!
	//! The same logic as the keyPressEvent() method.
	//!
	//! @param ke the KeyEvent to handle.
	void keyReleaseEvent(QKeyEvent* ke) override;


protected:

	//! @brief Register a change to this piano display view
	void modelChanged() override;

	//! @brief Handle a context menu selection on the piano display view
	//! @param me The ContextMenuEvent to handle.
	//! @todo Is this right, or does this create the context menu?
	void contextMenuEvent(QContextMenuEvent* me) override;

	//! @brief Paint the piano display view in response to an event
	//!
	//! This method draws the piano and the 'root note' base. It draws the base first, then all the white keys, then all
	//! the black keys.
	void paintEvent(QPaintEvent*) override;

	//! @brief Handle a mouse click on this piano display view
	//!
	//! We first determine the key number using the getKeyFromMouse() method.
	//!
	//! If we're below the 'root key selection' area, we set the volume of the note to be proportional to the vertical
	//! position on the keyboard - lower down the key is louder, within the boundaries of the (white or black) key
	//! pressed. We then tell the instrument to play that note, scaling for MIDI max loudness = 127.
	//!
	//! If we're in the 'root key selection' area, of course, we set the
	//! root key to be that key.
	//!
	//! We finally update ourselves to show the key press
	//!
	//! @param me the mouse click to handle.
	void mousePressEvent(QMouseEvent* me) override;

	//! @brief Handle a mouse release event on the piano display view
	//!
	//! If a key was pressed by the in the mousePressEvent() function, we turn the note off.
	//!
	//! @param me the mousePressEvent to handle.
	void mouseReleaseEvent(QMouseEvent* me) override;

	//! @brief Handle a mouse move event on the piano display view
	//!
	//! This handles the user dragging the mouse across the keys. It uses code from mousePressEvent() and
	//! mouseReleaseEvent(), also correcting for if the mouse movement has stayed within one key and if the mouse has
	//! moved outside the vertical area of the keyboard (which is still allowed but won't make the volume go up to 11).
	//!
	//! @param me the ContextMenuEvent to handle.
	//! @todo Paul Wayper thinks that this code should be refactored to reduce or remove the duplication between this,
	//! the mousePressEvent() and mouseReleaseEvent() methods.
	void mouseMoveEvent(QMouseEvent* me) override;

	//! @brief Handle the focus leaving the piano display view
	//!
	//! Turn off all notes if we lose focus.
	//!
	//! @todo Is there supposed to be a parameter given here?
	void focusOutEvent(QFocusEvent*) override;

	void focusInEvent( QFocusEvent * fe ) override;

	//! @brief update scrollbar range after resize
	//!
	//! After resizing we need to adjust range of scrollbar for not allowing to scroll too far to the right.
	void resizeEvent(QResizeEvent*) override;


private:
	//! @brief Get the key from the mouse position in the piano display
	//! @param p The point that the mouse was pressed.
	int getKeyFromMouse(const QPoint& p) const;

	//! @brief Convert a key number to an X coordinate in the piano display view
	//!
	//! We can immediately discard the trivial case of when the key number is less than our starting key. We then iterate
	//! through the keys from the start key to this key, adding the width of each key as we go. For black keys, and the
	//! first white key if there is no black key between two white keys, we add half a white key width; for that second
	//! white key, we add a whole width. That takes us to the boundary of a white key - subtract half a width to get to
	//! the middle.
	//!
	//! @param key_num the keyboard key to translate
	//! @todo Is this description of what the method does correct?
	int getKeyX(int key_num) const;

	//! @brief Return the width of a given key
	int getKeyWidth(int key_num) const;

	//! @brief Return the height of a given key
	int getKeyHeight(int key_num) const;

	//! @brief Return model and title of the marker closest to the given key
	IntModel *getNearestMarker(int key, QString* title = nullptr);

	QPixmap m_whiteKeyPm = embed::getIconPixmap("white_key");
	QPixmap m_blackKeyPm = embed::getIconPixmap("black_key");
	QPixmap m_whiteKeyPressedPm = embed::getIconPixmap("white_key_pressed");
	QPixmap m_blackKeyPressedPm = embed::getIconPixmap("black_key_pressed");
	QPixmap m_whiteKeyDisabledPm = embed::getIconPixmap("white_key_disabled");
	QPixmap m_blackKeyDisabledPm = embed::getIconPixmap("black_key_disabled");

	Piano* m_piano = nullptr;

	QScrollBar * m_pianoScroll;
	int m_startKey; //!< first key when drawing
	int m_lastKey = -1; //!< previously pressed key
	IntModel *m_movedNoteModel = nullptr; //!< note marker which is being moved

private slots:
	//! @brief Handle the scrolling on the piano display view
	//!
	//! We need to update our start key position based on the new position.
	//!
	//! @param newPos the new key position, counting only white keys.
	void pianoScrolled(int newPos);

signals:
	void keyPressed( int );
	void baseNoteChanged();
};


} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_PIANO_VIEW_H
