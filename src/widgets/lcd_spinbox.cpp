#ifndef SINGLE_SOURCE_COMPILE

/*
 * lcd_spinbox.cpp - class lcdSpinBox, an improved QLCDNumber
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "lcd_spinbox.h"

#include <QtGui/QApplication>
#include <QtGui/QLabel>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QFontMetrics>
#include <QtGui/QStyleOptionFrameV2>


#include "automatable_model_templates.h"
#include "caption_menu.h"
#include "embed.h"
#include "gui_templates.h"
#include "templates.h"


lcdSpinBox::lcdSpinBox( int _num_digits, QWidget * _parent,
						const QString & _name ) :
	QWidget( _parent ),
	autoModelView( new autoModel( 0, 0, 0, 1, NULL, TRUE ) ),
	m_label(),
	m_numDigits( _num_digits ),
	m_origMousePos()
{	
	setEnabled( TRUE );

	setAccessibleName( _name );

	m_lcdPixmap = new QPixmap( embed::getIconPixmap( "lcd_19red" ) );

	int margin = 1; //QStyle::PM_DefaultFrameWidth;

	m_cellWidth = m_lcdPixmap->size().width() / lcdSpinBox::charsPerPixmap;
	m_cellHeight = m_lcdPixmap->size().height() / 2;

	setFixedSize( m_cellWidth * (_num_digits+1)  + (2*margin),
			m_cellHeight + (2*margin) );
}

lcdSpinBox::lcdSpinBox( int _num_digits, const QString & _lcd_style, 
			QWidget * _parent, const QString & _name ) :
	QWidget( _parent ),
	autoModelView( new autoModel( 0, 0, 0, 1, NULL, TRUE ) ),
	m_label(),
	m_numDigits( _num_digits ),
	m_origMousePos()
{
	setEnabled( TRUE );

	setAccessibleName( _name );

	m_lcdPixmap = new QPixmap( embed::getIconPixmap( QString( "lcd_" +
			_lcd_style ).toAscii().constData() ) );

	int margin = 1; //QStyle::PM_DefaultFrameWidth;

	m_cellWidth = m_lcdPixmap->size().width() / lcdSpinBox::charsPerPixmap;
	m_cellHeight = m_lcdPixmap->size().height() / 2;

	setFixedSize( m_cellWidth * (_num_digits+1)  + (2*margin),
			m_cellHeight + (2*margin) );
}




lcdSpinBox::~lcdSpinBox()
{
}


void lcdSpinBox::paintEvent( QPaintEvent * _me )
{
	QRect ur = _me->rect();

	QPainter p( this );
	
	QSize cellSize( m_cellWidth, m_cellHeight );

	QRect cellRect( 0, 0, m_cellWidth, m_cellHeight );
	
	int i;

	int margin = 1;// QStyle::PM_DefaultFrameWidth;
	int lcdWidth = m_cellWidth * (m_numDigits+1) + (margin*2);

	p.translate( width() / 2 - lcdWidth / 2, 0 ); 
	p.save();
	
	p.translate( margin, margin );

	// Left Margin
	p.drawPixmap( cellRect, *m_lcdPixmap, 
			QRect( QPoint( charsPerPixmap*m_cellWidth, 
				isEnabled()?0:m_cellHeight ), 
			cellSize ) );
	
	p.translate( (m_cellWidth+1) / 2, 0 );

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
	p.drawPixmap( QRect( 0, 0, m_cellWidth / 2, m_cellHeight ), *m_lcdPixmap, 
			QRect( charsPerPixmap*m_cellWidth, isEnabled()?0:m_cellHeight,
				m_cellWidth / 2, m_cellHeight ) );


	p.restore();

	// Border
	QStyleOptionFrame opt;
	opt.initFrom( this );
	opt.state = QStyle::State_Sunken;
	opt.rect = QRect( 0, 0, m_cellWidth * (m_numDigits+1) + (margin*2), 
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
				height() - 1, m_label );
		p.setPen( QColor( 255, 255, 255 ) );
		p.drawText( width() / 2 -
				p.fontMetrics().width( m_label ) / 2,
				height() - 2, m_label );
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
	int margin = 1;
	m_label = _txt;
	
	setFixedSize( m_cellWidth * (m_numDigits+1)  + (2*margin),
			m_cellHeight + (2*margin) );

	setFixedSize( tMax<int>( m_cellWidth*(m_numDigits+1) + (2*margin),
				QFontMetrics( pointSize<6>( font() ) ).width( m_label ) ),
			m_cellHeight + (2*margin) + 10 );
	update();
}




void lcdSpinBox::setEnabled( bool _on )
{
	QWidget::setEnabled( _on );
}




void lcdSpinBox::contextMenuEvent( QContextMenuEvent * _me )
{
	m_origMousePos = _me->globalPos();

	if( model()->nullTrack() )
	{
		QWidget::contextMenuEvent( _me );
		return;
	}

	// for the case, the user clicked right while pressing left mouse-
	// button, the context-menu appears while mouse-cursor is still hidden
	// and it isn't shown again until user does something which causes
	// an QApplication::restoreOverrideCursor()-call...
	mouseReleaseEvent( NULL );

	captionMenu contextMenu( accessibleName() );
	contextMenu.addAction( embed::getIconPixmap( "automation" ),
					tr( "&Open in automation editor" ),
					model()->getAutomationPattern(),
					SLOT( openInAutomationEditor() ) );
	contextMenu.exec( QCursor::pos() );
}




void lcdSpinBox::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton && _me->y() < m_cellHeight + 2  )
	{
		m_origMousePos = _me->globalPos();
		QApplication::setOverrideCursor( Qt::BlankCursor );
		model()->prepareJournalEntryFromOldVal();
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
						dy / 2 * model()->step() );
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
			( ( _we->delta() > 0 ) ? 1 : -1 ) * model()->step() );
	emit manualChange();
}




#include "lcd_spinbox.moc"


#endif
