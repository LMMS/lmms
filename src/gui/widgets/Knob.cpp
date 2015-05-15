/*
 * Knob.cpp - powerful knob-widget
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#include <QApplication>
#include <QBitmap>
#include <QFontMetrics>
#include <QInputDialog>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QWhatsThis>

#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif

#include "lmms_math.h"
#include "Knob.h"
#include "CaptionMenu.h"
#include "ConfigManager.h"
#include "ControllerConnection.h"
#include "embed.h"
#include "Engine.h"
#include "gui_templates.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "ProjectJournal.h"
#include "Song.h"
#include "StringPairDrag.h"
#include "templates.h"
#include "TextFloat.h"


namespace lmms
{


TextFloat * Knob::s_textFloat = NULL;



//! @todo: in C++11, we can use delegating ctors
#define DEFAULT_KNOB_INITIALIZER_LIST \
	QWidget( _parent ), \
	FloatModelView( new FloatModel( 0, 0, 0, 1, NULL, _name, true ), this ), \
	m_label( "" ), \
	m_knobPixmap( NULL ), \
	m_volumeKnob( false ), \
	m_volumeRatio( 100.0, 0.0, 1000000.0 ), \
	m_buttonPressed( false ), \
	m_angle( -10 ), \
	m_lineWidth(0)

Knob::Knob( knobTypes _knob_num, QWidget * _parent, const QString & _name ) :
	DEFAULT_KNOB_INITIALIZER_LIST,
	m_knobNum( _knob_num )
{
	initUi( _name );
}

Knob::Knob( QWidget * _parent, const QString & _name ) :
	DEFAULT_KNOB_INITIALIZER_LIST,
	m_knobNum( knobBright_26 )
{
	initUi( _name );
}

#undef DEFAULT_KNOB_INITIALIZER_LIST




void Knob::initUi( const QString & _name )
{
	if( s_textFloat == NULL )
	{
		s_textFloat = new TextFloat;
	}

	setWindowTitle( _name );

	onKnobNumUpdated();
	setTotalAngle( 270.0f );
	setInnerRadius( 1.0f );
	setOuterRadius( 10.0f );
	setFocusPolicy( Qt::ClickFocus );
	doConnections();
}




void Knob::onKnobNumUpdated()
{
	if( m_knobNum != knobStyled )
	{
		m_knobPixmap = new QPixmap( embed::getIconPixmap( QString( "knob0" +
			QString::number( m_knobNum + 1 ) ).toUtf8().constData() ) );

		setFixedSize( m_knobPixmap->width(), m_knobPixmap->height() );
	}
}




Knob::~Knob()
{
	if( m_knobPixmap )
	{
		delete m_knobPixmap;
	}
}




void Knob::setLabel( const QString & _txt )
{
	m_label = _txt;
	if( m_knobPixmap )
	{
		setFixedSize( qMax<int>( m_knobPixmap->width(),
					QFontMetrics( pointSizeF( font(), 6.5) ).width( m_label ) ),
						m_knobPixmap->height() + 10 );
	}
	update();
}




void Knob::setTotalAngle( float _angle )
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




float Knob::innerRadius() const
{
	return m_innerRadius;
}



void Knob::setInnerRadius( float _r )
{
	m_innerRadius = _r;
}



float Knob::outerRadius() const
{
	return m_outerRadius;
}



void Knob::setOuterRadius( float _r )
{
	m_outerRadius = _r;
}




knobTypes Knob::knobNum() const
{
	return m_knobNum;
}




void Knob::setknobNum( knobTypes _k )
{
	if( m_knobNum != _k )
	{
		m_knobNum = _k;
		onKnobNumUpdated();
	}
}




QPointF Knob::centerPoint() const
{
	return m_centerPoint;
}



float Knob::centerPointX() const
{
	return m_centerPoint.x();
}



void Knob::setCenterPointX( float _c )
{
	m_centerPoint.setX( _c );
}



float Knob::centerPointY() const
{
	return m_centerPoint.y();
}



void Knob::setCenterPointY( float _c )
{
	m_centerPoint.setY( _c );
}



float Knob::lineWidth() const
{
	return m_lineWidth;
}



void Knob::setLineWidth( float _w )
{
	m_lineWidth = _w;
}



QColor Knob::outerColor() const
{
	return m_outerColor;
}



void Knob::setOuterColor( const QColor & _c )
{
	m_outerColor = _c;
}



QColor Knob::lineColor() const
{
	return m_lineColor;
}



void Knob::setlineColor( const QColor & _c )
{
	m_lineColor = _c;
}



QColor Knob::arcColor() const
{
	return m_arcColor;
}



void Knob::setarcColor( const QColor & _c )
{
	m_arcColor = _c;
}




QLineF Knob::calculateLine( const QPointF & _mid, float _radius, float _innerRadius ) const
{
	const float rarc = m_angle * F_PI / 180.0;
	const float ca = cos( rarc );
	const float sa = -sin( rarc );

	return QLineF( _mid.x() - sa*_innerRadius, _mid.y() - ca*_innerRadius,
					_mid.x() - sa*_radius, _mid.y() - ca*_radius );
}



bool Knob::updateAngle()
{
	int angle = 0;
	if( model() && model()->maxValue() != model()->minValue() )
	{
		angle = angleFromValue( model()->inverseScaledValue( model()->value() ), model()->minValue(), model()->maxValue(), m_totalAngle );
	}
	if( qAbs( angle - m_angle ) > 3 )
	{
		m_angle = angle;
		return true;
	}
	return false;
}




void Knob::drawKnob( QPainter * _p )
{
	if( updateAngle() == false && !m_cache.isNull() )
	{
		_p->drawImage( 0, 0, m_cache );
		return;
	}

	m_cache = QImage( size(), QImage::Format_ARGB32 );
	m_cache.fill( qRgba( 0, 0, 0, 0 ) );

	QPainter p( &m_cache );

	QPoint mid;

	if( m_knobNum == knobStyled )
	{
		p.setRenderHint( QPainter::Antialiasing );

		// Perhaps this can move to setOuterRadius()
		if( m_outerColor.isValid() )
		{
			QRadialGradient gradient( centerPoint(), outerRadius() );
			gradient.setColorAt( 0.4, _p->pen().brush().color() );
			gradient.setColorAt( 1, m_outerColor );

			p.setPen( QPen( gradient, lineWidth(),
						Qt::SolidLine, Qt::RoundCap ) );
		}
		else {
			QPen pen = p.pen();
			pen.setWidth( (int) lineWidth() );
			pen.setCapStyle( Qt::RoundCap );

			p.setPen( pen );
		}

		p.drawLine( calculateLine( centerPoint(), outerRadius(),
							innerRadius() ) );
		p.end();
		_p->drawImage( 0, 0, m_cache );
		return;
	}


	// Old-skool knobs
	const float radius = m_knobPixmap->width() / 2.0f - 1;
	mid = QPoint( width() / 2, m_knobPixmap->height() / 2 );

	p.drawPixmap( static_cast<int>(
				width() / 2 - m_knobPixmap->width() / 2 ), 0,
				*m_knobPixmap );

	p.setRenderHint( QPainter::Antialiasing );

	const int centerAngle = angleFromValue( model()->inverseScaledValue( model()->centerValue() ), model()->minValue(), model()->maxValue(), m_totalAngle );

	const int arcLineWidth = 2;
	const int arcRectSize = m_knobPixmap->width() - arcLineWidth;

	QColor col;
	if( m_knobNum == knobVintage_32 )
	{	col = QApplication::palette().color( QPalette::Active, QPalette::Shadow ); }
	else
	{	col = QApplication::palette().color( QPalette::Active, QPalette::WindowText ); }
	col.setAlpha( 70 );

	p.setPen( QPen( col, 2 ) );
	p.drawArc( mid.x() - arcRectSize/2, 1, arcRectSize, arcRectSize, 315*16, 16*m_totalAngle );

	switch( m_knobNum )
	{
		case knobSmall_17:
		{
			p.setPen( QPen( QApplication::palette().color( QPalette::Active,
							QPalette::WindowText ), 2 ) );
			p.drawLine( calculateLine( mid, radius-2 ) );
			break;
		}
		case knobBright_26:
		{
			p.setPen( QPen( QApplication::palette().color( QPalette::Active, QPalette::WindowText ), 2 ) );
			p.drawLine( calculateLine( mid, radius-5 ) );
			break;
		}
		case knobDark_28:
		{
			p.setPen( QPen( QApplication::palette().color( QPalette::Active, QPalette::WindowText ), 2 ) );
			const float rb = qMax<float>( ( radius - 10 ) / 3.0,
									0.0 );
			const float re = qMax<float>( ( radius - 4 ), 0.0 );
			QLineF ln = calculateLine( mid, re, rb );
			ln.translate( 1, 1 );
			p.drawLine( ln );
			break;
		}
		case knobGreen_17:
		{
			p.setPen( QPen( QApplication::palette().color( QPalette::Active,
							QPalette::BrightText), 2 ) );
			p.drawLine( calculateLine( mid, radius ) );
			break;
		}
		case knobVintage_32:
		{
			p.setPen( QPen( QApplication::palette().color( QPalette::Active,
							QPalette::Shadow), 2 ) );
			p.drawLine( calculateLine( mid, radius-2, 2 ) );
			break;
		}
		case knobStyled:
			break;
	}

	p.drawArc( mid.x() - arcRectSize/2, 1, arcRectSize, arcRectSize, (90-centerAngle)*16, -16*(m_angle-centerAngle) );

	p.end();

	_p->drawImage( 0, 0, m_cache );
}

float Knob::getValue( const QPoint & _p )
{
	float value;

	// arcane mathemagicks for calculating knob movement
	value = ( ( _p.y() + _p.y() * qMin( qAbs( _p.y() / 2.5f ), 6.0f ) ) ) / 12.0f;

	// if shift pressed we want slower movement
	if( gui->mainWindow()->isShiftPressed() )
	{
		value /= 4.0f;
		value = qBound( -4.0f, value, 4.0f );
	}
	return value * pageSize();
}




void Knob::contextMenuEvent( QContextMenuEvent * )
{
	// for the case, the user clicked right while pressing left mouse-
	// button, the context-menu appears while mouse-cursor is still hidden
	// and it isn't shown again until user does something which causes
	// an QApplication::restoreOverrideCursor()-call...
	mouseReleaseEvent( NULL );

	CaptionMenu contextMenu( model()->displayName(), this );
	addDefaultActions( &contextMenu );
	contextMenu.addAction( QPixmap(),
		model()->isScaleLogarithmic() ? tr( "Set linear" ) : tr( "Set logarithmic" ),
		this, SLOT( toggleScale() ) );
	contextMenu.addSeparator();
	contextMenu.addHelpAction();
	contextMenu.exec( QCursor::pos() );
}


void Knob::toggleScale()
{
	model()->setScaleLogarithmic( ! model()->isScaleLogarithmic() );
}



void Knob::dragEnterEvent( QDragEnterEvent * _dee )
{
	StringPairDrag::processDragEnterEvent( _dee, "float_value,"
							"automatable_model" );
}




void Knob::dropEvent( QDropEvent * _de )
{
	QString type = StringPairDrag::decodeKey( _de );
	QString val = StringPairDrag::decodeValue( _de );
	if( type == "float_value" )
	{
		model()->setValue( val.toFloat() );
		_de->accept();
	}
	else if( type == "automatable_model" )
	{
		AutomatableModel * mod = dynamic_cast<AutomatableModel *>(
				Engine::projectJournal()->
					journallingObject( val.toInt() ) );
		if( mod != NULL )
		{
			AutomatableModel::linkModels( model(), mod );
			mod->setValue( model()->value() );
		}
	}
}




void Knob::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton &&
			! ( _me->modifiers() & Qt::ControlModifier ) &&
			! ( _me->modifiers() & Qt::ShiftModifier ) )
	{
		AutomatableModel *thisModel = model();
		if( thisModel )
		{
			thisModel->addJournalCheckPoint();
			thisModel->saveJournallingState( false );
		}

		const QPoint & p = _me->pos();
		m_origMousePos = p;
		m_mouseOffset = QPoint(0, 0);
		m_leftOver = 0.0f;

		emit sliderPressed();

		QApplication::setOverrideCursor( Qt::BlankCursor );
		s_textFloat->setText( displayValue() );
		s_textFloat->moveGlobal( this,
				QPoint( width() + 2, 0 ) );
		s_textFloat->show();
		m_buttonPressed = true;
	}
	else if( _me->button() == Qt::LeftButton &&
			gui->mainWindow()->isShiftPressed() == true )
	{
		new StringPairDrag( "float_value",
					QString::number( model()->value() ),
							QPixmap(), this );
	}
	else
	{
		FloatModelView::mousePressEvent( _me );
	}
}




void Knob::mouseMoveEvent( QMouseEvent * _me )
{
	if( m_buttonPressed && _me->pos() != m_origMousePos )
	{
		m_mouseOffset = _me->pos() - m_origMousePos;
		setPosition( m_mouseOffset );
		emit sliderMoved( model()->value() );
		QCursor::setPos( mapToGlobal( m_origMousePos ) );
	}

	s_textFloat->setText( displayValue() );
}




void Knob::mouseReleaseEvent( QMouseEvent* event )
{
	if( event && event->button() == Qt::LeftButton )
	{
		AutomatableModel *thisModel = model();
		if( thisModel )
		{
			thisModel->restoreJournallingState();
		}
	}

	m_buttonPressed = false;

	emit sliderReleased();

	QApplication::restoreOverrideCursor();

	s_textFloat->hide();
}




void Knob::focusOutEvent( QFocusEvent * _fe )
{
	// make sure we don't loose mouse release event
	mouseReleaseEvent( NULL );
	QWidget::focusOutEvent( _fe );
}




void Knob::mouseDoubleClickEvent( QMouseEvent * )
{
	enterValue();
}




void Knob::paintEvent( QPaintEvent * _me )
{
	QPainter p( this );

	drawKnob( &p );
	if( !m_label.isEmpty() )
	{
		p.setFont( pointSizeF( p.font(), 6.5 ) );
/*		p.setPen( QColor( 64, 64, 64 ) );
		p.drawText( width() / 2 -
			p.fontMetrics().width( m_label ) / 2 + 1,
				height() - 1, m_label );*/
		p.setPen( QColor( 255, 255, 255 ) );
		p.drawText( width() / 2 -
				p.fontMetrics().width( m_label ) / 2,
				height() - 2, m_label );
	}
}




void Knob::wheelEvent( QWheelEvent * _we )
{
	_we->accept();
	const int inc = ( _we->delta() > 0 ) ? 1 : -1;
	model()->incValue( inc );


	s_textFloat->setText( displayValue() );
	s_textFloat->moveGlobal( this, QPoint( width() + 2, 0 ) );
	s_textFloat->setVisibilityTimeOut( 1000 );

	emit sliderMoved( model()->value() );
}




void Knob::setPosition( const QPoint & _p )
{
	const float value = getValue( _p ) + m_leftOver;
	const float step = model()->step<float>();
	const float oldValue = model()->value();


	if( model()->isScaleLogarithmic() ) // logarithmic code
	{
		const float pos = model()->minValue() < 0
			? oldValue / qMax( qAbs( model()->maxValue() ), qAbs( model()->minValue() ) )
			: ( oldValue - model()->minValue() ) / model()->range();
		const float ratio = 0.1f + qAbs( pos ) * 15.f;
		float newValue = value * ratio;
		if( qAbs( newValue ) >= step )
		{
			model()->setValue( oldValue - newValue );
			m_leftOver = 0.0f;
		}
		else
		{
			m_leftOver = value;
		}
	}


	else // linear code
	{
		if( qAbs( value ) >= step )
		{
			model()->setValue( oldValue - value );
			m_leftOver = 0.0f;
		}
		else
		{
			m_leftOver = value;
		}
	}
}




void Knob::enterValue()
{
	bool ok;
	float new_val;
	if( isVolumeKnob() &&
		ConfigManager::inst()->value( "app", "displaydbv" ).toInt() )
	{
		new_val = QInputDialog::getDouble(
			this, windowTitle(),
			tr( "Please enter a new value between "
					"-96.0 dBV and 6.0 dBV:" ),
				20.0 * log10( model()->value() / 100.0 ),
							-96.0, 6.0, 4, &ok );
		if( new_val <= -96.0 )
		{
			new_val = 0.0f;
		}
		else
		{
			new_val = dbvToAmp( new_val ) * 100.0;
		}
	}
	else
	{
		new_val = QInputDialog::getDouble(
				this, windowTitle(),
				tr( "Please enter a new value between "
						"%1 and %2:" ).
						arg( model()->minValue() ).
						arg( model()->maxValue() ),
					model()->value(),
					model()->minValue(),
					model()->maxValue(), 4, &ok );
	}

	if( ok )
	{
		model()->setValue( new_val );
	}
}




void Knob::friendlyUpdate()
{
	if( model()->controllerConnection() == NULL ||
		model()->controllerConnection()->getController()->frequentUpdates() == false ||
				Controller::runningFrames() % (256*4) == 0 )
	{
		update();
	}
}




QString Knob::displayValue() const
{
	if( isVolumeKnob() &&
		ConfigManager::inst()->value( "app", "displaydbv" ).toInt() )
	{
		return m_description.trimmed() + QString( " %1 dBV" ).
				arg( 20.0 * log10( model()->value() / volumeRatio() ),
								3, 'f', 2 );
	}
	return m_description.trimmed() + QString( " %1" ).
					arg( model()->value() ) + m_unit;
}




void Knob::doConnections()
{
	if( model() != NULL )
	{
		QObject::connect( model(), SIGNAL( dataChanged() ),
					this, SLOT( friendlyUpdate() ) );

		QObject::connect( model(), SIGNAL( propertiesChanged() ),
						this, SLOT( update() ) );
	}
}




void Knob::displayHelp()
{
	QWhatsThis::showText( mapToGlobal( rect().bottomRight() ),
								whatsThis() );
}



}
