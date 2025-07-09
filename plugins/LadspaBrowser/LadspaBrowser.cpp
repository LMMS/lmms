/*
 * LadspaBrowser.cpp - dialog to display information about installed LADSPA
 *                      plugins
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "LadspaBrowser.h"


#include <QHBoxLayout>
#include <QLabel>


#include "LadspaDescription.h"
#include "LadspaPortDialog.h"
#include "TabBar.h"
#include "TabButton.h"

#include "embed.h"
#include "plugin_export.h"

namespace lmms
{


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT ladspabrowser_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"LADSPA Plugin Browser",
	QT_TRANSLATE_NOOP( "PluginBrowser",
				"List installed LADSPA plugins" ),
	"Danny McRae <khjklujn/at/users.sourceforge.net>",
	0x0100,
	Plugin::Type::Tool,
	new PluginPixmapLoader("logo"),
	nullptr,
	nullptr,
	nullptr,
} ;


// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model * _parent, void * _data )
{
	return new LadspaBrowser;
}

}




LadspaBrowser::LadspaBrowser() :
	ToolPlugin( &ladspabrowser_plugin_descriptor, nullptr )
{
}




QString LadspaBrowser::nodeName() const
{
	return ladspabrowser_plugin_descriptor.name;
}



namespace gui
{


LadspaBrowserView::LadspaBrowserView( ToolPlugin * _tool ) :
	ToolPluginView( _tool  )
{
	auto hlayout = new QHBoxLayout(this);
	hlayout->setSpacing( 0 );
	hlayout->setContentsMargins(0, 0, 0, 0);

	m_tabBar = new TabBar( this, QBoxLayout::TopToBottom );
	m_tabBar->setExclusive( true );
	m_tabBar->setFixedWidth( 72 );

	auto ws = new QWidget(this);
	ws->setFixedSize( 500, 480 );

	QWidget * available = createTab( ws, tr( "Available Effects" ), LadspaPluginType::Valid );
	QWidget * unavailable = createTab( ws, tr( "Unavailable Effects" ),
								LadspaPluginType::Invalid );
	QWidget * instruments = createTab( ws, tr( "Instruments" ), LadspaPluginType::Source );
	QWidget * analysis = createTab( ws, tr( "Analysis Tools" ), LadspaPluginType::Sink );
	QWidget * other = createTab( ws, tr( "Don't know" ), LadspaPluginType::Other );


	m_tabBar->addTab( available, tr( "Available Effects" ), 
				0, false, true 
			)->setIcon( embed::getIconPixmap( "setup_audio" ) );
	m_tabBar->addTab( unavailable, tr( "Unavailable Effects" ), 
				1, false, true 
			)->setIcon( embed::getIconPixmap(
						"unavailable_sound" ) );
	m_tabBar->addTab( instruments, tr( "Instruments" ), 
				2, false, true 
			)->setIcon( embed::getIconPixmap(
							"setup_midi" ) );
	m_tabBar->addTab( analysis, tr( "Analysis Tools" ), 
				3, false, true
			)->setIcon( embed::getIconPixmap( "analysis" ) );
	m_tabBar->addTab( other, tr( "Don't know" ), 
				4, true, true
			)->setIcon( embed::getIconPixmap( "uhoh" ) );


	m_tabBar->setActiveTab( 0 );

	hlayout->addWidget( m_tabBar );
	hlayout->addSpacing( 10 );
	hlayout->addWidget( ws );
	hlayout->addSpacing( 10 );
	hlayout->addStretch();

	hide();
	if( parentWidget() )
	{
		parentWidget()->hide();
		parentWidget()->layout()->setSizeConstraint(
							QLayout::SetFixedSize );
		
		Qt::WindowFlags flags = parentWidget()->windowFlags();
		flags |= Qt::MSWindowsFixedSizeDialogHint;
		flags &= ~Qt::WindowMaximizeButtonHint;
		parentWidget()->setWindowFlags( flags );
	}
}




QWidget * LadspaBrowserView::createTab( QWidget * _parent, const QString & _txt,
							LadspaPluginType _type )
{
	auto tab = new QWidget(_parent);
	tab->setFixedSize( 500, 400 );
	auto layout = new QVBoxLayout(tab);
	layout->setSpacing( 0 );
	layout->setContentsMargins(0, 0, 0, 0);

	const QString type = "<b>" + tr( "Type:" ) + "</b> ";
	auto title = new QLabel(type + _txt, tab);
	QFont f = title->font();
	f.setBold( true );

	layout->addSpacing( 5 );
	layout->addWidget( title );
	layout->addSpacing( 10 );

	auto description = new LadspaDescription(tab, _type);
	connect( description, SIGNAL( doubleClicked( const ::lmms::ladspa_key_t & ) ),
				SLOT( showPorts( const ::lmms::ladspa_key_t & ) ) );
	layout->addWidget( description, 1 );

	return tab;
}




void LadspaBrowserView::showPorts( const ::lmms::ladspa_key_t & _key )
{
	LadspaPortDialog ports( _key );
	ports.exec();
}


} // namespace gui

} // namespace lmms
