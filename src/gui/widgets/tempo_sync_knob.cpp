/*
 * tempo_sync_knob.cpp - adds bpm to ms conversion for knob class
 *
 * Copyright (c) 2005-2007 Danny McRae <khjklujn/at/yahoo.com>
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <cstdio>

#include "tempo_sync_knob.h"

#include <QtGui/QMouseEvent>
#include <QtGui/QMdiArea>
#include <QtXml/QDomElement>

#include "engine.h"
#include "caption_menu.h"
#include "embed.h"
#include "MainWindow.h"
#include "meter_dialog.h"
#include "song.h"


tempoSyncKnobModel::tempoSyncKnobModel( const float _val, const float _min,
				const float _max, const float _step,
				const float _scale, ::model * _parent,
				const QString & _display_name ) :
	knobModel( _val, _min, _max, _step, _parent, _display_name ),
	m_tempoSyncMode( SyncNone ),
	m_tempoLastSyncMode( SyncNone ),
	m_scale( _scale ),
	m_custom( _parent )
{
	connect( engine::getSong(), SIGNAL( tempoChanged( bpm_t ) ),
			this, SLOT( calculateTempoSyncTime( bpm_t ) ) );
}




tempoSyncKnobModel::~tempoSyncKnobModel()
{
}




void tempoSyncKnobModel::setTempoSync( QAction * _item )
{
	setTempoSync( _item->data().toInt() );
}




void tempoSyncKnobModel::setTempoSync( int _note_type )
{
	setSyncMode( ( tempoSyncMode ) _note_type );
	engine::getSong()->setModified();
}




void tempoSyncKnobModel::calculateTempoSyncTime( bpm_t _bpm )
{
	float conversionFactor = 1.0;
	
	if( m_tempoSyncMode )
	{
		switch( m_tempoSyncMode )
		{
			case SyncCustom:
				conversionFactor = 
			static_cast<float>( m_custom.getDenominator() ) /
			static_cast<float>( m_custom.getNumerator() );
				break;
			case SyncDoubleWholeNote:
				conversionFactor = 0.125;
				break;
			case SyncWholeNote:
				conversionFactor = 0.25;
				break;
			case SyncHalfNote:
				conversionFactor = 0.5;
				break;
			case SyncQuarterNote:
				conversionFactor = 1.0;
				break;
			case SyncEighthNote:
				conversionFactor = 2.0;
				break;
			case SyncSixteenthNote:
				conversionFactor = 4.0;
				break;
			case SyncThirtysecondNote:
				conversionFactor = 8.0;
				break;
			default: ;
		}
		bool journalling = testAndSetJournalling( FALSE );
		float oneUnit = 60000.0 / ( _bpm * conversionFactor * m_scale );
		setValue( oneUnit * maxValue() );
		setJournalling( journalling );
	}

	if( m_tempoSyncMode != m_tempoLastSyncMode )
	{
		emit syncModeChanged( m_tempoSyncMode );
		m_tempoLastSyncMode = m_tempoSyncMode;
	}
}




void tempoSyncKnobModel::saveSettings( QDomDocument & _doc, QDomElement & _this,
							const QString & _name )
{
 	_this.setAttribute( "syncmode", ( int ) getSyncMode() );
	m_custom.saveSettings( _doc, _this, _name );
	knobModel::saveSettings( _doc, _this, _name );
}




void tempoSyncKnobModel::loadSettings( const QDomElement & _this,
							const QString & _name )
{
 	setSyncMode( ( tempoSyncMode ) _this.attribute( "syncmode" ).toInt() );
	m_custom.loadSettings( _this, _name );
	knobModel::loadSettings( _this, _name );
}




tempoSyncKnobModel::tempoSyncMode tempoSyncKnobModel::getSyncMode( void )
{
	return( m_tempoSyncMode );
}




void tempoSyncKnobModel::setSyncMode( tempoSyncMode _new_mode )
{
	if( m_tempoSyncMode != _new_mode )
	{
		m_tempoSyncMode = _new_mode;
		if( _new_mode == SyncCustom )
		{
			connect( &m_custom, SIGNAL( dataChanged() ),
					this, SLOT( updateCustom() ) );
		}
	}
	calculateTempoSyncTime( engine::getSong()->getTempo() );
}




float tempoSyncKnobModel::getScale( void )
{
	return( m_scale );
}




void tempoSyncKnobModel::setScale( float _new_scale )
{
	m_scale = _new_scale;
	calculateTempoSyncTime( engine::getSong()->getTempo() );
	emit scaleChanged( _new_scale );
}




void tempoSyncKnobModel::updateCustom( void )
{
	setSyncMode( SyncCustom );
}






tempoSyncKnob::tempoSyncKnob( int _knob_num, QWidget * _parent,
						const QString & _name ) :
	knob( _knob_num, _parent, _name ),
	m_tempoSyncIcon( embed::getIconPixmap( "tempo_sync" ) ),
	m_tempoSyncDescription( tr( "Tempo Sync" ) ),
	m_custom( NULL )
{
}




tempoSyncKnob::~tempoSyncKnob()
{
	if( m_custom )
	{
		delete m_custom->parentWidget();
	}
}




void tempoSyncKnob::modelChanged( void )
{
	if( model() == NULL )
	{
		printf( "no tempoSyncKnobModel has been set!\n" );
	}
	if( m_custom != NULL )
	{
		m_custom->setModel( &model()->m_custom );
	}
	connect( model(), SIGNAL( syncModeChanged( tempoSyncMode ) ),
			this, SLOT( updateDescAndIcon() ) );
	connect( this, SIGNAL( sliderMoved( float ) ),
			model(), SLOT( disableSync() ) );
	updateDescAndIcon();
}




void tempoSyncKnob::contextMenuEvent( QContextMenuEvent * )
{
	mouseReleaseEvent( NULL );

	captionMenu contextMenu( model()->displayName() );
	addDefaultActions( &contextMenu );
	contextMenu.addSeparator();
	
	float limit = 60000.0f / ( engine::getSong()->getTempo() *
							model()->m_scale );
	
	QMenu * syncMenu = contextMenu.addMenu( m_tempoSyncIcon,
						m_tempoSyncDescription );
	if( limit / 8.0f <= model()->maxValue() )
	{

	connect( syncMenu, SIGNAL( triggered( QAction * ) ),
			model(), SLOT( setTempoSync( QAction * ) ) );
	syncMenu->addAction( embed::getIconPixmap( "note_none" ),
		tr( "No Sync" ) )->setData( (int) tempoSyncKnobModel::SyncNone );
	if( limit / 0.125f <= model()->maxValue() )
	{
		syncMenu->addAction( embed::getIconPixmap( "note_double_whole" ),
				tr( "Eight beats" ) )->setData(
				(int) tempoSyncKnobModel::SyncDoubleWholeNote );
	}
	if( limit / 0.25f <= model()->maxValue() )
	{
		syncMenu->addAction( embed::getIconPixmap( "note_whole" ),
					tr( "Whole note" ) )->setData(
					(int) tempoSyncKnobModel::SyncWholeNote );
	}
	if( limit / 0.5f <= model()->maxValue() )
	{
		syncMenu->addAction( embed::getIconPixmap( "note_half" ),
					tr( "Half note" ) )->setData(
					(int) tempoSyncKnobModel::SyncHalfNote );
	}
	if( limit <= model()->maxValue() )
	{
		syncMenu->addAction( embed::getIconPixmap( "note_quarter" ),
					tr( "Quarter note" ) )->setData(
				(int) tempoSyncKnobModel::SyncQuarterNote );
	}
	if( limit / 2.0f <= model()->maxValue() )
	{
		syncMenu->addAction( embed::getIconPixmap( "note_eighth" ),
					tr( "8th note" ) )->setData(
				(int) tempoSyncKnobModel::SyncEighthNote );
	}
	if( limit / 4.0f <= model()->maxValue() )
	{
		syncMenu->addAction( embed::getIconPixmap( "note_sixteenth" ),
					tr( "16th note" ) )->setData(
				(int) tempoSyncKnobModel::SyncSixteenthNote );
	}
	syncMenu->addAction( embed::getIconPixmap( "note_thirtysecond" ),
					tr( "32nd note" ) )->setData(
				(int) tempoSyncKnobModel::SyncThirtysecondNote );
	syncMenu->addAction( embed::getIconPixmap( "dont_know" ),
				tr( "Custom..." ),
				this, SLOT( showCustom( void ) )
						)->setData(
					(int) tempoSyncKnobModel::SyncCustom );
	contextMenu.addSeparator();

	}

	contextMenu.addSeparator();
	contextMenu.addAction( embed::getIconPixmap( "help" ), tr( "&Help" ),
						this, SLOT( displayHelp() ) );
	contextMenu.exec( QCursor::pos() );
	
	delete syncMenu;
}




void tempoSyncKnob::updateDescAndIcon( void )
{
	if( model()->m_tempoSyncMode )
	{
		switch( model()->m_tempoSyncMode )
		{
			case tempoSyncKnobModel::SyncCustom:
				m_tempoSyncDescription = tr( "Custom " ) + 
						"(" +
			QString::number( model()->m_custom.getNumerator() ) +
						"/" +
			QString::number( model()->m_custom.getDenominator() ) +
						")";
				break;
			case tempoSyncKnobModel::SyncDoubleWholeNote:
				m_tempoSyncDescription = tr(
						"Synced to Eight Beats" );
				break;
			case tempoSyncKnobModel::SyncWholeNote:
				m_tempoSyncDescription = tr(
						"Synced to Whole Note" );
				break;
			case tempoSyncKnobModel::SyncHalfNote:
				m_tempoSyncDescription = tr(
							"Synced to Half Note" );
				break;
			case tempoSyncKnobModel::SyncQuarterNote:
				m_tempoSyncDescription = tr(
						"Synced to Quarter Note" );
				break;
			case tempoSyncKnobModel::SyncEighthNote:
				m_tempoSyncDescription = tr(
							"Synced to 8th Note" );
				break;
			case tempoSyncKnobModel::SyncSixteenthNote:
				m_tempoSyncDescription = tr(
							"Synced to 16th Note" );
				break;
			case tempoSyncKnobModel::SyncThirtysecondNote:
				m_tempoSyncDescription = tr(
							"Synced to 32nd Note" );
				break;
			default: ;
		}
	}
	else
	{
		m_tempoSyncDescription = tr( "Tempo Sync" );
	}
	if( m_custom != NULL &&
		model()->m_tempoSyncMode != tempoSyncKnobModel::SyncCustom )
	{
		m_custom->parentWidget()->hide();
	}

	switch( model()->m_tempoSyncMode )
	{
		case tempoSyncKnobModel::SyncNone:
			m_tempoSyncIcon = embed::getIconPixmap(
							"tempo_sync" );
			break;
		case tempoSyncKnobModel::SyncCustom:
			m_tempoSyncIcon = embed::getIconPixmap(
							"dont_know" );
			break;
		case tempoSyncKnobModel::SyncDoubleWholeNote:
			m_tempoSyncIcon = embed::getIconPixmap(
						"note_double_whole" );
			break;
		case tempoSyncKnobModel::SyncWholeNote:
			m_tempoSyncIcon = embed::getIconPixmap(
							"note_whole" );
			break;
		case tempoSyncKnobModel::SyncHalfNote:
			m_tempoSyncIcon = embed::getIconPixmap(
							"note_half" );
			break;
		case tempoSyncKnobModel::SyncQuarterNote:
			m_tempoSyncIcon = embed::getIconPixmap(
						"note_quarter" );
			break;
		case tempoSyncKnobModel::SyncEighthNote:
			m_tempoSyncIcon = embed::getIconPixmap(
							"note_eighth" );
			break;
		case tempoSyncKnobModel::SyncSixteenthNote:
			m_tempoSyncIcon = embed::getIconPixmap(
						"note_sixteenth" );
			break;
		case tempoSyncKnobModel::SyncThirtysecondNote:
			m_tempoSyncIcon = embed::getIconPixmap(
						"note_thirtysecond" );
			break;
		default:
			printf( "tempoSyncKnob::calculateTempoSyncTime"
					": invalid tempoSyncMode" );
			break;
	}

	emit syncDescriptionChanged( m_tempoSyncDescription );
	emit syncIconChanged();
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




void tempoSyncKnob::showCustom( void )
{
	if( m_custom == NULL )
	{
	 	m_custom = new meterDialog(
					engine::mainWindow()->workspace() );
		engine::mainWindow()->workspace()->addSubWindow( m_custom );
		m_custom->setWindowTitle( "Meter" );
		m_custom->setModel( &model()->m_custom );
	}
	m_custom->parentWidget()->show();
	model()->setTempoSync( tempoSyncKnobModel::SyncCustom );
}



#include "moc_tempo_sync_knob.cxx"


