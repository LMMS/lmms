/*
 * VstEffectControlDialog.cpp - dialog for displaying VST-effect GUI
 *
 * Copyright (c) 2006-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtGui/QLayout>
#include <QtGui/QMdiArea>
#include <QtGui/QPushButton>

#include "VstEffectControlDialog.h"
#include "VstEffect.h"



VstEffectControlDialog::VstEffectControlDialog( VstEffectControls * _ctl ) :
	EffectControlDialog( _ctl ),
	m_pluginWidget( NULL )
{
	QVBoxLayout * l = new QVBoxLayout( this );
	l->setMargin( 0 );
	l->setSpacing( 0 );

#ifdef LMMS_BUILD_LINUX
	_ctl->m_effect->m_plugin->showEditor();
	m_pluginWidget = _ctl->m_effect->m_plugin->pluginWidget();
	if( m_pluginWidget )
	{
		setWindowTitle( m_pluginWidget->windowTitle() );
		QPushButton * btn = new QPushButton( tr( "Show/hide VST FX GUI" ) );
		btn->setCheckable( true );
		l->addWidget( btn );
		connect( btn, SIGNAL( toggled( bool ) ),
					m_pluginWidget, SLOT( setVisible( bool ) ) );
	}
#endif
#ifdef LMMS_BUILD_WIN32
	_ctl->m_effect->m_plugin->showEditor( this );
	QWidget * w = _ctl->m_effect->m_plugin->pluginWidget( false );
	if( w )
	{
		setWindowTitle( w->windowTitle() );
		l->addWidget( w );
	}
#endif
}




VstEffectControlDialog::~VstEffectControlDialog()
{
	delete m_pluginWidget;
}


