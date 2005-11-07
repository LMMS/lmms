/*
 * bb_editor.h - declaration of class bbEditor, a basic-component of LMMS
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include "lmms_main_win.h"


class pixmapButton;
class songEditor;
class QPixmap;


class bbEditor : public trackContainer
{
	Q_OBJECT
public:
	static inline bbEditor * inst( void )
	{
		if( s_instanceOfMe == NULL )
		{
			s_instanceOfMe = new bbEditor();
		}
		return( s_instanceOfMe );
	}

	virtual bool FASTCALL play( midiTime _start, Uint32 _start_frame,
					Uint32 _frames, Uint32 _frame_base,
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
	void FASTCALL setCurrentBB( csize _bb );
	tact FASTCALL lengthOfBB( csize _bb );
	inline tact lengthOfCurrentBB( void )
	{
		return( lengthOfBB( currentBB() ) );
	}
	void FASTCALL removeBB( csize _bb );
	csize numOfBBs( void ) const;

	void FASTCALL swapBB( csize _bb1, csize _bb2 );


protected:
	virtual void closeEvent( QCloseEvent * _ce );
	virtual void keyPressEvent( QKeyEvent * _ke );
	virtual void resizeEvent( QResizeEvent * _re );

	void updateBackground( void );


protected slots:
	void play( void );
	void stop( void );


private:
	bbEditor();
	//bbEditor( const bbEditor & );
	~bbEditor();

	void FASTCALL createTCOsForBB( csize _bb );


	static bbEditor * s_instanceOfMe;
	static QPixmap * s_titleArtwork;

	pixmapButton * m_playButton;
	pixmapButton * m_stopButton;


	friend class songEditor;
	friend lmmsMainWin::~lmmsMainWin();

} ;


#endif
