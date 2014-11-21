/*
 * VstEffectControlDialog.cpp - dialog for displaying VST-effect GUI
 *
 * Copyright (c) 2006-2011 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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
#include <QtGui/QMenu>
#include <QtGui/QPushButton>

#include "VstEffectControlDialog.h"
#include "VstEffect.h"

#include "pixmap_button.h"
#include "embed.h"
#include "tooltip.h"

#include <QObject>
#include <QtGui/QPainter>
#include "gui_templates.h"
#include <QtGui/QToolBar>
#include <QtGui/QLabel>


VstEffectControlDialog::VstEffectControlDialog( VstEffectControls * _ctl ) :
	EffectControlDialog( _ctl ),
	m_pluginWidget( NULL ),
	m_plugin( NULL ),
	tbLabel( NULL )
{
	QGridLayout * l = new QGridLayout( this );
	l->setContentsMargins( 10, 10, 10, 10 );
	l->setVerticalSpacing( 2 );
	l->setHorizontalSpacing( 2 );

	if( _ctl != NULL && _ctl->m_effect != NULL &&
					_ctl->m_effect->m_plugin != NULL )
	{
		m_plugin = _ctl->m_effect->m_plugin;
		m_plugin->showEditor( NULL, true );
		m_pluginWidget = m_plugin->pluginWidget();

#ifdef LMMS_BUILD_WIN32

		if( !m_pluginWidget )
		{
			m_pluginWidget = m_plugin->pluginWidget( false );
		}
#endif
	}

	if( m_pluginWidget )
	{
		setWindowTitle( m_pluginWidget->windowTitle() );
		setMinimumWidth( 250 );

		QPushButton * btn = new QPushButton( tr( "Show/hide" ) );
		btn->setCheckable( true );
		connect( btn, SIGNAL( toggled( bool ) ),
					m_pluginWidget, SLOT( setVisible( bool ) ) );
		emit btn->click();

		btn->setMinimumWidth( 78 );
		btn->setMaximumWidth( 78 );
		btn->setMinimumHeight( 24 );
		btn->setMaximumHeight( 24 );

		m_managePluginButton = new pixmapButton( this, "" );
		m_managePluginButton->setCheckable( false );
		m_managePluginButton->setCursor( Qt::PointingHandCursor );
		m_managePluginButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"track_op_menu" ) );
		m_managePluginButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"track_op_menu" ) );
		connect( m_managePluginButton, SIGNAL( clicked() ),  _ctl,
						SLOT( managePlugin() ) );
		toolTip::add( m_managePluginButton, tr( "Control VST-plugin from LMMS host" ) );

		m_managePluginButton->setWhatsThis(
			tr( "Click here, if you want to control VST-plugin from host." ) );

		m_managePluginButton->setMinimumWidth( 21 );
		m_managePluginButton->setMaximumWidth( 21 );
		m_managePluginButton->setMinimumHeight( 21 );
		m_managePluginButton->setMaximumHeight( 21 );

		m_openPresetButton = new pixmapButton( this, "" );
		m_openPresetButton->setCheckable( false );
		m_openPresetButton->setCursor( Qt::PointingHandCursor );
		m_openPresetButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"stepper-up-press" ) );
		m_openPresetButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"stepper-up" ) );
		connect( m_openPresetButton, SIGNAL( clicked() ), _ctl,
						SLOT( openPreset() ) );
		toolTip::add( m_openPresetButton, tr( "Open VST-plugin preset" ) );

		m_openPresetButton->setWhatsThis(
			tr( "Click here, if you want to open another *.fxp, *.fxb VST-plugin preset." ) );

		m_openPresetButton->setMinimumWidth( 16 );
		m_openPresetButton->setMaximumWidth( 16 );
		m_openPresetButton->setMinimumHeight( 16 );
		m_openPresetButton->setMaximumHeight( 16 );

		m_rolLPresetButton = new pixmapButton( this, "" );
		m_rolLPresetButton->setCheckable( false );
		m_rolLPresetButton->setCursor( Qt::PointingHandCursor );
		m_rolLPresetButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"stepper-left-press" ) );
		m_rolLPresetButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"stepper-left" ) );
		connect( m_rolLPresetButton, SIGNAL( clicked() ), _ctl,
						SLOT( rolrPreset() ) );

		connect( m_rolLPresetButton, SIGNAL( clicked() ), this,
						SLOT( update() ) );

		toolTip::add( m_rolLPresetButton, tr( "Previous (-)" ) );

		m_rolLPresetButton->setShortcut( Qt::Key_Minus );

		m_rolLPresetButton->setWhatsThis(
			tr( "Click here, if you want to switch to another VST-plugin preset program." ) );

		m_rolLPresetButton->setMinimumWidth( 16 );
		m_rolLPresetButton->setMaximumWidth( 16 );
		m_rolLPresetButton->setMinimumHeight( 16 );
		m_rolLPresetButton->setMaximumHeight( 16 );

		m_rolRPresetButton = new pixmapButton( this, "" );
		m_rolRPresetButton->setCheckable( false );
		m_rolRPresetButton->setCursor( Qt::PointingHandCursor );
		m_rolRPresetButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"stepper-right-press" ) );
		m_rolRPresetButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"stepper-right" ) );
		connect( m_rolRPresetButton, SIGNAL( clicked() ), _ctl,
						SLOT( rollPreset() ) );

		connect( m_rolRPresetButton, SIGNAL( clicked() ), this,
						SLOT( update() ) );

		toolTip::add( m_rolRPresetButton, tr( "Next (+)" ) );

		m_rolRPresetButton->setShortcut( Qt::Key_Plus );

		m_rolRPresetButton->setWhatsThis(
			tr( "Click here, if you want to switch to another VST-plugin preset program." ) );

		m_rolRPresetButton->setMinimumWidth( 16 );
		m_rolRPresetButton->setMaximumWidth( 16 );
		m_rolRPresetButton->setMinimumHeight( 16 );
		m_rolRPresetButton->setMaximumHeight( 16 );

 		_ctl->m_selPresetButton = new QPushButton( tr( "" ), this );

		_ctl->m_selPresetButton->setCheckable( false );
		_ctl->m_selPresetButton->setCursor( Qt::PointingHandCursor );
		_ctl->m_selPresetButton->setIcon( PLUGIN_NAME::getIconPixmap( "stepper-down" ) );
		_ctl->m_selPresetButton->setWhatsThis(
			tr( "Click here to select presets that are currently loaded in VST." ) );

 		_ctl->m_selPresetButton->setMenu(_ctl->menu);

		_ctl->m_selPresetButton->setMinimumWidth( 16 );
		_ctl->m_selPresetButton->setMaximumWidth( 16 );
		_ctl->m_selPresetButton->setMinimumHeight( 16 );
		_ctl->m_selPresetButton->setMaximumHeight( 16 );

		m_savePresetButton = new pixmapButton( this, "" );
		m_savePresetButton->setCheckable( false );
		m_savePresetButton->setCursor( Qt::PointingHandCursor );
		m_savePresetButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"project_save", 21, 21  ) );
		m_savePresetButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"project_save", 21, 21  ) );
		connect( m_savePresetButton, SIGNAL( clicked() ), _ctl,
						SLOT( savePreset() ) );
		toolTip::add( m_savePresetButton, tr( "Save preset" ) );

		m_savePresetButton->setWhatsThis(
			tr( "Click here, if you want to save current VST-plugin preset program." ) );

		m_savePresetButton->setMinimumWidth( 21 );
		m_savePresetButton->setMaximumWidth( 21 );
		m_savePresetButton->setMinimumHeight( 21 );
		m_savePresetButton->setMaximumHeight( 21 );

		int newSize = m_pluginWidget->width() + 20;
		newSize = (newSize < 250) ? 250 : newSize;
		QWidget* resize = new QWidget(this);
		resize->resize( newSize, 10 );
		QWidget* space0 = new QWidget(this);
		space0->resize(8, 10);
		QWidget* space1 = new QWidget(this);
		space1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
		QFont f( "Arial", 10 );

		l->addItem( new QSpacerItem( newSize - 20, 30, QSizePolicy::Fixed, 
						QSizePolicy::Fixed ), 1, 0 );
		l->addWidget( resize, 2, 0, 1, 1, Qt::AlignCenter );
		l->addWidget( m_pluginWidget, 3, 0, 1, 1, Qt::AlignCenter );
		l->setRowStretch( 5, 1 );
		l->setColumnStretch( 1, 1 );

		QToolBar * tb = new QToolBar( this );
		tb->resize( newSize , 32 );
		tb->addWidget(space0);
		tb->addWidget( m_rolLPresetButton );
		tb->addWidget( m_rolRPresetButton );
		tb->addWidget( _ctl->m_selPresetButton );
		tb->addWidget( m_openPresetButton );
		tb->addWidget( m_savePresetButton );
		tb->addWidget( m_managePluginButton );
		tb->addWidget( btn );
		tb->addWidget(space1);

		tbLabel = new QLabel( tr( "Effect by: " ), this );
		tbLabel->setFont( pointSize<7>( f ) );
		tbLabel->setTextFormat(Qt::RichText);
		tbLabel->setAlignment( Qt::AlignTop | Qt::AlignLeft );
		tb->addWidget( tbLabel );
	}
}




void VstEffectControlDialog::paintEvent( QPaintEvent * )
{
	if( m_plugin != NULL && tbLabel != NULL )
	{
		tbLabel->setText( tr( "Effect by: " ) + m_plugin->vendorString() + 
			tr( "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<br />" ) + 
			m_plugin->currentProgramName() );
	}
}




VstEffectControlDialog::~VstEffectControlDialog()
{
	//delete m_pluginWidget;
}

#include "moc_VstEffectControlDialog.cxx"
