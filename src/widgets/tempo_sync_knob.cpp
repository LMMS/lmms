#ifndef SINGLE_SOURCE_COMPILE

/*
 * tempo_sync_knob.cpp - adds bpm to ms conversion for knob class
 *
 * Copyright (c) 2005-2007 Danny McRae <khjklujn/at/yahoo.com>
 * Copyright (c) 2005-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "tempo_sync_knob.h"

#include <QtGui/QMouseEvent>
#include <QtGui/QWorkspace>

#include "automatable_object_templates.h"
#include "caption_menu.h"
#include "embed.h"
#include "main_window.h"
#include "meter_dialog.h"
#include "song_editor.h"


tempoSyncKnob::tempoSyncKnob( int _knob_num, QWidget * _parent,
							const QString & _name,
							track * _track,
							float _scale ) :
	knob( _knob_num, _parent, _name, _track ),
	m_tempoSyncMode( NO_SYNC ),
	m_scale( _scale ),
	m_tempoSyncIcon( embed::getIconPixmap( "tempo_sync" ) ),
	m_tempoSyncDescription( tr( "Tempo Sync" ) ),
	m_tempoLastSyncMode( NO_SYNC )
{
	connect( engine::getSongEditor(), SIGNAL( tempoChanged( bpm_t ) ),
			this, SLOT( calculateTempoSyncTime( bpm_t ) ) );
	m_custom = new meterDialog( engine::getMainWindow()->workspace(),
								_track );
	m_custom->hide();
	m_custom->setWindowTitle( "Meter" );
}




tempoSyncKnob::~tempoSyncKnob()
{
	if( m_custom )
	{
		m_custom->deleteLater();
	}
}




void tempoSyncKnob::contextMenuEvent( QContextMenuEvent * )
{
	captionMenu contextMenu( accessibleName() );
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
	
	float limit = 60000.0f / ( engine::getSongEditor()->getTempo() *
								m_scale );
	
	QMenu * syncMenu = contextMenu.addMenu( m_tempoSyncIcon,
						m_tempoSyncDescription );
	if( limit / 8.0f <= maxValue() )
	{
	connect( syncMenu, SIGNAL( triggered( QAction * ) ),
			this, SLOT( setTempoSync( QAction * ) ) );
	syncMenu->addAction( embed::getIconPixmap( "note_none" ),
				tr( "No Sync" ) )->setData( (int) NO_SYNC );
	if( limit / 0.125f <= maxValue() )
	{
		syncMenu->addAction( embed::getIconPixmap( "note_double_whole" ),
				tr( "Eight beats" ) )->setData(
						(int) DOUBLE_WHOLE_NOTE );
	}
	if( limit / 0.25f <= maxValue() )
	{
		syncMenu->addAction( embed::getIconPixmap( "note_whole" ),
					tr( "Whole note" ) )->setData(
						(int) WHOLE_NOTE );
	}
	if( limit / 0.5f <= maxValue() )
	{
		syncMenu->addAction( embed::getIconPixmap( "note_half" ),
					tr( "Half note" ) )->setData(
						(int) HALF_NOTE );
	}
	if( limit <= maxValue() )
	{
		syncMenu->addAction( embed::getIconPixmap( "note_quarter" ),
					tr( "Quarter note" ) )->setData(
						(int) QUARTER_NOTE );
	}
	if( limit / 2.0f <= maxValue() )
	{
		syncMenu->addAction( embed::getIconPixmap( "note_eighth" ),
					tr( "8th note" ) )->setData(
						(int) EIGHTH_NOTE );
	}
	if( limit / 4.0f <= maxValue() )
	{
		syncMenu->addAction( embed::getIconPixmap( "note_sixteenth" ),
					tr( "16th note" ) )->setData(
					(int) SIXTEENTH_NOTE );
	}
	syncMenu->addAction( embed::getIconPixmap( "note_thirtysecond" ),
					tr( "32nd note" ) )->setData(
					(int) THIRTYSECOND_NOTE );
	syncMenu->addAction( embed::getIconPixmap( "dont_know" ),
				tr( "Custom..." ),
				this, SLOT( showCustom( void ) )
						)->setData( (int) CUSTOM );
	contextMenu.addSeparator();
	}
	
	contextMenu.addAction( embed::getIconPixmap( "automation" ),
					tr( "&Open in automation editor" ),
					getAutomationPattern(),
					SLOT( openInAutomationEditor() ) );
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
	calculateTempoSyncTime( engine::getSongEditor()->getTempo() );
	knob::mouseMoveEvent( _me );
}




void tempoSyncKnob::wheelEvent( QWheelEvent * _we )
{
	knob::wheelEvent( _we );
	m_tempoSyncMode = NO_SYNC;
	calculateTempoSyncTime( engine::getSongEditor()->getTempo() );
}




void tempoSyncKnob::setTempoSync( QAction * _item )
{
	setTempoSync( _item->data().toInt() );
}



void tempoSyncKnob::setTempoSync( int _note_type )
{
	setSyncMode( ( tempoSyncMode ) _note_type );
	engine::getSongEditor()->setModified();
}




void tempoSyncKnob::calculateTempoSyncTime( bpm_t _bpm )
{
	float conversionFactor = 1.0;
	
	if( m_tempoSyncMode )
	{
		switch( m_tempoSyncMode )
		{
			case CUSTOM:
				m_tempoSyncDescription = tr( "Custom " ) + 
						"(" +
				QString::number( m_custom->getNumerator() ) +
						"/" +
				QString::number( m_custom->getDenominator() ) +
						")";
				conversionFactor = 
			static_cast<float>( m_custom->getDenominator() ) /
			static_cast<float>( m_custom->getNumerator() );
				break;
			case DOUBLE_WHOLE_NOTE:
				m_tempoSyncDescription = tr(
						"Synced to Eight Beats" );
				conversionFactor = 0.125;
				break;
			case WHOLE_NOTE:
				m_tempoSyncDescription = tr(
						"Synced to Whole Note" );
				conversionFactor = 0.25;
				break;
			case HALF_NOTE:
				m_tempoSyncDescription = tr(
							"Synced to Half Note" );
				conversionFactor = 0.5;
				break;
			case QUARTER_NOTE:
				m_tempoSyncDescription = tr(
						"Synced to Quarter Note" );
				conversionFactor = 1.0;
				break;
			case EIGHTH_NOTE:
				m_tempoSyncDescription = tr(
							"Synced to 8th Note" );
				conversionFactor = 2.0;
				break;
			case SIXTEENTH_NOTE:
				m_tempoSyncDescription = tr(
							"Synced to 16th Note" );
				conversionFactor = 4.0;
				break;
			case THIRTYSECOND_NOTE:
				m_tempoSyncDescription = tr(
							"Synced to 32nd Note" );
				conversionFactor = 8.0;
				break;
			default: ;
		}
		bool journalling = testAndSetJournalling( FALSE );
		setValue( 60000.0 / ( _bpm * conversionFactor * m_scale ) );
		setJournalling( journalling );
	}
	else
	{
		m_tempoSyncDescription = tr( "Tempo Sync" );
	}
	
	if( m_tempoSyncMode != m_tempoLastSyncMode )
	{
		switch( m_tempoSyncMode )
		{
			case NO_SYNC:
				m_tempoSyncIcon = embed::getIconPixmap(
								"tempo_sync" );
				break;
			case CUSTOM:
				m_tempoSyncIcon = embed::getIconPixmap(
								"dont_know" );
				break;
			case DOUBLE_WHOLE_NOTE:
				m_tempoSyncIcon = embed::getIconPixmap(
							"note_double_whole" );
				break;
			case WHOLE_NOTE:
				m_tempoSyncIcon = embed::getIconPixmap(
								"note_whole" );
				break;
			case HALF_NOTE:
				m_tempoSyncIcon = embed::getIconPixmap(
								"note_half" );
				break;
			case QUARTER_NOTE:
				m_tempoSyncIcon = embed::getIconPixmap(
							"note_quarter" );
				break;
			case EIGHTH_NOTE:
				m_tempoSyncIcon = embed::getIconPixmap(
								"note_eighth" );
				break;
			case SIXTEENTH_NOTE:
				m_tempoSyncIcon = embed::getIconPixmap(
							"note_sixteenth" );
				break;
			case THIRTYSECOND_NOTE:
				m_tempoSyncIcon = embed::getIconPixmap(
							"note_thirtysecond" );
				break;
			default:
				printf( "tempoSyncKnob::calculateTempoSyncTime"
						": invalid tempoSyncMode" );
				break;
		}

		emit syncModeChanged( m_tempoSyncMode );
		emit syncDescriptionChanged( m_tempoSyncDescription );
		emit syncIconChanged();

		m_tempoLastSyncMode = m_tempoSyncMode;
	}
}




void tempoSyncKnob::saveSettings( QDomDocument & _doc, QDomElement & _this,
							const QString & _name )
{
 	_this.setAttribute( "syncmode", ( int ) getSyncMode() );
	automatableObject<float>::saveSettings( _doc, _this, _name );
	m_custom->saveSettings( _doc, _this, _name );
}




void tempoSyncKnob::loadSettings( const QDomElement & _this,
							const QString & _name )
{
 	setSyncMode( ( tempoSyncMode ) _this.attribute(
 						"syncmode" ).toInt() );
	automatableObject<float>::loadSettings( _this, _name );
	m_custom->loadSettings( _this, _name );
}




tempoSyncKnob::tempoSyncMode tempoSyncKnob::getSyncMode( void )
{
	return( m_tempoSyncMode );
}




void tempoSyncKnob::setSyncMode( tempoSyncMode _new_mode )
{
	if( m_tempoSyncMode != _new_mode )
	{
		m_tempoSyncMode = _new_mode;
		if( _new_mode == CUSTOM )
		{
			connect( m_custom, SIGNAL( numeratorChanged( int ) ),
					this, SLOT( updateCustom( int ) ) );
			connect( m_custom, SIGNAL( denominatorChanged( int ) ),
					this, SLOT( updateCustom( int ) ) );
		}
		else
		{
			m_custom->hide();
			disconnect( m_custom, 0,
					this, SLOT( updateCustom( int ) ) );
		}
	}
	calculateTempoSyncTime( engine::getSongEditor()->getTempo() );
}




float tempoSyncKnob::getScale( void )
{
	return( m_scale );
}




void tempoSyncKnob::setScale( float _new_scale )
{
	m_scale = _new_scale;
	calculateTempoSyncTime( engine::getSongEditor()->getTempo() );
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




void tempoSyncKnob::updateCustom( int )
{
	setSyncMode( CUSTOM );
}




void tempoSyncKnob::showCustom( void )
{
	m_custom->show();
	setTempoSync( CUSTOM );
}



#include "tempo_sync_knob.moc"


#endif
