/*
 * knob.cpp - powerful knob-widget
 *
 * This file is based on the knob-widget of the Qwt Widget Library from
 * Josef Wilgen
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include "qt3support.h"

#ifdef QT4

#include <QPainter>
#include <QPalette>
#include <QBitmap>
#include <QLabel>
#include <QStatusBar>
#include <QMouseEvent>
#include <QMenu>
#include <QStatusBar>
#include <QFontMetrics>
#include <QApplication>

#else

#include <qpainter.h>
#include <qpalette.h>
#include <qbitmap.h>
#include <qlabel.h>
#include <qpopupmenu.h>
#include <qstatusbar.h>
#include <qfontmetrics.h>
#include <qapplication.h>

#define addSeparator insertSeparator

#endif

#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif
#include <math.h>

#include "knob.h"
#include "song_editor.h"
#include "midi_device.h"
#include "embed.h"
#include "spc_bg_hndl_widget.h"
#include "config_mgr.h"
#include "text_float.h"
#include "mixer.h"
#include "gui_templates.h"
#include "templates.h"


const int WHEEL_DELTA = 120;


static double MinRelStep = 1.0e-10;
static double DefaultRelStep = 1.0e-2;
static double MinEps = 1.0e-10;


float knob::s_copiedValue = 0.0f;
textFloat * knob::s_textFloat = NULL;



knob::knob( int _knob_num, QWidget * _parent, const QString & _name ) :
	QWidget( _parent
#ifndef QT4
			, _name.ascii()
#endif
		),
	m_scrollMode( ScrNone ),
	m_mouseOffset( 0.0f ),
	m_tracking( TRUE ),
	m_angle( 0.0f ),
	m_oldAngle( 0.0f ),
	m_nTurns( 0.0f ),	
	m_knobNum( _knob_num ),
	m_hintTextBeforeValue( "" ),
	m_hintTextAfterValue( "" ),
	m_label( "" ),
	m_minValue( 0.0f ),
	m_maxValue( 100.0f ),
	m_value( 0.0f ),
	m_exactValue( 0.0f ),
	m_exactPrevValue( 0.0f ),
	m_prevValue( 0.0f ),
	m_initValue( 0.0f )
{
	if( s_textFloat == NULL )
	{
		s_textFloat = new textFloat( this );
	}

#ifdef QT4
	setAccessibleName( _name );
#endif
	setRange( 0.0, 100.0, 1.0 );

#ifdef QT4
	m_knobPixmap = new QPixmap( embed::getIconPixmap( QString( "knob0" +
		QString::number( m_knobNum + 1 ) ).toAscii().constData() ) );
#else
	m_knobPixmap = new QPixmap( embed::getIconPixmap( "knob0" +
					QString::number( m_knobNum + 1 ) ) );
#endif
#ifdef QT4
//	setAttribute( Qt::WA_NoBackground );
#else
	setBackgroundMode( Qt::NoBackground );
#endif

	m_knobWidth = m_knobPixmap->width();

	setFixedSize( m_knobPixmap->width(), m_knobPixmap->height() );
	setTotalAngle( 270.0f );
	recalcAngle();

}




// Destructor
knob::~knob()
{
	// make sure pointer to this knob isn't used anymore in active
	// midi-device-class
	if( mixer::inst()->getMIDIDevice()->pitchBendKnob() == this )
	{
		mixer::inst()->getMIDIDevice()->setPitchBendKnob( NULL );
	}
}




void knob::setHintText( const QString & _txt_before,
						const QString & _txt_after )
{
	m_hintTextBeforeValue = _txt_before;
	m_hintTextAfterValue = _txt_after;
/*	toolTip::add( this, m_hintTextBeforeValue + QString::number( value() ) +
						m_hintTextAfterValue );*/
}




void knob::setLabel( const QString & _txt )
{
	m_label = _txt;
	setFixedSize( tMax<int>( m_knobPixmap->width(),
					QFontMetrics( pointSize<6>( font()
							) ).width( m_label ) ),
						m_knobPixmap->height() + 10 );
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

	layoutKnob();
}




void knob::drawKnob( QPainter * _p )
{
	_p->drawPixmap( 0, 0, specialBgHandlingWidget::getBackground( this ) );

	const float radius = m_knobPixmap->width() / 2 - 1;
	const float xm = m_knobPixmap->width() / 2;//radius + 1;
	const float ym = m_knobPixmap->height() / 2;//radius+1;

	const float rarc = m_angle * M_PI / 180.0;
	const float ca = cos( rarc );
	const float sa = -sin( rarc );

	_p->drawPixmap( static_cast<int>( xm - m_knobPixmap->width() / 2 ), 0,
								*m_knobPixmap );

	_p->setPen( QPen( QColor( 200, 0, 0 ), 2 ) );

	switch( m_knobNum )
	{
		case knobSmall_17:
		{
			_p->drawLine( (int)( xm-sa ), (int)( ym-ca ),
					(int)( xm - sa*radius ),
					(int)( ym - ca*radius ) );
			break;
		}
		case knobBright_26:
		{
			_p->drawLine( (int)( xm-sa ), (int)( ym-ca ),
					(int)( xm - sa*( radius-5 ) ),
					(int)( ym - ca*( radius-5 ) ) );
			break;
		}
		case knobDark_28:
		{
			const float rb = tMax<float>( ( radius - 10 ) / 3.0,
									0.0 );
			const float re = tMax<float>( ( radius - 4 ), 0.0 );
			_p->drawLine( (int)( xm-sa*rb ) + 1,
					(int)( ym - ca*rb ) + 1,
					(int)( xm - sa*re ) + 1,
					(int)( ym - ca*re ) + 1 );
			break;
		}
	}
}




void knob::valueChange( void )
{
	recalcAngle();
	update();
/*	toolTip::add( this, m_hintTextBeforeValue + QString::number( value() ) +
						m_hintTextAfterValue );*/
	if( m_tracking )
	{
		emit valueChanged( value() );
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

		float new_value = 0.5 * ( m_minValue + m_maxValue ) +
					( arc + m_nTurns * 360.0 ) *
					( m_maxValue - m_minValue ) /
								m_totalAngle;

		const float oneTurn = tAbs<float>( m_maxValue - m_minValue ) *
							360.0 / m_totalAngle;
		const float eqValue = value() + m_mouseOffset;

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

	return( ( _p.y() - m_origMousePos.y() ) * m_step );
}




void knob::getScrollMode( const QPoint &, int & _scroll_mode, int & _direction )
{
	_scroll_mode = ScrMouse;
	_direction = 0;
}




void knob::rangeChange()
{
	layoutKnob();
	recalcAngle();
}




void knob::resizeEvent( QResizeEvent * )
{
	layoutKnob( FALSE );
}




// Recalculate the slider's geometry and layout based on
// the current rect and fonts.
void knob::layoutKnob( bool _update_geometry )
{
	if( _update_geometry )
	{
		updateGeometry();
		update();
	}
}




void knob::paintEvent( QPaintEvent * _me )
{
	// Use double-buffering
	QRect ur = _me->rect();
#ifndef QT4
	if( ur.isValid() )
	{
#endif
#ifdef QT4
		QPainter p( this );
#else
		QPixmap pix( ur.size() );
		pix.fill( this, ur.topLeft() );
		QPainter p( &pix, this );
#endif
		p.translate( -ur.x(), -ur.y() );
		drawKnob( &p );
		if( m_label != "" )
		{
			p.setFont( pointSize<6>( p.font() ) );
			p.setPen( QColor( 255, 255, 255 ) );
			p.drawText( width() / 2 -
				QFontMetrics( p.font() ).width( m_label ) / 2,
					height() - 2, m_label );
		}
#ifndef QT4
		p.end();
		bitBlt( this, ur.topLeft(), &pix );
	}
#endif

}



void knob::recalcAngle( void )
{
	m_oldAngle = m_angle;

	//
	// calculate the angle corresponding to the value
	//
	if( m_maxValue == m_minValue )
	{
		m_angle = 0;
		m_nTurns = 0;
	}
	else
	{
		m_angle = ( value() - 0.5 * ( m_minValue + m_maxValue ) ) /
				( m_maxValue - m_minValue ) * m_totalAngle;
		m_nTurns = floor( ( m_angle + 180.0 ) / 360.0 );
		m_angle = m_angle - m_nTurns * 360.0;
	}
}




//! Mouse press event handler
void knob::mousePressEvent( QMouseEvent * _me )
{
	const QPoint & p = _me->pos();
	m_origMousePos = p;

	getScrollMode( p, m_scrollMode, m_direction );

	switch( m_scrollMode )
	{
		case ScrMouse:
			m_mouseOffset = getValue( p ) - value();
			emit sliderPressed();
			break;

		default:
			m_mouseOffset = 0;
			m_direction = 0;
			break;
	}

	if( _me->button() == Qt::LeftButton )
	{
		if( !configManager::inst()->value( "knobs", "classicalusability"
								).toInt() )
		{
			QApplication::setOverrideCursor( Qt::BlankCursor );
		}
		s_textFloat->reparent( this );
		s_textFloat->setText( m_hintTextBeforeValue +
						QString::number( value() ) +
							m_hintTextAfterValue );
		s_textFloat->move( mapTo( topLevelWidget(), QPoint( 0, 0 ) ) +
				QPoint( m_knobPixmap->width() + 2, 0 ) );
		s_textFloat->show();
	}
}




//! Mouse Move Event handler
void knob::mouseMoveEvent( QMouseEvent * _me )
{
	if( m_scrollMode == ScrMouse )
	{
		setPosition( _me->pos() );
		if( value() != m_prevValue )
		{
			emit sliderMoved( value() );
			if( !configManager::inst()->value( "knobs",
						"classicalusability").toInt() )
			{
				QCursor::setPos( mapToGlobal(
							m_origMousePos ) );
			}
		}
	}
	songEditor::inst()->setModified();

	s_textFloat->setText( m_hintTextBeforeValue +
						QString::number( value() ) +
							m_hintTextAfterValue );

}




//! Mouse Release Event handler
void knob::mouseReleaseEvent( QMouseEvent * _me )
{
	m_scrollMode = ScrNone;
	buttonReleased();

	switch( m_scrollMode )
	{
		case ScrMouse:
			setPosition( _me->pos() );
			m_direction = 0;
			m_mouseOffset = 0;
			emit sliderReleased ();
			break;

		case ScrDirect:
			setPosition( _me->pos() );
			m_direction = 0;
			m_mouseOffset = 0;
			break;

		default:
			break;
	}

	if( !configManager::inst()->value( "knobs", "classicalusability"
								).toInt() )
	{
		QApplication::restoreOverrideCursor();
	}

	s_textFloat->hide();
}




//! Qt wheel event
void knob::wheelEvent( QWheelEvent * _me )
{
	_me->accept();
	const int inc = _me->delta() / WHEEL_DELTA;
	incPages( inc );

	songEditor::inst()->setModified();

	s_textFloat->reparent( this );
	s_textFloat->setText( m_hintTextBeforeValue +
					QString::number( value() ) +
						m_hintTextAfterValue );
	s_textFloat->move( mapTo( topLevelWidget(), QPoint( 0, 0 ) ) +
				QPoint( m_knobPixmap->width() + 2, 0 ) );
	s_textFloat->setVisibilityTimeOut( 1000 );

/*	toolTip::add( this, m_hintTextBeforeValue+QString::number( value() ) +
						m_hintTextAfterValue );*/

	if( value() != m_prevValue )
	{
		emit sliderMoved( value() );
	}
}




//! Emits a valueChanged() signal if necessary
void knob::buttonReleased( void )
{
	if( ( !m_tracking ) || ( value() != m_prevValue ) )
	{
		emit valueChanged( value() );
	}
}




void knob::setPosition( const QPoint & _p )
{
	if( configManager::inst()->value( "knobs", "classicalusability"
								).toInt() )
	{
		setNewValue( getValue( _p ) - m_mouseOffset, 1 );
	}
	else
	{
		setNewValue( m_value - getValue( _p ), 1 );
	}
}




void knob::setTracking( bool _enable )
{
	m_tracking = _enable;
}





void knob::setValue( float _val, bool _is_init_value )
{
	if( _is_init_value )
	{
		m_initValue = _val;
	}

	setNewValue( _val, 0 );
}




void knob::fitValue( float _val )
{
	setNewValue( _val, 1 );
}




void knob::incValue( int _steps )
{
	setNewValue( m_value + float( _steps ) * m_step, 1 );
}




void knob::setRange( float _vmin, float _vmax, float _vstep, int _page_size )
{
	int rchg = ( ( m_maxValue != _vmax ) || ( m_minValue != _vmin ) );

	if( rchg )
	{
		m_minValue = _vmin;
		m_maxValue = _vmax;
	}

	//
	// look if the step width has an acceptable 
	// value or otherwise change it.
	//
	setStep( _vstep );

	//
	// limit page size
	//
/*	m_pageSize = tLimit( pageSize, 0, int( tAbs<float>( ( m_maxValue -
						m_minValue ) / m_step ) ) ); */
	m_pageSize = tMax<float>( ( m_maxValue - m_minValue ) / 40.0f, _vstep );

	// 
	// If the value lies out of the range, it 
	// will be changed. Note that it will not be adjusted to 
	// the new step width.
	setNewValue( m_value, 0 );

	// call notifier after the step width has been 
	// adjusted.
	if( rchg )
	{
		rangeChange();
	}
}




void knob::setNewValue( float _x, int _align )
{
	m_prevValue = m_value;

	m_value = tLimit( _x, m_minValue, m_maxValue );

	m_exactPrevValue = m_exactValue;
	m_exactValue = m_value;

	// align to grid
	if( _align )
	{
		if( m_step != 0.0 )
		{
			m_value = floorf( m_value / m_step ) * m_step;
		}
		else
		{
			m_value = m_minValue;
		}

		// correct rounding error at the border
		if( tAbs<float>( m_value - m_maxValue ) < MinEps *
							tAbs<float>( m_step ) )
		{
			m_value = m_maxValue;
		}

		// correct rounding error if value = 0
		if( tAbs<float>( m_value ) < MinEps * tAbs<float>( m_step ) )
		{
			m_value = 0.0;
		}
	}

	if( m_prevValue != m_value )
	{
		valueChange();
	}
}




void knob::setStep( float _vstep )
{
	float intv = m_maxValue - m_minValue;

	float newStep;

	if( _vstep == 0.0 )
	{
		newStep = intv * DefaultRelStep;
	}
	else
	{
		if( ( intv > 0 ) && ( _vstep < 0 ) || ( intv < 0 ) &&
								( _vstep > 0 ) )
		{
			newStep = -_vstep;
		}
		else
		{
			newStep = _vstep;
		}
		if( tAbs<float>( newStep ) <
					tAbs<float>( MinRelStep * intv ) )
		{
			newStep = MinRelStep * intv;
		}
	}
	if( newStep != m_step )
	{
		m_step = newStep;
	}
}




void knob::contextMenuEvent( QContextMenuEvent * )
{
	QMenu contextMenu( this );
#ifdef QT4
	contextMenu.setTitle( accessibleName() );
#else
	QLabel * caption = new QLabel( "<font color=white><b>" +
			QString( accessibleName() ) + "</b></font>", this );
	caption->setPaletteBackgroundColor( QColor( 0, 0, 192 ) );
	caption->setAlignment( Qt::AlignCenter );
	contextMenu.addAction( caption );
#endif
	contextMenu.addAction( embed::getIconPixmap( "reload" ),
				tr( "&Reset (%1%2)" ).arg( m_initValue ).arg(
							m_hintTextAfterValue ),
							this, SLOT( reset() ) );
	contextMenu.addSeparator();
	contextMenu.addAction( embed::getIconPixmap( "edit_copy" ),
				tr( "&Copy value (%1%2)" ).arg( value() ).arg(
							m_hintTextAfterValue ),
						this, SLOT( copyValue() ) );
	contextMenu.addAction( embed::getIconPixmap( "edit_paste" ),
				tr( "&Paste value (%1%2)"
						).arg( s_copiedValue ).arg(
							m_hintTextAfterValue ),
				this, SLOT( pasteValue() ) );
	contextMenu.addSeparator();
	contextMenu.addAction( tr( "Connect to MIDI-device" ), this,
						SLOT( connectToMidiDevice() ) );
	contextMenu.addSeparator();
	contextMenu.addAction( embed::getIconPixmap( "help" ), tr( "&Help" ),
						this, SLOT( displayHelp() ) );
	contextMenu.exec( QCursor::pos() );
}




void knob::reset( void )
{
	setValue( m_initValue );
}




void knob::copyValue( void )
{
	s_copiedValue = value();
}




void knob::pasteValue( void )
{
	setValue( s_copiedValue );
}




void knob::connectToMidiDevice( void )
{
	mixer::inst()->getMIDIDevice()->setPitchBendKnob( this );
}




void knob::displayHelp( void )
{
#ifdef QT4
	QWhatsThis::showText( mapToGlobal( rect().bottomRight() ),
								whatsThis() );
#else
	QWhatsThis::display( QWhatsThis::textFor( this ), mapToGlobal(
						rect().bottomRight() ) );
#endif
}




#include "knob.moc"


#ifndef QT4
#undef addSeparator
#endif
