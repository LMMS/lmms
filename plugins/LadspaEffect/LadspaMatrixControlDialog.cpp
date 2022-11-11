/*
 * LadspaMatrixControlDialog.h - Dialog for displaying and editing control port
 *                               values for LADSPA plugins in a matrix display
 *
 * Copyright (c) 2015 Michael Gregorius <michaelgregorius/at/web[dot]de>
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


#include <QGroupBox>
#include <QLayout>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>

#include "LadspaBase.h"
#include "LadspaControl.h"
#include "LadspaEffect.h"
#include "LadspaMatrixControlDialog.h"
#include "LadspaMatrixControlView.h"
#include "LadspaControlView.h"
#include "LedCheckBox.h"



namespace lmms::gui
{

static int const s_linkBaseColumn = 0;
static int const s_parameterNameBaseColumn = 2;
static int const s_channelBaseColumn = 4;




LadspaMatrixControlDialog::LadspaMatrixControlDialog( LadspaControls * ladspaControls ) :
	EffectControlDialog( ladspaControls ),
	m_effectGridLayout( NULL ),
	m_stereoLink( NULL )
{
	QVBoxLayout * mainLayout = new QVBoxLayout( this );

	m_effectGridLayout = new QGridLayout();
	mainLayout->addLayout(m_effectGridLayout);

	updateEffectView( ladspaControls );

	if( ladspaControls->m_processors > 1 )
	{
		mainLayout->addSpacing( 3 );
		QHBoxLayout * center = new QHBoxLayout();
		mainLayout->addLayout( center );
		m_stereoLink = new LedCheckBox( tr( "Link Channels" ), this );
		m_stereoLink->setModel( &ladspaControls->m_stereoLinkModel );
		center->addWidget( m_stereoLink );
	}
}




LadspaMatrixControlDialog::~LadspaMatrixControlDialog()
{
}




void LadspaMatrixControlDialog::updateEffectView( LadspaControls * ladspaControls )
{
	QList<QWidget *> list = findChildren<QWidget *>();
	for( QList<QWidget *>::iterator it = list.begin(); it != list.end(); ++it )
	{
		delete *it;
	}

	m_effectControls = ladspaControls;

	QWidget *widget = new QWidget(this);
	QGridLayout *gridLayout = new QGridLayout(widget);
	widget->setLayout(gridLayout);

	gridLayout->addWidget( new QLabel( "<b>" + tr( "Parameter" )  + "</b>", widget ), 0, s_parameterNameBaseColumn, Qt::AlignRight );

	bool linkLabelAdded = false;
	ch_cnt_t const numberOfChannels = ladspaControls->m_processors;

	gridLayout->setColumnMinimumWidth(1, 20);

	for ( ch_cnt_t i = 0; i < numberOfChannels; ++i )
	{
		QString channelString( tr( "Channel %1" ) );
		int currentChannelColumn = s_channelBaseColumn + 2*i;

		gridLayout->addWidget( new QLabel( "<b>" + channelString.arg( QString::number( i + 1 ) ) + "</b>", widget ), 0, currentChannelColumn, Qt::AlignHCenter );

		control_list_t & controls = ladspaControls->m_controls[i];

		int currentRow = 1;
		control_list_t::iterator end = controls.end();
		for( control_list_t::iterator it = controls.begin(); it != end; ++it )
		{
			LadspaControl * ladspaControl = *it;

			if ( i == 0 )
			{
				// TODO Assumes that all processors are equal! Change to more general approach, e.g. map from name to row

				// Link
				if ( ladspaControl->m_link )
				{
					if ( !linkLabelAdded )
					{
						gridLayout->addWidget( new QLabel( "<b>" + tr( "Link" ) + "</b>", widget ), 0, s_linkBaseColumn, Qt::AlignHCenter );
						linkLabelAdded = true;
					}
					LedCheckBox * linkCheckBox = new LedCheckBox( "", widget );
					linkCheckBox->setModel( &ladspaControl->m_linkEnabledModel );
					linkCheckBox->setToolTip( tr( "Link channels" ) );
					gridLayout->addWidget( linkCheckBox, currentRow, s_linkBaseColumn, Qt::AlignHCenter );
				}

				// Parameter name
				QString portName = ladspaControl->port()->name;
				QLabel *portNameLabel = new QLabel( portName, widget );
				gridLayout->addWidget( portNameLabel, currentRow, s_parameterNameBaseColumn, Qt::AlignRight );
			}

			LadspaMatrixControlView *ladspaMatrixControlView = new LadspaMatrixControlView( widget, ladspaControl );
			gridLayout->addWidget( ladspaMatrixControlView, currentRow, currentChannelColumn, Qt::AlignHCenter );
			gridLayout->setColumnMinimumWidth(currentChannelColumn - 1, 20);

			++currentRow;
		}
	}

	QScrollArea *scrollArea = new QScrollArea( this );
	scrollArea->setWidgetResizable( true );
	scrollArea->setWidget( widget );
	scrollArea->setFrameShape( QFrame::NoFrame );

	m_effectGridLayout->addWidget( scrollArea, 0, 0 );
	m_effectGridLayout->setMargin(0);

	if( numberOfChannels > 1 && m_stereoLink != NULL )
	{
		m_stereoLink->setModel( &ladspaControls->m_stereoLinkModel );
	}

	connect( ladspaControls, SIGNAL( effectModelChanged( lmms::LadspaControls * ) ),
				this, SLOT( updateEffectView( lmms::LadspaControls * ) ),
							Qt::DirectConnection );
}

} // namespace lmms::gui
