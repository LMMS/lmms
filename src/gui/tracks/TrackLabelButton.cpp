/*
 * TrackLabelButton.cpp - implementation of class trackLabelButton, a label
 *                          which is renamable by double-clicking it
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


#include "TrackLabelButton.h"

#include <QMouseEvent>

#include "ConfigManager.h"
#include "embed.h"
#include "Instrument.h"
#include "InstrumentTrack.h"
#include "RenameDialog.h"
#include "TrackRenameLineEdit.h"
#include "TrackView.h"
#include "Track.h"

namespace lmms::gui
{

TrackLabelButton::TrackLabelButton( TrackView * _tv, QWidget * _parent ) :
	QToolButton( _parent ),
	m_trackView( _tv ),
	m_iconName()
{
	setAcceptDrops( true );
	setFocusPolicy(Qt::NoFocus);
	setCursor( QCursor( embed::getIconPixmap( "hand" ), 3, 3 ) );
	setToolButtonStyle( Qt::ToolButtonTextBesideIcon );

	m_renameLineEdit = new TrackRenameLineEdit( this );
	m_renameLineEdit->hide();
	
	if (isInCompactMode())
	{
		setFixedSize( 32, 29 );
	}
	else
	{
		setFixedSize( 160, 29 );
		connect( m_renameLineEdit, SIGNAL(editingFinished()), this, SLOT(renameFinished()));
	}
	
	setIconSize( QSize( 24, 24 ) );
	connect( m_trackView->getTrack(), SIGNAL(dataChanged()), this, SLOT(update()));
	connect( m_trackView->getTrack(), SIGNAL(nameChanged()), this, SLOT(nameChanged()));
}






void TrackLabelButton::rename()
{
	if (isInCompactMode())
	{
		QString txt = m_trackView->getTrack()->name();
		RenameDialog renameDlg( txt );
		renameDlg.exec();
		if( txt != text() )
		{
			m_trackView->getTrack()->setName( txt );
		}
	}
	else
	{
		const auto & trackName = m_trackView->getTrack()->name();
		m_renameLineEdit->setText(trackName);
		m_renameLineEdit->selectAll();
		m_renameLineEdit->setFocus();

		// Make sure that the rename line edit uses the same font as the widget
		// which is set via style sheets
		m_renameLineEdit->setFont(font());

		// Move the line edit to the correct position by taking the size of the
		// icon into account.
		const auto iconWidth = iconSize().width();
		m_renameLineEdit->move(iconWidth + 1, (height() / 2  - m_renameLineEdit->sizeHint().height() / 2) + 1);
		m_renameLineEdit->setFixedWidth(width() - (iconWidth + 6));

		m_renameLineEdit->show();
	}
}




void TrackLabelButton::renameFinished()
{
	if (!isInCompactMode())
	{
		m_renameLineEdit->clearFocus();
		m_renameLineEdit->hide();
		if( m_renameLineEdit->text() != "" )
		{
			if( m_renameLineEdit->text() != m_trackView->getTrack()->name() )
			{
				setText( elideName( m_renameLineEdit->text() ) );
				m_trackView->getTrack()->setName( m_renameLineEdit->text() );
			}
		}
	}
}




void TrackLabelButton::nameChanged()
{
	setText( elideName( m_trackView->getTrack()->name() ) );
}




void TrackLabelButton::dragEnterEvent( QDragEnterEvent * _dee )
{
	m_trackView->dragEnterEvent( _dee );
}




void TrackLabelButton::dropEvent( QDropEvent * _de )
{
	m_trackView->dropEvent( _de );
	setChecked( true );
}





void TrackLabelButton::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::RightButton )
	{
		rename();
	}
	else
	{
		m_buttonRect = QRect( this->mapToGlobal( pos() ), size() );
		_me->ignore();
	}
}




void TrackLabelButton::mouseDoubleClickEvent( QMouseEvent * _me )
{
	rename();
}




void TrackLabelButton::mouseReleaseEvent( QMouseEvent *_me )
{
	if( m_buttonRect.contains( _me->globalPos(), true ) && m_renameLineEdit->isHidden() )
	{
		QToolButton::mousePressEvent( _me );
	}
	QToolButton::mouseReleaseEvent( _me );
	_me->ignore();
}


void TrackLabelButton::paintEvent(QPaintEvent* pe)
{
	if (m_trackView->getTrack()->type() == Track::Type::Instrument)
	{
		auto it = dynamic_cast<InstrumentTrack*>(m_trackView->getTrack());
		const PixmapLoader* pl;
		auto get_logo = [](InstrumentTrack* it) -> const PixmapLoader*
		{
			return it->instrument()->key().isValid()
				? it->instrument()->key().logo()
				: it->instrument()->descriptor()->logo;
		};
		if (it && it->instrument() &&
			it->instrument()->descriptor() &&
			(pl = get_logo(it)))
		{
			if (pl->pixmapName() != m_iconName)
			{
				m_iconName = pl->pixmapName();
				setIcon(pl->pixmap());
			}
		}
	}
	QToolButton::paintEvent(pe);
}


void TrackLabelButton::resizeEvent(QResizeEvent *_re)
{
	setText( elideName( m_trackView->getTrack()->displayName() ) );
}




QString TrackLabelButton::elideName( const QString &name )
{
	const int spacing = 16;
	const int maxTextWidth = width() - spacing - iconSize().width();
	if( maxTextWidth < 1 )
	{
		setToolTip( m_trackView->getTrack()->displayName() );
		return QString( " " );
	}
	setToolTip( "" );
	QFontMetrics metrics( font() );
	QString elidedName = metrics.elidedText( name, Qt::ElideRight, maxTextWidth );
	return elidedName;
}

bool TrackLabelButton::isInCompactMode() const
{
	return ConfigManager::inst()->value("ui", "compacttrackbuttons").toInt();
}

} // namespace lmms::gui
