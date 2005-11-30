/*
 * tempo_sync_knob.h - adds bpm to ms conversion for knob class
 *
 * Copyright (c) 2005 Danny McRae <khjklujn@yahoo.com>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include "qt3support.h"

#ifdef QT4

#include <QPainter>
#include <QPalette>
#include <QBitmap>
#include <QLabel>
#include <QStatusBar>
#include <QMouseEvent>
#include <QMenu>
#include <QStatusBar>
#include <QFontMetrics>
#include <QApplication>

#else

#include <qlabel.h>
#include <qpopupmenu.h>

#define addSeparator insertSeparator
#define addMenu insertItem

#endif


#include "tempo_sync_knob.h"
#include "song_editor.h"
#include "embed.h"
#include "tooltip.h"
#include "config_mgr.h"
#include "text_float.h"




tempoSyncKnob::tempoSyncKnob( int _knob_num, QWidget * _parent,
					const QString & _name, float _scale ) :
	knob( _knob_num, _parent, _name ),
	m_tempoSyncMode( NO_SYNC ),
	m_scale( _scale ),
	m_tempoSyncIcon( embed::getIconPixmap( "xclock" ) ),
	m_tempoSyncDescription( tr( "Tempo Sync" ) ),
	m_tempoLastSyncMode( NO_SYNC )
{
	connect( songEditor::inst(), SIGNAL( bpmChanged( int ) ),
		 this, SLOT( calculateTempoSyncTime( int ) ) );
}




tempoSyncKnob::~tempoSyncKnob()
{
}




void tempoSyncKnob::contextMenuEvent( QContextMenuEvent * )
{
	QMenu contextMenu( this );
#ifdef QT4
	contextMenu.setTitle( accessibleName() );
#else
	QLabel * caption = new QLabel( "<font color=white><b>" +
			QString( accessibleName() ) + "</b></font>", this );
	caption->setPaletteBackgroundColor( QColor( 0, 0, 192 ) );
	caption->setAlignment( Qt::AlignCenter );
	contextMenu.addAction( caption );
#endif
	contextMenu.addAction( embed::getIconPixmap( "reload" ),
				tr( "&Reset (%1%2)" ).arg( m_initValue ).arg(
							m_hintTextAfterValue ),
							this, SLOT( reset() ) );
	contextMenu.addSeparator();
	contextMenu.addAction( embed::getIconPixmap( "edit_copy" ),
				tr( "&Copy value (%1%2)" ).arg( value() ).arg(
							m_hintTextAfterValue ),
						this, SLOT( copyValue() ) );
	contextMenu.addAction( embed::getIconPixmap( "edit_paste" ),
				tr( "&Paste value (%1%2)"
						).arg( s_copiedValue ).arg(
							m_hintTextAfterValue ),
				this, SLOT( pasteValue() ) );
	contextMenu.addSeparator();
	
	QMenu * syncMenu = new QMenu( this );
	int menuId;
	menuId = syncMenu->addAction( embed::getIconPixmap( "note_none" ),
					tr( "No Sync" ),
					this, SLOT( setTempoSync( int ) ) );
	syncMenu->setItemParameter( menuId, ( int ) NO_SYNC );
	menuId = syncMenu->addAction( embed::getIconPixmap(
							"note_double_whole" ),
					tr( "Eight beats" ),
					this, SLOT( setTempoSync( int ) ) );
	syncMenu->setItemParameter( menuId, ( int ) DOUBLE_WHOLE_NOTE );
	menuId = syncMenu->addAction( embed::getIconPixmap( "note_whole" ),
					tr( "Whole note" ),
					this, SLOT( setTempoSync( int ) ) );
	syncMenu->setItemParameter( menuId, ( int ) WHOLE_NOTE );
	menuId = syncMenu->addAction( embed::getIconPixmap( "note_half" ),
					tr( "Half note" ),
					this, SLOT( setTempoSync( int ) ) );
	syncMenu->setItemParameter( menuId, ( int ) HALF_NOTE );
	menuId = syncMenu->addAction( embed::getIconPixmap( "note_quarter" ),
					tr( "Quarter note" ),
			     		this, SLOT( setTempoSync( int ) ) );
	syncMenu->setItemParameter( menuId, ( int ) QUARTER_NOTE );
	menuId = syncMenu->addAction( embed::getIconPixmap( "note_eighth" ),
					tr( "8th note" ),
					this, SLOT( setTempoSync( int ) ) );
	syncMenu->setItemParameter( menuId, ( int ) EIGHTH_NOTE );
	menuId = syncMenu->addAction( embed::getIconPixmap( "note_sixteenth" ),
					tr( "16th note" ),
					this, SLOT( setTempoSync( int ) ) );
	syncMenu->setItemParameter( menuId, ( int ) SIXTEENTH_NOTE );
	menuId = syncMenu->addAction( embed::getIconPixmap(
							"note_thirtysecond" ),
					tr( "32nd note" ),
					this, SLOT( setTempoSync( int ) ) );
	syncMenu->setItemParameter( menuId, ( int ) THIRTYSECOND_NOTE );
	
	contextMenu.addMenu( m_tempoSyncIcon, m_tempoSyncDescription,
								syncMenu );
	contextMenu.addSeparator();
	
	contextMenu.addAction( tr( "Connect to MIDI-device" ), this,
						SLOT( connectToMidiDevice() ) );
	contextMenu.addSeparator();
	contextMenu.addAction( embed::getIconPixmap( "help" ), tr( "&Help" ),
						this, SLOT( displayHelp() ) );
	contextMenu.exec( QCursor::pos() );
	
	delete syncMenu;
}




void tempoSyncKnob::mouseMoveEvent( QMouseEvent * _me )
{
	m_tempoSyncMode = NO_SYNC;
	calculateTempoSyncTime( songEditor::inst()->getBPM() );
	knob::mouseMoveEvent( _me );
}




void tempoSyncKnob::wheelEvent( QWheelEvent * _we )
{
	knob::wheelEvent( _we );
	m_tempoSyncMode = NO_SYNC;
	calculateTempoSyncTime( songEditor::inst()->getBPM() );
}




void tempoSyncKnob::setTempoSync( int _note_type )
{
	m_tempoSyncMode = ( tempoSyncMode ) _note_type;
	calculateTempoSyncTime( songEditor::inst()->getBPM() );
	songEditor::inst()->setModified();
}




void tempoSyncKnob::calculateTempoSyncTime( int _bpm )
{
	float conversionFactor = 1.0;
	
	if( m_tempoSyncMode )
	{
		switch( m_tempoSyncMode )
		{
			case DOUBLE_WHOLE_NOTE:
				m_tempoSyncDescription = tr(
						"Synced to Eight Beats" );
				m_tempoSyncIcon = embed::getIconPixmap(
							"note_double_whole" );
				conversionFactor = 0.125;
				break;
			case WHOLE_NOTE:
				m_tempoSyncDescription = tr(
						"Synced to Whole Note" );
				m_tempoSyncIcon = embed::getIconPixmap(
								"note_whole" );
				conversionFactor = 0.25;
				break;
			case HALF_NOTE:
				m_tempoSyncDescription = tr(
							"Synced to Half Note" );
				m_tempoSyncIcon = embed::getIconPixmap(
								"note_half" );
				conversionFactor = 0.5;
				break;
			case QUARTER_NOTE:
				m_tempoSyncDescription = tr(
						"Synced to Quarter Note" );
				m_tempoSyncIcon = embed::getIconPixmap(
							"note_quarter" );
				conversionFactor = 1.0;
				break;
			case EIGHTH_NOTE:
				m_tempoSyncDescription = tr(
							"Synced to 8th Note" );
				m_tempoSyncIcon = embed::getIconPixmap(
								"note_eighth" );
				conversionFactor = 2.0;
				break;
			case SIXTEENTH_NOTE:
				m_tempoSyncDescription = tr(
							"Synced to 16th Note" );
				m_tempoSyncIcon = embed::getIconPixmap(
							"note_sixteenth" );
				conversionFactor = 4.0;
				break;
			case THIRTYSECOND_NOTE:
				m_tempoSyncDescription = tr(
							"Synced to 32nd Note" );
				m_tempoSyncIcon = embed::getIconPixmap(
							"note_thirtysecond" );
				conversionFactor = 8.0;
				break;
			default:
				printf( "tempoSyncKnob::calculateTempoSyncTime"
						": invalid tempoSyncMode" );
				break;
		}
		setValue( 60000.0 / ( _bpm * conversionFactor * m_scale ),
				 FALSE );
	}
	else
	{
		m_tempoSyncDescription = tr( "Tempo Sync" );
		m_tempoSyncIcon = embed::getIconPixmap( "xclock" );
	}
	
	if( m_tempoSyncMode != m_tempoLastSyncMode )
	{
		emit syncModeChanged( m_tempoSyncMode );
		emit syncDescriptionChanged( m_tempoSyncDescription );
		emit syncIconChanged();
	}
	
	m_tempoLastSyncMode = m_tempoSyncMode;
}




tempoSyncMode tempoSyncKnob::getSyncMode( void )
{
	return( m_tempoSyncMode );
}




void tempoSyncKnob::setSyncMode( tempoSyncMode _new_mode )
{
	m_tempoSyncMode = _new_mode;
	calculateTempoSyncTime( songEditor::inst()->getBPM() );
}




float tempoSyncKnob::getScale( void )
{
	return( m_scale );
}




void tempoSyncKnob::setScale( float _new_scale )
{
	m_scale = _new_scale;
	calculateTempoSyncTime( songEditor::inst()->getBPM() );
	emit scaleChanged( _new_scale );
}




const QString & tempoSyncKnob::getSyncDescription( void )
{
	return( m_tempoSyncDescription );
}




void tempoSyncKnob::setSyncDescription( const QString & _new_description )
{
	m_tempoSyncDescription = _new_description;
	emit syncDescriptionChanged( _new_description );
}




const QPixmap & tempoSyncKnob::getSyncIcon( void )
{
	return( m_tempoSyncIcon );
}




void tempoSyncKnob::setSyncIcon( const QPixmap & _new_icon )
{
	m_tempoSyncIcon = _new_icon;
	emit syncIconChanged();
}



#ifndef QT4
#undef addSeparator
#endif


#include "tempo_sync_knob.moc"

