#ifndef ZYNSEMA_H
#define ZYNSEMA_H

#if defined __APPLE__ || defined WIN32

#include <pthread.h>

class ZynSema
{
public:
    ZynSema (void) : _count (0)
    {
    }

    ~ZynSema (void)
    {
        pthread_mutex_destroy (&_mutex);
        pthread_cond_destroy (&_cond);
    }

    int init (int, int v)
    {
        _count = v;
        return pthread_mutex_init (&_mutex, 0) || pthread_cond_init (&_cond, 0);
    }

    int post (void)
    {
        pthread_mutex_lock (&_mutex);
        if (++_count == 1) pthread_cond_signal (&_cond);
        pthread_mutex_unlock (&_mutex);
        return 0;
    }

    int wait (void)
    {
        pthread_mutex_lock (&_mutex);
        while (_count < 1) pthread_cond_wait (&_cond, &_mutex);
        --_count;
        pthread_mutex_unlock (&_mutex);
        return 0;
    }

    int trywait (void)
    {
        if (pthread_mutex_trylock (&_mutex)) return -1;
        if (_count < 1)
        {
            pthread_mutex_unlock (&_mutex);
            return -1;
        }
        --_count;
        pthread_mutex_unlock (&_mutex);
        return 0;
    }

    int getvalue (void) const
    {
        return _count;
    }


private:
    int              _count;
    pthread_mutex_t  _mutex;
    pthread_cond_t   _cond;
};

#else // POSIX sempahore

#include <semaphore.h>

class ZynSema
{
public:
    ZynSema (void)
    {
    }
    ~ZynSema (void)
    {
        sem_destroy (&_sema);
    }
    int init (int s, int v)
    {
        return sem_init (&_sema, s, v);
    }
    int post (void)
    {
        return sem_post (&_sema);
    }
    int wait (void)
    {
        return sem_wait (&_sema);
    }
    int trywait (void)
    {
        return sem_trywait (&_sema);
    }
    int getvalue(void)
    {
        int v = 0;
        sem_getvalue(&_sema, &v);
        return v;
    }

private:
    sem_t _sema;
};

#endif // POSIX semapore

#endif // ZYNSEMA_H
