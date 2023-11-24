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
	PianoView( QWidget * _parent );
	~PianoView() override = default;

	static int getKeyFromKeyEvent( QKeyEvent * _ke );


public:
	void keyPressEvent( QKeyEvent * ke ) override;
	void keyReleaseEvent( QKeyEvent * ke ) override;


protected:
	void modelChanged() override;
	void contextMenuEvent( QContextMenuEvent * _me ) override;
	void paintEvent( QPaintEvent * ) override;
	void mousePressEvent( QMouseEvent * me ) override;
	void mouseReleaseEvent( QMouseEvent * me ) override;
	void mouseMoveEvent( QMouseEvent * me ) override;
	void focusOutEvent( QFocusEvent * _fe ) override;
	void focusInEvent( QFocusEvent * fe ) override;
	void resizeEvent( QResizeEvent * _event ) override;


private:
	int getKeyFromMouse( const QPoint & _p ) const;
	int getKeyX( int _key_num ) const;
	int getKeyWidth(int key_num) const;
	int getKeyHeight(int key_num) const;
	IntModel *getNearestMarker(int key, QString* title = nullptr);

	QPixmap m_whiteKeyPm = embed::getIconPixmap("white_key");
	QPixmap m_blackKeyPm = embed::getIconPixmap("black_key");
	QPixmap m_whiteKeyPressedPm = embed::getIconPixmap("white_key_pressed");
	QPixmap m_blackKeyPressedPm = embed::getIconPixmap("black_key_pressed");
	QPixmap m_whiteKeyDisabledPm = embed::getIconPixmap("white_key_disabled");
	QPixmap m_blackKeyDisabledPm = embed::getIconPixmap("black_key_disabled");

	Piano * m_piano;

	QScrollBar * m_pianoScroll;
	int m_startKey;					//!< first key when drawing
	int m_lastKey;					//!< previously pressed key
	IntModel *m_movedNoteModel;		//!< note marker which is being moved



private slots:
	void pianoScrolled( int _new_pos );

signals:
	void keyPressed( int );
	void baseNoteChanged();

} ;


} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_PIANO_VIEW_H
