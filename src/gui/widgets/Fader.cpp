/*
 * Fader.cpp - fader-widget used in mixer - partly taken from Hydrogen
 *
 * Copyright (c) 2008-2012 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 *
 * http://www.hydrogen-music.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "Fader.h"

#include <QInputDialog>
#include <QMouseEvent>
#include <QPainter>

#include "lmms_math.h"
#include "embed.h"
#include "CaptionMenu.h"
#include "ConfigManager.h"
#include "TextFloat.h"
#include "MainWindow.h"


TextFloat * Fader::s_textFloat = NULL;
QPixmap * Fader::s_back = NULL;
QPixmap * Fader::s_leds = NULL;
QPixmap * Fader::s_knob = NULL;

Fader::Fader( FloatModel * _model, const QString & _name, QWidget * _parent ) :
	QWidget( _parent ),
	FloatModelView( _model, this ),
	m_fPeakValue_L( 0.0 ),
	m_fPeakValue_R( 0.0 ),
	m_persistentPeak_L( 0.0 ),
	m_persistentPeak_R( 0.0 ),
	m_fMinPeak( 0.01f ),
	m_fMaxPeak( 1.1 ),
	m_displayConversion( true ),
	m_levelsDisplayedInDBFS(false),
	m_moveStartPoint( -1 ),
	m_startValue( 0 ),
	m_peakGreen( 0, 0, 0 ),
	m_peakRed( 0, 0, 0 ),
	m_peakYellow( 0, 0, 0 )
{
	if( s_textFloat == NULL )
	{
		s_textFloat = new TextFloat;
	}
	if( ! s_back )
	{
		s_back = new QPixmap( embed::getIconPixmap( "fader_background" ) );
	}
	if( ! s_leds )
	{
		s_leds = new QPixmap( embed::getIconPixmap( "fader_leds" ) );
	}
	if( ! s_knob )
	{
		s_knob = new QPixmap( embed::getIconPixmap( "fader_knob" ) );
	}

	m_back = s_back;
	m_leds = s_leds;
	m_knob = s_knob;

	init(_model, _name);
}


Fader::Fader( FloatModel * model, const QString & name, QWidget * parent, QPixmap * back, QPixmap * leds, QPixmap * knob ) :
	QWidget( parent ),
	FloatModelView( model, this ),
	m_fPeakValue_L( 0.0 ),
	m_fPeakValue_R( 0.0 ),
	m_persistentPeak_L( 0.0 ),
	m_persistentPeak_R( 0.0 ),
	m_fMinPeak( 0.01f ),
	m_fMaxPeak( 1.1 ),
	m_displayConversion( false ),
	m_levelsDisplayedInDBFS(false),
	m_moveStartPoint( -1 ),
	m_startValue( 0 ),
	m_peakGreen( 0, 0, 0 ),
	m_peakRed( 0, 0, 0 )
{
	if( s_textFloat == NULL )
	{
		s_textFloat = new TextFloat;
	}

	m_back = back;
	m_leds = leds;
	m_knob = knob;

	init(model, name);
}

void Fader::init(FloatModel * model, QString const & name)
{
	setWindowTitle( name );
	setAttribute( Qt::WA_OpaquePaintEvent, false );
	QSize backgroundSize = m_back->size();
	setMinimumSize( backgroundSize );
	setMaximumSize( backgroundSize );
	resize( backgroundSize );
	setModel( model );
	setHintText( "Volume:","%");
}



void Fader::contextMenuEvent( QContextMenuEvent * _ev )
{
	CaptionMenu contextMenu( windowTitle() );
	addDefaultActions( &contextMenu );
	contextMenu.exec( QCursor::pos() );
	_ev->accept();
}




void Fader::mouseMoveEvent( QMouseEvent *mouseEvent )
{
	if( m_moveStartPoint >= 0 )
	{
		int dy = m_moveStartPoint - mouseEvent->globalY();

		float delta = dy * ( model()->maxValue() - model()->minValue() ) / (float) ( height() - ( *m_knob ).height() );

		const float step = model()->step<float>();
		float newValue = static_cast<float>( static_cast<int>( ( m_startValue + delta ) / step + 0.5 ) ) * step;
		model()->setValue( newValue );

		updateTextFloat();
	}
}




void Fader::mousePressEvent( QMouseEvent* mouseEvent )
{
	if( mouseEvent->button() == Qt::LeftButton &&
			! ( mouseEvent->modifiers() & Qt::ControlModifier ) )
	{
		AutomatableModel *thisModel = model();
		if( thisModel )
		{
			thisModel->addJournalCheckPoint();
			thisModel->saveJournallingState( false );
		}

		if( mouseEvent->y() >= knobPosY() - ( *m_knob ).height() && mouseEvent->y() < knobPosY() )
		{
			updateTextFloat();
			s_textFloat->show();

			m_moveStartPoint = mouseEvent->globalY();
			m_startValue = model()->value();

			mouseEvent->accept();
		}
		else
		{
			m_moveStartPoint = -1;
		}
	}
	else
	{
		AutomatableModelView::mousePressEvent( mouseEvent );
	}
}



void Fader::mouseDoubleClickEvent( QMouseEvent* mouseEvent )
{
	bool ok;
	float newValue;
	// TODO: dbV handling
	if( m_displayConversion )
	{
		newValue = QInputDialog::getDouble( this, tr( "Set value" ),
					tr( "Please enter a new value between %1 and %2:" ).
							arg( model()->minValue() * 100 ).
							arg( model()->maxValue() * 100 ),
						model()->getRoundedValue() * 100,
						model()->minValue() * 100,
						model()->maxValue() * 100, model()->getDigitCount(), &ok ) * 0.01f;
	}
	else
	{
		newValue = QInputDialog::getDouble( this, tr( "Set value" ),
					tr( "Please enter a new value between %1 and %2:" ).
							arg( model()->minValue() ).
							arg( model()->maxValue() ),
						model()->getRoundedValue(),
						model()->minValue(),
						model()->maxValue(), model()->getDigitCount(), &ok );
	}

	if( ok )
	{
		model()->setValue( newValue );
	}
}



void Fader::mouseReleaseEvent( QMouseEvent * mouseEvent )
{
	if( mouseEvent && mouseEvent->button() == Qt::LeftButton )
	{
		AutomatableModel *thisModel = model();
		if( thisModel )
		{
			thisModel->restoreJournallingState();
		}
	}

	s_textFloat->hide();
}


void Fader::wheelEvent ( QWheelEvent *ev )
{
	ev->accept();

	if ( ev->delta() > 0 )
	{
		model()->incValue( 1 );
	}
	else
	{
		model()->incValue( -1 );
	}
	updateTextFloat();
	s_textFloat->setVisibilityTimeOut( 1000 );
}



///
/// Set peak value (0.0 .. 1.0)
///
void Fader::setPeak( float fPeak, float &targetPeak, float &persistentPeak, QTime &lastPeakTime )
{
	if( fPeak <  m_fMinPeak )
	{
		fPeak = m_fMinPeak;
	}
	else if( fPeak > m_fMaxPeak )
	{
		fPeak = m_fMaxPeak;
	}

	if( targetPeak != fPeak)
	{
		targetPeak = fPeak;
		if( targetPeak >= persistentPeak )
		{
			persistentPeak = targetPeak;
			lastPeakTime.restart();
		}
		update();
	}

	if( persistentPeak > 0 && lastPeakTime.elapsed() > 1500 )
	{
		persistentPeak = qMax<float>( 0, persistentPeak-0.05 );
		update();
	}
}



void Fader::setPeak_L( float fPeak )
{
	setPeak( fPeak, m_fPeakValue_L, m_persistentPeak_L, m_lastPeakTime_L );
}



void Fader::setPeak_R( float fPeak )
{
	setPeak( fPeak, m_fPeakValue_R, m_persistentPeak_R, m_lastPeakTime_R );
}



// update tooltip showing value and adjust position while changing fader value
void Fader::updateTextFloat()
{
	if( ConfigManager::inst()->value( "app", "displaydbfs" ).toInt() && m_displayConversion )
	{
		s_textFloat->setText( QString("Volume: %1 dBFS").
				arg( ampToDbfs( model()->value() ), 3, 'f', 2 ) );
	}
	else
	{
		s_textFloat->setText( m_description + " " + QString("%1 ").arg( m_displayConversion ? model()->value() * 100 : model()->value() ) + " " + m_unit );
	}
	s_textFloat->moveGlobal( this, QPoint( width() - ( *m_knob ).width() - 5, knobPosY() - 46 ) );
}


inline int Fader::calculateDisplayPeak( float fPeak )
{
	int peak = (int)( m_back->height() - ( fPeak / ( m_fMaxPeak - m_fMinPeak ) ) * m_back->height() );

	return qMin( peak, m_back->height() );
}


void Fader::paintEvent( QPaintEvent * ev)
{
	QPainter painter(this);

	// Draw the background
	painter.drawPixmap( ev->rect(), *m_back, ev->rect() );

	// Draw the levels with peaks
	if (getLevelsDisplayedInDBFS())
	{
		paintDBFSLevels(ev, painter);
	}
	else
	{
		paintLinearLevels(ev, painter);
	}

	// Draw the knob
	painter.drawPixmap( 0, knobPosY() - m_knob->height(), *m_knob );
}

void Fader::paintDBFSLevels(QPaintEvent * ev, QPainter & painter)
{
	int height = m_back->height();
	int width = m_back->width() / 2;
	int center = m_back->width() - width;

	float const maxDB(ampToDbfs(m_fMaxPeak));
	float const minDB(ampToDbfs(m_fMinPeak));

	// We will need to divide by the span between min and max several times. It's more
	// efficient to calculate the reciprocal once and then to multiply.
	float const fullSpanReciprocal = 1 / (maxDB - minDB);


	// Draw left levels
	float const leftSpan = ampToDbfs(qMax<float>(0.0001, m_fPeakValue_L)) - minDB;
	int peak_L = height * leftSpan * fullSpanReciprocal;
	QRect drawRectL( 0, height - peak_L, width, peak_L ); // Source and target are identical
	painter.drawPixmap( drawRectL, *m_leds, drawRectL );

	float const persistentLeftPeakDBFS = ampToDbfs(qMax<float>(0.0001, m_persistentPeak_L));
	int persistentPeak_L = height * (1 - (persistentLeftPeakDBFS - minDB) * fullSpanReciprocal);
	// the LED's have a  4px padding and we don't want the peaks
	// to draw on the fader background
	if( persistentPeak_L <= 4 )
	{
		persistentPeak_L = 4;
	}
	if( persistentLeftPeakDBFS > minDB )
	{
		QColor const & peakColor = clips(m_persistentPeak_L) ? peakRed() :
			persistentLeftPeakDBFS >= -6 ? peakYellow() : peakGreen();
		painter.fillRect( QRect( 2, persistentPeak_L, 7, 1 ), peakColor );
	}


	// Draw right levels
	float const rightSpan = ampToDbfs(qMax<float>(0.0001, m_fPeakValue_R)) - minDB;
	int peak_R = height * rightSpan * fullSpanReciprocal;
	QRect const drawRectR( center, height - peak_R, width, peak_R ); // Source and target are identical
	painter.drawPixmap( drawRectR, *m_leds, drawRectR );

	float const persistentRightPeakDBFS = ampToDbfs(qMax<float>(0.0001, m_persistentPeak_R));
	int persistentPeak_R = height * (1 - (persistentRightPeakDBFS - minDB) * fullSpanReciprocal);
	// the LED's have a  4px padding and we don't want the peaks
	// to draw on the fader background
	if( persistentPeak_R <= 4 )
	{
		persistentPeak_R = 4;
	}
	if( persistentRightPeakDBFS > minDB )
	{
		QColor const & peakColor = clips(m_persistentPeak_R) ? peakRed() :
			persistentRightPeakDBFS >= -6 ? peakYellow() : peakGreen();
		painter.fillRect( QRect( 14, persistentPeak_R, 7, 1 ), peakColor );
	}
}

void Fader::paintLinearLevels(QPaintEvent * ev, QPainter & painter)
{
	// peak leds
	//float fRange = abs( m_fMaxPeak ) + abs( m_fMinPeak );

	int height = m_back->height();
	int width = m_back->width() / 2;
	int center = m_back->width() - width;

	int peak_L = calculateDisplayPeak( m_fPeakValue_L - m_fMinPeak );
	int persistentPeak_L = qMax<int>( 3, calculateDisplayPeak( m_persistentPeak_L - m_fMinPeak ) );
	painter.drawPixmap( QRect( 0, peak_L, width, height - peak_L ), *m_leds, QRect( 0, peak_L, width, height - peak_L ) );

	if( m_persistentPeak_L > 0.05 )
	{
		painter.fillRect( QRect( 2, persistentPeak_L, 7, 1 ), ( m_persistentPeak_L < 1.0 )
			? peakGreen()
			: peakRed() );
	}

	int peak_R = calculateDisplayPeak( m_fPeakValue_R - m_fMinPeak );
	int persistentPeak_R = qMax<int>( 3, calculateDisplayPeak( m_persistentPeak_R - m_fMinPeak ) );
	painter.drawPixmap( QRect( center, peak_R, width, height - peak_R ), *m_leds, QRect( center, peak_R, width, height - peak_R ) );

	if( m_persistentPeak_R > 0.05 )
	{
		painter.fillRect( QRect( 14, persistentPeak_R, 7, 1 ), ( m_persistentPeak_R < 1.0 )
			? peakGreen()
			: peakRed() );
	}
}


QColor const & Fader::peakGreen() const
{
	return m_peakGreen;
}

QColor const & Fader::peakRed() const
{
	return m_peakRed;
}

QColor const & Fader::peakYellow() const
{
	return m_peakYellow;
}

void Fader::setPeakGreen( const QColor & c )
{
	m_peakGreen = c;
}

void Fader::setPeakRed( const QColor & c )
{
	m_peakRed = c;
}

void Fader::setPeakYellow( const QColor & c )
{
	m_peakYellow = c;
}
