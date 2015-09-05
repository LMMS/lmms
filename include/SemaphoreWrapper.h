/*
 * SemaphoreWrapper.h - Class for wrapping Semaphore calls splits on USE_QT_SEMAPHORES to use Qt semaphores or native linux semaphores
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


#ifndef SEMAPHORE_WRAPPER_H
#define SEMAPHORE_WRAPPER_H

#ifdef USE_QT_SEMAPHORES
  #include <QSystemSemaphore>
#endif
#include <semaphore.h>

class SemaphoreWrapper 
{
  private:
#ifdef USE_QT_SEMAPHORES
    QSystemSemaphore m_sem;
#else
    sem_t* m_sem;
#endif
  
  public:
	SemaphoreWrapper()
	#ifdef USE_QT_SEMAPHORES
	   : m_sem(QString::null)
	#endif
	{ };
	
	~SemaphoreWrapper()
	{
	#ifndef USE_QT_SEMAPHORES
	    sem_destroy(m_sem);
	#endif
	}
	
	int getValue()
	{
	    #ifdef USE_QT_SEMAPHORES
	      return 0;
	    #else
	      int v;
	      sem_getvalue(m_sem, &v);
	      return v;
	    #endif
	}
	
	bool acquire()
	{
	  #ifdef USE_QT_SEMAPHORES
	    return m_sem.acquire();
	  #else
	    sem_wait(m_sem);
	  #endif
	  return true;
	}
	
	void release()
	{
	  #ifdef USE_QT_SEMAPHORES
	    m_sem.release();
	  #else
	    const int err = sem_post(m_sem);
            (void)err; //currently unused
	  #endif
	}
	
	#ifdef USE_QT_SEMAPHORES
	void createKey(const QString& key, const int initialvalue)
	{
	   m_sem.setKey(key, initialvalue, QSystemSemaphore::Create); 
	}
	#endif
	
	//set semaphore key, only used in qt
	#ifdef USE_QT_SEMAPHORES
	void setKey(const QString& key)
	{
	   m_sem.setKey(key); 
	}
	#endif
	
	//set semaphore pointer, only used in native
	#ifndef USE_QT_SEMAPHORES
	void setSemaphore(sem_t* s, bool init = false)
	{
	   m_sem = s;
	   if (init)
	   {
	      sem_init(m_sem, 1, 1);
	   }
	}
	#endif
};

#endif
