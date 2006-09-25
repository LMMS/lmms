/*
 * vst_control_dialog.cpp - dialog for displaying GUI of VST-effect-plugin
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifdef QT4

#include <QtGui/QMessageBox>
#include <QtGui/QLayout>

#else

#include <qmessagebox.h>
#include <qlayout.h>

#endif

#include "vst_effect.h"


vstControlDialog::vstControlDialog( QWidget * _parent, 
						vstEffect * _eff ) :
	effectControlDialog( _parent, _eff ),
	m_effect( _eff )
{
	QVBoxLayout * l = new QVBoxLayout( this );
	QWidget * pw = m_effect->m_plugin->pluginWidget();
	if( pw )
	{
		pw->reparent( this, QPoint( 0, 0 ) );
		pw->show();
		l->addWidget( pw );
	}
}




vstControlDialog::~vstControlDialog()
{
}




void FASTCALL vstControlDialog::loadSettings( const QDomElement & _this )
{
	m_effect->closePlugin();
	m_effect->openPlugin( _this.attribute( "plugin" ) );
	m_effect->m_pluginMutex.lock();
	if( m_effect->m_plugin != NULL )
	{
		m_effect->m_plugin->loadSettings( _this );
	}
	m_effect->m_pluginMutex.unlock();
}




void FASTCALL vstControlDialog::saveSettings( QDomDocument & _doc, 
							QDomElement & _this )
{
	_this.setAttribute( "plugin", m_effect->m_key.user.toString() );
	m_effect->m_pluginMutex.lock();
	if( m_effect->m_plugin != NULL )
	{
		m_effect->m_plugin->saveSettings( _doc, _this );
	}
	m_effect->m_pluginMutex.unlock();
}




