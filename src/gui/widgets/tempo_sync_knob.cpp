/*
 * TempoSyncKnob.cpp - adds bpm to ms conversion for knob class
 *
 * Copyright (c) 2005-2007 Danny McRae <khjklujn/at/yahoo.com>
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtGui/QMdiArea>

#include "TempoSyncKnob.h"
#include "engine.h"
#include "caption_menu.h"
#include "embed.h"
#include "MainWindow.h"
#include "MeterDialog.h"
#include "song.h"



TempoSyncKnob::TempoSyncKnob( knobTypes _knob_num, QWidget * _parent,
						const QString & _name ) :
	knob( _knob_num, _parent, _name ),
	m_tempoSyncIcon( embed::getIconPixmap( "tempo_sync" ) ),
	m_tempoSyncDescription( tr( "Tempo Sync" ) ),
	m_custom( NULL )
{
}




TempoSyncKnob::~TempoSyncKnob()
{
	if( m_custom )
	{
		delete m_custom->parentWidget();
	}
}




void TempoSyncKnob::modelChanged()
{
	if( model() == NULL )
	{
		qWarning( "no TempoSyncKnobModel has been set!" );
	}
	if( m_custom != NULL )
	{
		m_custom->setModel( &model()->m_custom );
	}
	connect( model(), SIGNAL( syncModeChanged( TempoSyncMode ) ),
			this, SLOT( updateDescAndIcon() ) );
	connect( this, SIGNAL( sliderMoved( float ) ),
			model(), SLOT( disableSync() ) );
	updateDescAndIcon();
}




void TempoSyncKnob::contextMenuEvent( QContextMenuEvent * )
{
	mouseReleaseEvent( NULL );

	captionMenu contextMenu( model()->displayName(), this );
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
		tr( "No Sync" ) )->setData( (int) TempoSyncKnobModel::SyncNone );
	if( limit / 0.125f <= model()->maxValue() )
	{
		syncMenu->addAction( embed::getIconPixmap( "note_double_whole" ),
				tr( "Eight beats" ) )->setData(
				(int) TempoSyncKnobModel::SyncDoubleWholeNote );
	}
	if( limit / 0.25f <= model()->maxValue() )
	{
		syncMenu->addAction( embed::getIconPixmap( "note_whole" ),
					tr( "Whole note" ) )->setData(
					(int) TempoSyncKnobModel::SyncWholeNote );
	}
	if( limit / 0.5f <= model()->maxValue() )
	{
		syncMenu->addAction( embed::getIconPixmap( "note_half" ),
					tr( "Half note" ) )->setData(
					(int) TempoSyncKnobModel::SyncHalfNote );
	}
	if( limit <= model()->maxValue() )
	{
		syncMenu->addAction( embed::getIconPixmap( "note_quarter" ),
					tr( "Quarter note" ) )->setData(
				(int) TempoSyncKnobModel::SyncQuarterNote );
	}
	if( limit / 2.0f <= model()->maxValue() )
	{
		syncMenu->addAction( embed::getIconPixmap( "note_eighth" ),
					tr( "8th note" ) )->setData(
				(int) TempoSyncKnobModel::SyncEighthNote );
	}
	if( limit / 4.0f <= model()->maxValue() )
	{
		syncMenu->addAction( embed::getIconPixmap( "note_sixteenth" ),
					tr( "16th note" ) )->setData(
				(int) TempoSyncKnobModel::SyncSixteenthNote );
	}
	syncMenu->addAction( embed::getIconPixmap( "note_thirtysecond" ),
					tr( "32nd note" ) )->setData(
				(int) TempoSyncKnobModel::SyncThirtysecondNote );
	syncMenu->addAction( embed::getIconPixmap( "dont_know" ),
				tr( "Custom..." ),
				this, SLOT( showCustom() )
						)->setData(
					(int) TempoSyncKnobModel::SyncCustom );
	contextMenu.addSeparator();

	}

	contextMenu.addHelpAction();
	contextMenu.exec( QCursor::pos() );
	
	delete syncMenu;
}




void TempoSyncKnob::updateDescAndIcon()
{
	if( model()->m_tempoSyncMode )
	{
		switch( model()->m_tempoSyncMode )
		{
			case TempoSyncKnobModel::SyncCustom:
				m_tempoSyncDescription = tr( "Custom " ) + 
						"(" +
			QString::number( model()->m_custom.numeratorModel().value() ) +
						"/" +
			QString::number( model()->m_custom.denominatorModel().value() ) +
						")";
				break;
			case TempoSyncKnobModel::SyncDoubleWholeNote:
				m_tempoSyncDescription = tr(
						"Synced to Eight Beats" );
				break;
			case TempoSyncKnobModel::SyncWholeNote:
				m_tempoSyncDescription = tr(
						"Synced to Whole Note" );
				break;
			case TempoSyncKnobModel::SyncHalfNote:
				m_tempoSyncDescription = tr(
							"Synced to Half Note" );
				break;
			case TempoSyncKnobModel::SyncQuarterNote:
				m_tempoSyncDescription = tr(
						"Synced to Quarter Note" );
				break;
			case TempoSyncKnobModel::SyncEighthNote:
				m_tempoSyncDescription = tr(
							"Synced to 8th Note" );
				break;
			case TempoSyncKnobModel::SyncSixteenthNote:
				m_tempoSyncDescription = tr(
							"Synced to 16th Note" );
				break;
			case TempoSyncKnobModel::SyncThirtysecondNote:
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
		model()->m_tempoSyncMode != TempoSyncKnobModel::SyncCustom )
	{
		m_custom->parentWidget()->hide();
	}

	switch( model()->m_tempoSyncMode )
	{
		case TempoSyncKnobModel::SyncNone:
			m_tempoSyncIcon = embed::getIconPixmap( "tempo_sync" );
			break;
		case TempoSyncKnobModel::SyncCustom:
			m_tempoSyncIcon = embed::getIconPixmap( "dont_know" );
			break;
		case TempoSyncKnobModel::SyncDoubleWholeNote:
			m_tempoSyncIcon = embed::getIconPixmap( "note_double_whole" );
			break;
		case TempoSyncKnobModel::SyncWholeNote:
			m_tempoSyncIcon = embed::getIconPixmap( "note_whole" );
			break;
		case TempoSyncKnobModel::SyncHalfNote:
			m_tempoSyncIcon = embed::getIconPixmap( "note_half" );
			break;
		case TempoSyncKnobModel::SyncQuarterNote:
			m_tempoSyncIcon = embed::getIconPixmap( "note_quarter" );
			break;
		case TempoSyncKnobModel::SyncEighthNote:
			m_tempoSyncIcon = embed::getIconPixmap( "note_eighth" );
			break;
		case TempoSyncKnobModel::SyncSixteenthNote:
			m_tempoSyncIcon = embed::getIconPixmap( "note_sixteenth" );
			break;
		case TempoSyncKnobModel::SyncThirtysecondNote:
			m_tempoSyncIcon = embed::getIconPixmap( "note_thirtysecond" );
			break;
		default:
			qWarning( "TempoSyncKnob::calculateTempoSyncTime:"
						"invalid TempoSyncMode" );
			break;
	}

	emit syncDescriptionChanged( m_tempoSyncDescription );
	emit syncIconChanged();
}




const QString & TempoSyncKnob::syncDescription()
{
	return m_tempoSyncDescription;
}




void TempoSyncKnob::setSyncDescription( const QString & _new_description )
{
	m_tempoSyncDescription = _new_description;
	emit syncDescriptionChanged( _new_description );
}




const QPixmap & TempoSyncKnob::syncIcon()
{
	return m_tempoSyncIcon;
}




void TempoSyncKnob::setSyncIcon( const QPixmap & _new_icon )
{
	m_tempoSyncIcon = _new_icon;
	emit syncIconChanged();
}




void TempoSyncKnob::showCustom()
{
	if( m_custom == NULL )
	{
	 	m_custom = new MeterDialog( engine::mainWindow()->workspace() );
		engine::mainWindow()->workspace()->addSubWindow( m_custom );
		m_custom->setWindowTitle( "Meter" );
		m_custom->setModel( &model()->m_custom );
	}
	m_custom->parentWidget()->show();
	model()->setTempoSync( TempoSyncKnobModel::SyncCustom );
}



#include "moc_TempoSyncKnob.cxx"


