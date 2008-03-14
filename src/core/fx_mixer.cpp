#ifndef SINGLE_SOURCE_COMPILE

/*
 * fx_mixer.cpp - effect-mixer for LMMS
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtGui/QInputDialog>
#include <QtGui/QLayout>
#include <QtGui/QMdiArea>
#include <QtGui/QPainter>
#include <QtGui/QScrollArea>
#include <QtGui/QScrollBar>

#include "fx_mixer.h"
#include "fader.h"
#include "effect_chain.h"
#include "effect_rack_view.h"
#include "embed.h"
#include "main_window.h"
#include "lcd_spinbox.h"
#include "gui_templates.h"


struct fxChannel
{
	fxChannel( model * _parent ) :
		m_fxChain( NULL ),
		m_used( FALSE ),
		m_peakLeft( 0.0f ),
		m_peakRight( 0.0f ),
		m_buffer( new surroundSampleFrame[
			engine::getMixer()->framesPerPeriod()] ),
		m_muteModel( FALSE, _parent ),
		m_soloModel( FALSE, _parent ),
		m_volumeModel( 1.0, 0.0, 2.0, 0.01, _parent ),
		m_lock()
	{
		engine::getMixer()->clearAudioBuffer( m_buffer,
				engine::getMixer()->framesPerPeriod() );
	}

	effectChain m_fxChain;
	bool m_used;
	float m_peakLeft;
	float m_peakRight;
	surroundSampleFrame * m_buffer;
	boolModel m_muteModel;
	boolModel m_soloModel;
	floatModel m_volumeModel;
	QMutex m_lock;
} ;



fxMixer::fxMixer() :
	journallingObject(),
	model( NULL )
{
	for( int i = 0; i < NUM_FX_CHANNELS+1; ++i )
	{
		m_fxChannels[i] = new fxChannel( this );
	}
}




fxMixer::~fxMixer()
{
}




void fxMixer::mixToChannel( const surroundSampleFrame * _buf, fx_ch_t _ch )
{
	surroundSampleFrame * buf = m_fxChannels[_ch]->m_buffer;
	for( f_cnt_t f = 0; f < engine::getMixer()->framesPerPeriod(); ++f )
	{
		buf[f][0] += _buf[f][0];
		buf[f][1] += _buf[f][1];
	}
	m_fxChannels[_ch]->m_used = TRUE;
}



void fxMixer::processChannel( fx_ch_t _ch )
{
	if( m_fxChannels[_ch]->m_used || _ch == 0 )
	{
		const fpp_t f = engine::getMixer()->framesPerPeriod();
		m_fxChannels[_ch]->m_fxChain.startRunning();
		m_fxChannels[_ch]->m_fxChain.processAudioBuffer(
						m_fxChannels[_ch]->m_buffer, f );
		m_fxChannels[_ch]->m_peakLeft = engine::getMixer()->peakValueLeft( m_fxChannels[_ch]->m_buffer, f ) * m_fxChannels[_ch]->m_volumeModel.value();
		m_fxChannels[_ch]->m_peakRight = engine::getMixer()->peakValueRight( m_fxChannels[_ch]->m_buffer, f ) * m_fxChannels[_ch]->m_volumeModel.value();
	}
	else
	{
		m_fxChannels[_ch]->m_peakLeft =
					m_fxChannels[_ch]->m_peakRight = 0.0f; 
	}
}




void fxMixer::prepareMasterMix( void )
{
	engine::getMixer()->clearAudioBuffer( m_fxChannels[0]->m_buffer,
					engine::getMixer()->framesPerPeriod() );
}




const surroundSampleFrame * fxMixer::masterMix( void )
{
	surroundSampleFrame * buf = m_fxChannels[0]->m_buffer;
	for( int i = 1; i < NUM_FX_CHANNELS+1; ++i )
	{
		if( m_fxChannels[i]->m_used )
		{
			surroundSampleFrame * _buf = m_fxChannels[i]->m_buffer;
			for( f_cnt_t f = 0; f <
				engine::getMixer()->framesPerPeriod(); ++f )
			{
				buf[f][0] += _buf[f][0]*m_fxChannels[i]->m_volumeModel.value();
				buf[f][1] += _buf[f][1]*m_fxChannels[i]->m_volumeModel.value();
			}
			engine::getMixer()->clearAudioBuffer( _buf,
					engine::getMixer()->framesPerPeriod() );
			m_fxChannels[i]->m_used = FALSE;
		}
	}
	processChannel( 0 );
	for( f_cnt_t f = 0; f < engine::getMixer()->framesPerPeriod(); ++f )
	{
		buf[f][0] *= m_fxChannels[0]->m_volumeModel.value();
		buf[f][1] *= m_fxChannels[0]->m_volumeModel.value();
	}
	m_fxChannels[0]->m_peakLeft *= engine::getMixer()->masterGain();
	m_fxChannels[0]->m_peakRight *= engine::getMixer()->masterGain();
	return( buf );
}




void fxMixer::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
/*	m_chordsEnabledModel.saveSettings( _doc, _this, "chord-enabled" );
	m_chordsModel.saveSettings( _doc, _this, "chord" );
	m_chordRangeModel.saveSettings( _doc, _this, "chordrange" );*/
}




void fxMixer::loadSettings( const QDomElement & _this )
{
/*	m_chordsEnabledModel.loadSettings( _this, "chord-enabled" );
	m_chordsModel.loadSettings( _this, "chord" );
	m_chordRangeModel.loadSettings( _this, "chordrange" );*/
}


class fxLine : public QWidget
{
public:
	fxLine( QWidget * _parent, fxMixerView * _mv ) :
		QWidget( _parent ),
		m_mv( _mv )
	{
		setFixedSize( 32, 200 );
		setAttribute( Qt::WA_OpaquePaintEvent, TRUE );
	}

	virtual void setName( const QString & _name )
	{
		m_name = _name;
	}
	virtual void paintEvent( QPaintEvent * )
	{
		QPainter p( this );
		p.fillRect( rect(), QColor( 72, 76, 88 ) );
/*		p.setPen( QColor( 144, 152, 176 ) );
		p.drawLine( 0, 0, width()-1, 0 );
		p.drawLine( 0, 0, 0, height()-1 );
		p.setPen( QColor( 36, 38, 44 ) );
		p.drawLine( 0, height()-1, width()-1, height()-1 );
		p.drawLine( width()-1, 0, width()-1, height()-1 );*/
		p.setPen( QColor( 40, 42, 48 ) );
		p.drawRect( 0, 0, width()-2, height()-2 );
		p.setPen( QColor( 108, 114, 132 ) );
		p.drawRect( 1, 1, width()-2, height()-2 );
		p.setPen( QColor( 20, 24, 32 ) );
		p.drawRect( 0, 0, width()-1, height()-1 );
		
		p.rotate( -90 );
		p.setPen( m_mv->currentFxLine() == this ?
					QColor( 0, 255, 0 ) : Qt::white );
		p.setFont( pointSizeF( font(), 7.5f ) );
		p.drawText( -70, 20, m_name );
	}

	virtual void mousePressEvent( QMouseEvent * )
	{
		m_mv->setCurrentFxLine( this );
	}

	virtual void mouseDoubleClickEvent( QMouseEvent * )
	{
		bool ok;
		QString new_name = QInputDialog::getText( this,
				fxMixerView::tr( "Rename FX channel" ),
				fxMixerView::tr( "Enter the new name for this "
							"FX channel" ),
				QLineEdit::Normal, m_name, &ok );
		if( ok && !new_name.isEmpty() )
		{
			m_name = new_name;
			update();
		}
				
	}

private:
	fxMixerView * m_mv;
	QString m_name;

} ;




fxMixerView::fxMixerView() :
	QWidget(),
	modelView( NULL )
{
	fxMixer * m = engine::getFxMixer();

	QPalette pal = palette();
	pal.setColor( QPalette::Background, QColor( 72, 76, 88 ) );
	setPalette( pal );
	setAutoFillBackground( TRUE );

//	setFixedHeight( 250+216 );
	setWindowTitle( tr( "FX-Mixer" ) );
//	setWindowIcon( embed::getIconPixmap( "fxmixer" ) );
	engine::getMainWindow()->workspace()->addSubWindow( this );
	parentWidget()->setAttribute( Qt::WA_DeleteOnClose, FALSE );
	QVBoxLayout * ml = new QVBoxLayout( this );
	ml->setMargin( 0 );
	ml->setSpacing( 0 );

	QScrollArea * a = new QScrollArea( this );
	a->setFrameShape( QFrame::NoFrame );
	a->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	a->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
	a->setFixedHeight( 216 );
	ml->addWidget( a );

	m_fxRackArea = new QWidget( this );
	m_fxRackArea->setFixedHeight( 250 );
	ml->addWidget( m_fxRackArea );
	ml->addStretch();

	QWidget * base = new QWidget;
	QHBoxLayout * bl = new QHBoxLayout( base );
	bl->setSpacing( 0 );
	bl->setMargin( 0 );
	a->setWidget( base );

	base->setFixedSize( (NUM_FX_CHANNELS+1)*33+6+10, 200 );
	pal = base->palette();
	pal.setColor( QPalette::Background, QColor( 72, 76, 88 ) );
	base->setPalette( pal );
	base->setAutoFillBackground( TRUE );

	bl->addSpacing( 6 );
	for( int i = 0; i < NUM_FX_CHANNELS+1; ++i )
	{
		fxChannelView * cv = &m_fxChannelViews[i];
		cv->m_fxLine = new fxLine( base, this );
		bl->addWidget( cv->m_fxLine );
		if( i == 0 )
		{
			bl->addSpacing( 10 );
			cv->m_fxLine->setName( tr( "Master" ).arg( i ) );
		}
		else
		{
			bl->addSpacing( 1 );
			cv->m_fxLine->setName( tr( "FX %1" ).arg( i ) );
		}
		lcdSpinBox * l = new lcdSpinBox( 2, cv->m_fxLine );
		l->model()->setRange( i, i );
		l->model()->setValue( i );
		l->update();
		cv->m_fader = new fader( &m->m_fxChannels[i]->m_volumeModel,
								cv->m_fxLine );
		cv->m_fader->move( 15-cv->m_fader->width()/2, 80 );
		cv->m_rackView = new effectRackView(
				&m->m_fxChannels[i]->m_fxChain, m_fxRackArea );
		cv->m_rackView->setFixedSize( 250, 250 );
	}

	show();
	setCurrentFxLine( m_fxChannelViews[0].m_fxLine );

	QTimer * t = new QTimer( this );
	connect( t, SIGNAL( timeout() ), this, SLOT( updateFaders() ) );
	t->start( 50 );
}




fxMixerView::~fxMixerView()
{
}




void fxMixerView::setCurrentFxLine( fxLine * _line )
{
	m_currentFxLine = _line;
	for( int i = 0; i < NUM_FX_CHANNELS+1; ++i )
	{
		if( m_fxChannelViews[i].m_fxLine == _line )
		{
			m_fxChannelViews[i].m_rackView->show();
		}
		else
		{
			m_fxChannelViews[i].m_rackView->hide();
		}
		m_fxChannelViews[i].m_fxLine->update();
	}
}




void fxMixerView::updateFaders( void )
{
	fxMixer * m = engine::getFxMixer();
	for( int i = 0; i < NUM_FX_CHANNELS+1; ++i )
	{
		const float opl = m_fxChannelViews[i].m_fader->getPeak_L();
		const float opr = m_fxChannelViews[i].m_fader->getPeak_R();
		const float fall_off = 1.1;
		if( m->m_fxChannels[i]->m_peakLeft > opl )
		{
			m_fxChannelViews[i].m_fader->setPeak_L(
					m->m_fxChannels[i]->m_peakLeft );
		}
		else
		{
			m_fxChannelViews[i].m_fader->setPeak_L( opl/fall_off );
		}
		if( m->m_fxChannels[i]->m_peakRight > opr )
		{
			m_fxChannelViews[i].m_fader->setPeak_R(
					m->m_fxChannels[i]->m_peakRight );
		}
		else
		{
			m_fxChannelViews[i].m_fader->setPeak_R( opr/fall_off );
		}
	}
}



#include "fx_mixer.moc"

#endif
