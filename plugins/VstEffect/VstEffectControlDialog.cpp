/*
 * VstEffectControlDialog.cpp - dialog for displaying VST-effect GUI
 *
 * Copyright (c) 2006-2011 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QLayout>
#include <QMenu>
#include <QPushButton>

#include "VstEffectControlDialog.h"
#include "VstEffect.h"
#include "VstPlugin.h"

#include "PixmapButton.h"
#include "embed.h"

#include "gui_templates.h"
#include <QToolBar>
#include <QLabel>

namespace lmms::gui
{


VstEffectControlDialog::VstEffectControlDialog( VstEffectControls * _ctl ) :
	EffectControlDialog( _ctl ),
	m_pluginWidget( nullptr ),

	m_plugin( nullptr ),
	tbLabel( nullptr )
{
#if QT_VERSION < 0x50C00
	// Workaround for a bug in Qt versions below 5.12,
	// where argument-dependent-lookup fails for QFlags operators
	// declared inside a namepsace.
	// This affects the Q_DECLARE_OPERATORS_FOR_FLAGS macro in Instrument.h
	// See also: https://codereview.qt-project.org/c/qt/qtbase/+/225348

	using ::operator|;
#endif

	auto l = new QGridLayout(this);
	l->setContentsMargins( 10, 10, 10, 10 );
	l->setVerticalSpacing( 2 );
	l->setHorizontalSpacing( 2 );

	bool embed_vst = false;

	if( _ctl != nullptr && _ctl->m_effect != nullptr &&
					_ctl->m_effect->m_plugin != nullptr )
	{
		m_plugin = _ctl->m_effect->m_plugin;
		embed_vst = m_plugin->embedMethod() != WindowEmbed::Method::Floating;

		if (embed_vst) {
			if (m_plugin->hasEditor() && ! m_plugin->pluginWidget()) {
				m_plugin->createUI(this);
			}
			m_pluginWidget = m_plugin->pluginWidget();
		}
	}

	if (m_plugin)
	{
		setWindowTitle( m_plugin->name() );

		auto btn = new QPushButton(tr("Show/hide"));

		if (embed_vst) {
			btn->setCheckable( true );
			btn->setChecked( true );
			connect( btn, SIGNAL( toggled( bool ) ),
						SLOT( togglePluginUI( bool ) ) );
		} else {
			connect( btn, SIGNAL( clicked() ),
						m_plugin.data(), SLOT( toggleUI() ) );
		}

		btn->setMinimumWidth( 78 );
		btn->setMaximumWidth( 78 );
		btn->setMinimumHeight( 24 );
		btn->setMaximumHeight( 24 );
		m_togglePluginButton = btn;

		m_managePluginButton = new PixmapButton( this, "" );
		m_managePluginButton->setCheckable( false );
		m_managePluginButton->setCursor( Qt::PointingHandCursor );
		m_managePluginButton->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"controls_active" ) );
		m_managePluginButton->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"controls" ) );
		connect( m_managePluginButton, SIGNAL( clicked() ),  _ctl,
						SLOT( managePlugin() ) );
		m_managePluginButton->setToolTip(tr("Control VST plugin from LMMS host"));

		m_managePluginButton->setMinimumWidth( 26 );
		m_managePluginButton->setMaximumWidth( 26 );
		m_managePluginButton->setMinimumHeight( 21 );
		m_managePluginButton->setMaximumHeight( 21 );

		m_openPresetButton = new PixmapButton( this, "" );
		m_openPresetButton->setCheckable( false );
		m_openPresetButton->setCursor( Qt::PointingHandCursor );
		m_openPresetButton->setActiveGraphic( embed::getIconPixmap(
							"project_open" ) );
		m_openPresetButton->setInactiveGraphic( embed::getIconPixmap(
							"project_open" ) );
		connect( m_openPresetButton, SIGNAL( clicked() ), _ctl,
						SLOT( openPreset() ) );
		m_openPresetButton->setToolTip(tr("Open VST plugin preset"));

		m_openPresetButton->setMinimumWidth( 16 );
		m_openPresetButton->setMaximumWidth( 16 );
		m_openPresetButton->setMinimumHeight( 16 );
		m_openPresetButton->setMaximumHeight( 16 );

		m_rolLPresetButton = new PixmapButton( this, "" );
		m_rolLPresetButton->setCheckable( false );
		m_rolLPresetButton->setCursor( Qt::PointingHandCursor );
		m_rolLPresetButton->setActiveGraphic( embed::getIconPixmap(
							"stepper-left-press" ) );
		m_rolLPresetButton->setInactiveGraphic( embed::getIconPixmap(
							"stepper-left" ) );
		connect( m_rolLPresetButton, SIGNAL( clicked() ), _ctl,
						SLOT( rolrPreset() ) );

		connect( m_rolLPresetButton, SIGNAL( clicked() ), this,
						SLOT( update() ) );

		m_rolLPresetButton->setToolTip(tr("Previous (-)"));

		m_rolLPresetButton->setShortcut( Qt::Key_Minus );

		m_rolLPresetButton->setMinimumWidth( 16 );
		m_rolLPresetButton->setMaximumWidth( 16 );
		m_rolLPresetButton->setMinimumHeight( 16 );
		m_rolLPresetButton->setMaximumHeight( 16 );

		m_rolRPresetButton = new PixmapButton( this, "" );
		m_rolRPresetButton->setCheckable( false );
		m_rolRPresetButton->setCursor( Qt::PointingHandCursor );
		m_rolRPresetButton->setActiveGraphic( embed::getIconPixmap(
							"stepper-right-press" ) );
		m_rolRPresetButton->setInactiveGraphic( embed::getIconPixmap(
							"stepper-right" ) );
		connect( m_rolRPresetButton, SIGNAL( clicked() ), _ctl,
						SLOT( rollPreset() ) );

		connect( m_rolRPresetButton, SIGNAL( clicked() ), this,
						SLOT( update() ) );

		m_rolRPresetButton->setToolTip(tr("Next (+)"));

		m_rolRPresetButton->setShortcut( Qt::Key_Plus );

		m_rolRPresetButton->setMinimumWidth( 16 );
		m_rolRPresetButton->setMaximumWidth( 16 );
		m_rolRPresetButton->setMinimumHeight( 16 );
		m_rolRPresetButton->setMaximumHeight( 16 );

 		_ctl->m_selPresetButton = new QPushButton( tr( "" ), this );

		_ctl->m_selPresetButton->setCheckable( false );
		_ctl->m_selPresetButton->setCursor( Qt::PointingHandCursor );
		_ctl->m_selPresetButton->setIcon( embed::getIconPixmap( "stepper-down" ) );

		auto menu = new QMenu;
		connect( menu, SIGNAL( aboutToShow() ), _ctl, SLOT( updateMenu() ) );

 		_ctl->m_selPresetButton->setMenu(menu);

		_ctl->m_selPresetButton->setMinimumWidth( 16 );
		_ctl->m_selPresetButton->setMaximumWidth( 16 );
		_ctl->m_selPresetButton->setMinimumHeight( 16 );
		_ctl->m_selPresetButton->setMaximumHeight( 16 );

		m_savePresetButton = new PixmapButton( this, "" );
		m_savePresetButton->setCheckable( false );
		m_savePresetButton->setCursor( Qt::PointingHandCursor );
		m_savePresetButton->setActiveGraphic( embed::getIconPixmap(
							"project_save", 21, 21  ) );
		m_savePresetButton->setInactiveGraphic( embed::getIconPixmap(
							"project_save", 21, 21  ) );
		connect( m_savePresetButton, SIGNAL( clicked() ), _ctl,
						SLOT( savePreset() ) );
		m_savePresetButton->setToolTip(tr("Save preset"));

		m_savePresetButton->setMinimumWidth( 21 );
		m_savePresetButton->setMaximumWidth( 21 );
		m_savePresetButton->setMinimumHeight( 21 );
		m_savePresetButton->setMaximumHeight( 21 );

		int newSize = 0;

		if (m_pluginWidget) {
			newSize = m_pluginWidget->width() + 20;
		}
		newSize = std::max(newSize, 250);

		auto resize = new QWidget(this);
		resize->resize( newSize, 10 );
		auto space0 = new QWidget(this);
		space0->resize(8, 10);
		auto space1 = new QWidget(this);
		space1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
		QFont f( "Arial", 10 );

		l->addItem( new QSpacerItem( newSize - 20, 30, QSizePolicy::Fixed,
						QSizePolicy::Fixed ), 1, 0 );
		l->addWidget( resize, 2, 0, 1, 1, Qt::AlignCenter );
		if (m_pluginWidget) {
			l->addWidget( m_pluginWidget, 3, 0, 1, 1, Qt::AlignCenter );
		}
		l->setRowStretch( 5, 1 );
		l->setColumnStretch( 1, 1 );

		auto tb = new QToolBar(this);
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
	if( m_plugin != nullptr && tbLabel != nullptr )
	{
		tbLabel->setText( tr( "Effect by: " ) + m_plugin->vendorString() +
			tr( "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<br />" ) +
			m_plugin->currentProgramName() );
	}
}

void VstEffectControlDialog::showEvent(QShowEvent *_se)
{
	EffectControlDialog::showEvent( _se );
	// Workaround for a (unexplained) bug where on project-load the effect
	// control window has size 0 and would only restore to the proper size upon
	// moving the window or interacting with it.
	if (parentWidget()) {
		parentWidget()->adjustSize();
	}
}




VstEffectControlDialog::~VstEffectControlDialog()
{
	if (m_pluginWidget && layout()) {
		layout()->removeWidget(m_pluginWidget);
		m_pluginWidget->setParent(nullptr);
	}
}




void VstEffectControlDialog::togglePluginUI( bool checked )
{
	if( !m_plugin ) {
		return;
	}

	if ( m_togglePluginButton->isChecked() != checked ) {
		m_togglePluginButton->setChecked( checked );
	}

	if ( checked ) {
		m_plugin->showUI();
	} else {
		m_plugin->hideUI();
	}
}


} // namespace lmms::gui