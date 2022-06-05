/*
 * Knob.cpp - powerful knob-widget
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <memory>
#include <QApplication>
#include <QFontMetrics>
#include <QInputDialog>
#include <QMouseEvent>
#include <QPainter>

#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif

#include "lmms_math.h"
#include "Knob.h"
#include "CaptionMenu.h"
#include "ConfigManager.h"
#include "ControllerConnection.h"
#include "DeprecationHelper.h"
#include "embed.h"
#include "gui_templates.h"
#include "GuiApplication.h"
#include "LocaleHelper.h"
#include "MainWindow.h"
#include "ProjectJournal.h"
#include "StringPairDrag.h"
#include "TextFloat.h"

TextFloat * Knob::s_textFloat = nullptr;




Knob::Knob( knobTypes _knob_num, QWidget * _parent, const QString & _name ) :
	QWidget( _parent ),
	FloatModelView( new FloatModel( 0, 0, 0, 1, nullptr, _name, true ), this ),
	m_label( "" ),
	m_isHtmlLabel(false),
	m_tdRenderer(nullptr),
	m_volumeKnob( false ),
	m_volumeRatio( 100.0, 0.0, 1000000.0 ),
	m_buttonPressed( false ),
	m_angle( -10 ),
	m_lineWidth( 0 ),
	m_textColor( 255, 255, 255 ),
	m_knobNum( _knob_num )
{
	initUi( _name );
}

Knob::Knob( QWidget * _parent, const QString & _name ) :
	Knob( knobBright_26, _parent, _name )
{
}




void Knob::initUi( const QString & _name )
{
	if( s_textFloat == nullptr )
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
		QString knobFilename;
		switch (m_knobNum)
		{
		case knobDark_28:
			knobFilename = "knob01";
			setOrigSize( 28, 28 );
			break;
		case knobBright_26:
			knobFilename = "knob02";
			setOrigSize( 26, 26 );
			break;
		case knobSmall_17:
			knobFilename = "knob03";
			setOrigSize( 18, 18 );
			break;
		case knobVintage_32:
			knobFilename = "knob05";
			setOrigSize( 32, 32 );
			break;
		case knobStyled: // only here to stop the compiler from complaining
			break;
		}

		// If knobFilename is still empty here we should get the fallback pixmap of size 1x1
		m_knobPixmap = std::make_unique<QPixmap>(QPixmap(embed::getIconPixmap(knobFilename.toUtf8().constData())));
		if (!this->isEnabled())
		{
			convertPixmapToGrayScale(*m_knobPixmap.get());
		}
		setFixedSize( m_knobPixmap->width(), m_knobPixmap->height() );
	}
}


void Knob::setOrigSize( float knobWidth, float knobHeight )
{
	setFixedSize(knobWidth, knobHeight);
	m_origWidth = knobWidth;
	m_origHeight = knobHeight;
}



void Knob::setLabel( const QString & txt )
{
	m_label = txt;
	m_isHtmlLabel = false;
	if( m_knobPixmap )
	{
		setFixedSize(qMax<int>( m_knobPixmap->width(),
					horizontalAdvance(QFontMetrics(pointSizeF(font(), 6.5)), m_label)),
						m_knobPixmap->height() + 10);
	}

	update();
}


void Knob::setHtmlLabel(const QString &htmltxt)
{
	m_label = htmltxt;
	m_isHtmlLabel = true;
	// Put the rendered HTML content into cache
	if (!m_tdRenderer)
	{
		m_tdRenderer = new QTextDocument(this);
	}

	m_tdRenderer->setHtml(QString("<span style=\"color:%1;\">%2</span>").arg(textColor().name(), m_label));

	if (m_knobPixmap)
	{
		setFixedSize( qMax<int>( m_origWidth,
					QFontMetrics( pointSizeF( font(), 6.5) ).width( m_label ) ),
						m_origHeight + 10 );
	}

	update();
}




void Knob::setTotalAngle( float angle )
{
	if( angle < 10.0 )
	{
		m_totalAngle = 10.0;
	}
	else
	{
		m_totalAngle = angle;
	}

	update();
}




float Knob::innerRadius() const
{
	return m_innerRadius;
}



void Knob::setInnerRadius( float r )
{
	m_innerRadius = r;
}



float Knob::outerRadius() const
{
	return m_outerRadius;
}



void Knob::setOuterRadius( float r )
{
	m_outerRadius = r;
}




knobTypes Knob::knobNum() const
{
	return m_knobNum;
}




void Knob::setknobNum( knobTypes k )
{
	if( m_knobNum != k )
	{
		m_knobNum = k;
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



void Knob::setCenterPointX( float c )
{
	m_centerPoint.setX( c );
}



float Knob::centerPointY() const
{
	return m_centerPoint.y();
}



void Knob::setCenterPointY( float c )
{
	m_centerPoint.setY( c );
}



float Knob::lineWidth() const
{
	return m_lineWidth;
}



void Knob::setLineWidth( float w )
{
	m_lineWidth = w;
}



QColor Knob::outerColor() const
{
	return m_outerColor;
}



void Knob::setOuterColor( const QColor & c )
{
	m_outerColor = c;
}



QColor Knob::textColor() const
{
	return m_textColor;
}



void Knob::setTextColor( const QColor & c )
{
	m_textColor = c;
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
	if( qAbs( angle - m_angle ) > 0 )
	{
		m_angle = angle;
		return true;
	}
	return false;
}




void Knob::drawKnob( QPainter * _p )
{
	bool enabled = this->isEnabled();
	QColor currentArcTopColor;
	QColor currentArcBottomColor;
	QColor currentLineColor;
	QColor currentCenterTopColor;
	QColor currentCenterBottomColor;

	switch (knobNum())
	{
		case knobSmall_17: case knobBright_26: case knobDark_28:
		{
			currentArcTopColor = enabled ? m_arcTopActiveColor : m_arcTopInactiveColor;
			currentArcBottomColor = enabled ? m_arcBottomActiveColor : m_arcBottomInactiveColor;
			currentLineColor = enabled ? m_lineActiveColor : m_lineInactiveColor;
			currentCenterTopColor = enabled ? m_centerTopActiveColor : m_centerTopInactiveColor;
			currentCenterBottomColor = enabled ? m_centerBottomActiveColor : m_centerBottomInactiveColor;
			break;
		}
		case knobVintage_32:
		{
			currentLineColor = QApplication::palette().color(QPalette::Active, QPalette::Shadow);
			currentArcTopColor = QColor(QApplication::palette().color(QPalette::Active, QPalette::Shadow));
			currentArcBottomColor = QColor(QApplication::palette().color(QPalette::Active, QPalette::Shadow));
			currentArcBottomColor.setAlpha(70);
			currentCenterTopColor = enabled ? m_centerTopActiveColor : m_centerTopInactiveColor;
			currentCenterBottomColor = enabled ? m_centerBottomActiveColor : m_centerBottomInactiveColor;
			break;
		}
		default:
			break;
	}

	if( updateAngle() == false && !m_cache.isNull() )
	{
		_p->drawImage( 0, 0, m_cache );
		return;
	}

	m_cache = QImage( size(), QImage::Format_ARGB32 );
	m_cache.fill( qRgba( 0, 0, 0, 0 ) );

	QPainter p( &m_cache );

	QPointF mid;

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


	const float radius = m_origWidth / 2.f - 4.5f;
	mid = QPointF( width() / 2, ceil(m_origHeight / 2.f) );

	p.setRenderHint( QPainter::Antialiasing );

	if (m_knobNum == knobSmall_17 || m_knobNum == knobBright_26 || m_knobNum == knobDark_28)
	{
		QLinearGradient centerGrad(0, 5, 0.f, m_origHeight - 5.f);
		centerGrad.setColorAt(0, QColor(77, 80, 91));
		centerGrad.setColorAt(1, QColor(37, 38, 43));
		//p.setPen(QPen(centerGrad, 0));
		p.setPen( QPen( QColor(11, 12, 17), 1, Qt::SolidLine, Qt::RoundCap ) );
		p.setBrush(centerGrad);
		p.drawEllipse(mid, radius, radius);
	}
	else
	{
		p.drawPixmap( static_cast<int>(
					width() / 2 - m_knobPixmap->width() / 2 ), 0,
					*m_knobPixmap );
	}

	const int centerAngle = angleFromValue( model()->inverseScaledValue( model()->centerValue() ), model()->minValue(), model()->maxValue(), m_totalAngle );

	const int arcLineWidth = 2;
	const int arcRectSize = m_origWidth - arcLineWidth;

	p.setPen(QPen(currentArcBottomColor, 2));
	p.drawArc( mid.x() - arcRectSize/2, 1, arcRectSize, arcRectSize, 315*16, 16*m_totalAngle );

	p.setPen(QPen(currentLineColor, 2));
	switch( m_knobNum )
	{
		case knobSmall_17:
		{
			p.drawLine( calculateLine( mid, radius-1.5f, 4 ) );
			break;
		}
		case knobBright_26:
		{
			p.drawLine( calculateLine( mid, radius-1.5f, 4 ) );
			break;
		}
		case knobDark_28:
		{
			const float rb = qMax<float>( ( radius - 10 ) / 3.0, 0.0 );
			const float re = qMax<float>( ( radius - 1.5f ), 0.0 );
			QLineF ln = calculateLine( mid, re, rb );
			ln.translate( 1, 1 );
			p.drawLine( ln );
			break;
		}
		case knobVintage_32:
		{
			p.drawLine( calculateLine( mid, radius + 2, 2 ) );
			break;
		}
		case knobStyled:
			break;
	}

	p.setPen( QPen( currentArcTopColor, 2 ) );
	p.drawArc( mid.x() - arcRectSize/2, 1, arcRectSize, arcRectSize, (90-centerAngle)*16, -16*(m_angle-centerAngle) );

	p.end();

	_p->drawImage( 0, 0, m_cache );
}

float Knob::getValue( const QPoint & _p )
{
	float value;

	// knob value increase is linear to mouse movement
	value = .4f * _p.y();

	// if shift pressed we want slower movement
	if( getGUI()->mainWindow()->isShiftPressed() )
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
	mouseReleaseEvent( nullptr );

	CaptionMenu contextMenu( model()->displayName(), this );
	addDefaultActions( &contextMenu );
	contextMenu.addAction( QPixmap(),
		model()->isScaleLogarithmic() ? tr( "Set linear" ) : tr( "Set logarithmic" ),
		this, SLOT( toggleScale() ) );
	contextMenu.addSeparator();
	contextMenu.exec( QCursor::pos() );
}


void Knob::toggleScale()
{
	model()->setScaleLogarithmic( ! model()->isScaleLogarithmic() );
	update();
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
		model()->setValue( LocaleHelper::toFloat(val) );
		_de->accept();
	}
	else if( type == "automatable_model" )
	{
		AutomatableModel * mod = dynamic_cast<AutomatableModel *>(
				Engine::projectJournal()->
					journallingObject( val.toInt() ) );
		if( mod != nullptr )
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
		m_lastMousePos = p;
		m_leftOver = 0.0f;

		emit sliderPressed();

		s_textFloat->setText( displayValue() );
		s_textFloat->moveGlobal( this,
				QPoint( width() + 2, 0 ) );
		s_textFloat->show();
		m_buttonPressed = true;
	}
	else if( _me->button() == Qt::LeftButton &&
			(_me->modifiers() & Qt::ShiftModifier) )
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
	if( m_buttonPressed && _me->pos() != m_lastMousePos )
	{
		// knob position is changed depending on last mouse position
		setPosition( _me->pos() - m_lastMousePos );
		emit sliderMoved( model()->value() );
		// original position for next time is current position
		m_lastMousePos = _me->pos();
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
	mouseReleaseEvent( nullptr );
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
		if (!m_isHtmlLabel)
		{
			p.setFont(pointSizeF(p.font(), 6.5));
			p.setPen(textColor());
			p.drawText(width() / 2 -
				horizontalAdvance(p.fontMetrics(), m_label) / 2,
				height() - 2, m_label);
		}
		else
		{
			m_tdRenderer->setDefaultFont(pointSizeF(p.font(), 6.5));
			p.translate((width() - m_tdRenderer->idealWidth()) / 2, (height() - m_tdRenderer->pageSize().height()) / 2);
			m_tdRenderer->drawContents(&p);
		}
	}
}




void Knob::wheelEvent(QWheelEvent * we)
{
	we->accept();
	const float stepMult = model()->range() / 2000 / model()->step<float>();
	const int inc = ((we->angleDelta().y() > 0 ) ? 1 : -1) * ((stepMult < 1 ) ? 1 : stepMult);
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
			float roundedValue = qRound( ( oldValue - value ) / step ) * step;
			model()->setValue( roundedValue );
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
			float roundedValue = qRound( ( oldValue - value ) / step ) * step;
			model()->setValue( roundedValue );
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
		ConfigManager::inst()->value( "app", "displaydbfs" ).toInt() )
	{
		new_val = QInputDialog::getDouble(
			this, tr( "Set value" ),
			tr( "Please enter a new value between "
					"-96.0 dBFS and 6.0 dBFS:" ),
				ampToDbfs( model()->getRoundedValue() / 100.0 ),
							-96.0, 6.0, model()->getDigitCount(), &ok );
		if( new_val <= -96.0 )
		{
			new_val = 0.0f;
		}
		else
		{
			new_val = dbfsToAmp( new_val ) * 100.0;
		}
	}
	else
	{
		new_val = QInputDialog::getDouble(
				this, tr( "Set value" ),
				tr( "Please enter a new value between "
						"%1 and %2:" ).
						arg( model()->minValue() ).
						arg( model()->maxValue() ),
					model()->getRoundedValue(),
					model()->minValue(),
					model()->maxValue(), model()->getDigitCount(), &ok );
	}

	if( ok )
	{
		model()->setValue( new_val );
	}
}




void Knob::friendlyUpdate()
{
	if (model() && (model()->controllerConnection() == nullptr ||
		model()->controllerConnection()->getController()->frequentUpdates() == false ||
				Controller::runningFrames() % (256*4) == 0))
	{
		update();
	}
}




QString Knob::displayValue() const
{
	if( isVolumeKnob() &&
		ConfigManager::inst()->value( "app", "displaydbfs" ).toInt() )
	{
		return m_description.trimmed() + QString( " %1 dBFS" ).
				arg( ampToDbfs( model()->getRoundedValue() / volumeRatio() ),
								3, 'f', 2 );
	}
	return m_description.trimmed() + QString( " %1" ).
					arg( model()->getRoundedValue() ) + m_unit;
}




void Knob::doConnections()
{
	if( model() != nullptr )
	{
		QObject::connect( model(), SIGNAL( dataChanged() ),
					this, SLOT( friendlyUpdate() ) );

		QObject::connect( model(), SIGNAL( propertiesChanged() ),
						this, SLOT( update() ) );
	}
}


void Knob::changeEvent(QEvent * ev)
{
	if (ev->type() == QEvent::EnabledChange)
	{
		onKnobNumUpdated();
		if (!m_label.isEmpty())
		{
			setLabel(m_label);
		}
		m_cache = QImage();
		update();
	}
}


void convertPixmapToGrayScale(QPixmap& pixMap)
{
	QImage temp = pixMap.toImage().convertToFormat(QImage::Format_ARGB32);
	for (int i = 0; i < temp.height(); ++i)
	{
		for (int j = 0; j < temp.width(); ++j)
		{
			const auto pix = temp.pixelColor(i, j);
			const auto gscale = 0.2126 * pix.redF() + 0.7152 * pix.greenF() + 0.0722 * pix.blueF();
			const auto pixGray = QColor::fromRgbF(gscale, gscale, gscale, pix.alphaF());
			temp.setPixelColor(i, j, pixGray);
		}
	}
	pixMap.convertFromImage(temp);
}
