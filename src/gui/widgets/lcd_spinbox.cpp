/*
 * lcd_spinbox.cpp - class lcdSpinBox, an improved QLCDNumber
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2008-2009 Paul Giblock <pgib/at/users.sourceforge.net>
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



#include <QtGui/QApplication>
#include <QtGui/QLabel>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QFontMetrics>
#include <QtGui/QStyleOptionFrameV2>

#include "lcd_spinbox.h"
#include "caption_menu.h"
#include "engine.h"
#include "embed.h"
#include "gui_templates.h"
#include "templates.h"
#include "MainWindow.h"



lcdSpinBox::lcdSpinBox( int _num_digits, QWidget * _parent,
			const QString & _name ) :
	QWidget( _parent ),
	intModelView( new intModel( 0, 0, 0, NULL, _name, true ), this ),
	m_label(),
	m_numDigits( _num_digits ),
	m_origMousePos()
{
	setEnabled( true );

	setAccessibleName( _name );

	m_lcdPixmap = new QPixmap( embed::getIconPixmap( "lcd_19green" ) );

	m_cellWidth = m_lcdPixmap->size().width() / lcdSpinBox::charsPerPixmap;
	m_cellHeight = m_lcdPixmap->size().height() / 2;

	m_marginWidth =  m_cellWidth / 2;

	updateSize();
}




lcdSpinBox::lcdSpinBox( int _num_digits, const QString & _lcd_style,
			QWidget * _parent, const QString & _name ) :
	QWidget( _parent ),
	intModelView( new intModel( 0, 0, 0, NULL, _name, true ), this ),
	m_label(),
	m_numDigits( _num_digits ),
	m_origMousePos()
{
	setEnabled( true );

	setAccessibleName( _name );

	// We should make a factory for these or something.
	m_lcdPixmap = new QPixmap( embed::getIconPixmap( QString( "lcd_" +
			_lcd_style ).toUtf8().constData() ) );

	m_cellWidth = m_lcdPixmap->size().width() / lcdSpinBox::charsPerPixmap;
	m_cellHeight = m_lcdPixmap->size().height() / 2;

	m_marginWidth =  m_cellWidth / 2;

	updateSize();
}




lcdSpinBox::~lcdSpinBox()
{
	delete m_lcdPixmap;
}




void lcdSpinBox::paintEvent( QPaintEvent * _me )
{
	QRect ur = _me->rect();

	QPainter p( this );

	QSize cellSize( m_cellWidth, m_cellHeight );

	QRect cellRect( 0, 0, m_cellWidth, m_cellHeight );

	int margin = 1;  // QStyle::PM_DefaultFrameWidth;
	//int lcdWidth = m_cellWidth * m_numDigits + (margin*m_marginWidth)*2;

//	p.translate( width() / 2 - lcdWidth / 2, 0 );
	p.save();

	p.translate( margin, margin );

	// Left Margin
	p.drawPixmap( cellRect, *m_lcdPixmap,
			QRect( QPoint( charsPerPixmap*m_cellWidth,
				isEnabled()?0:m_cellHeight ),
			cellSize ) );

	p.translate( m_marginWidth, 0 );

	// Padding
	for( int i=0; i < m_numDigits - m_display.length(); i++ )
	{
		p.drawPixmap( cellRect, *m_lcdPixmap,
			QRect( QPoint( 10 * m_cellWidth, isEnabled()?0:m_cellHeight) , cellSize ) );
		p.translate( m_cellWidth, 0 );
	}

	// Digits
	for( int i=0; i < m_display.length(); i++ )
	{
		int val = m_display[i].digitValue();
		if( val < 0 )
		{
			if( m_display[i] == '-' )
				val = 11;
			else
				val = 10;
		}
		p.drawPixmap( cellRect, *m_lcdPixmap,
				QRect( QPoint( val*m_cellWidth,
					isEnabled()?0:m_cellHeight ),
				cellSize ) );
		p.translate( m_cellWidth, 0 );
	}

	// Right Margin
	p.drawPixmap( QRect( 0, 0, m_marginWidth-1, m_cellHeight ), *m_lcdPixmap,
			QRect( charsPerPixmap*m_cellWidth, isEnabled()?0:m_cellHeight,
				m_cellWidth / 2, m_cellHeight ) );


	p.restore();

	// Border
	QStyleOptionFrame opt;
	opt.initFrom( this );
	opt.state = QStyle::State_Sunken;
	opt.rect = QRect( 0, 0, m_cellWidth * m_numDigits + (margin+m_marginWidth)*2 - 1,
			m_cellHeight + (margin*2) );

	style()->drawPrimitive( QStyle::PE_Frame, &opt, &p, this );

	p.resetTransform();

	// Label
	if( !m_label.isEmpty() )
	{
		p.setFont( pointSize<6>( p.font() ) );
		p.setPen( QColor( 64, 64, 64 ) );
		p.drawText( width() / 2 -
				p.fontMetrics().width( m_label ) / 2 + 1,
						height(), m_label );
		p.setPen( palette().text().color() );
		p.drawText( width() / 2 -
				p.fontMetrics().width( m_label ) / 2,
						height() - 1, m_label );
	}

}




void lcdSpinBox::update( void )
{
	QString s = m_textForValue[model()->value()];
	if( s == "" )
	{
		s = QString::number( model()->value() );
		// TODO: if pad == true
		/*
		while( (int) s.length() < m_numDigits )
		{
			s = "0" + s;
		}
		*/
	}
	m_display = s;

	QWidget::update();
}




void lcdSpinBox::setLabel( const QString & _txt )
{
	m_label = _txt;
	updateSize();
}




void lcdSpinBox::setEnabled( bool _on )
{
	QWidget::setEnabled( _on );
}




void lcdSpinBox::setMarginWidth( int _width )
{
	m_marginWidth = _width;

	updateSize();
}




void lcdSpinBox::updateSize()
{
	int margin = 1;
	if (m_label.isEmpty()) {
		setFixedSize( m_cellWidth * m_numDigits + 2*(margin+m_marginWidth),
				m_cellHeight + (2*margin) );
	}
	else {
		setFixedSize( qMax<int>(
				m_cellWidth * m_numDigits + 2*(margin+m_marginWidth),
				QFontMetrics( pointSize<6>( font() ) ).width( m_label ) ),
				m_cellHeight + (2*margin) + 10 );
	}

	update();
}




void lcdSpinBox::contextMenuEvent( QContextMenuEvent * _me )
{
	m_origMousePos = _me->globalPos();

	// for the case, the user clicked right while pressing left mouse-
	// button, the context-menu appears while mouse-cursor is still hidden
	// and it isn't shown again until user does something which causes
	// an QApplication::restoreOverrideCursor()-call...
	mouseReleaseEvent( NULL );

	captionMenu contextMenu( model()->displayName() );
	addDefaultActions( &contextMenu );
	contextMenu.exec( QCursor::pos() );
}




void lcdSpinBox::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton &&
		! ( _me->modifiers() & Qt::ControlModifier ) &&
						_me->y() < m_cellHeight + 2  )
	{
		m_origMousePos = _me->globalPos();
		QApplication::setOverrideCursor( Qt::BlankCursor );
		model()->prepareJournalEntryFromOldVal();
	}
	else
	{
		automatableModelView::mousePressEvent( _me );
	}
}




void lcdSpinBox::mouseMoveEvent( QMouseEvent * _me )
{
	if( _me->buttons() & Qt::LeftButton )
	{
		int dy = _me->globalY() - m_origMousePos.y();
		if( dy > 1 || dy < -1 )
		{
			model()->setInitValue( model()->value() -
						dy / 2 * model()->step<int>() );
			emit manualChange();
			QCursor::setPos( m_origMousePos );
		}
	}
}




void lcdSpinBox::mouseReleaseEvent( QMouseEvent * _me )
{
	model()->addJournalEntryFromOldToCurVal();

	QCursor::setPos( m_origMousePos );
	QApplication::restoreOverrideCursor();
}




void lcdSpinBox::wheelEvent( QWheelEvent * _we )
{
	_we->accept();
	model()->setInitValue( model()->value() +
			( ( _we->delta() > 0 ) ? 1 : -1 ) * model()->step<int>() );
	emit manualChange();
}



#include "moc_lcd_spinbox.cxx"

