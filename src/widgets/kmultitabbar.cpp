/***************************************************************************
                          kmultitabbar.cpp -  description
                             -------------------
    begin                :  2001
    copyright            : (C) 2001,2002,2003 by Joseph Wenninger <jowenn@kde.org>
 ***************************************************************************/

/***************************************************************************
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
 ***************************************************************************/

#include "kmultitabbar.h"

#include "qt3support.h"

#ifdef QT4

#include <QMenu>
#include <QAbstractButton>
#include <QPainter>
#include <QApplication>
#include <QStyle>
#include <QStyleOptionButton>
#include <QFontMetrics>
#include <QColorGroup>
#include <QMouseEvent>

#else

//#include "kmultitabbar.moc"
//#include "kmultitabbar_p.h"
//#include "kmultitabbar_p.moc"
#include <qbutton.h>
#include <qpopupmenu.h>
#include <qabstractlayout.h>
#include <qpainter.h>
#include <qfontmetrics.h>
#include <qstyle.h>
#include <qapplication.h>

//#define palette colorGroup

/*#define #palette().shadow().color() #colorGroup().shadow()
#define #palette().background().color() #colorGroup().background()
#define #palette().light().color() #colorGroup().light()
#define #palette().text().color() #colorGroup().text()*/

#define isChecked isOn
#define TextShowMnemonic ShowPrefix
#define State SFlags
#define State_None Style_Default
#define State_Enabled Style_Enabled
#define State_On Style_On

#endif

#include "templates.h"
#include "tooltip.h"

#include "kmultitabbar.moc"

//#include <kiconloader.h>
//#include <kdebug.h>


class KMultiTabBarTabPrivate {
public:
	QPixmap pix;
};


KMultiTabBarInternal::KMultiTabBarInternal(QWidget *parent, Qt::Orientation o):Q3ScrollView(parent)
{
	m_expandedTabSize=-1;
	m_showActiveTabTexts=false;
#ifndef QT4
	m_tabs.setAutoDelete(true);
#endif
	setHScrollBarMode(AlwaysOff);
	setVScrollBarMode(AlwaysOff);
	if (o==Qt::Vertical)
	{
		box=new QWidget(viewport());
		mainLayout=new QVBoxLayout(box);
#ifndef QT4
		mainLayout->setAutoAdd(true);
#endif
		box->setFixedWidth(24);
		setFixedWidth(24);
	}
	else
	{
		box=new QWidget(viewport());
		mainLayout=new QHBoxLayout(box);
#ifndef QT4
		mainLayout->setAutoAdd(true);
#endif
		box->setFixedHeight(24);
		setFixedHeight(24);
	}
	mainLayout->setMargin( 0 );
	mainLayout->setSpacing( 0 );
	addChild(box);
	setFrameStyle(NoFrame);
#ifndef QT4
	viewport()->setBackgroundMode(Qt::PaletteBackground);
#endif
}

void KMultiTabBarInternal::setStyle(enum KMultiTabBar::KMultiTabBarStyle style)
{
	m_style=style;
        for (csize i=0;i<m_tabs.count();i++)
                m_tabs.at(i)->setStyle(m_style);

	if  ( (m_style==KMultiTabBar::KDEV3) ||
		(m_style==KMultiTabBar::KDEV3ICON ) ) {
		resizeEvent(0);
	}
        viewport()->repaint();
}

void KMultiTabBarInternal::drawContents ( QPainter * paint, int clipx, int clipy, int clipw, int cliph )
{
	Q3ScrollView::drawContents (paint , clipx, clipy, clipw, cliph );

	if (m_position==KMultiTabBar::DockRight)
	{

                paint->setPen(palette().shadow()
#ifdef QT4
				.color()
#endif
				);
                paint->drawLine(0,0,0,viewport()->height());
                paint->setPen(palette().background()
#ifdef QT4
				.color()
#endif
				.dark(120));
                paint->drawLine(1,0,1,viewport()->height());


	}
	else
	if (m_position==KMultiTabBar::DockLeft)
	{
                paint->setPen(palette().light()
#ifdef QT4
				.color()
#endif
				);
		paint->drawLine(23,0,23,viewport()->height());
                paint->drawLine(22,0,22,viewport()->height());

                paint->setPen(palette().shadow()
#ifdef QT4
				.color()
#endif
				);
                paint->drawLine(0,0,0,viewport()->height());
	}
	else
	if (m_position==KMultiTabBar::DockBottom)
	{
		paint->setPen(palette().shadow()
#ifdef QT4
				.color()
#endif
				);
		paint->drawLine(0,0,viewport()->width(),0);
                paint->setPen(palette().background()
#ifdef QT4
				.color()
#endif
				.dark(120));
                paint->drawLine(0,1,viewport()->width(),1);
	}
	else
	{
	        paint->setPen(palette().light()
#ifdef QT4
				.color()
#endif
				);
		paint->drawLine(0,23,viewport()->width(),23);
                paint->drawLine(0,22,viewport()->width(),22);

/*                paint->setPen(palette().shadow().color());
                paint->drawLine(0,0,0,viewport()->height());*/

	}


}

void KMultiTabBarInternal::contentsMousePressEvent(QMouseEvent *ev)
{
	ev->ignore();
}

void KMultiTabBarInternal::mousePressEvent(QMouseEvent *ev)
{
	ev->ignore();
}

void KMultiTabBarInternal::resizeEvent(QResizeEvent *ev) {
	//kdDebug()<<"KMultiTabBarInternal::resizeEvent"<<endl;
	if ( (m_style==KMultiTabBar::KDEV3) ||
		(m_style==KMultiTabBar::KDEV3ICON) ){
		box->setGeometry(0,0,width(),height());
		int lines=1;
		int space;
		int tmp=0;
		if ((m_position==KMultiTabBar::DockBottom) || (m_position==KMultiTabBar::DockTop))
			space=width();
		else
			space=height();

		int cnt=0;
//CALCULATE LINES
	        for (csize i=0;i<m_tabs.count();i++) {
			cnt++;
			tmp+=m_tabs.at(i)->neededSize();
			if (tmp>space) {
				if (cnt>1)i--;
				cnt=0;
				tmp=0;
				lines++;
			}
		}
//SET SIZE & PLACE
		if ((m_position==KMultiTabBar::DockBottom) || (m_position==KMultiTabBar::DockTop)) {
			setFixedHeight(lines*24);
			box->setFixedHeight(lines*24);
			tmp=0;
			cnt=0;
			m_lines=lines;
			lines=0;
		        for (csize i=0;i<m_tabs.count();i++) {
				cnt++;
				tmp+=m_tabs.at(i)->neededSize();
				if (tmp>space) {
					if (cnt>1) i--;
					else
						m_tabs.at(i)->move(tmp-m_tabs.at(i)->neededSize(),lines*24);
					cnt=0;
					tmp=0;
					lines++;
				} else 	m_tabs.at(i)->move(tmp-m_tabs.at(i)->neededSize(),lines*24);
			}
		}
		else {
			setFixedWidth(lines*24);
			box->setFixedWidth(lines*24);
			tmp=0;
			cnt=0;
			m_lines=lines;
			lines=0;
		        for (csize i=0;i<m_tabs.count();i++) {
				cnt++;
				tmp+=m_tabs.at(i)->neededSize();
				if (tmp>space) {
					if (cnt>1) i--;
					else
						m_tabs.at(i)->move(lines*24,tmp-m_tabs.at(i)->neededSize());
					cnt=0;
					tmp=0;
					lines++;
				} else 	m_tabs.at(i)->move(lines*24,tmp-m_tabs.at(i)->neededSize());
			}
		}


		//kdDebug()<<"needed lines:"<<m_lines<<endl;
	}
	if (ev) Q3ScrollView::resizeEvent(ev);
}


void KMultiTabBarInternal::showActiveTabTexts(bool show)
{
	m_showActiveTabTexts=show;
}


KMultiTabBarTab* KMultiTabBarInternal::tab(int id) const
{
#ifdef QT4
	for (QListIterator<KMultiTabBarTab *> it(m_tabs);it.hasNext();it.next()){
		if (it.peekNext()->id()==id) return it.peekNext();
	}
#else
	for (QPtrListIterator<KMultiTabBarTab> it(m_tabs);it.current();++it){
		if (it.current()->id()==id) return it.current();
	}
#endif
        return 0;
}


int KMultiTabBarInternal::appendTab(const QPixmap &pic ,int id,const QString& text)
{
	KMultiTabBarTab  *tab;
	m_tabs.append(tab= new KMultiTabBarTab(pic,text,id,box,m_position,m_style));
	tab->showActiveTabText(m_showActiveTabTexts);
	if (m_style==KMultiTabBar::KONQSBC)
	{
		if (m_expandedTabSize<tab->neededSize()) {
			m_expandedTabSize=tab->neededSize();
			for (csize i=0;i<m_tabs.count();i++)
				m_tabs.at(i)->setSize(m_expandedTabSize);

		} else tab->setSize(m_expandedTabSize);
	} else tab->updateState();
	tab->show();
	return 0;
}

void KMultiTabBarInternal::removeTab(int id)
{
	for (csize pos=0;pos<m_tabs.count();pos++)
	{
		if (m_tabs.at(pos)->id()==id)
		{
			delete m_tabs.at(pos);
#ifdef QT4
			m_tabs.removeAt(pos);
#else
			m_tabs.remove(pos);
#endif
			resizeEvent(0);
			break;
		}
	}
}

void KMultiTabBarInternal::setPosition(enum KMultiTabBar::KMultiTabBarPosition pos)
{
	m_position=pos;
	for (csize i=0;i<m_tabs.count();i++)
		m_tabs.at(i)->setTabsPosition(m_position);
	viewport()->repaint();
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
	toolTip::add( this, text );
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
	toolTip::add(this,text);
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
	toolTip::add(this,text);
}

void KMultiTabBarButton::slotClicked()
{
	emit clicked(m_id);
}

void KMultiTabBarButton::setPosition(KMultiTabBar::KMultiTabBarPosition pos)
{
	m_position=pos;
	repaint();
}

void KMultiTabBarButton::setStyle(KMultiTabBar::KMultiTabBarStyle style)
{
	m_style=style;
	repaint();
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
    ensurePolished();

    int w = 0, h = 0;

    // calculate contents size...
#ifndef QT_NO_ICONSET
#ifndef QT4
    if ( iconSet() && !iconSet()->isNull() ) {
        int iw = iconSet()->pixmap( QIcon::Small, QIcon::Normal ).width() + 4;
        int ih = iconSet()->pixmap( QIcon::Small, QIcon::Normal ).height();
        w += iw;
        h = tMax( h, ih );
    }
#endif
#endif
#ifdef QT4
    QStyleOptionButton sob;
    if ( menu() != 0 )
        w += style()->pixelMetric(QStyle::PM_MenuButtonIndicator, &sob, this);
#else
    if( isMenuButton() )
        w += style().pixelMetric(QStyle::PM_MenuButtonIndicator, this);
#endif

#ifdef QT4
    if ( !icon().isNull() ) {
	const QPixmap & pm = icon().pixmap( QSize( 16, 16 ) );
        w += pm.width();
        h += pm.height();
#else
    if ( pixmap() ) {
        QPixmap *pm = (QPixmap *)pixmap();
        w += pm->width();
        h += pm->height();
#endif
    } else {
        QString s( text() );
        bool empty = s.isEmpty();
        if ( empty )
            s = QString::fromLatin1("XXXX");
        QFontMetrics fm = fontMetrics();
        QSize sz = fm.size( Qt::TextShowMnemonic, s );
        if(!empty || !w)
            w += sz.width();
        if(!empty || !h)
            h = tMax(h, sz.height());
    }
#ifdef QT4
    return (style()->sizeFromContents(QStyle::CT_ToolButton, &sob, QSize(w, h), this).
            expandedTo(QApplication::globalStrut()));
#else
    return (style().sizeFromContents(QStyle::CT_ToolButton, this, QSize(w, h)).
            expandedTo(QApplication::globalStrut()));
#endif
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
}

KMultiTabBarTab::~KMultiTabBarTab() {
	delete d;
}


void KMultiTabBarTab::setTabsPosition(KMultiTabBar::KMultiTabBarPosition pos)
{
	if ((pos!=m_position) && ((pos==KMultiTabBar::DockLeft) || (pos==KMultiTabBar::DockRight))) {
		if (!d->pix.isNull()) {
			QMatrix temp;// (1.0F, 0.0F, 0.0F, -1.0F, 0.0F, 0.0F);
			temp.rotate(180);
			d->pix=d->pix.transformed(temp);
			setIcon(d->pix);
		}
	}

	setPosition(pos);
//	repaint();
}

void KMultiTabBarTab::setIcon(const QString& icon)
{
	//QPixmap pic=SmallIcon(icon);
	QPixmap pic (icon);
	setIcon(pic);
}

void KMultiTabBarTab::setIcon(const QPixmap& icon)
{

	if (m_style!=KMultiTabBar::KDEV3) {
		if ((m_position==KMultiTabBar::DockLeft) || (m_position==KMultiTabBar::DockRight)) {
		        QMatrix rotateMatrix;
			if (m_position==KMultiTabBar::DockLeft)
		        	rotateMatrix.rotate(90);
			else
				rotateMatrix.rotate(-90);
			QPixmap pic=icon.transformed(rotateMatrix);
			d->pix=pic;
#ifdef QT4
		        QPushButton::setIcon(pic);
#else
		        setIconSet(pic);
#endif
		} else
#ifdef QT4
		        QPushButton::setIcon(icon);
#else
		        setIconSet(icon);
#endif
	}
}

void KMultiTabBarTab::slotClicked()
{
	updateState();
	KMultiTabBarButton::slotClicked();
}

void KMultiTabBarTab::setState(bool b)
{
#ifdef QT4
	setChecked(b);
#else
	setOn(b);
#endif
	updateState();
}

void KMultiTabBarTab::updateState()
{

	if (m_style!=KMultiTabBar::KONQSBC) {
		if ((m_style==KMultiTabBar::KDEV3) || (m_style==KMultiTabBar::KDEV3ICON) || (isChecked())) {
			QPushButton::setText(m_text);
		} else {
			//kdDebug()<<"KMultiTabBarTab::updateState(): setting text to an empty QString***************"<<endl;
			QPushButton::setText(QString::null);
		}

		if ((m_position==KMultiTabBar::DockRight || m_position==KMultiTabBar::DockLeft)) {
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
                if ((m_position==KMultiTabBar::DockRight || m_position==KMultiTabBar::DockLeft))
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
	if ((m_style==KMultiTabBar::KDEV3) || (m_style==KMultiTabBar::KDEV3ICON) || (isChecked()))
		 sh=KMultiTabBarButton::sizeHint();
	else
		sh=QSize(width,height);

	QPixmap pixmap( sh.width(),height); ///,sh.height());
#ifdef QT4
	pixmap.fill(backgroundRole());
#else
	pixmap.fill(eraseColor());
#endif
	QPainter painter(&pixmap);


	QStyle::State st=QStyle::State_None;

	st|=QStyle::State_Enabled;

	if (isChecked()) st|=QStyle::State_On;

#ifndef QT4
	// TODO: port to Qt4!!!!!!!!
	style().drawControl(QStyle::CE_PushButton,&painter,this, QRect(0,0,pixmap.width(),pixmap.height()), colorGroup(),st);
	style().drawControl(QStyle::CE_PushButtonLabel,&painter,this, QRect(0,0,pixmap.width(),pixmap.height()), colorGroup(),st);
#endif

	switch (m_position) {
		case KMultiTabBar::DockLeft:
			paint->rotate(-90);
			paint->drawPixmap(1-pixmap.width(),0,pixmap);
			break;
		case KMultiTabBar::DockRight:
			paint->rotate(90);
			paint->drawPixmap(0,1-pixmap.height(),pixmap);
			break;

		default:
			paint->drawPixmap(0,0,pixmap);
			break;
	}
//	style().drawControl(QStyle::CE_PushButtonLabel,painter,this, QRect(0,0,pixmap.width(),pixmap.height()),
//		colorGroup(),QStyle::Style_Enabled);


}

void KMultiTabBarTab::drawButtonClassic(QPainter *paint)
{
        QPixmap pixmap;
	//if ( iconSet())
        //	pixmap = iconSet()->pixmap( QIcon::Small, QIcon::Normal );
#ifdef QT4
	if( !icon().isNull() )
	{
		pixmap = icon().pixmap( 16, 16 );
	}
#else
	if( iconSet() )
		pixmap = iconSet()->pixmap( QIconSet::Small, QIconSet::Normal );
#endif

	paint->fillRect(0, 0, 24, 24, palette().background()
#ifdef QT4
			.color()
#endif
			);

	if (!isChecked())
	{

		if (m_position==KMultiTabBar::DockRight)
		{
			paint->fillRect(0,0,21,21,QBrush(palette().background()
#ifdef QT4
					.color()
#endif
					));

			paint->setPen(palette().background()
#ifdef QT4
					.color()
#endif
					.dark(150));
			paint->drawLine(0,22,23,22);

			paint->drawPixmap(12-pixmap.width()/2,12-pixmap.height()/2,pixmap);

			paint->setPen(palette().shadow()
#ifdef QT4
					.color()
#endif
					);
			paint->drawLine(0,0,0,23);
			paint->setPen(palette().background()
#ifdef QT4
					.color()
#endif
					.dark(120));
			paint->drawLine(1,0,1,23);

		}
		else
		if ((m_position==KMultiTabBar::DockBottom) || (m_position==KMultiTabBar::DockTop))
		{
                        paint->fillRect(0,1,23,22,QBrush(palette().background()
#ifdef QT4
					.color()
#endif
					));

                        paint->drawPixmap(12-pixmap.width()/2,12-pixmap.height()/2,pixmap);

                        paint->setPen(palette().background()
#ifdef QT4
					.color()
#endif
					.dark(120));
                        paint->drawLine(23,0,23,23);


                        paint->setPen(palette().light()
#ifdef QT4
					.color()
#endif
					);
                        paint->drawLine(0,22,23,22);
                        paint->drawLine(0,23,23,23);
                	paint->setPen(palette().shadow()
#ifdef QT4
					.color()
#endif
					);
                	paint->drawLine(0,0,23,0);
                        paint->setPen(palette().background()
#ifdef QT4
					.color()
#endif
					.dark(120));
                        paint->drawLine(0,1,23,1);

		}
		else
		{
			paint->setPen(palette().background()
#ifdef QT4
					.color()
#endif
					.dark(120));
			paint->drawLine(0,23,23,23);
			paint->fillRect(0,0,23,21,QBrush(palette().background()
#ifdef QT4
					.color()
#endif
					));
			paint->drawPixmap(12-pixmap.width()/2,12-pixmap.height()/2,pixmap);

			paint->setPen(palette().light()
#ifdef QT4
					.color()
#endif
					);
			paint->drawLine(23,0,23,23);
			paint->drawLine(22,0,22,23);

			paint->setPen(palette().shadow()
#ifdef QT4
.color()
#endif
					);
			paint->drawLine(0,0,0,23);

		}


	}
	else
	{
		if (m_position==KMultiTabBar::DockRight)
		{
			paint->setPen(palette().shadow()
#ifdef QT4
					.color()
#endif
					);
			paint->drawLine(0,height()-1,23,height()-1);
			paint->drawLine(0,height()-2,23,height()-2);
			paint->drawLine(23,0,23,height()-1);
			paint->drawLine(22,0,22,height()-1);
			paint->fillRect(0,0,21,height()-3,QBrush(palette().light()
#ifdef QT4
					.color()
#endif
					));
			paint->drawPixmap(10-pixmap.width()/2,10-pixmap.height()/2,pixmap);

			if (m_showActiveTabText)
			{
				if (height()<25+4) return;

				QPixmap tpixmap(height()-25-3, width()-2);
				QPainter painter(&tpixmap);

				painter.fillRect(0,0,tpixmap.width(),tpixmap.height(),QBrush(palette().light()
#ifdef QT4
							.color()
#endif
							));

				painter.setPen(palette().text()
#ifdef QT4
						.color()
#endif
						);
				painter.drawText(0,+width()/2+QFontMetrics(QFont()).height()/2,m_text);

				paint->rotate(90);
				//kdDebug()<<"tpixmap.width:"<<tpixmap.width()<<endl;
				paint->drawPixmap(25,-tpixmap.height()+1,tpixmap);
			}

		}
		else
		if (m_position==KMultiTabBar::DockTop)
		{
			paint->fillRect(0,0,width()-1,23,QBrush(palette().light()
#ifdef QT4
					.color()
#endif
					));
			paint->drawPixmap(10-pixmap.width()/2,10-pixmap.height()/2,pixmap);
			if (m_showActiveTabText)
			{
				paint->setPen(palette().text()
#ifdef QT4
						.color()
#endif
						);
				paint->drawText(25,height()/2+QFontMetrics(QFont()).height()/2,m_text);
			}
		}
		else
		if (m_position==KMultiTabBar::DockBottom)
		{
			paint->setPen(palette().shadow()
#ifdef QT4
					.color()
#endif
					);
			paint->drawLine(0,23,width()-1,23);
			paint->drawLine(0,22,width()-1,22);
			paint->fillRect(0,0,width()-1,21,QBrush(palette().light()
#ifdef QT4
					.color()
#endif
					));
			paint->drawPixmap(10-pixmap.width()/2,10-pixmap.height()/2,pixmap);
			if (m_showActiveTabText)
			{
				paint->setPen(palette().text()
#ifdef QT4
						.color()
#endif
						);
				paint->drawText(25,height()/2+QFontMetrics(QFont()).height()/2,m_text);
			}

		}
		else
		{


			paint->setPen(palette().shadow()
#ifdef QT4
					.color()
#endif
					);
			paint->drawLine(0,height()-1,23,height()-1);
			paint->drawLine(0,height()-2,23,height()-2);
			paint->fillRect(0,0,23,height()-3,QBrush(palette().light()
#ifdef QT4
					.color()
#endif
					));
			paint->drawPixmap(10-pixmap.width()/2,10-pixmap.height()/2,pixmap);
			if (m_showActiveTabText)
			{

		       		if (height()<25+4) return;

                                QPixmap tpixmap(height()-25-3, width()-2);
                                QPainter painter(&tpixmap);

                                painter.fillRect(0,0,tpixmap.width(),tpixmap.height(),QBrush(palette().light()
#ifdef QT4
							.color()
#endif
							));

                                painter.setPen(palette().text()
#ifdef QT4
						.color()
#endif
						);
                                painter.drawText(tpixmap.width()-QFontMetrics(QFont()).width(m_text),+width()/2+QFontMetrics(QFont()).height()/2,m_text);

                                paint->rotate(-90);
                                //kdDebug()<<"tpixmap.width:"<<tpixmap.width()<<endl;

				paint->drawPixmap(-24-tpixmap.width(),2,tpixmap);

			}

		}

	}
}







KMultiTabBar::KMultiTabBar( Qt::Orientation o, QWidget *parent):QWidget(parent)
{
#ifndef QT4
	m_buttons.setAutoDelete(false);
#endif
	if (o==Qt::Vertical)
	{
		m_l=new QVBoxLayout(this);
		setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding
#ifndef QT4
				, true
#endif
				));
//		setFixedWidth(24);
	}
	else
	{
		m_l=new QHBoxLayout(this);
		setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed
#ifndef QT4
				, true
#endif
				));
//		setFixedHeight(24);
	}
	m_l->setMargin(0);
#ifndef QT4
	m_l->setAutoAdd(false);
#endif
	m_internal=new KMultiTabBarInternal(this,o);
	setPosition((o==Qt::Vertical)?KMultiTabBar::DockRight:KMultiTabBar::DockBottom);
	setStyle(VSNET);
	//	setStyle(KDEV3);
	//setStyle(KONQSBC);
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

/*int KMultiTabBar::insertButton(QPixmap pic,int id ,const QString&)
{
  (new KToolbarButton(pic,id,m_internal))->show();
  return 0;
}*/

int KMultiTabBar::appendButton(const QPixmap &pic ,int id,QMenu *popup,const QString&)
{
	KMultiTabBarButton  *btn;
	m_buttons.append(btn= new KMultiTabBarButton(pic,QString::null,
			popup,id,this,m_position,m_internal->m_style));
	m_l->insertWidget(0,btn);
	btn->show();
	m_btnTabSep->show();
	return 0;
}

void KMultiTabBar::updateSeparator() {
	bool hideSep=true;
#ifdef QT4
	for (QListIterator<KMultiTabBarButton *> it(m_buttons);it.hasNext();){
		if (it.next()->isVisibleTo(this)) {
			hideSep=false;
			break;
		}
	}
#else
	for (QPtrListIterator<KMultiTabBarButton> it(m_buttons);it.current();++it){
		if (it.current()->isVisibleTo(this)) {
			hideSep=false;
			break;
		}
	}
#endif
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
#ifdef QT4
	for (QListIterator<KMultiTabBarButton *> it(m_buttons);it.hasNext();it.next()){
		if (it.peekNext()->id()==id) return it.peekNext();
	}
#else
	for (QPtrListIterator<KMultiTabBarButton> it(m_buttons);it.current();++it){
		if (it.current()->id()==id) return it.current();
	}
#endif
        return 0;
}

KMultiTabBarTab* KMultiTabBar::tab(int id) const
{
	return m_internal->tab(id);
}



void KMultiTabBar::removeButton(int id)
{
	for (csize pos=0;pos<m_buttons.count();pos++)
	{
		if (m_buttons.at(pos)->id()==id)
		{
			m_buttons.at(pos)->deleteLater();
#ifdef QT4
			m_buttons.removeAt(pos);
#else
			m_buttons.remove(pos);
#endif
			//m_buttons.take(pos)->deleteLater();
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

void KMultiTabBar::setPosition(KMultiTabBarPosition pos)
{
	m_position=pos;
	m_internal->setPosition(pos);
	for (csize i=0;i<m_buttons.count();i++)
		m_buttons.at(i)->setPosition(pos);
}
void KMultiTabBar::fontChange(const QFont& /* oldFont */)
{
	for (csize i=0;i<tabs()->count();i++)
		tabs()->at(i)->resize();
	repaint();
}

tabList * KMultiTabBar::tabs() {return m_internal->tabs();}
buttonList * KMultiTabBar::buttons() {return &m_buttons;}

