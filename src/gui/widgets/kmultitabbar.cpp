#ifndef SINGLE_SOURCE_COMPILE

/*
 * kmultitabbar.cpp - widget for horizontal and vertical tabs
 *
 * Copyright (c) 2001-2003 Joseph Wenninger <jowenn@kde.org>
 * Copyright (c) 2004-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "kmultitabbar.h"

#include <QtCore/QEvent>
#include <QtGui/QApplication>
#include <QtGui/QFontMetrics>
#include <QtGui/QFrame>
#include <QtGui/QLayout>
#include <QtGui/QPainter>
#include <QtGui/QStyle>
#include <QtGui/QStyleOptionButton>

#include <math.h>

#include "templates.h"
#include "tooltip.h"

#define NEARBYINT(i) ((int(float(i) + 0.5)))

class KMultiTabBarTabPrivate {
public:
	QPixmap pix;
};


KMultiTabBarInternal::KMultiTabBarInternal(QWidget *parent, KMultiTabBar::KMultiTabBarMode bm):QWidget(parent)
{
	m_expandedTabSize=-1;
	m_showActiveTabTexts=false;
	m_barMode=bm;
	if (bm==KMultiTabBar::Vertical)
	{
		box=new QWidget(this);
		mainLayout=new QVBoxLayout(box);
		box->setFixedWidth(24);
		setFixedWidth(24);
	}
	else
	{
		box=new QWidget(this);
		mainLayout=new QHBoxLayout(box);
		box->setFixedHeight(24);
		setFixedHeight(24);
	}
	mainLayout->setMargin(0);
	mainLayout->setSpacing(0);
}

void KMultiTabBarInternal::setStyle(enum KMultiTabBar::KMultiTabBarStyle style)
{
	m_style=style;
        for (int i=0;i<m_tabs.count();i++)
                m_tabs.at(i)->setStyle(m_style);

	if  ( (m_style==KMultiTabBar::KDEV3) ||
		(m_style==KMultiTabBar::KDEV3ICON ) ) {
		delete mainLayout;
		mainLayout=0;
		resizeEvent(0);
	} else if (mainLayout==0) {
		if (m_barMode==KMultiTabBar::Vertical)
		{
			box=new QWidget(this);
			mainLayout=new QVBoxLayout(box);
			box->setFixedWidth(24);
			setFixedWidth(24);
		}
		else
		{
			box=new QWidget(this);
			mainLayout=new QHBoxLayout(box);
			box->setFixedHeight(24);
			setFixedHeight(24);
		}
	        for (int i=0;i<m_tabs.count();i++)
        	        mainLayout->addWidget(m_tabs.at(i));

	}
	update();
}



#define CALCDIFF(m_tabs,diff,i) if (m_lines>(int)lines) {\
					int ulen=0;\
					diff=0; \
					for (int i2=i;i2<tabCount;i2++) {\
						int l1=m_tabs.at(i2)->neededSize();\
						if ((ulen+l1)>(int)space){\
							if (ulen==0) diff=0;\
							else diff=((float)(space-ulen))/(i2-i);\
							break;\
						}\
						ulen+=l1;\
					}\
				} else {diff=0; }


void KMultiTabBarInternal::resizeEvent(QResizeEvent *ev) {
	if (ev) QWidget::resizeEvent(ev);

	if ( (m_style==KMultiTabBar::KDEV3) ||
		(m_style==KMultiTabBar::KDEV3ICON) ){
		box->setGeometry(0,0,width(),height());
		int lines=1;
		uint space;
		float tmp=0;
		if ((m_position==KMultiTabBar::Bottom) || (m_position==KMultiTabBar::Top))
			space=width();
		else
			space=height();

		int cnt=0;
//CALCULATE LINES
		const int tabCount=m_tabs.count();
	        for (int i=0;i<tabCount;i++) {
			cnt++;
			tmp+=m_tabs.at(i)->neededSize();
			if (tmp>space) {
				if (cnt>1)i--;
				else if (i==(tabCount-1)) break;
				cnt=0;
				tmp=0;
				lines++;
			}
		}
//SET SIZE & PLACE
		float diff=0;
		cnt=0;

		if ((m_position==KMultiTabBar::Bottom) || (m_position==KMultiTabBar::Top)) {

			setFixedHeight(lines*24);
			box->setFixedHeight(lines*24);
			m_lines=height()/24-1;
			lines=0;
			CALCDIFF(m_tabs,diff,0)
			tmp=-diff;

		        for (int i=0;i<tabCount;i++) {
				KMultiTabBarTab *tab=m_tabs.at(i);
				cnt++;
				tmp+=tab->neededSize()+diff;
				if (tmp>space) {
					if (cnt>1) {
						CALCDIFF(m_tabs,diff,i)
						i--;
					}
					else {
						tab->removeEventFilter(this);
						tab->move(NEARBYINT(tmp-tab->neededSize()),lines*24);
						tab->setFixedWidth(NEARBYINT(tmp+diff)-tab->x());;
						tab->installEventFilter(this);
						CALCDIFF(m_tabs,diff,(i+1))

					}
					tmp=-diff;
					cnt=0;
					lines++;

				} else 	{
					tab->removeEventFilter(this);
					tab->move(NEARBYINT(tmp-tab->neededSize()),lines*24);
					tab->setFixedWidth(NEARBYINT(tmp+diff)-tab->x());;

					tab->installEventFilter(this);

				}
			}
		}
		else {
			setFixedWidth(lines*24);
			box->setFixedWidth(lines*24);
			m_lines=lines=width()/24;
			lines=0;
			CALCDIFF(m_tabs,diff,0)
			tmp=-diff;

		        for (int i=0;i<tabCount;i++) {
				KMultiTabBarTab *tab=m_tabs.at(i);
				cnt++;
				tmp+=tab->neededSize()+diff;
				if (tmp>space) {
					if (cnt>1) {
						CALCDIFF(m_tabs,diff,i);
						tmp=-diff;
						i--;
					}
					else {
						tab->removeEventFilter(this);
						tab->move(lines*24,NEARBYINT(tmp-tab->neededSize()));
                                                tab->setFixedHeight(NEARBYINT(tmp+diff)-tab->y());;
						tab->installEventFilter(this);
					}
					cnt=0;
					tmp=-diff;
					lines++;
				} else 	{
					tab->removeEventFilter(this);
					tab->move(lines*24,NEARBYINT(tmp-tab->neededSize()));
                                        tab->setFixedHeight(NEARBYINT(tmp+diff)-tab->y());;
					tab->installEventFilter(this);
				}
			}
		}


	} else {
		int size=0; /*move the calculation into another function and call it only on add tab and tab click events*/
		for (int i=0;i<(int)m_tabs.count();i++)
			size+=(m_barMode==KMultiTabBar::Vertical?m_tabs.at(i)->height():m_tabs.at(i)->width());
		if ((m_position==KMultiTabBar::Bottom) || (m_position==KMultiTabBar::Top))
			box->setGeometry(0,0,size,height());
		else box->setGeometry(0,0,width(),size);

	}
}


void KMultiTabBarInternal::showActiveTabTexts(bool show)
{
	m_showActiveTabTexts=show;
}


KMultiTabBarTab* KMultiTabBarInternal::tab(int id) const
{
	for (QListIterator<KMultiTabBarTab *> it(m_tabs);it.hasNext();it.next()){
		if (it.peekNext()->id()==id) return it.peekNext();
	}
        return 0;
}

bool KMultiTabBarInternal::eventFilter(QObject *, QEvent *e) {
	if (e->type()==QEvent::Resize) resizeEvent(0);
	return false;
}

int KMultiTabBarInternal::appendTab(const QPixmap &pic ,int id,const QString& text)
{
	KMultiTabBarTab  *tab;
	m_tabs.append(tab= new KMultiTabBarTab(pic,text,id,box,m_position,m_style));
	mainLayout->addWidget( tab );
	tab->installEventFilter(this);
	tab->showActiveTabText(m_showActiveTabTexts);

	if (m_style==KMultiTabBar::KONQSBC)
	{
		if (m_expandedTabSize<tab->neededSize()) {
			m_expandedTabSize=tab->neededSize();
			for (int i=0;i<m_tabs.count();i++)
				m_tabs.at(i)->setSize(m_expandedTabSize);

		} else tab->setSize(m_expandedTabSize);
	} else tab->updateState();
	tab->show();
	resizeEvent(0);
	return 0;
}

void KMultiTabBarInternal::removeTab(int id)
{
	for (int pos=0;pos<m_tabs.count();pos++)
	{
		if (m_tabs.at(pos)->id()==id)
		{
			delete m_tabs.at(pos);
			m_tabs.removeAt(pos);
			resizeEvent(0);
			break;
		}
	}
}

void KMultiTabBarInternal::setPosition(enum KMultiTabBar::KMultiTabBarPosition pos)
{
	m_position=pos;
	for (int i=0;i<m_tabs.count();i++)
		m_tabs.at(i)->setTabsPosition(m_position);
	update();
}

KMultiTabBarButton::KMultiTabBarButton(const QPixmap& pic,const QString& text, QMenu *popup,
		int id,QWidget *parent,KMultiTabBar::KMultiTabBarPosition pos,KMultiTabBar::KMultiTabBarStyle style)
	:QPushButton(QIcon(),text,parent),m_style(style)
{
	setIcon(pic);
	setText(text);
	m_position=pos;
  	if (popup) setMenu(popup);
	setFlat(true);
	setFixedHeight(24);
	setFixedWidth(24);
	m_id=id;
	setToolTip(text);
	connect(this,SIGNAL(clicked()),this,SLOT(slotClicked()));
}

KMultiTabBarButton::KMultiTabBarButton(const QString& text, QMenu *popup,
		int id,QWidget *parent,KMultiTabBar::KMultiTabBarPosition pos,KMultiTabBar::KMultiTabBarStyle style)
	:QPushButton(QIcon(),text,parent),m_style(style)
{
	setText(text);
	m_position=pos;
  	if (popup) setMenu(popup);
	setFlat(true);
	setFixedHeight(24);
	setFixedWidth(24);
	m_id=id;
	setToolTip(text);
	connect(this,SIGNAL(clicked()),this,SLOT(slotClicked()));
}

KMultiTabBarButton::~KMultiTabBarButton() {
}

int KMultiTabBarButton::id() const{
	return m_id;
}

void KMultiTabBarButton::setText(const QString& text)
{
	QPushButton::setText(text);
	m_text=text;
	setToolTip(text);
}

void KMultiTabBarButton::slotClicked()
{
	emit clicked(m_id);
}

void KMultiTabBarButton::setPosition(KMultiTabBar::KMultiTabBarPosition pos)
{
	m_position=pos;
	update();
}

void KMultiTabBarButton::setStyle(KMultiTabBar::KMultiTabBarStyle style)
{
	m_style=style;
	update();
}

void KMultiTabBarButton::hideEvent( QHideEvent* he) {
	QPushButton::hideEvent(he);
	KMultiTabBar *tb=dynamic_cast<KMultiTabBar*>(parentWidget());
	if (tb) tb->updateSeparator();
}

void KMultiTabBarButton::showEvent( QShowEvent* he) {
	QPushButton::showEvent(he);
	KMultiTabBar *tb=dynamic_cast<KMultiTabBar*>(parentWidget());
	if (tb) tb->updateSeparator();
}


QSize KMultiTabBarButton::sizeHint() const
{
    //constPolish();

    int w = 0, h = 0;

    // calculate contents size...
    int iw = 0, ih = 0;
    if ( !icon().isNull() ) {
        iw = 20;
        ih = 16;
        w += iw;
        h = qMax( h, ih );
    }
    QStyleOptionButton sob;
    if ( menu() != 0 )
        w += style()->pixelMetric(QStyle::PM_MenuButtonIndicator, &sob, this);

        QString s( text() );
        bool empty = s.isEmpty();
        if ( empty )
            s = QLatin1String("XXXX");
        QFontMetrics fm = fontMetrics();
        QSize sz = fm.size( Qt::TextShowMnemonic, s );
        if(!empty || !w)
            w += sz.width();
        if(!empty || !h)
            h = qMax(h, sz.height());


    QStyleOptionToolButton opt;
    opt.init(this);
    opt.rect = QRect(0, 0, w, h);
    opt.subControls       = QStyle::SC_All;
    opt.activeSubControls = 0;
    opt.text              = text();
    opt.font              = font();
    opt.icon              = icon();
    opt.iconSize          = QSize(iw, ih);
    return (style()->sizeFromContents(QStyle::CT_ToolButton, &opt, QSize(w, h), this).
            expandedTo(QApplication::globalStrut()));
}


KMultiTabBarTab::KMultiTabBarTab(const QPixmap& pic, const QString& text,
		int id,QWidget *parent,KMultiTabBar::KMultiTabBarPosition pos,
		KMultiTabBar::KMultiTabBarStyle style)
	:KMultiTabBarButton(text,0,id,parent,pos,style),
 	m_showActiveTabText(false)
{
	d=new KMultiTabBarTabPrivate();
	setIcon(pic);
	m_expandedSize=24;
	setCheckable(true);
	setAttribute( Qt::WA_OpaquePaintEvent, true );
}

KMultiTabBarTab::~KMultiTabBarTab() {
	delete d;
}


void KMultiTabBarTab::setTabsPosition(KMultiTabBar::KMultiTabBarPosition pos)
{
	if ((pos!=m_position) && ((pos==KMultiTabBar::Left) || (pos==KMultiTabBar::Right))) {
		if (!d->pix.isNull()) {
			QMatrix temp;// (1.0F, 0.0F, 0.0F, -1.0F, 0.0F, 0.0F);
			temp.rotate(180);
			d->pix=d->pix.transformed(temp);
			setIcon(d->pix);
		}
	}
	setPosition(pos);
}

void KMultiTabBarTab::setIcon(const QString& icon)
{
	QPixmap pic(icon);
	setIcon(pic);
}

void KMultiTabBarTab::setIcon(const QPixmap& icon)
{

	if (m_style!=KMultiTabBar::KDEV3) {
		if ((m_position==KMultiTabBar::Left) || (m_position==KMultiTabBar::Right)) {
		        QMatrix rotateMatrix;
/*			if (m_position==KMultiTabBar::Left)
		        	rotateMatrix.rotate(-270);
			else*/
				rotateMatrix.rotate(90);
			d->pix=icon.transformed(rotateMatrix); //TODO FIX THIS, THIS SHOWS WINDOW
			KMultiTabBarButton::setIcon(d->pix);
		} else KMultiTabBarButton::setIcon(icon);
	}
}

void KMultiTabBarTab::slotClicked()
{
	updateState();
	KMultiTabBarButton::slotClicked();
}

void KMultiTabBarTab::setState(bool b)
{
	setChecked(b);
	updateState();
}

void KMultiTabBarTab::updateState()
{

	if (m_style!=KMultiTabBar::KONQSBC) {
		if ((m_style==KMultiTabBar::KDEV3) || (m_style==KMultiTabBar::KDEV3ICON) || (isChecked())) {
			QPushButton::setText(m_text);
		} else {
			QPushButton::setText(QString());
		}

		if ((m_position==KMultiTabBar::Right || m_position==KMultiTabBar::Left)) {
			setFixedWidth(24);
			if ((m_style==KMultiTabBar::KDEV3)  || (m_style==KMultiTabBar::KDEV3ICON) || (isChecked())) {
				setFixedHeight(KMultiTabBarButton::sizeHint().width());
			} else setFixedHeight(36);
		} else {
			setFixedHeight(24);
			if ((m_style==KMultiTabBar::KDEV3)  || (m_style==KMultiTabBar::KDEV3ICON) || (isChecked())) {
				setFixedWidth(KMultiTabBarButton::sizeHint().width());
			} else setFixedWidth(36);
		}
	} else {
                if ((!isChecked()) || (!m_showActiveTabText))
                {
	                setFixedWidth(24);
	                setFixedHeight(24);
                        return;
                }
                if ((m_position==KMultiTabBar::Right || m_position==KMultiTabBar::Left))
                        setFixedHeight(m_expandedSize);
                else
                        setFixedWidth(m_expandedSize);
	}
}

int KMultiTabBarTab::neededSize()
{
	return (((m_style!=KMultiTabBar::KDEV3)?24:0)+QFontMetrics(QFont()).width(m_text)+6);
}

void KMultiTabBarTab::setSize(int size)
{
	m_expandedSize=size;
	updateState();
}

void KMultiTabBarTab::showActiveTabText(bool show)
{
	m_showActiveTabText=show;
}

void KMultiTabBarTab::paintEvent(QPaintEvent *) {
	QPainter painter(this);
	drawButton(&painter);
}

void KMultiTabBarTab::drawButtonLabel(QPainter *p) {
	drawButton(p);
}
void KMultiTabBarTab::drawButton(QPainter *paint)
{
	if (m_style!=KMultiTabBar::KONQSBC) drawButtonStyled(paint);
	else  drawButtonClassic(paint);
}

void KMultiTabBarTab::drawButtonStyled(QPainter *paint) {

	QSize sh;
	const int width = 36; // rotated
	const int height = 24;
	if ((m_style==KMultiTabBar::KDEV3) || (m_style==KMultiTabBar::KDEV3ICON) || (isChecked())) {
		 if ((m_position==KMultiTabBar::Left) || (m_position==KMultiTabBar::Right))
			sh=QSize(this->height(),this->width());//KMultiTabBarButton::sizeHint();
			else sh=QSize(this->width(),this->height());
	}
	else
		sh=QSize(width,height);

	QPixmap pixmap( sh.width(),height); ///,sh.height());
	pixmap.fill(backgroundRole());
	QPainter painter(&pixmap);


	QStyle::State st=QStyle::State_None;

	st|=QStyle::State_Enabled;

	if (isChecked()) st|=QStyle::State_On;

	QStyleOptionButton options;
	options.init(this);
	options.state = st;
	options.rect = QRect(0,0,pixmap.width(),pixmap.height());
	options.palette  = palette();
	options.text     = text();
	options.icon     = icon();
	options.iconSize = iconSize();

	style()->drawControl(QStyle::CE_PushButton, &options, &painter, this);

	switch (m_position) {
		case KMultiTabBar::Left:
			paint->rotate(-90);
			paint->drawPixmap(1-pixmap.width(),0,pixmap);
			break;
		case KMultiTabBar::Right:
			paint->rotate(90);
			paint->drawPixmap(0,1-pixmap.height(),pixmap);
			break;

		default:
			paint->drawPixmap(0,0,pixmap);
			break;
	}

}

void KMultiTabBarTab::drawButtonClassic(QPainter *paint)
{
        QPixmap pixmap;
	if( !icon().isNull() )
        	pixmap = icon().pixmap( 16, 16 );
	paint->fillRect(0, 0, 24, 24, palette().background().color());

	if (!isChecked())
	{

		if (m_position==KMultiTabBar::Right)
		{
			paint->fillRect(0,0,21,21,QBrush(palette().background().color()));

			paint->setPen(palette().background().color().dark(150));
			paint->drawLine(0,22,23,22);

			paint->drawPixmap(12-pixmap.width()/2,12-pixmap.height()/2,pixmap);

			paint->setPen(palette().shadow().color());
			paint->drawLine(0,0,0,23);
			paint->setPen(palette().background().color().dark(120));
			paint->drawLine(1,0,1,23);

		}
		else
		if ((m_position==KMultiTabBar::Bottom) || (m_position==KMultiTabBar::Top))
		{
                        paint->fillRect(0,1,23,22,QBrush(palette().background().color()));

                        paint->drawPixmap(12-pixmap.width()/2,12-pixmap.height()/2,pixmap);

                        paint->setPen(palette().background().color().dark(120));
                        paint->drawLine(23,0,23,23);


                        paint->setPen(palette().light().color());
                        paint->drawLine(0,22,23,22);
                        paint->drawLine(0,23,23,23);
                	paint->setPen(palette().shadow().color());
                	paint->drawLine(0,0,23,0);
                        paint->setPen(palette().background().color().dark(120));
                        paint->drawLine(0,1,23,1);

		}
		else
		{
			paint->setPen(palette().background().color().dark(120));
			paint->drawLine(0,23,23,23);
			paint->fillRect(0,0,23,21,QBrush(palette().background()));
			paint->drawPixmap(12-pixmap.width()/2,12-pixmap.height()/2,pixmap);

			paint->setPen(palette().light().color());
			paint->drawLine(23,0,23,23);
			paint->drawLine(22,0,22,23);

			paint->setPen(palette().shadow().color());
			paint->drawLine(0,0,0,23);

		}


	}
	else
	{
		if (m_position==KMultiTabBar::Right)
		{
			paint->setPen(palette().shadow().color());
			paint->drawLine(0,height()-1,23,height()-1);
			paint->drawLine(0,height()-2,23,height()-2);
			paint->drawLine(23,0,23,height()-1);
			paint->drawLine(22,0,22,height()-1);
			paint->fillRect(0,0,21,height()-3,QBrush(palette().light().color()));
			paint->drawPixmap(10-pixmap.width()/2,10-pixmap.height()/2,pixmap);

			if (m_showActiveTabText)
			{
				if (height()<25+4) return;

				QPixmap tpixmap(height()-25-3, width()-2);
				QPainter painter(&tpixmap);

				painter.fillRect(0,0,tpixmap.width(),tpixmap.height(),QBrush(palette().light().color()));

				painter.setPen(palette().text().color());
				painter.drawText(0,+width()/2+QFontMetrics(QFont()).height()/2,m_text);

				paint->rotate(90);
				paint->drawPixmap(25,-tpixmap.height()+1,tpixmap);
			}

		}
		else
		if (m_position==KMultiTabBar::Top)
		{
			paint->fillRect(0,0,width()-1,23,QBrush(palette().light().color()));
			paint->drawPixmap(10-pixmap.width()/2,10-pixmap.height()/2,pixmap);
			if (m_showActiveTabText)
			{
				paint->setPen(palette().text().color());
				paint->drawText(25,height()/2+QFontMetrics(QFont()).height()/2,m_text);
			}
		}
		else
		if (m_position==KMultiTabBar::Bottom)
		{
			paint->setPen(palette().shadow().color());
			paint->drawLine(0,23,width()-1,23);
			paint->drawLine(0,22,width()-1,22);
			paint->fillRect(0,0,width()-1,21,QBrush(palette().light().color()));
			paint->drawPixmap(10-pixmap.width()/2,10-pixmap.height()/2,pixmap);
			if (m_showActiveTabText)
			{
				paint->setPen(palette().text().color());
				paint->drawText(25,height()/2+QFontMetrics(QFont()).height()/2,m_text);
			}

		}
		else
		{


			paint->setPen(palette().shadow().color());
			paint->drawLine(0,height()-1,23,height()-1);
			paint->drawLine(0,height()-2,23,height()-2);
			paint->fillRect(0,0,23,height()-3,QBrush(palette().light().color()));
			paint->drawPixmap(10-pixmap.width()/2,10-pixmap.height()/2,pixmap);
			if (m_showActiveTabText)
			{

		       		if (height()<25+4) return;

                                QPixmap tpixmap(height()-25-3, width()-2);
                                QPainter painter(&tpixmap);

                                painter.fillRect(0,0,tpixmap.width(),tpixmap.height(),QBrush(palette().light().color()));

                                painter.setPen(palette().text().color());
                                painter.drawText(tpixmap.width()-QFontMetrics(QFont()).width(m_text),+width()/2+QFontMetrics(QFont()).height()/2,m_text);

                                paint->rotate(-90);

				paint->drawPixmap(-24-tpixmap.width(),2,tpixmap);

			}

		}

	}
}

KMultiTabBar::KMultiTabBar(KMultiTabBarMode bm, QWidget *parent)
    : QWidget(parent)
{
	if (bm==Vertical)
	{
		m_l=new QVBoxLayout(this);
		setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding));
	}
	else
	{
		m_l=new QHBoxLayout(this);
		setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	}
	m_l->setMargin(0);

	m_internal=new KMultiTabBarInternal(this,bm);
	setPosition((bm==KMultiTabBar::Vertical)?KMultiTabBar::Right:KMultiTabBar::Bottom);
	setStyle(VSNET);
	m_l->insertWidget(0,m_internal);
	m_l->insertWidget(0,m_btnTabSep=new QFrame(this));
	m_btnTabSep->setFixedHeight(4);
	m_btnTabSep->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	m_btnTabSep->setLineWidth(2);
	m_btnTabSep->hide();

	updateGeometry();
}

KMultiTabBar::~KMultiTabBar() {
}

int KMultiTabBar::appendButton(const QPixmap &pic ,int id,QMenu *popup,const QString&)
{
	KMultiTabBarButton  *btn;
	m_buttons.append(btn= new KMultiTabBarButton(pic,QString(),
			popup,id,this,m_position,m_internal->m_style));
	m_l->insertWidget(0,btn);
	btn->show();
	m_btnTabSep->show();
	return 0;
}

void KMultiTabBar::updateSeparator() {
	bool hideSep=true;
	for (QListIterator<KMultiTabBarButton *> it(m_buttons);it.hasNext();){
		if (it.next()->isVisibleTo(this)) {
			hideSep=false;
			break;
		}
	}
	if (hideSep) m_btnTabSep->hide();
		else m_btnTabSep->show();

}

int KMultiTabBar::appendTab(const QPixmap &pic ,int id ,const QString& text)
{
 m_internal->appendTab(pic,id,text);
 return 0;
}

KMultiTabBarButton* KMultiTabBar::button(int id) const
{
	for (QListIterator<KMultiTabBarButton *> it(m_buttons);it.hasNext();it.next()){
		if (it.peekNext()->id()==id) return it.peekNext();
	}
        return 0;
}

KMultiTabBarTab* KMultiTabBar::tab(int id) const
{
	return m_internal->tab(id);
}



void KMultiTabBar::removeButton(int id)
{
	for (int pos=0;pos<m_buttons.count();pos++)
	{
		if (m_buttons.at(pos)->id()==id)
		{
			m_buttons.at(pos)->deleteLater();
			m_buttons.removeAt(pos);
			break;
		}
	}
	if (m_buttons.count()==0) m_btnTabSep->hide();
}

void KMultiTabBar::removeTab(int id)
{
	m_internal->removeTab(id);
}

void KMultiTabBar::setTab(int id,bool state)
{
	KMultiTabBarTab *ttab=tab(id);
	if (ttab)
	{
		ttab->setState(state);
	}
}

bool KMultiTabBar::isTabRaised(int id) const
{
	KMultiTabBarTab *ttab=tab(id);
	if (ttab)
	{
		return ttab->isChecked();
	}

	return false;
}


void KMultiTabBar::showActiveTabTexts(bool show)
{
	m_internal->showActiveTabTexts(show);
}

void KMultiTabBar::setStyle(KMultiTabBarStyle style)
{
	m_internal->setStyle(style);
}

KMultiTabBar::KMultiTabBarStyle KMultiTabBar::tabStyle() const
{
	return m_internal->m_style;
}

void KMultiTabBar::setPosition(KMultiTabBarPosition pos)
{
	m_position=pos;
	m_internal->setPosition(pos);
	for (int i=0;i<m_buttons.count();i++)
		m_buttons.at(i)->setPosition(pos);
}

KMultiTabBar::KMultiTabBarPosition KMultiTabBar::position() const
{
	return m_position;
}
void KMultiTabBar::fontChange(const QFont& /* oldFont */)
{
	for (int i=0;i<tabs()->count();i++)
		tabs()->at(i)->resize();
	update();
}

QList<KMultiTabBarTab *>* KMultiTabBar::tabs() {return m_internal->tabs();}
QList<KMultiTabBarButton *>* KMultiTabBar::buttons() {return &m_buttons;}


#include "moc_kmultitabbar.cxx"



#endif
