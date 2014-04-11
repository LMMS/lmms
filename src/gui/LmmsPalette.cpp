/*
 * LmmsPalette.h - dummy class for fetching palette qproperties from CSS
 *                
 *
 * Copyright (c) 2007-2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
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

#include <QApplication>
#include <QStyle>
#include "LmmsPalette.h"
#include "LmmsStyle.h"


LmmsPalette::LmmsPalette( QWidget * parent ) : QWidget( parent ),
	m_background( 0,0,0 ),
	m_windowText( 0,0,0 ),
	m_base( 0,0,0 ),
	m_text( 0,0,0 ),
	m_button( 0,0,0 ),
	m_shadow( 0,0,0 ),
	m_buttonText( 0,0,0 ),
	m_brightText( 0,0,0 ),
	m_highlight( 0,0,0 ),
	m_highlightedText( 0,0,0 )
{
	setStyle( QApplication::style() );
}

LmmsPalette::~LmmsPalette()
{
}


QPalette LmmsPalette::palette() const
{
	QPalette pal = style()->standardPalette();
	
	pal.setColor( QPalette::Background, 		background() );	
	pal.setColor( QPalette::WindowText, 		windowText() );	
	pal.setColor( QPalette::Base, 				base() );	
	pal.setColor( QPalette::ButtonText, 		buttonText() );	
	pal.setColor( QPalette::BrightText, 		brightText() );	
	pal.setColor( QPalette::Text, 				text() );	
	pal.setColor( QPalette::Button, 			button() );	
	pal.setColor( QPalette::Shadow, 			shadow() );	
	pal.setColor( QPalette::Highlight, 			highlight() );	
	pal.setColor( QPalette::HighlightedText, 	highlightedText() );
	
	return pal;
}


#include "moc_LmmsPalette.cxx"
