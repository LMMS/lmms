/*
 * ladspa_browser.cpp - dialog to display information about installed LADSPA
 *                      plugins
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "ladspa_browser.h"


#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>


#include "gui_templates.h"
#include "ladspa_description.h"
#include "ladspa_port_dialog.h"
#include "tab_bar.h"
#include "tab_button.h"

#include "embed.cpp"



extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT ladspabrowser_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"LADSPA Plugin Browser",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"List installed LADSPA plugins" ),
	"Danny McRae <khjklujn/at/users.sourceforge.net>",
	0x0100,
	Plugin::Tool,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
} ;


// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model * _parent, void * _data )
{
	return new ladspaBrowser;
}

}




ladspaBrowser::ladspaBrowser() :
	ToolPlugin( &ladspabrowser_plugin_descriptor, NULL )
{
}




ladspaBrowser::~ladspaBrowser()
{
}




QString ladspaBrowser::nodeName() const
{
	return ladspabrowser_plugin_descriptor.name;
}






ladspaBrowserView::ladspaBrowserView( ToolPlugin * _tool ) :
	ToolPluginView( _tool  )
{
	QHBoxLayout * hlayout = new QHBoxLayout( this );
	hlayout->setSpacing( 0 );
	hlayout->setMargin( 0 );

	m_tabBar = new tabBar( this, QBoxLayout::TopToBottom );
	m_tabBar->setExclusive( true );
	m_tabBar->setFixedWidth( 72 );

	QWidget * ws = new QWidget( this );
	ws->setFixedSize( 500, 480 );

	QWidget * available = createTab( ws, tr( "Available Effects" ), VALID );
	QWidget * unavailable = createTab( ws, tr( "Unavailable Effects" ),
								INVALID );
	QWidget * instruments = createTab( ws, tr( "Instruments" ), SOURCE );
	QWidget * analysis = createTab( ws, tr( "Analysis Tools" ), SINK );
	QWidget * other = createTab( ws, tr( "Don't know" ), OTHER );


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

	setWhatsThis( tr(
"This dialog displays information on all of the LADSPA plugins LMMS was "
"able to locate. The plugins are divided into five categories based "
"upon an interpretation of the port types and names.\n\n"

"Available Effects are those that can be used by LMMS. In order for LMMS "
"to be able to use an effect, it must, first and foremost, be an effect, "
"which is to say, it has to have both input channels and output channels. "
"LMMS identifies an input channel as an audio rate port containing 'in' in "
"the name. Output channels are identified by the letters 'out'. Furthermore, "
"the effect must have the same number of inputs and outputs and be real time "
"capable.\n\n"

"Unavailable Effects are those that were identified as effects, but either "
"didn't have the same number of input and output channels or weren't real "
"time capable.\n\n"

"Instruments are plugins for which only output channels were identified.\n\n"

"Analysis Tools are plugins for which only input channels were identified.\n\n"

"Don't Knows are plugins for which no input or output channels were "
"identified.\n\n"

"Double clicking any of the plugins will bring up information on the "
"ports." ) );

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




ladspaBrowserView::~ladspaBrowserView()
{
}




QWidget * ladspaBrowserView::createTab( QWidget * _parent, const QString & _txt,
							ladspaPluginType _type )
{
	QWidget * tab = new QWidget( _parent );
	tab->setFixedSize( 500, 400 );
	QVBoxLayout * layout = new QVBoxLayout( tab );
	layout->setSpacing( 0 );
	layout->setMargin( 0 );

	const QString type = "<b>" + tr( "Type:" ) + "</b> ";
	QLabel * title = new QLabel( type + _txt, tab );
	QFont f = title->font();
	f.setBold( true );
	title->setFont( pointSize<12>( f ) );

	layout->addSpacing( 5 );
	layout->addWidget( title );
	layout->addSpacing( 10 );

	ladspaDescription * description = new ladspaDescription( tab, _type );
	connect( description, SIGNAL( doubleClicked( const ladspa_key_t & ) ),
				SLOT( showPorts( const ladspa_key_t & ) ) );
	layout->addWidget( description, 1 );

	return tab;
}




void ladspaBrowserView::showPorts( const ladspa_key_t & _key )
{
	ladspaPortDialog ports( _key );
	ports.exec();
}




#include "moc_ladspa_browser.cxx"
