
#ifndef SAFEQUEUE_H
#define SAFEQUEUE_H
#include <cstdlib>
#include "ZynSema.h"
#include <pthread.h>
#include <semaphore.h>

/**
 * C++ thread safe lockless queue
 * Based off of jack's ringbuffer*/
template<class T>
class SafeQueue
{
    public:
        SafeQueue(size_t maxlen);
        ~SafeQueue();

        /**Return read size*/
        unsigned int size() const;

        /**Returns 0 for normal
         * Returns -1 on error*/
        int push(const T &in);
        int peak(T &out) const;
        int pop(T &out);

        //clears reading space
        void clear();

    private:
        unsigned int wSpace() const;
        unsigned int rSpace() const;

        //write space
        mutable ZynSema w_space;
        //read space
        mutable ZynSema r_space;

        //next writing spot
        size_t writePtr;
        //next reading spot
        size_t readPtr;
        const size_t bufSize;
        T *buffer;
};

#include "SafeQueue.cpp"
#endif
