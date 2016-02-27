/*
 * ShmWrapper.h - Class for wrapping Shared Memory calls splits on USE_QT_SHM to use Qt shared memory or native linux shared memory
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef SHM_WRAPPER_H
#define SHM_WRAPPER_H

#ifdef USE_QT_SHMEM
#include <QSharedMemory>
#else
#include <sys/shm.h>
#endif

class ShmWrapper
{
public:
    ShmWrapper() 
      #ifndef USE_QT_SHMEM
      : m_shmID(-1)
      #endif
    {};
    
    ShmWrapper(key_t key) :
    #ifdef USE_QT_SHMEM
	m_shm(QString::number( key ))
    #else
	m_shmID(shmget( key, 0, 0 ))
    #endif
    {}
    
    ~ShmWrapper()
    {}
    
    void initializeData()
    {
      m_shmKey = 0;
      #ifdef USE_QT_SHMEM
		do
		{
			m_shm.setKey( QString( "%1" ).arg( ++m_shmKey ) );
			m_shm.create( sizeof( int* ) );
		} while( m_shm.error() != QSharedMemory::NoError );
      #else
		while( ( m_shmID = shmget( ++m_shmKey, sizeof( int* ),
					IPC_CREAT | IPC_EXCL | 0600 ) ) == -1 )
		{
		}
      #endif
    }
    
    key_t key() const
    {
        return m_shmKey;      
    }
    
    
    void detach()
    {
	#ifndef USE_QT_SHMEM
	  void* memory = shmat( m_shmID, 0, 0 );
	  shmdt( memory );
	#endif
    }
    
    void destroy()
    {
	#ifndef USE_QT_SHMEM
	  shmctl( m_shmID, IPC_RMID, NULL );
	#endif
    }
    
    bool attach()
    {
	#ifdef USE_QT_SHMEM
	  return m_shm.attach();
	#else
	  return (m_shmID != -1);
	#endif
    }
    
    void* data()
    {
	#ifdef USE_QT_SHMEM
	  return m_shm.data();
	#else
	  return shmat( m_shmID, 0, 0 );
	#endif
    }

  
    private:
      int m_shmID;
      key_t m_shmKey;
      #ifdef USE_QT_SHMEM
	QSharedMemory m_shm;
      #endif
};
#endif
