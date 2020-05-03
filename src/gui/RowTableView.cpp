/*
 * RowTableView.cpp - table with rows that act like single cells
 *
 * Copyright (c) 2016 Javier Serrano Polo <javier@jasp.net>
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

#include "RowTableView.h"

#include <QKeyEvent>
#include <QPainter>
#include <QStyledItemDelegate>


class RowDelegate : public QStyledItemDelegate
{
public:
	RowDelegate( QAbstractItemView * table, QObject * parent = 0 ) :
		QStyledItemDelegate( parent ),
		m_table( table )
		{
		}
	virtual void paint( QPainter * painter,
					const QStyleOptionViewItem & option,
					const QModelIndex & index ) const override;


protected:
	virtual void initStyleOption( QStyleOptionViewItem * option,
					const QModelIndex & index ) const override;


private:
	QAbstractItemView * m_table;

} ;




void RowDelegate::initStyleOption( QStyleOptionViewItem * option,
					const QModelIndex & index ) const
{
	QStyledItemDelegate::initStyleOption( option, index );
	option->state &= ~QStyle::State_HasFocus;
}




void RowDelegate::paint( QPainter * painter,
	const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
	QStyledItemDelegate::paint( painter, option, index );
	if ( index.row() == m_table->currentIndex().row() )
	{
		const QRect rect( option.rect );
		painter->drawLine( rect.topLeft(), rect.topRight() );
		painter->drawLine( rect.bottomLeft(), rect.bottomRight() );
		if ( index.column() == 0 )
		{
			painter->drawLine( rect.topLeft(), rect.bottomLeft() );
		}
		if ( index.column() == index.model()->columnCount() - 1 )
		{
			painter->drawLine( rect.topRight(),
							rect.bottomRight() );
		}
	}
}




RowTableView::RowTableView( QWidget * parent ) :
	QTableView( parent )
{
	m_rowDelegate = new RowDelegate( this, this );
}




RowTableView::~RowTableView()
{
	delete m_rowDelegate;
}




void RowTableView::setModel( QAbstractItemModel * model )
{
	QTableView::setModel( model );
	for ( int i = 0; i < model->rowCount(); i++ )
	{
		setItemDelegateForRow( i, m_rowDelegate );
	}

}




void RowTableView::keyPressEvent( QKeyEvent * event )
{
	switch( event->key() )
	{
		case Qt::Key_Tab:
		case Qt::Key_Backtab:
			for( int i = 0; i < model()->columnCount() - 1; i++ )
			{
				QTableView::keyPressEvent( event );
			}
		default:
			QTableView::keyPressEvent( event );
	}
}




