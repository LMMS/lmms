/*
 * LmmsPalette.cpp - dummy class for fetching palette qproperties from CSS
 *                
 *
 * Copyright (c) 2007-2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
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

#include <QApplication>
#include <QStyle>
#include "LmmsPalette.h"
#include "LmmsStyle.h"


LmmsPalette::LmmsPalette( QWidget * parent, QStyle * stylearg ) : 
	QWidget( parent ),
	
/*	sane defaults in case fetching from stylesheet fails*/	
	
	m_background( 91, 101, 113 ),
	m_windowText( 240, 240, 240 ),
	m_base( 128, 128, 128 ),
	m_text( 224, 224, 224 ),
	m_button( 201, 201, 201 ),
	m_shadow( 0,0,0 ),
	m_buttonText( 0,0,0 ),
	m_brightText( 74, 253, 133 ),
	m_highlight( 100, 100, 100 ),
	m_highlightedText( 255, 255, 255  ),
	m_toolTipText( 0, 0, 0 ),
	m_toolTipBase( 128, 128, 128 )
{
	setStyle( stylearg );
	stylearg->polish( this );
	ensurePolished();
}

LmmsPalette::~LmmsPalette()
{
}

#define ACCESSMET( read, write ) \
	QColor LmmsPalette:: read () const \
	{	return m_##read ; } \
	void LmmsPalette:: write ( const QColor & c ) \
	{	m_##read = QColor( c ); }
	

	ACCESSMET( background, setBackground )
	ACCESSMET( windowText, setWindowText )
	ACCESSMET( base, setBase )
	ACCESSMET( text, setText )
	ACCESSMET( button, setButton )
	ACCESSMET( shadow, setShadow )
	ACCESSMET( buttonText, setButtonText )
	ACCESSMET( brightText, setBrightText )
	ACCESSMET( highlight, setHighlight )
	ACCESSMET( highlightedText, setHighlightedText )
	ACCESSMET( toolTipText, setToolTipText )
	ACCESSMET( toolTipBase, setToolTipBase )


QPalette LmmsPalette::palette() const
{
	QPalette pal = QApplication::style()->standardPalette();
	
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
	pal.setBrush( QPalette::ToolTipText,		QBrush( toolTipText() ) );
	pal.setBrush( QPalette::ToolTipBase,		QBrush( toolTipBase() ) );  
	return pal;
}


#include "moc_LmmsPalette.cxx"
