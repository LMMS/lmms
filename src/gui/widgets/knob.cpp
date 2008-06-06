#ifndef SINGLE_SOURCE_COMPILE

/*
 * knob.cpp - powerful knob-widget
 *
 * This file is partly based on the knob-widget of the Qwt Widget Library by
 * Josef Wilgen.
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "knob.h"

#include <QtGui/QApplication>
#include <QtGui/QBitmap>
#include <QtGui/QFontMetrics>
#include <QtGui/QInputDialog>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QPalette>

#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif
#include <math.h>


#include "caption_menu.h"
#include "config_mgr.h"
#include "embed.h"
#include "engine.h"
#include "gui_templates.h"
#include "main_window.h"
#include "string_pair_drag.h"
#include "templates.h"
#include "text_float.h"
#include "song.h"


textFloat * knob::s_textFloat = NULL;



knob::knob( int _knob_num, QWidget * _parent, const QString & _name ) :
	QWidget( _parent ),
	floatModelView( new knobModel( 0, 0, 0, 1, NULL, _name, TRUE ) ),
	m_mouseOffset( 0.0f ),
	m_buttonPressed( FALSE ),
	m_knobPixmap( NULL ),
	m_outerColor( NULL ),
	m_knobNum( _knob_num ),
	m_label( "" )
{
	if( s_textFloat == NULL )
	{
		s_textFloat = new textFloat;
	}

	setAcceptDrops( TRUE );

	setAccessibleName( _name );
	
	if( m_knobNum != knobStyled ) {
		m_knobPixmap = new QPixmap( embed::getIconPixmap( QString( "knob0" +
			QString::number( m_knobNum + 1 ) ).toAscii().constData() ) );

		setFixedSize( m_knobPixmap->width(), m_knobPixmap->height() );
	}
	setTotalAngle( 270.0f );
	setInnerRadius( 1.0f );
	setOuterRadius( 10.0f );
	doConnections();
}




knob::~knob()
{
	if( m_knobPixmap )
	{
		delete m_knobPixmap;
	}
}




void knob::setLabel( const QString & _txt )
{
	m_label = _txt;
	if( m_knobPixmap )
	{
		setFixedSize( tMax<int>( m_knobPixmap->width(),
						QFontMetrics( pointSize<6>( font()
								) ).width( m_label ) ),
							m_knobPixmap->height() + 10 );
	}
	update();
}




void knob::setTotalAngle( float _angle )
{
	if( _angle < 10.0 )
	{
		m_totalAngle = 10.0;
	}
	else
	{
		m_totalAngle = _angle;
	}

	update();
}




float knob::innerRadius( void ) const
{
	return m_innerRadius;
}



void knob::setInnerRadius( float _r )
{
	m_innerRadius = _r;
}



float knob::outerRadius( void ) const
{
	return m_outerRadius;
}



void knob::setOuterRadius( float _r )
{
	m_outerRadius = _r;
}



QPointF knob::centerPoint( void ) const
{
	return m_centerPoint;
}



float knob::centerPointX( void ) const
{
	return m_centerPoint.x();
}



void knob::setCenterPointX( float _c )
{
	m_centerPoint.setX( _c );
}



float knob::centerPointY( void ) const
{
	return m_centerPoint.y();
}



void knob::setCenterPointY( float _c )
{
	m_centerPoint.setY( _c );
}



float knob::lineWidth( void ) const
{
	return m_lineWidth;
}



void knob::setLineWidth( float _w )
{
	m_lineWidth = _w;
}



QColor knob::outerColor( void ) const
{
	if( m_outerColor )
	{
		return *m_outerColor;
	}
	else
	{
		return QColor();
	}
}



void knob::setOuterColor( const QColor & _c )
{
	if( m_outerColor )
	{
		*m_outerColor = _c;
	}
	else
	{
		m_outerColor = new QColor( _c );
	}
}




QLineF knob::calculateLine( const QPointF & _mid, float _radius, float _innerRadius ) const
{
	float angle = 0.0f;
	if( model()->maxValue() != model()->minValue() )
	{
		angle = ( model()->value() - 0.5 * ( model()->minValue() +
						model()->maxValue() ) ) /
				( model()->maxValue() - model()->minValue() ) *
								m_totalAngle;
		angle = static_cast<int>( angle ) % 360;
	}

	const float rarc = angle * M_PI / 180.0;
	const float ca = cos( rarc );
	const float sa = -sin( rarc );

	return QLineF( _mid.x() - sa*_innerRadius, _mid.y() - ca*_innerRadius,
					_mid.x() - sa*_radius, _mid.y() - ca*_radius );
}



void knob::drawKnob( QPainter * _p )
{
	QPoint mid;

	if( m_knobNum == knobStyled )
	{
		_p->setRenderHint( QPainter::Antialiasing );

		// Perhaps this can move to setOuterRadius()
		if( m_outerColor )
		{
			QRadialGradient gradient( centerPoint(), outerRadius() );
			gradient.setColorAt(0.4, _p->pen().brush().color() );
			gradient.setColorAt(1, *m_outerColor );

			_p->setPen( QPen( gradient, lineWidth(),
						Qt::SolidLine, Qt::RoundCap ) );
		}
		else {
			QPen pen = _p->pen();
			pen.setWidth( (int) lineWidth() );
			pen.setCapStyle( Qt::RoundCap );
			
			_p->setPen( pen );
		}

		_p->drawLine( calculateLine( centerPoint(), outerRadius(),
							innerRadius() ) );
		return;
	}


	// Old-skool knobs
	const float radius = m_knobPixmap->width() / 2.0f - 1;
	mid = QPoint( width() / 2, m_knobPixmap->height() / 2 );

	_p->drawPixmap( static_cast<int>( 
				width() / 2 - m_knobPixmap->width() / 2 ), 0,
				*m_knobPixmap );

	_p->setPen( QPen( QColor( 200, 0, 0 ), 2 ) );
	_p->setRenderHint( QPainter::Antialiasing );

	switch( m_knobNum )
	{
		case knobSmall_17:
		{
			_p->drawLine( calculateLine( mid, radius ) );
			break;
		}
		case knobBright_26:
		{
			_p->drawLine( calculateLine( mid, radius-5 ) );
			break;
		}
		case knobDark_28:
		{
			const float rb = tMax<float>( ( radius - 10 ) / 3.0,
									0.0 );
			const float re = tMax<float>( ( radius - 4 ), 0.0 );
			QLineF ln = calculateLine( mid, re, rb );
			ln.translate( 1, 1 );
			_p->drawLine( ln );
			break;
		}
		case knobGreen_17:
		{
			_p->setPen( QPen( QColor( 0, 200, 0 ), 2 ) );
			_p->drawLine( calculateLine( mid, radius ) );
			break;
		}
	}
}




float knob::getValue( const QPoint & _p )
{
	if( configManager::inst()->value( "knobs", "classicalusability"
								).toInt() )
	{
		const float dx = float( ( rect().x() + rect().width() / 2 ) -
								_p.x() );
		const float dy = float( ( rect().y() + rect().height() / 2 ) -
								_p.y() );

		const float arc = atan2( -dx, dy ) * 180.0 / M_PI;

		float new_value = 0.5 * ( model()->minValue() +
							model()->maxValue() ) +
					arc * ( model()->maxValue() -
							model()->minValue() ) /
								m_totalAngle;

		const float oneTurn = tAbs<float>( model()->maxValue() -
							model()->minValue() ) *
							360.0 / m_totalAngle;
		const float eqValue = model()->value() + m_mouseOffset;

		if( tAbs<float>( new_value - eqValue ) > 0.5 * oneTurn )
		{
			if( new_value < eqValue )
			{
				new_value += oneTurn;
			}
			else
			{
				new_value -= oneTurn;
			}
		}
		return( new_value );
	}
	if( engine::getMainWindow()->isShiftPressed() )
	{
		return( ( _p.y() - m_origMousePos.y() ) * model()->step<float>() );
	}
	return( ( _p.y() - m_origMousePos.y() ) * pageSize() );
}




void knob::contextMenuEvent( QContextMenuEvent * )
{
	// for the case, the user clicked right while pressing left mouse-
	// button, the context-menu appears while mouse-cursor is still hidden
	// and it isn't shown again until user does something which causes
	// an QApplication::restoreOverrideCursor()-call...
	mouseReleaseEvent( NULL );

	captionMenu contextMenu( model()->displayName() );
	addDefaultActions( &contextMenu );
	contextMenu.addSeparator();
	contextMenu.addAction( embed::getIconPixmap( "help" ), tr( "&Help" ),
						this, SLOT( displayHelp() ) );
	contextMenu.exec( QCursor::pos() );
}




void knob::dragEnterEvent( QDragEnterEvent * _dee )
{
	stringPairDrag::processDragEnterEvent( _dee, "float_value,"
								"link_object" );
}




void knob::dropEvent( QDropEvent * _de )
{
	QString type = stringPairDrag::decodeKey( _de );
	QString val = stringPairDrag::decodeValue( _de );
	if( type == "float_value" )
	{
		model()->setValue( val.toFloat() );
		_de->accept();
	}
	else if( type == "link_object" )
	{
		knobModel * mod = (knobModel *)( val.toULong() );
		automatableModel::linkModels( model(), mod );
		mod->setValue( model()->value() );
	}
}




void knob::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton &&
			engine::getMainWindow()->isCtrlPressed() == FALSE &&
			engine::getMainWindow()->isShiftPressed() == FALSE )
	{
		model()->prepareJournalEntryFromOldVal();

		const QPoint & p = _me->pos();
		m_origMousePos = p;

		if( configManager::inst()->value( "knobs",
						"classicalusability").toInt() )
		{
			m_mouseOffset = getValue( p ) - model()->value();
		}
		emit sliderPressed();

		if( !configManager::inst()->value( "knobs", "classicalusability"
								).toInt() )
		{
			QApplication::setOverrideCursor( Qt::BlankCursor );
		}
//		s_textFloat->reparent( this );
		s_textFloat->setText( m_description +
						QString::number(
							model()->value() ) +
							m_unit );
		s_textFloat->moveGlobal( this,
				QPoint( width() + 2, 0 ) );
		s_textFloat->show();
		m_buttonPressed = TRUE;
	}
	else if( _me->button() == Qt::LeftButton &&
			engine::getMainWindow()->isCtrlPressed() == TRUE/* &&
			engine::getMainWindow()->isShiftPressed() == FALSE*/ )
	{
		new stringPairDrag( "float_value",
					QString::number( model()->value() ),
							QPixmap(), this );
	}
	else if( _me->button() == Qt::LeftButton &&
/*			engine::getMainWindow()->isCtrlPressed() == TRUE &&*/
			engine::getMainWindow()->isShiftPressed() == TRUE )
	{
        /* this pointer was casted to uint, 
         * compile time error on 64 bit systems */
		new stringPairDrag( "link_object",
					QString::number( (ulong) model() ),
							QPixmap(), this );
	}
	else if( _me->button() == Qt::MidButton )
	{
		model()->reset();
	}
}




void knob::mouseMoveEvent( QMouseEvent * _me )
{
	if( m_buttonPressed == TRUE )
	{
		setPosition( _me->pos() );
		emit sliderMoved( model()->value() );
//		emit valueChanged();
		if( !configManager::inst()->value( "knobs",
						"classicalusability").toInt() )
		{
			QCursor::setPos( mapToGlobal( m_origMousePos ) );
		}
	}

	s_textFloat->setText( m_description +
				QString::number( model()->value() ) +
							 m_unit );
}




void knob::mouseReleaseEvent( QMouseEvent * /* _me*/ )
{
	model()->addJournalEntryFromOldToCurVal();

	if( m_buttonPressed )
	{
		m_buttonPressed = TRUE;
		buttonReleased();
	}

	m_mouseOffset = 0;
	emit sliderReleased();

	if( !configManager::inst()->value( "knobs", "classicalusability"
								).toInt() )
	{
		QApplication::restoreOverrideCursor();
	}

	s_textFloat->hide();
}




void knob::mouseDoubleClickEvent( QMouseEvent * )
{
	enterValue();
}




void knob::paintEvent( QPaintEvent * _me )
{
	QPainter p( this );

	drawKnob( &p );
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




void knob::wheelEvent( QWheelEvent * _we )
{
	_we->accept();
	const int inc = ( _we->delta() > 0 ) ? 1 : -1;
	model()->incValue( inc );


	s_textFloat->setText( m_description +
					QString::number( model()->value() ) +
						m_unit );
	s_textFloat->moveGlobal( this, QPoint( width() + 2, 0 ) );
	s_textFloat->setVisibilityTimeOut( 1000 );

	emit sliderMoved( model()->value() );
//	emit valueChanged();
}




void knob::buttonReleased( void )
{
//	emit valueChanged( model()->value() );
//	emit valueChanged();
}




void knob::setPosition( const QPoint & _p )
{
	if( configManager::inst()->value( "knobs", "classicalusability"
								).toInt() )
	{
		model()->setValue( getValue( _p ) - m_mouseOffset );
	}
	else
	{
		model()->setValue( model()->value() - getValue( _p ) );
	}
}







/*void knob::reset( void )
{
	model()->setValue( model()->initValue() );
	s_textFloat->setText( m_description +
					QString::number( model()->value() ) +
							m_unit );
	s_textFloat->moveGlobal( this, QPoint( width() + 2, 0 ) );
	s_textFloat->setVisibilityTimeOut( 1000 );
}




void knob::copyValue( void )
{
	s_copiedValue = model()->value();
}




void knob::pasteValue( void )
{
	model()->setValue( s_copiedValue );
	s_textFloat->setText( m_description +
					QString::number( model()->value() ) +
							m_unit );
	s_textFloat->moveGlobal( this, QPoint( width() + 2, 0 ) );
	s_textFloat->setVisibilityTimeOut( 1000 );
}*/




void knob::enterValue( void )
{
	bool ok;
	float new_val = QInputDialog::getDouble(
					this,
					accessibleName(),
					tr( "Please enter a new value between "
						"%1 and %2:" ).
						arg( model()->minValue() ).
						arg( model()->maxValue() ),
					model()->value(),
					model()->minValue(),
					model()->maxValue(),
					4, &ok );
	if( ok )
	{
		model()->setValue( new_val );
	}
}




void knob::connectToMidiDevice( void )
{
	//engine::getMixer()->getMIDIDevice()->setPitchBendKnob( this );
}




void knob::friendlyUpdate( void )
{
	if( model()->getControllerConnection() == NULL ||
				controller::runningFrames() % (256*4) == 0 )
	{
		update();
	}
}




void knob::doConnections( void )
{
	if( model() != NULL )
	{
		QObject::connect( model(), SIGNAL( dataChanged() ),
				this, SLOT( friendlyUpdate() ),
				Qt::QueuedConnection );

		QObject::connect( model(), SIGNAL( propertiesChanged() ),
				this, SLOT( update() ), Qt::QueuedConnection );
	}
}




void knob::displayHelp( void )
{
	QWhatsThis::showText( mapToGlobal( rect().bottomRight() ),
								whatsThis() );
}




#include "knob.moc"


#endif
