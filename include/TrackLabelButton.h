/*
 * TrackLabelButton.h - class trackLabelButton
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef TRACK_LABEL_BUTTON_H
#define TRACK_LABEL_BUTTON_H

#include <QToolButton>
#include <QLineEdit>

class TrackView;

class TrackRenameLineEdit;


class TrackLabelButton : public QToolButton
{
	Q_OBJECT
public:
	TrackLabelButton( TrackView * _tv, QWidget * _parent );
	virtual ~TrackLabelButton();


public slots:
	void rename();
	void renameFinished();
	void nameChanged();


protected:
	void dragEnterEvent( QDragEnterEvent * _dee ) override;
	void dropEvent( QDropEvent * _de ) override;
	void mousePressEvent( QMouseEvent * _me ) override;
	void mouseDoubleClickEvent( QMouseEvent * _me ) override;
	void mouseReleaseEvent( QMouseEvent * _me ) override;
	void paintEvent( QPaintEvent * _pe ) override;
	void resizeEvent( QResizeEvent * _re ) override;


private:
	TrackView * m_trackView;
	QString m_iconName;
	TrackRenameLineEdit * m_renameLineEdit;
	QRect m_buttonRect;
	QString elideName( const QString &name );

} ;

#endif
