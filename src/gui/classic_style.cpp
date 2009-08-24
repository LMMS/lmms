/*
 * classic_style.cpp - the graphical style used by LMMS for original look&feel
 *
 * Copyright (c) 2009 Paul Giblock <pgib/at/users.sourceforge.net>
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


#include <QtCore/QFile>
#include <QtGui/QApplication>
#include <QtGui/QFrame>
#include <QtGui/QPainter>
#include <QtGui/QStyleOption>

#include "lmms_style.h"
#include "classic_style.h"
#include "gui_templates.h"
#include "track.h"
#include "gui/tracks/track_container_scene.h"
#include "embed.h"
#include "bb_track.h"
#include "pattern.h"
#include "InstrumentTrack.h"



ClassicStyle::ClassicStyle() :
	QPlastiqueStyle(), LmmsStyle()
{
	QFile file( "resources:style.css" );
	file.open( QIODevice::ReadOnly );
	qApp->setStyleSheet( file.readAll() );

	qApp->setPalette( standardPalette() );

	colors[AutomationBarFill] = QColor( 0xFF, 0xB0, 0x00 );
	colors[AutomationBarValue] = QColor( 0xFF, 0xDF, 0x20 );
	colors[AutomationSelectedBarFill] = QColor( 0x00, 0x40, 0xC0 );
	colors[AutomationCrosshair] = QColor( 0xFF, 0x33, 0x33 );
	colors[PianoRollDefaultNote] = QColor( 0x00, 0xAA, 0x00 ); // 0.3.x: QColor( 0x99, 0xff, 0x00 )
	colors[PianoRollFrozenNote] = QColor( 0x00, 0xE0, 0xFF );
	colors[PianoRollMutedNote] = QColor( 160, 160, 160 );
	colors[PianoRollStepNote] = QColor( 0x00, 0xFF, 0x00 );
	colors[PianoRollSelectedNote] = QColor( 0x00, 0x40, 0xC0 );
	colors[PianoRollEditHandle] = QColor( 0xEA, 0xA1, 0x00 );
	colors[PianoRollSelectedLevel] = QColor( 0x00, 0x40, 0xC0 );
	colors[PianoRollVolumeLevel] = QColor( 0x00, 0xFF, 0x00 );
	colors[PianoRollPanningLevel] = QColor( 0xFF, 0xB0, 0x00 );

	colors[TimelineForecolor] = QColor( 192, 192, 192 );

	colors[StandardGraphLine] = QColor( 96, 255, 128 );
	colors[StandardGraphHandle] = QColor( 0xFF, 0xBF, 0x22 );
	colors[StandardGraphHandleBorder] = QColor( 0x00, 0x00, 0x02 );
	colors[StandardGraphCrosshair] = QColor( 0xAA, 0xFF, 0x00, 0x70 );

	colors[TextFloatForecolor] = QColor( 0, 0, 0 );
	colors[TextFloatFill] = QColor( 224, 224, 224 );

	colors[VisualizationLevelLow] = QColor( 128, 224, 128 );
	colors[VisualizationLevelMid] = QColor( 255, 192, 64 );
	colors[VisualizationLevelPeak] = QColor( 255, 64, 64 );
}



QColor ClassicStyle::color( LmmsStyle::ColorRole _role ) const
{
	return colors[_role];
}



QPalette ClassicStyle::standardPalette() const
{
	QPalette pal = QPlastiqueStyle::standardPalette();
	pal.setColor( QPalette::Background, QColor( 72, 76, 88 ) );
	pal.setColor( QPalette::WindowText, QColor( 240, 240, 240 ) );
	pal.setColor( QPalette::Base, QColor( 128, 128, 128 ) );
	pal.setColor( QPalette::Text, QColor( 224, 224, 224 ) );
	pal.setColor( QPalette::Button, QColor( 172, 176, 188 ) );
	pal.setColor( QPalette::Shadow, QColor( 0, 0, 0 ) );
	pal.setColor( QPalette::ButtonText, QColor( 255, 255, 255 ) );
	pal.setColor( QPalette::BrightText, QColor( 0, 255, 0 ) );
	pal.setColor( QPalette::Highlight, QColor( 224, 224, 224 ) );
	pal.setColor( QPalette::HighlightedText, QColor( 0, 0, 0 ) );
	return( pal );
}




void ClassicStyle::drawComplexControl( ComplexControl control,
				const QStyleOptionComplex * option,
				QPainter *painter,
				const QWidget *widget ) const
{
	// fix broken titlebar styling on win32
	if( control == CC_TitleBar )
	{
		const QStyleOptionTitleBar * titleBar =
			qstyleoption_cast<const QStyleOptionTitleBar *>(option );
		if( titleBar )
		{
			QStyleOptionTitleBar so( *titleBar );
			so.palette = standardPalette();
			so.palette.setColor( QPalette::HighlightedText,
				( titleBar->titleBarState & State_Active ) ?
					QColor( 255, 255, 255 ) :
						QColor( 192, 192, 192 ) );
			so.palette.setColor( QPalette::Text,
							QColor( 64, 64, 64 ) );
			QPlastiqueStyle::drawComplexControl( control, &so,
							painter, widget );
			return;
		}
	}
	QPlastiqueStyle::drawComplexControl( control, option, painter, widget );
}




void ClassicStyle::drawPrimitive( PrimitiveElement element,
				const QStyleOption *option,
				QPainter *painter,
				const QWidget *widget ) const
{
	if( element == QStyle::PE_Frame ||
			element == QStyle::PE_FrameLineEdit ||
			element == QStyle::PE_PanelLineEdit )
	{
		const QRect rect = option->rect;

		QColor black = QColor( 0, 0, 0 );
		QColor shadow = option->palette.shadow().color();
		QColor highlight = option->palette.highlight().color();

		int a100 = 165;
		int a75 = static_cast<int>( a100 * .75 );
		int a50 = static_cast<int>( a100 * .6 );
		int a25 = static_cast<int>( a100 * .33 );

		QLine lines[4];
		QPoint points[4];

		// black inside lines
		// 50%
		black.setAlpha( a100 );
		painter->setPen( QPen( black, 0 ) );
		lines[0] = QLine( rect.left() + 2, rect.top() + 1,
					rect.right() - 2, rect.top() + 1 );
		lines[1] = QLine( rect.left() + 2, rect.bottom() - 1,
					rect.right() - 2, rect.bottom() - 1 );
		lines[2] = QLine( rect.left() + 1, rect.top() + 2,
					rect.left() + 1, rect.bottom() - 2 );
		lines[3] = QLine( rect.right() - 1, rect.top() + 2,
					rect.right() - 1, rect.bottom() - 2 );
		painter->drawLines( lines, 4 );

		// black inside dots
		black.setAlpha( a50 );
		painter->setPen( QPen( black, 0 ) );
		points[0] = QPoint( rect.left() + 2, rect.top() + 2 );
		points[1] = QPoint( rect.left() + 2, rect.bottom() - 2 );
		points[2] = QPoint( rect.right() - 2, rect.top() + 2 );
		points[3] = QPoint( rect.right() - 2, rect.bottom() - 2 );
		painter->drawPoints( points, 4 );


		// outside lines - shadow
		// 100%
		shadow.setAlpha( a75 );
		painter->setPen( QPen( shadow, 0 ) );
		lines[0] = QLine( rect.left() + 2, rect.top(),
						rect.right() - 2, rect.top() );
		lines[1] = QLine( rect.left(), rect.top() + 2,
						rect.left(), rect.bottom() - 2 );
		painter->drawLines( lines, 2 );

		// outside corner dots - shadow
		// 75%
		shadow.setAlpha( a50 );
		painter->setPen( QPen( shadow, 0 ) );
		points[0] = QPoint( rect.left() + 1, rect.top() + 1 );
		points[1] = QPoint( rect.right() - 1, rect.top() + 1 );
		painter->drawPoints( points, 2 );

		// outside end dots - shadow
		// 50%
		shadow.setAlpha( a25 );
		painter->setPen( QPen( shadow, 0 ) );
		points[0] = QPoint( rect.left() + 1, rect.top() );
		points[1] = QPoint( rect.left(), rect.top() + 1 );
		points[2] = QPoint( rect.right() - 1, rect.top() );
		points[3] = QPoint( rect.left(), rect.bottom() - 1 );
		painter->drawPoints( points, 4 );


		// outside lines - highlight
		// 100%
		highlight.setAlpha( a75 );
		painter->setPen( QPen( highlight, 0 ) );
		lines[0] = QLine( rect.left() + 2, rect.bottom(),
					rect.right() - 2, rect.bottom() );
		lines[1] = QLine( rect.right(), rect.top() + 2,
					rect.right(), rect.bottom() - 2 );
		painter->drawLines( lines, 2 );

		// outside corner dots - highlight
		// 75%
		highlight.setAlpha( a50 );
		painter->setPen( QPen(highlight, 0 ) );
		points[0] = QPoint( rect.left() + 1, rect.bottom() - 1 );
		points[1] = QPoint( rect.right() - 1, rect.bottom() - 1 );
		painter->drawPoints( points, 2 );

		// outside end dots - highlight
		// 50%
		highlight.setAlpha( a25 );
		painter->setPen( QPen(highlight, 0 ) );
		points[0] = QPoint( rect.right() - 1, rect.bottom() );
		points[1] = QPoint( rect.right(), rect.bottom() - 1 );
		points[2] = QPoint( rect.left() + 1, rect.bottom() );
		points[3] = QPoint( rect.right(), rect.top() + 1 );
		painter->drawPoints( points, 4 );
	}
	else
	{
		QPlastiqueStyle::drawPrimitive( element, option, painter, widget );
	}

}


int ClassicStyle::pixelMetric( PixelMetric _metric,
				const QStyleOption * _option,
				const QWidget * _widget ) const
{
	switch( _metric )
	{
		case QStyle::PM_ButtonMargin:
			return 3;

		case QStyle::PM_ButtonIconSize:
			return 20;

		case QStyle::PM_ToolBarItemMargin:
			return 1;

		case QStyle::PM_ToolBarItemSpacing:
			return 2;

		case QStyle::PM_TitleBarHeight:
			return 24;

		default:
			return QPlastiqueStyle::pixelMetric(
					_metric, _option, _widget );
	}
}


void ClassicStyle::drawFxLine( QPainter * _painter, const QWidget *_fxLine,
				const QString & _name, bool _active )
{
	int width = _fxLine->rect().width();
	int height = _fxLine->rect().height();

	QPainter * p = _painter;
	p->fillRect( _fxLine->rect(), QColor( 72, 76, 88 ) );
	p->setPen( QColor( 40, 42, 48 ) );
	p->drawRect( 0, 0, width-2, height-2 );
	p->setPen( QColor( 108, 114, 132 ) );
	p->drawRect( 1, 1, width-2, height-2 );
	p->setPen( QColor( 20, 24, 32 ) );
	p->drawRect( 0, 0, width-1, height-1 );

	p->rotate( -90 );
	p->setPen( _active ? QColor( 0, 255, 0 ) : Qt::white );
	p->setFont( pointSizeF( _fxLine->font(), 7.5f ) );
	p->drawText( -90, 20, _name );
}

void ClassicStyle::drawTrackContentBackground(QPainter * _painter,
        const QSize & _size, const int _pixelsPerTact)
{
    const int w = _size.width();
    const int h = _size.height();

    QLinearGradient grad( 0, 1, 0, h-2 );
    _painter->fillRect( 0, 0, 1, h, QColor( 96, 96, 96 ) );
    _painter->fillRect( 1, 0, w+1, h, QColor( 128, 128, 128 ) );
    grad.setColorAt( 0.0, QColor( 64, 64, 64 ) );
    grad.setColorAt( 0.3, QColor( 128, 128, 128 ) );
    grad.setColorAt( 0.5, QColor( 128, 128, 128 ) );
    grad.setColorAt( 0.95, QColor( 160, 160, 160 ) );
    _painter->fillRect( 0, 1, w, h-2, grad );

    QLinearGradient grad2( 0,1, 0, h-2 );
    _painter->fillRect( w+1, 0, w , h, QColor( 96, 96, 96 ) );
    grad2.setColorAt( 0.0, QColor( 48, 48, 48 ) );
    grad2.setColorAt( 0.3, QColor( 96, 96, 96 ) );
    grad2.setColorAt( 0.5, QColor( 96, 96, 96 ) );
    grad2.setColorAt( 0.95, QColor( 120, 120, 120 ) );
    _painter->fillRect( w, 1, w , h-2, grad2 );

    // draw vertical lines
    _painter->setPen( QPen( QColor( 0, 0, 0, 112 ), 1 ) );
    for( float x = 0.5; x < w * 2; x += _pixelsPerTact )
    {
        _painter->drawLine( QLineF( x, 1.0, x, h-2.0 ) );
    }
    _painter->drawLine( 0, 1, w*2, 1 );

    _painter->setPen( QPen( QColor( 255, 255, 255, 32 ), 1 ) );
    for( float x = 1.5; x < w * 2; x += _pixelsPerTact )
    {
        _painter->drawLine( QLineF( x, 1.0, x, h-2.0 ) );
    }
    _painter->drawLine( 0, h-2, w*2, h-2 );
}



void ClassicStyle::drawTrackContentObject( QPainter * _painter,
		const trackContentObject * _model, const LmmsStyleOptionTCO * _option )
{
	QRectF rc = _option->rect.adjusted( 0, 2, 0, -2 );
	_painter->setRenderHint( QPainter::Antialiasing, false );

	if( const bbTCO * bbTco = dynamic_cast<const bbTCO *>( _model ) )
	{
		QColor col( bbTco->color() );

		if( _model->getTrack()->isMuted() || _model->isMuted() )
		{
			col = QColor( 160, 160, 160 );
		}
		if( _option->selected )
		{
			col = QColor( qMax( col.red() - 128, 0 ),
						qMax( col.green() - 128, 0 ), 255 );
		}
		if( _option->hovered )
		{
			col = col.light(130);
		}

		QLinearGradient lingrad( 0, 0, 0, rc.height() );
		lingrad.setColorAt( 0, col.light( 130 ) );
		lingrad.setColorAt( 1, col.light( 70 ) );
		_painter->fillRect( rc, lingrad );

		const float cellW = TrackContainerScene::DEFAULT_CELL_WIDTH;
		const tick_t t = _option->duration;
		if( _model->length() > midiTime::ticksPerTact() && t > 0 )
		{
			for( int x = t * cellW; x < rc.width()-2; x += t * cellW )
			{
				_painter->setPen( col.light( 80 ) );
				_painter->drawLine( x, 3, x, 7 );
				_painter->setPen( col.light( 120 ) );
				_painter->drawLine( x, rc.height() - 4,
						x, rc.height() - 0 );
			}
		}

		_painter->setPen( col.dark() );
		_painter->drawRect( rc );

		_painter->setFont( pointSize<7>( _painter->font() ) );
		_painter->setPen( QColor( 0, 0, 0 ) );
		_painter->setClipRect( rc );
		_painter->drawText( QPointF( rc.left(), _painter->fontMetrics().height() + rc.top() ), _model->name() );
		_painter->setClipping( false );

		if( _model->isMuted() )
		{
			_painter->drawPixmap( 3, _painter->fontMetrics().height() + 1,
					embed::getIconPixmap( "muted", 16, 16 ) );
		}
	}
	

	else
	{
		QPainter * p = _painter;
		QLinearGradient lingrad( 0, 0, 0, rc.height() );
		QColor c = _option->selected ? 
				QColor( 0, 0, 224 ) :
				QColor( 96, 96, 96 );
		if( _option->hovered )
		{
			c = c.light(120);
		}
		lingrad.setColorAt( 0, c );
		lingrad.setColorAt( 0.5, Qt::black );
		lingrad.setColorAt( 1, c );
		p->setBrush( lingrad );
		p->setPen( QColor( 0, 0, 0 ) );
		//p.drawRect( 0, 0, width() - 1, height() - 1 );
		p->drawRect( rc );

		const float TCO_BORDER_WIDTH = 1.0f;
		float ppt = TrackContainerScene::DEFAULT_CELL_WIDTH;

		if( const pattern * pat = dynamic_cast<const pattern *>( _model ) )
		{
			ppt = rc.width() / (float) pat->length().getTact();

			const float x_base = TCO_BORDER_WIDTH;
			p->setPen( QColor( 0, 0, 0 ) );

			for( tact_t t = 1; t < pat->length().getTact(); ++t )
			{
				float x = x_base + static_cast<int>( ppt * t ) - 1;
				p->drawLine( x, rc.top() + 0.5, x, rc.top() + 4 );
				p->drawLine( x,	rc.bottom() - 4, x, rc.bottom() - 0.5 );
			}

			p->setClipRect( rc.adjusted( 1, 1, -1, -1 ) );
			if( pat->type() == pattern::MelodyPattern )
			{
				int central_key = 0;
				if( pat->notes().size() > 0 )
				{
					// first determine the central tone so that we can
					// display the area where most of the notes are
					int total_notes = 0;
					for( NoteVector::ConstIterator it = pat->notes().begin();
							it != pat->notes().end(); ++it )
					{
						if( ( *it )->length() > 0 )
						{
							central_key += ( *it )->key();
							++total_notes;
						}
					}

					if( total_notes > 0 )
					{
						central_key = central_key / total_notes;

						const float central_y = rc.height() / 2.0f;
						float y_base = central_y + TCO_BORDER_WIDTH - 1.0f;

						if( pat->getTrack()->isMuted() ||
									pat->isMuted() )
						{
							p->setPen( color( LmmsStyle::PianoRollMutedNote ) );
						}
						else if( pat->frozen() )
						{
							p->setPen( color( LmmsStyle::PianoRollFrozenNote ) );
						}
						else
						{
							p->setPen( color( LmmsStyle::PianoRollDefaultNote ) );
						}

						for( NoteVector::ConstIterator it =
									pat->notes().begin();
							it != pat->notes().end(); ++it )
						{
							const float y_pos = central_key -
										( *it )->key();

							if( ( *it )->length() > 0 &&
									y_pos > -central_y &&
									y_pos < central_y )
							{
								const float x1 =
										( *it )->pos() * ppt / 
										midiTime::ticksPerTact();
								const float x2 =
										( ( *it )->pos() + ( *it )->length() ) * 
										ppt / midiTime::ticksPerTact();

								p->drawLine( x1, y_base + y_pos,	
										x2-1, y_base + y_pos );
							}
						}

					}
				}
			}
			/*
			else if( pat->type() == pattern::BeatPattern &&
					( fixedTCOs() || 
				  	  ppt >= 96 || 
				  	  pat->m_steps != midiTime::stepsPerTact() ) )
			{
				QPixmap stepon;
				QPixmap stepoverlay;
				QPixmap stepoff;
				QPixmap stepoffl;
				const int steps = m_pat->length() / DefaultBeatsPerTact;
				const int w = width() - 2 * TCO_BORDER_WIDTH;
				stepon = s_stepBtnOn->scaled( w / steps,
					      	  	  s_stepBtnOn->height(),
					      	  	  Qt::IgnoreAspectRatio,
					      	  	  Qt::SmoothTransformation );
				stepoverlay = s_stepBtnOverlay->scaled( w / steps,
					      	  	  s_stepBtnOn->height(),
					      	  	  Qt::IgnoreAspectRatio,
					      	  	  Qt::SmoothTransformation );
				stepoff = s_stepBtnOff->scaled( w / steps,
								s_stepBtnOff->height(),
								Qt::IgnoreAspectRatio,
								Qt::SmoothTransformation );
				stepoffl = s_stepBtnOffLight->scaled( w / steps,
								s_stepBtnOffLight->height(),
								Qt::IgnoreAspectRatio,
								Qt::SmoothTransformation );
				for( NoteVector::Iterator it = m_pat->m_notes.begin();
							it != m_pat->m_notes.end(); ++it )
				{
					const int no = ( *it )->pos() / DefaultBeatsPerTact;
					const int x = TCO_BORDER_WIDTH + static_cast<int>( no *
										w / steps );
					const int y = height() - s_stepBtnOff->height() - 1;

					const int vol = ( *it )->getVolume();

					if( ( *it )->length() < 0 )
					{
						p.drawPixmap( x, y, stepoff );
						for( int i = 0; i < vol / 5 + 1; ++i )
						{
							p.drawPixmap( x, y, stepon );
						}
						for( int i = 0; i < ( 25 + ( vol - 75 ) ) / 5;
											++i )
						{
							p.drawPixmap( x, y, stepoverlay );
						}
					}
					else if( ( no / 4 ) % 2 )
					{
						p.drawPixmap( x, y, stepoff );
					}
					else
					{
						p.drawPixmap( x, y, stepoffl );
					}
				}
			}
			END BEAT TRACK */

			p->setFont( pointSize<7>( p->font() ) );
			if( pat->isMuted() || pat->getTrack()->isMuted() )
			{
				p->setPen( QColor( 192, 192, 192 ) );
			}
			else
			{
				p->setPen( QColor( 32, 240, 32 ) );
			}

			if( pat->name() != pat->instrumentTrack()->name() )
			{
				p->drawText( 2, p->fontMetrics().height() - 1, pat->name() );
			}

			if( pat->isMuted() )
			{
				p->drawPixmap( 3, p->fontMetrics().height() + 1,
						embed::getIconPixmap( "muted", 16, 16 ) );
			}
			else if( pat->frozen() )
			{
				p->setBrush( QBrush() );
				p->setPen( QColor( 0, 224, 255 ) );
				p->drawRect( rc );
				/* TODO:
				p->drawPixmap( rc.left() + 3, 
						rc.top() + height() - s_frozen->height() - 4, 
						*s_frozen );
				*/
			}

			p->setClipping( false );
		}
	}
	_painter->setRenderHint( QPainter::Antialiasing, true );
}

