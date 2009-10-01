#include "FxLine.h"

#include <QDebug>
#include <QtGui/QInputDialog>
#include <QtGui/QPainter>
#include <QtGui/QLineEdit>

#include "FxMixer.h"
#include "FxMixerView.h"
#include "embed.h"
#include "engine.h"
#include "lcd_spinbox.h"
#include "SendButtonIndicator.h"

FxLine::FxLine( QWidget * _parent, FxMixerView * _mv, int _channelIndex) :
	QWidget( _parent ),
	m_mv( _mv ),
	m_channelIndex( _channelIndex )
{
	setFixedSize( 32, 287 );
	setAttribute( Qt::WA_OpaquePaintEvent, true );
	setCursor( QCursor( embed::getIconPixmap( "hand" ), 0, 0 ) );

	// mixer sends knob
	m_sendKnob = new knob(0, this, tr("Channel send amount"));
	m_sendKnob->move(0, 22);
	m_sendKnob->setVisible(false);

	// send button indicator
	m_sendBtn = new SendButtonIndicator(this, this, m_mv);
	m_sendBtn->setPixmap(embed::getIconPixmap("mixer_send_off", 23, 16));
	m_sendBtn->move(4,4);

	// channel number
	lcdSpinBox * l = new lcdSpinBox( 2, this );
	l->model()->setRange( m_channelIndex, m_channelIndex );
	l->model()->setValue( m_channelIndex );
	l->move( 2, 58 );
	l->setMarginWidth( 1 );
}


void FxLine::paintEvent( QPaintEvent * )
{
	FxMixer * mix = engine::fxMixer();
	bool sendToThis = mix->channelSendModel(
		m_mv->currentFxLine()->m_channelIndex, m_channelIndex) != NULL;
	QPainter painter;
	painter.begin( this );
	engine::getLmmsStyle()->drawFxLine( &painter, this,
		mix->effectChannel(m_channelIndex)->m_name,
		m_mv->currentFxLine() == this, sendToThis );
	painter.end();
}

void FxLine::mousePressEvent( QMouseEvent * )
{
	m_mv->setCurrentFxLine( this );
}

void FxLine::mouseDoubleClickEvent( QMouseEvent * )
{
	bool ok;
	FxMixer * mix = engine::fxMixer();
	QString new_name = QInputDialog::getText( this,
			FxMixerView::tr( "Rename FX channel" ),
			FxMixerView::tr( "Enter the new name for this "
						"FX channel" ),
			QLineEdit::Normal, mix->effectChannel(m_channelIndex)->m_name, &ok );
	if( ok && !new_name.isEmpty() )
	{
		mix->effectChannel(m_channelIndex)->m_name = new_name;
		update();
	}
}

#include "moc_FxLine.cxx"
