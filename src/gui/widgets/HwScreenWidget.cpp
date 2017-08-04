/*
 * HwScreenWidget.cpp - a widget emulating hardware screens
 *
 * Copyright (c) 2017
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

//#include <QApplication>
//#include <QLabel>
//#include <QMouseEvent>
#include <QPainter>
#include <QStyleOptionFrameV2>

#include "HwScreenWidget.h"
#include "embed.h"
#include "gui_templates.h"

HwScreenWidget::HwScreenWidget(QWidget* parent, int columns, int rows,
			       const QString & label, const QString & colorSet) :
	QWidget(parent)
{
	initUi(columns,rows,label,colorSet);
}

HwScreenWidget::~HwScreenWidget()
{
	delete m_screenPixmap;
}

void HwScreenWidget::initUi(int columns, int rows, const QString & label, const QString & _colorSet)
{
	setEnabled(true);
	setColumns(columns);
	setRows(rows);
	setLabel(label);
	setColorSet(_colorSet);

	connect(this, SIGNAL(columnsChanged(int)), this, SLOT(columnsUpdated()));
	connect(this, SIGNAL(rowChanged(int)), this, SLOT(columnsUpdated()));
	connect(this, SIGNAL(labelChanged(QString)), this, SLOT(columnsUpdated()));
	connect(this, SIGNAL(colorSetChanged(QString)), this, SLOT(columnsUpdated()));
}

void HwScreenWidget::paintEvent(QPaintEvent*)
{
	QPainter p(this);


	p.fillRect(0,0,width(),height(),p.background());//Qt::red);

	/*
	// Left Margin
	p.drawPixmap( cellRect, *m_lcdPixmap, 
			QRect( QPoint( charsPerPixmap*m_cellWidth, 
				isEnabled()?0:m_cellHeight ), 
			cellSize ) );
	*/

	/*
	// Padding
	for( int i=0; i < m_numDigits - m_display.length(); i++ ) 
	{
		p.drawPixmap( cellRect, *m_lcdPixmap, 
			QRect( QPoint( 10 * m_cellWidth, isEnabled()?0:m_cellHeight) , cellSize ) );
		p.translate( m_cellWidth, 0 );
	}
	*/

	int wc=m_cellSize.width();
	int hc=m_cellSize.height();

	// Border
	QStyleOptionFrame opt;
	opt.initFrom( this );
	opt.state = QStyle::State_Sunken;
	opt.rect = QRect(0,0,m_columns*wc+4,m_rows*hc+4);
	style()->drawPrimitive( QStyle::PE_Frame, &opt, &p, this );
	//p.resetTransform();

	int wr=wc;//width()/m_columns;
	int hr=hc;//height()/m_rows;

	int cc[m_columns][m_rows];
	for(int j=0;j<m_rows;j++)
		for(int i=0;i<m_columns;i++)
			cc[i][j]=0;
	int i=0;
	int j=0;
	for(int k=0;k<m_text.size();k++)
	{
		if(j>=m_rows) break;
		int c=(int)(m_text.at(k).cell()&0x7F);//????
		//qWarning("C=%d",c);
		if(c==10)
		{
			i=0;
			j++;
			continue;
		}
		if(c<32) continue;
		if(c>=128) continue;
		if(i>=m_columns) continue;

		c-=32;
		cc[i][j]=c;
		i++;
	}

	for(int j=0;j<m_rows;j++)
		for(int i=0;i<m_columns;i++)
		{
			int c=cc[i][j];
			QRect r(2+wr*i,2+hr*j,wr,hr);
			//p.fillRect(r,Qt::blue);
			p.drawPixmap(r,*m_screenPixmap,
				     QRect(QPoint(c*wc,(isEnabled()?0:hc)),
					   QSize(wc,hc)));
		}

	/*
	// Right Margin
	p.drawPixmap( QRect( 0, 0, m_marginWidth-1, m_cellHeight ), *m_lcdPixmap, 
			QRect( charsPerPixmap*m_cellWidth, isEnabled()?0:m_cellHeight,
				m_cellWidth / 2, m_cellHeight ) );
	p.restore();
	*/


	// Label
	if(!m_label.isEmpty())
	{
		p.setFont( pointSizeF( p.font(), 6.5 ) );
		p.setPen(QColor(0,0,0));// textShadowColor() );
		p.drawText( width() / 2 -
				p.fontMetrics().width( m_label ) / 2 + 1,
						height(), m_label );
		p.setPen(QColor(255,255,255));// textColor() );
		p.drawText( width() / 2 -
				p.fontMetrics().width( m_label ) / 2,
						height() - 1, m_label );
	}
}

void HwScreenWidget::updateSize()
{
	int ws=m_cellSize.width()*m_columns +4;
	int hs=m_cellSize.height()*m_rows   +4;
	if (!m_label.isEmpty())
	{
		ws=qMax<int>(ws,QFontMetrics( pointSizeF( font(), 6.5 ) ).width( m_label ) );
		hs+=9;
	}
	setFixedSize(ws,hs);
}

void HwScreenWidget::columnsUpdated()
{
	updateSize();
	update();
}

void HwScreenWidget::rowsUpdated()
{
	updateSize();
	update();
}

void HwScreenWidget::colorSetUpdated()
{
	QString rn="hwscreen_";
	rn.append(m_colorSet);
	m_screenPixmap = new QPixmap( embed::getIconPixmap( rn.toUtf8().constData() ) );
	m_cellSize=QSize(m_screenPixmap->size().width()  / HwScreenWidget::CHARS_PER_PIXMAP,
			 m_screenPixmap->size().height() / 2);

	updateSize();
	update();
}

void HwScreenWidget::labelUpdated()
{
	update();
}
