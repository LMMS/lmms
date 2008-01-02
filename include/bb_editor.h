/*
 * bb_editor.h - declaration of class bbEditor, a basic-component of LMMS
 *
 * Copyright (c) 2004-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _BB_EDITOR_H
#define _BB_EDITOR_H

#include "track_container.h"
#include "combobox.h"


class QPixmap;

class toolButton;


class bbEditor : public trackContainer
{
	Q_OBJECT
	mapPropertyFromModelPtr(int,currentBB,setCurrentBB,m_bbComboBoxModel);
public:
	virtual bool FASTCALL play( midiTime _start, const fpp_t _frames,
						const f_cnt_t _frame_base,
							Sint16 _tco_num = -1 );

	virtual void FASTCALL saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );
	virtual void updateAfterTrackAdd( void );

	inline virtual QString nodeName( void ) const
	{
		return( "bbeditor" );
	}

	virtual inline bool fixedTCOs( void ) const
	{
		return( TRUE );
	}

	tact FASTCALL lengthOfBB( const int _bb );
	inline tact lengthOfCurrentBB( void )
	{
		return( lengthOfBB( currentBB() ) );
	}
	void FASTCALL removeBB( const int _bb );
	int numOfBBs( void ) const;

	void FASTCALL swapBB( const int _bb1, const int _bb2 );

	void updateBBTrack( trackContentObject * _tco );


public slots:
	void play( void );
	void stop( void );
	void updateComboBox( void );
	void currentBBChanged( void );


protected:
	virtual void keyPressEvent( QKeyEvent * _ke );


private:
	bbEditor( void );
	//bbEditor( const bbEditor & );
	virtual ~bbEditor();

	void FASTCALL createTCOsForBB( const int _bb );


	QWidget * m_toolBar;

	toolButton * m_playButton;
	toolButton * m_stopButton;

	comboBox * m_bbComboBox;
	comboBoxModel * m_bbComboBoxModel;

	friend class engine;

} ;


#endif
