/*
 * combobox.h - class comboBox, a very cool combo-box
 *
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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


#ifndef _COMBOBOX_H
#define _COMBOBOX_H

#include <QtGui/QWidget>
#include <QtCore/QVector>
#include <QtGui/QMenu>
#include <QtCore/QPair>

#include "automatable_model.h"
#include "templates.h"


class comboBoxModel : public intModel
{
	Q_OBJECT
public:
	comboBoxModel( ::model * _parent = NULL ) :
		automatableModel<int>( 0, 0, 0, defaultRelStep(), _parent )
	{
	}

	void addItem( const QString & _item, QPixmap * _data = NULL );

	void clear( void );

	int findText( const QString & _txt ) const;

	inline const QString & currentText( void ) const
	{
		return( m_items[value()].first );
	}

	inline const QPixmap * currentData( void ) const
	{
		return( m_items[value()].second );
	}

	inline const QString & itemText( int _i ) const
	{
		return( m_items[tLimit<int>( _i, minValue(), maxValue() )].
									first );
	}

	inline const QPixmap * itemPixmap( int _i ) const
	{
		return( m_items[tLimit<int>( _i, minValue(), maxValue() )].
									second );
	}

	inline int size( void ) const
	{
		return( m_items.size() );
	}

private:
	typedef QPair<QString, QPixmap *> item;

	QVector<item> m_items;


signals:
	void itemPixmapRemoved( QPixmap * _item );
	
} ;



class comboBox : public QWidget, public intModelView
{
	Q_OBJECT
public:
	comboBox( QWidget * _parent, const QString & _name = QString::null );
	virtual ~comboBox();

        comboBoxModel * model( void )
        {
                return( castModel<comboBoxModel>() );
        }

        const comboBoxModel * model( void ) const
        {
                return( castModel<comboBoxModel>() );
        }


        virtual void modelChanged( void )
        {
                if( model() != NULL )
                {
                        connect( model(), SIGNAL( itemPixmapRemoved( QPixmap * ) ),
                                        this, SLOT( deletePixmap( QPixmap * ) ) );
                }                       
        }


protected:
	virtual void contextMenuEvent( QContextMenuEvent * _me );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void wheelEvent( QWheelEvent * _we );


private:
	static QPixmap * s_background;
	static QPixmap * s_arrow;

	QMenu m_menu;

	bool m_pressed;


private slots:
	void deletePixmap( QPixmap * _pixmap );
	void setItem( QAction * _item );

} ;

#endif
