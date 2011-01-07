/*
 * EffectRackView.cpp - view for effectChain model
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn@netscape.net>
 * Copyright (c) 2008-2011 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtGui/QApplication>
#include <QtGui/QLayout>
#include <QtGui/QPushButton>
#include <QtGui/QScrollArea>
#include <QtGui/QVBoxLayout>

#include "EffectRackView.h"
#include "EffectSelectDialog.h"
#include "EffectView.h"
#include "group_box.h"


EffectRackView::EffectRackView( EffectChain * _model, QWidget * _parent ) :
	QWidget( _parent ),
	ModelView( NULL, this )
{
	setFixedSize( 250, 250 );

	m_mainLayout = new QVBoxLayout( this );
	m_mainLayout->setSpacing( 0 );
	m_mainLayout->setMargin( 5 );

	m_effectsGroupBox = new groupBox( tr( "EFFECTS CHAIN" ) );
	m_mainLayout->addWidget( m_effectsGroupBox );

	m_scrollArea = new QScrollArea( m_effectsGroupBox );
	m_scrollArea->setFixedSize( 230, 184 );
	m_scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
	m_scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_scrollArea->setPalette( QApplication::palette( m_scrollArea ) );
	m_scrollArea->setWidget( new QWidget );
	m_scrollArea->move( 6, 22 );

	QPushButton * addButton = new QPushButton( m_effectsGroupBox );
	addButton->setText( tr( "Add effect" ) );
	addButton->move( 8, 210 );
	connect( addButton, SIGNAL( clicked() ), this, SLOT( addEffect() ) );


	m_lastY = 0;

	setModel( _model );
}



EffectRackView::~EffectRackView()
{
	clearViews();
}





void EffectRackView::clearViews()
{
	for( QVector<EffectView *>::Iterator it = m_effectViews.begin();
					it != m_effectViews.end(); ++it )
	{
		delete *it;
	}
	m_effectViews.clear();
}




void EffectRackView::moveUp( EffectView * _view )
{
	fxChain()->moveUp( _view->effect() );
	if( _view != m_effectViews.first() )
	{
		int i = 0;
		for( QVector<EffectView *>::Iterator it = m_effectViews.begin(); 
					it != m_effectViews.end(); it++, i++ )
		{
			if( *it == _view )
			{
				break;
			}
		}
		
		EffectView * temp = m_effectViews[ i - 1 ];
		
		m_effectViews[i - 1] = _view;
		m_effectViews[i] = temp;
		
		update();
	}
}




void EffectRackView::moveDown( EffectView * _view )
{
	if( _view != m_effectViews.last() )
	{
		// moving next effect up is the same
		moveUp( *( qFind( m_effectViews.begin(), m_effectViews.end(), _view ) + 1 ) );
	}
}




void EffectRackView::deletePlugin( EffectView * _view )
{
	Effect * e = _view->effect();
	m_effectViews.erase( qFind( m_effectViews.begin(), m_effectViews.end(),
								_view ) );
	delete _view;
	fxChain()->removeEffect( e );
	e->deleteLater();
	update();
}




void EffectRackView::update()
{
	QWidget * w = m_scrollArea->widget();
	QVector<bool> view_map( qMax<int>( fxChain()->m_effects.size(),
						m_effectViews.size() ), false );

	for( QVector<Effect *>::Iterator it = fxChain()->m_effects.begin();
					it != fxChain()->m_effects.end(); ++it )
	{
		int i = 0;
		for( QVector<EffectView *>::Iterator vit = m_effectViews.begin();
				vit != m_effectViews.end(); ++vit, ++i )
		{
			if( ( *vit )->model() == *it )
			{
				view_map[i] = true;
				break;
			}
		}
		if( i >= m_effectViews.size() )
		{
			EffectView * view = new EffectView( *it, w );
			connect( view, SIGNAL( moveUp( EffectView * ) ), 
					this, SLOT( moveUp( EffectView * ) ) );
			connect( view, SIGNAL( moveDown( EffectView * ) ),
				this, SLOT( moveDown( EffectView * ) ) );
			connect( view, SIGNAL( deletePlugin( EffectView * ) ),
				this, SLOT( deletePlugin( EffectView * ) ),
							Qt::QueuedConnection );
			view->show();
			m_effectViews.append( view );
			view_map[i] = true;

		}
	}

	int i = m_lastY = 0;
	for( QVector<EffectView *>::Iterator it = m_effectViews.begin(); 
					it != m_effectViews.end(); )
	{
		if( i < view_map.size() && i < m_effectViews.size() &&
							view_map[i] == false )
		{
			delete m_effectViews[i];
			it = m_effectViews.erase( it );
		}
		else
		{
			( *it )->move( 0, m_lastY );
			m_lastY += ( *it )->height();
			++it;
			++i;
		}
	}
	w->setFixedSize( 210, m_lastY );

	QWidget::update();
}




void EffectRackView::addEffect()
{
	EffectSelectDialog esd( this );
	esd.exec();

	if( esd.result() == QDialog::Rejected )
	{
		return;
	}

	Effect * fx = esd.instantiateSelectedPlugin( fxChain() );

	fxChain()->m_enabledModel.setValue( true );
	fxChain()->appendEffect( fx );
	update();

	// Find the effectView, and show the controls
	for( QVector<EffectView *>::Iterator vit = m_effectViews.begin();
					vit != m_effectViews.end(); ++vit )
	{
		if( ( *vit )->effect() == fx )
		{
			( *vit )->editControls();

			break;
		}
	}


}




void EffectRackView::modelChanged()
{
	clearViews();
	m_effectsGroupBox->setModel( &fxChain()->m_enabledModel );
	connect( fxChain(), SIGNAL( aboutToClear() ),
			this, SLOT( clearViews() ) );
	update();
}



#include "moc_EffectRackView.cxx"

