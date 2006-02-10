/*
 * bb_editor.h - declaration of class bbEditor, a basic-component of LMMS
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _BB_EDITOR_H
#define _BB_EDITOR_H

#include "qt3support.h"
#include "track_container.h"


class QPixmap;

class comboBox;
class toolButton;


class bbEditor : public trackContainer
{
	Q_OBJECT
public:
	virtual bool FASTCALL play( midiTime _start, const f_cnt_t _start_frame,
						const f_cnt_t _frames,
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

	csize currentBB( void ) const;
	tact FASTCALL lengthOfBB( const csize _bb );
	inline tact lengthOfCurrentBB( void )
	{
		return( lengthOfBB( currentBB() ) );
	}
	void FASTCALL removeBB( const csize _bb );
	csize numOfBBs( void ) const;

	void FASTCALL swapBB( const csize _bb1, const csize _bb2 );

	void updateBBTrack( trackContentObject * _tco );


public slots:
	void play( void );
	void stop( void );
	void updateComboBox( void );
	void setCurrentBB( int _bb );


protected:
	virtual void closeEvent( QCloseEvent * _ce );
	virtual void keyPressEvent( QKeyEvent * _ke );
	virtual void resizeEvent( QResizeEvent * _re );

	virtual QRect scrollAreaRect( void ) const;


private:
	bbEditor( engine * _engine );
	//bbEditor( const bbEditor & );
	virtual ~bbEditor();

	void FASTCALL createTCOsForBB( const csize _bb );


	QWidget * m_toolBar;

	toolButton * m_playButton;
	toolButton * m_stopButton;

	comboBox * m_bbComboBox;


	friend class engine;

} ;


#endif
