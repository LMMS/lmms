/*
 * TimeLineWidget.cpp - class timeLine, representing a time-line with position marker
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


#include <QDomElement>
#include <QTimer>
#include <QApplication>
#include <QLayout>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QToolBar>
#include <QToolButton>


#include "TimeLineWidget.h"
#include "embed.h"
#include "NStateButton.h"
#include "GuiApplication.h"
#include "TextFloat.h"
#include "SongEditor.h"


#if QT_VERSION < 0x040800
#define MiddleButton MidButton
#endif


QPixmap * TimeLineWidget::s_posMarkerPixmap = NULL;

TimeLineWidget::TimeLineWidget( const int xoff, const int yoff, const float ppt,
			Song::PlayPos & pos, const MidiTime & begin,
							QWidget * parent ) :
	QWidget( parent ),
	m_inactiveLoopColor(        52,  83,  53,  64 ),
	m_inactiveLoopBrush(QColor(255, 255, 255,  32 )),
	m_inactiveLoopInnerColor(  255, 255, 255,  32 ),
	m_inactiveLoopTextColor(     0,   0,   0, 255 ),
	m_activeLoopColor(          83,  52,  53, 255 ),
	m_activeLoopBrush(QColor(  141,  55,  96, 128 )),
	m_activeLoopInnerColor(    155,  74, 100, 255 ),
	m_activeLoopTextColor(     255,   0,   0, 255 ),
	m_selectedLoopColor(        52,  83,  53, 255 ),
	m_selectedLoopBrush(QColor( 55, 141,  96, 128 )),
	m_selectedLoopInnerColor(   74, 155, 100, 255 ),
	m_selectedLoopTextColor(     0, 255,   0, 255 ),
	m_loopRectangleVerticalPadding( 1 ),
	m_barLineColor( 192, 192, 192 ),
	m_barNumberColor( m_barLineColor.darker( 120 ) ),
	m_autoScroll( AutoScrollEnabled ),
	m_loopPoints( LoopPointsDisabled ),
	m_behaviourAtStop( BackToZero ),
	m_changedPosition( true ),
	m_xOffset( xoff ),
	m_posMarkerX( 0 ),
	m_ppt( ppt ),
	m_pos( pos ),
	m_begin( begin ),
	m_savedPos( -1 ),
	m_currentLoop( 0 ),
	m_loopButton( NULL ),
	m_hint( NULL ),
	m_action( NoAction ),
	m_moveXOff( 0 )
{
	//m_loopPos[0] = 0;
	//m_loopPos[1] = DefaultTicksPerTact;
	for(int i=0;i<2*NB_LOOPS;i++)
		m_loopPos[i]=((i/2)*NB_LOOPS+i%2)*DefaultTicksPerTact;
	for(int i=0;i<2*NB_LOOPS;i++)
		m_loopPos[i]=((i/2)*NB_LOOPS+i%2)*DefaultTicksPerTact;

	if( s_posMarkerPixmap == NULL )
	{
		s_posMarkerPixmap = new QPixmap( embed::getIconPixmap(
							"playpos_marker" ) );
	}

	setAttribute( Qt::WA_OpaquePaintEvent, true );
	move( 0, yoff );

	m_xOffset -= s_posMarkerPixmap->width() / 2;

	m_pos.m_timeLine = this;

	QTimer * updateTimer = new QTimer( this );
	connect( updateTimer, SIGNAL( timeout() ),
					this, SLOT( updatePosition() ) );
	updateTimer->start( 50 );
	connect( Engine::getSong(), SIGNAL( timeSignatureChanged( int,int ) ),
					this, SLOT( update() ) );
}




TimeLineWidget::~TimeLineWidget()
{
	if( gui->songEditor() )
	{
		m_pos.m_timeLine = NULL;
	}
	delete m_hint;
}




void TimeLineWidget::addToolButtons( QToolBar * _tool_bar )
{
	NStateButton * autoScroll = new NStateButton( _tool_bar );
	autoScroll->setGeneralToolTip( tr( "Enable/disable auto-scrolling" ) );
	autoScroll->addState( embed::getIconPixmap( "autoscroll_on" ) );
	autoScroll->addState( embed::getIconPixmap( "autoscroll_off" ) );
	connect( autoScroll, SIGNAL( changedState( int ) ), this,
					SLOT( toggleAutoScroll( int ) ) );

	NStateButton * loopPoints = new NStateButton( _tool_bar );
	loopPoints->setGeneralToolTip( tr( "Enable/disable loop-points" ) );
	loopPoints->addState( embed::getIconPixmap( "loop_points_off" ) );
	loopPoints->addState( embed::getIconPixmap( "loop_points_on" ) );
	connect( loopPoints, SIGNAL( changedState( int ) ), this,
					SLOT( toggleLoopPoints( int ) ) );
	connect( this, SIGNAL( loopPointStateLoaded( int ) ), loopPoints,
					SLOT( changeState( int ) ) );

	NStateButton * behaviourAtStop = new NStateButton( _tool_bar );
	behaviourAtStop->addState( embed::getIconPixmap( "back_to_zero" ),
					tr( "After stopping go back to begin" )
									);
	behaviourAtStop->addState( embed::getIconPixmap( "back_to_start" ),
					tr( "After stopping go back to "
						"position at which playing was "
						"started" ) );
	behaviourAtStop->addState( embed::getIconPixmap( "keep_stop_position" ),
					tr( "After stopping keep position" ) );
	connect( behaviourAtStop, SIGNAL( changedState( int ) ), this,
					SLOT( toggleBehaviourAtStop( int ) ) );

	_tool_bar->addWidget( autoScroll );
	_tool_bar->addWidget( loopPoints );
	_tool_bar->addWidget( behaviourAtStop );

	if(m_loopButton == NULL)
	{
		int const n=currentLoop();
		m_loopButton=new QToolButton(_tool_bar);
		QToolButton * b=m_loopButton;
		b->setPopupMode(QToolButton::InstantPopup);
		QMenu * m=new QMenu(QString("Loops"),b);
		QActionGroup * g=new QActionGroup(m);
		for(int i=0;i<NB_LOOPS;i++)
		{
			QAction* a=m->addAction(QString((char)(65+i)).append(QString(" loop")));
			a->setData( QVariant(i) );
			a->setCheckable(true);
			a->setShortcut((char)(65+i));
			a->setActionGroup(g);
			if(i==n) a->setChecked(true);
		}
		b->setMenu(m);
		b->setText(QString("&").append(QString((char)(65+n))));
		//b->setMinimumSize(autoScroll->sizeHint());
		connect(g, SIGNAL(triggered(QAction*)), this, SLOT(selectLoop(QAction*)));
		_tool_bar->addWidget(b);
	}
}




void TimeLineWidget::setCurrentLoop(const int n)
{
	if((n<0) || (n>=NB_LOOPS)) return;
	m_currentLoop=n;
	updatePosition(loopBegin());
	update();
	if(m_loopButton != NULL)
	{
		m_loopButton->setText(QString("&").append(QString((char)(65+n))));
		m_loopButton->update();
		m_loopButton->menu()->actions().at(n)->setChecked(true);
	}
}

int TimeLineWidget::findLoop(const MidiTime& t)
{
	int n=-1;
	for(int i=0;i<NB_LOOPS;i++)
		if((t>=m_loopPos[2*i+0])&&
		   ((t<m_loopPos[2*i+1])||
		    ((n==-1)&&(t==m_loopPos[2*i+1]))))
		{
			if(i==m_currentLoop) return i;
			n=i;
		}
	return n;
}

void TimeLineWidget::selectLoop(QAction * _a)
{
	int const n=_a->data().toInt();
	if((n<0) || (n>=NB_LOOPS)) return;
	setCurrentLoop(n);
}

void TimeLineWidget::selectLoop(const MidiTime& t)
{
	int n=findLoop(t);
	if((n<0) || (n>=NB_LOOPS)) return;
	if(n!=currentLoop()) setCurrentLoop(n);
}




void TimeLineWidget::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "lp0pos", (int) loopBegin() );
	_this.setAttribute( "lp1pos", (int) loopEnd() );
	_this.setAttribute( "lpstate", m_loopPoints );
}




void TimeLineWidget::loadSettings( const QDomElement & _this )
{
	m_loopPos[0] = _this.attribute( "lp0pos" ).toInt();
	m_loopPos[1] = _this.attribute( "lp1pos" ).toInt();
	m_loopPoints = static_cast<LoopPointStates>(
					_this.attribute( "lpstate" ).toInt() );
	update();
	emit loopPointStateLoaded( m_loopPoints );
}




void TimeLineWidget::updatePosition( const MidiTime & )
{
	const int new_x = markerX( m_pos );

	if( new_x != m_posMarkerX )
	{
		m_posMarkerX = new_x;
		m_changedPosition = true;
		emit positionChanged( m_pos );
		update();
	}
}




void TimeLineWidget::toggleAutoScroll( int _n )
{
	m_autoScroll = static_cast<AutoScrollStates>( _n );
}




void TimeLineWidget::toggleLoopPoints( int _n )
{
	m_loopPoints = static_cast<LoopPointStates>( _n );
	update();
}




void TimeLineWidget::toggleBehaviourAtStop( int _n )
{
	m_behaviourAtStop = static_cast<BehaviourAtStopStates>( _n );
}




void TimeLineWidget::paintEvent( QPaintEvent * )
{
	QPainter p( this );

	const int n  = m_currentLoop;
	const int x0 = m_xOffset + s_posMarkerPixmap->width() / 2;
	const int h0 = height();
	const int w0 = width();
	const QBrush &  bg = p.background();
	QBrush    bglight = bg;
	bglight.setColor(parentWidget()->palette().color(QPalette::Background));
	// Draw background
	p.fillRect( 0, 0, x0, h0, bglight );
	p.fillRect( w0-12, 0, 12, h0, bglight );
	p.setClipRect( x0, 0, w0 - x0 - 12, h0 );//12=sb width TODO!
	p.fillRect( x0, 0, w0-12, h0, bg );

	// Activate hinting on the font
	QFont font = p.font();
	font.setHintingPreference( QFont::PreferFullHinting );
	p.setFont(font);
	int const fontAscent = p.fontMetrics().ascent();
	int const fontHeight = p.fontMetrics().height();

	int const barBaseY = height()-2-fontHeight+fontAscent;
	int const loopBaseY= 2+fontAscent;

	for(int i=0;i<NB_LOOPS;i++) if(i!=n) paintLoop(i,p,loopBaseY);
	paintLoop(n,p,loopBaseY);

	// Draw the bar lines and numbers
	QColor const & barLineColor = getBarLineColor();
	QColor const & barNumberColor = getBarNumberColor();

	tact_t barNumber = m_begin.getTact();
	int const x = m_xOffset + s_posMarkerPixmap->width() / 2 -
			( ( static_cast<int>( m_begin * m_ppt ) / MidiTime::ticksPerTact() ) % static_cast<int>( m_ppt ) );

	/*
	int bnmod=qMax( 1, qRound( 1.0f / 3.0f *
				   MidiTime::ticksPerTact() / m_ppt ) );
	*/
	int bnmod=4;// most common case
	while((m_ppt>0)&&(bnmod>0)&&(bnmod%2==0)&&(bnmod*m_ppt>=128)) bnmod/=2;
	while((m_ppt>0)&&(bnmod>0)&&(bnmod*m_ppt<48)) bnmod*=2;
	for( int i = 0; x + i * m_ppt < width(); ++i )
	{
		++barNumber;
		if( ( barNumber - 1 ) % bnmod == 0 )
		{
			const int cx = x + qRound( i * m_ppt );
			p.setPen( barLineColor );
			p.drawLine( cx, 5, cx, height() - 6 );

			const QString s = QString::number( barNumber-1 );
			int const tw=p.fontMetrics().width(s);
			p.setPen( barNumberColor );
			p.drawText( cx-tw-2 /*13*/, barBaseY, s );
		}
	}

	// Draw the position marker
	p.setOpacity( 0.6 );
	p.drawPixmap( m_posMarkerX, height() - s_posMarkerPixmap->height(), *s_posMarkerPixmap );
}

void TimeLineWidget::paintLoop(int const n, QPainter& p, int const cy)
{
	// Draw the loop rectangle
	int const & loopRectMargin = getLoopRectangleVerticalPadding();
	int const loopRectHeight = this->height() - 2 * loopRectMargin;
	int const loopStart = markerX( loopBegin(n) ) + 8;
	int const loopEndR = markerX( loopEnd(n) ) + 9;
	int const loopRectWidth = loopEndR - loopStart;

	bool const loopPointsActive = loopPointsEnabled();
	bool const loopSelected = (n == m_currentLoop);

	// Draw the main rectangle (inner fill only)
	QRect outerRectangle( loopStart, loopRectMargin, loopRectWidth - 1, loopRectHeight - 1 );
	p.fillRect( outerRectangle, loopPointsActive
		    ? (loopSelected ? getSelectedLoopBrush() : getActiveLoopBrush())
		    : getInactiveLoopBrush());
	// Draw the main rectangle (outer border)
	p.setPen( loopPointsActive
		  ? (loopSelected ? getSelectedLoopColor() : getActiveLoopColor())
		  : getInactiveLoopColor() );
	p.setBrush( Qt::NoBrush );
	p.drawRect( outerRectangle );

	// Draw the inner border outline (no fill)
	QRect innerRectangle = outerRectangle.adjusted( 1, 1, -1, -1 );
	p.setPen( loopPointsActive
		  ? (loopSelected ? getSelectedLoopInnerColor() : getActiveLoopInnerColor())
		  : getInactiveLoopInnerColor() );
	p.setBrush( Qt::NoBrush );
	p.drawRect( innerRectangle );

	p.setPen( loopPointsActive
		  ? (loopSelected ? getSelectedLoopTextColor() : getActiveLoopTextColor())
		  : getInactiveLoopTextColor() );
	p.setBrush( Qt::NoBrush );
	p.drawText( innerRectangle.x() + 2, cy, QString((char)(65+n)) );
}




void TimeLineWidget::mousePressEvent( QMouseEvent* event )
{
	if( event->x() < m_xOffset )
	{
		return;
	}
	if( event->button() == Qt::LeftButton  && !(event->modifiers() & Qt::ShiftModifier) )
	{
		m_action = MovePositionMarker;
		if( event->x() - m_xOffset < s_posMarkerPixmap->width() )
		{
			m_moveXOff = event->x() - m_xOffset;
		}
		else
		{
			m_moveXOff = s_posMarkerPixmap->width() / 2;
		}
	}
	else if( event->button() == Qt::LeftButton  && (event->modifiers() & Qt::ShiftModifier) )
	{
		m_action = SelectSongTCO;
		m_initalXSelect = event->x();
	}
	else if( event->button() == Qt::RightButton || event->button() == Qt::MiddleButton )
	{
        	m_moveXOff = s_posMarkerPixmap->width() / 2;
		const MidiTime t = m_begin + static_cast<int>( event->x() * MidiTime::ticksPerTact() / m_ppt );
		const int n=m_currentLoop;

		if( m_loopPos[2*n+0] > m_loopPos[2*n+1]  )
		{
			qSwap( m_loopPos[2*n+0], m_loopPos[2*n+1] );
		}
		if( ( event->modifiers() & Qt::ShiftModifier ) || event->button() == Qt::MiddleButton )
		{
			m_action = MoveLoopBegin;
		}
		else
		{
			m_action = MoveLoopEnd;
		}
		m_loopPos[( m_action == MoveLoopBegin ) ? 2*n+0 : 2*n+1] = t;
	}

	if( m_action == MoveLoopBegin )
	{
		delete m_hint;
		m_hint = TextFloat::displayMessage( tr( "Hint" ),
					tr( "Press <%1> to disable magnetic loop points." ).arg(
						#ifdef LMMS_BUILD_APPLE
						"⌘"),
						#else
						"Ctrl"),
						#endif
					embed::getIconPixmap( "hint" ), 0 );
	}
	else if( m_action == MoveLoopEnd )
	{
		delete m_hint;
		m_hint = TextFloat::displayMessage( tr( "Hint" ),
					tr( "Hold <Shift> to move the begin loop point; Press <%1> to disable magnetic loop points." ).arg(
						#ifdef LMMS_BUILD_APPLE
						"⌘"),
						#else
						"Ctrl"),
						#endif
					embed::getIconPixmap( "hint" ), 0 );
	}

	mouseMoveEvent( event );
}




void TimeLineWidget::mouseMoveEvent( QMouseEvent* event )
{
	const MidiTime t = m_begin + static_cast<int>( qMax( event->x() - m_xOffset - m_moveXOff, 0 ) * MidiTime::ticksPerTact() / m_ppt );
	const int n=m_currentLoop;

	switch( m_action )
	{
		case MovePositionMarker:
			m_pos.setTicks( t.getTicks() );
			Engine::getSong()->setToTime(t);
			m_pos.setCurrentFrame( 0 );
			updatePosition();
			positionMarkerMoved();
			selectLoop(t);
			break;

		case MoveLoopBegin:
		case MoveLoopEnd:
		{
			const int i = m_action - MoveLoopBegin;
			if( event->modifiers() & Qt::ControlModifier )
			{
				// no ctrl-press-hint when having ctrl pressed
				delete m_hint;
				m_hint = NULL;
				m_loopPos[2*n+i] = t;
			}
			else
			{
				m_loopPos[2*n+i] = t.toNearestTact();
			}
			// Catch begin == end
			if( m_loopPos[2*n+0] == m_loopPos[2*n+1] )
			{
				// Note, swap 1 and 0 below and the behavior "skips" the other
				// marking instead of pushing it.
				if( m_action == MoveLoopBegin ) 
					m_loopPos[2*n+0] -= MidiTime::ticksPerTact();
				else
					m_loopPos[2*n+1] += MidiTime::ticksPerTact();
			}
			update();
			break;
		}
	case SelectSongTCO:
			emit regionSelectedFromPixels( m_initalXSelect , event->x() );
		break;

		default:
			break;
	}
}




void TimeLineWidget::mouseReleaseEvent( QMouseEvent* event )
{
	delete m_hint;
	m_hint = NULL;
	if ( m_action == SelectSongTCO ) { emit selectionFinished(); }
	m_action = NoAction;
}
